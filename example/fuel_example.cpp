#include <string>
#include <iostream>
#include <future>

#include <Windows.h>

#include <fuel_injector.hpp>

using std::string_literals::operator ""s;

static Fuel::Injector injector;

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " exePath args..." << std::endl;
    return 1;
  }

  STARTUPINFO         startupInfo;
  PROCESS_INFORMATION pInfo;
  ZeroMemory(&startupInfo, sizeof(startupInfo));
  ZeroMemory(&pInfo, sizeof(pInfo));
  startupInfo.cb = sizeof(startupInfo);

  char *argStart = GetCommandLine() + strlen(argv[0]);
  argStart += (GetCommandLine()[0] == '"' ? 2 : 0) + 1;

  bool created = CreateProcessA(
      nullptr,
      argStart,
      nullptr,
      nullptr,
      FALSE,
      CREATE_SUSPENDED,
      nullptr,
      nullptr,
      &startupInfo,
      &pInfo
  );

  if (!created) {
    std::cout << "Could not create process" << std::endl;
    return 2;
  }


  injector = Fuel::Injector(pInfo.hProcess, pInfo.dwProcessId);

  SetConsoleCtrlHandler([](DWORD signal) -> BOOL {
    if (signal == CTRL_C_EVENT) {
      injector.Unbind();
      exit(0);
    }
    return true;
  }, true);

  if (!injector.Bind()) {
    DWORD exitCode = -1;
    GetExitCodeProcess(pInfo.hProcess, &exitCode);
    std::cerr << "Could not bind! (" << exitCode << ")" << std::endl;
    return 3;
  }

  ResumeThread(pInfo.hThread);

  std::thread th([]() {
    char  buffer[128];
    DWORD err;
    while (injector) {
      size_t count = injector.Read(buffer, 128, err);
      if (err != 0) {
        std::cout << "Disconnected!" << std::endl;
        exit(0);
      }
      std::cout.write(buffer, count).flush();
    }
  });

  while (injector) {
    static std::string line;
    line.clear();
    std::getline(std::cin, line);
    if (line.empty()) {
      continue;
    }
    if (line == "\\exit") {
      break;
    }
    line.append("\n");
    injector.Send(line.c_str(), line.size());
  }
  exit(0);
}