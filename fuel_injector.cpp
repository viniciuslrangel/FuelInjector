#include "fuel_injector.hpp"

#include "injectable.hpp"

#include <string>
#include <iostream>
#include <fstream>

#include <Windows.h>

FuelInjector::FuelInjector(DWORD pid) : pid(pid) {
}

FuelInjector::~FuelInjector() {
  Unbind();
}

bool FuelInjector::Bind() {
  if(handle != nullptr) {
    return false;
  }
  handle = OpenProcess(PROCESS_ALL_ACCESS, false, pid); // NOLINT(hicpp-signed-bitwise)
  if(handle == nullptr) {
    return false;
  }

  std::string dllPath;
  dllPath.resize(MAX_PATH + 1);
  dllPath.resize(GetTempPathA(dllPath.size(), &dllPath[0]));
  dllPath.append("fuel.dll");

  std::ofstream ofs;
  ofs.open(dllPath, std::ios::trunc | std::ios::binary);
  if(ofs.is_open()) {
    ofs.write(Injectable::data, Injectable::size);
    ofs.close();
  }

  return true;
}

void FuelInjector::Unbind() {
  if(handle == nullptr) {
    return;
  }
  CloseHandle(handle);
  handle = nullptr;
}
