#include "cmd_queue.hpp"
#include "v8_adapters/base_adapter.hpp"

#include <Windows.h>
#include <psapi.h>

#include <string>

static HINSTANCE mod;

static struct {
  HANDLE rPipe{};
  HANDLE wPipe{};

} connection{};

static DWORD WINAPI MainThread(LPVOID) {
  HMODULE v8Module = Fuel::V8BaseAdapter::FindModule("v8.dll");
  if (v8Module == nullptr) {
    v8Module = Fuel::V8BaseAdapter::FindModule();
  }

  Fuel::V8BaseAdapter *adapter = Fuel::V8BaseAdapter::AutoSelectVersion(v8Module);

  DEBUG({
          FILE *debugOutput;
          FILE *debugInput;
          AllocConsole();
          freopen_s(&debugOutput, "CONOUT$", "w", stdout);
          freopen_s(&debugInput, "CONIN$", "r", stdin);
          SetStdHandle(STD_OUTPUT_HANDLE, debugOutput);
          SetStdHandle(STD_ERROR_HANDLE, debugOutput);
          SetStdHandle(STD_INPUT_HANDLE, debugInput);
        })

  DEFER({
          adapter->Shutdown();
          FreeLibraryAndExitThread(mod, 0);
          delete adapter;
        });

  adapter->Setup(v8Module);

  static std::string data;
  static std::string output;
  while (true) {
    char  buffer[128];
    DWORD count = 0;
readAgain:
    if (!ReadFile(connection.rPipe, &buffer, sizeof(buffer), &count, nullptr) && GetLastError() != ERROR_IO_PENDING) {
      break;
    }
    data.append(buffer, count);
    if (count == sizeof(buffer)) {
      goto readAgain; // If there is more data to be read
    }
    if (!data.empty()) {
      output = "Echo: " + data;
      DWORD o;
      Fuel::CmdQueue::GetCmdList()->push_back(std::move(data));
      data = std::string();
      WriteFile(connection.wPipe, output.c_str(), output.size(), &o, nullptr);
    }
  }
  return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
  if (fdwReason == DLL_PROCESS_ATTACH) {

    // Creates module own thread (connectivity)
    mod = hinstDLL;
    connection.rPipe    = CreateThread(nullptr, 0, MainThread, nullptr, CREATE_SUSPENDED, nullptr);

    // Uses the first 2 bytes of the dll entrypoint
    // as a communication standard to the parent process
    // to establish the pipe socket
    MODULEINFO modInfo;
    GetModuleInformation(GetCurrentProcess(), mod, &modInfo, sizeof(MODULEINFO));
    static void *backup[2];
    void        **entry = static_cast<void **>(modInfo.EntryPoint) + 1000;
    DWORD       _old;
    VirtualProtect(entry, sizeof(backup), PAGE_EXECUTE_READWRITE, &_old);
    backup[0] = entry[0];
    backup[1] = entry[1];
    entry[0]  = &connection;
    entry[1]  = &backup;
  }
  return true;
}
