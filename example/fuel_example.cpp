#include <string>
#include <iostream>

#include <Windows.h>

#include <fuel_injector.hpp>

using std::string_literals::operator ""s;

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
      auto pidStr = std::string(arg);
      if (pidStr.rfind("0x", 0) == 0) {
        pid = std::stoi(pidStr.substr(2), nullptr, 16);
      } else {
        pid = std::stoi(pidStr);
      }
    }
  }

  if(title.empty() && pid == 0) {
    std::cerr << "No argument: " << argv[0] << " [title WindowName] [pid 123] [pid 0x123]" << std::endl;
    return 1;
  }

  if(!title.empty()) {
    HWND hwnd = FindWindowA(nullptr, title.c_str());
    if(hwnd != nullptr) {
      GetWindowThreadProcessId(hwnd, &pid);
    }
  }

  FuelInjector injector(pid);
  injector.Bind();

}