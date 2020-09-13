#include <string>
#include <iostream>
#include <future>

#include <Windows.h>

#include <fuel_injector.hpp>

using std::string_literals::operator ""s;

FuelInjector *injector;

int main(int argc, char *argv[]) {
  std::string title;
  DWORD       pid = 0;

  for (int i = 1; i < argc - 1; i++) {
    static const auto titleArg = "-title"s;
    static const auto pidArg   = "-pid"s;

    auto arg  = argv[i];
    auto next = argv[i + 1];
    if (titleArg == arg) {
      title = next;
    } else if (pidArg == arg) {
      auto pidStr = std::string(next);
      if (pidStr.rfind("0x", 0) == 0) {
        pid = std::stoi(pidStr.substr(2), nullptr, 16);
      } else {
        pid = std::stoi(pidStr);
      }
    }
  }

  if (title.empty() && pid == 0) {
    std::cerr << "Help: " << argv[0] << " [-title WindowName] [-pid 123] [-pid 0x123]" << std::endl;
    return 1;
  }

  if (!title.empty()) {
    HWND hwnd = FindWindowA(nullptr, title.c_str());
    if (hwnd != nullptr) {
      GetWindowThreadProcessId(hwnd, &pid);
    }
  }

  if(pid == 0) {
    std::cerr << "Could not find process";
    return 2;
  }

  SetConsoleCtrlHandler([](DWORD signal) -> BOOL {
    if(signal == CTRL_C_EVENT) {
      delete injector;
      exit(0);
    }
    return true;
  }, true);

  injector = new FuelInjector(pid);
  if(!injector->Bind()) {
    std::cerr << "Could not bind!";
    return 3;
  }

  while (true) {
    static std::string line;
    line.clear();
    std::getline(std::cin, line);
    if (line == "\\exit") {
      break;
    }
    line.append("\n");
    injector->Send(line.c_str(), line.size());
  }
  delete injector;
}