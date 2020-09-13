#include <Windows.h>
#include <psapi.h>

#include <string>

static HINSTANCE mod;

static struct {
  HANDLE rPipe{}, wPipe{};
}                connection{};

DWORD WINAPI MainThread(LPVOID) {
  HWND hwnd;
  EnumWindows([](HWND hwnd, LPARAM param) -> BOOL {
    DWORD proc;
    GetWindowThreadProcessId(hwnd, &proc);
    if (proc == GetCurrentProcessId()) {
      *((HWND *) param) = hwnd;
      return false;
    }
    return true;
  }, (LPARAM) &hwnd);
  hwnd = FindWindowEx(hwnd, nullptr, TEXT("Edit"), nullptr);

  static std::string data;
  while (true) {
    char  buffer[128];
    DWORD count = 0;
    if (!ReadFile(connection.rPipe, &buffer, sizeof(buffer), &count, nullptr) && GetLastError() != ERROR_IO_PENDING) {
      break;
    }
    if(count > 0) {
      data.resize(SendMessage(hwnd, WM_GETTEXTLENGTH, 0, 0) + 1);
      SendMessage(hwnd, WM_GETTEXT, data.size(), reinterpret_cast<LPARAM>(data.data()));
      data.pop_back();
      data.append(buffer, count);
      SendMessage(hwnd, WM_SETTEXT, data.size(), reinterpret_cast<LPARAM>(data.data()));
    }
  }

  FreeLibraryAndExitThread(mod, 0);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
  if (fdwReason == DLL_PROCESS_ATTACH) {
    mod = hinstDLL;
    connection.rPipe    = CreateThread(nullptr, 0, MainThread, nullptr, CREATE_SUSPENDED, nullptr);
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
