#include <Windows.h>
#include "fuel_injector.hpp"

#include <psapi.h>

#include "injectable.hpp"

#include <fstream>
#include <functional>
#include <string>
#include <vector>

static const std::string dllName = "fuel.dll";

static const std::string &getDllPath() {
  static bool        flag = true;
  static std::string dllPath;
  if (flag) {
    flag = false;
    dllPath.resize(MAX_PATH + 1);
    dllPath.resize(GetTempPathA(dllPath.size(), &dllPath[0]));
    dllPath.append(dllName);
  }
  return dllPath;
}

class Memory {
  HANDLE h;
  size_t size;
  void   *addr;
public:
  Memory(HANDLE h, size_t size, const void *data = nullptr) : h(h), size(size) {
    addr = VirtualAllocEx(h, nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (data != nullptr) {
      (*this)(data);
    }
  }

  void operator()(const void *data, size_t dataSize = -1, size_t offset = 0) {
    WriteProcessMemory(h, static_cast<char *>(addr) + offset, data, dataSize == -1 ? size : dataSize, nullptr);
  }

  operator void *() const { // NOLINT(google-explicit-constructor)
    return addr;
  }

  void release() {
    if (addr != nullptr) {
      VirtualFreeEx(h, addr, size, MEM_RELEASE);
      addr = nullptr;
    }
  }

  ~Memory() {
    release();
  }
};

FuelInjector::FuelInjector(DWORD pid) : pid(pid) {
}

FuelInjector::~FuelInjector() {
  Unbind();
}

bool FuelInjector::Bind() {
  if (pHandle != nullptr) {
    return false;
  }
  pHandle = OpenProcess(PROCESS_ALL_ACCESS, false, pid); // NOLINT(hicpp-signed-bitwise)
  if (pHandle == nullptr) {
    return false;
  }

  void *infoAddr;
  {
    const auto &dllPath = getDllPath();

    {
      std::ofstream ofs;
      ofs.open(dllPath, std::ios::trunc | std::ios::binary);
      if (ofs.is_open()) {
        ofs.write(Injectable::data, Injectable::size);
        ofs.close();
      }
    }

    {
      Memory m(pHandle, dllPath.size() + 1, dllPath.c_str());
      HANDLE th = CreateRemoteThread(pHandle, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(&LoadLibraryA), m, 0, nullptr);
      WaitForSingleObject(th, INFINITE);
      CloseHandle(th);
    }

    {
      DWORD requiredSize;
      EnumProcessModules(pHandle, nullptr, 0, &requiredSize);
      std::vector<HMODULE> modules;
      modules.resize(requiredSize / sizeof(HMODULE));
      EnumProcessModules(pHandle, modules.data(), requiredSize, &requiredSize);
      for (HMODULE &mod : modules) {
        char name[255];
        GetModuleBaseNameA(pHandle, mod, name, 255);
        if (dllName == name) {
          module = mod;
          break;
        }
      }
      if (module == nullptr) {
        Unbind();
        return false;
      }

      MODULEINFO modInfo;
      GetModuleInformation(pHandle, module, &modInfo, sizeof(MODULEINFO));
      infoAddr = static_cast<void **>(modInfo.EntryPoint) + 1000;
    }

  }

  {
    void *entryData[2];
    ReadProcessMemory(pHandle, infoAddr, &entryData, sizeof(entryData), nullptr);
    void *remoteAddr = entryData[0];
    void *backupAsm  = entryData[1];
    ReadProcessMemory(pHandle, backupAsm, &entryData, sizeof(entryData), nullptr);
    WriteProcessMemory(pHandle, infoAddr, &backupAsm, sizeof(void *) * 2, nullptr);
    DWORD _old;
    VirtualProtectEx(pHandle, infoAddr, sizeof(void *) * 2, PAGE_EXECUTE, &_old);
    infoAddr = remoteAddr;
  }

  ReadProcessMemory(pHandle, infoAddr, &remote, sizeof(remote), nullptr);

  remoteThread = remote.rPipe;
  DuplicateHandle(pHandle, remoteThread, GetCurrentProcess(), &remoteThread, 0, false, DUPLICATE_SAME_ACCESS);

  CreatePipe(&rPipe, &remote.wPipe, nullptr, 4096);
  CreatePipe(&remote.rPipe, &wPipe, nullptr, 4096);
  DuplicateHandle(GetCurrentProcess(), remote.wPipe, pHandle, &remote.wPipe, 0, false, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS);
  DuplicateHandle(GetCurrentProcess(), remote.rPipe, pHandle, &remote.rPipe, 0, false, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS);

  WriteProcessMemory(pHandle, infoAddr, &remote, sizeof(remote), nullptr);

  ResumeThread(remoteThread);

  return true;
}

void FuelInjector::Unbind() {
  if (pHandle == nullptr) {
    return;
  }
  /*if (module != nullptr) {
    HANDLE th = CreateRemoteThread(pHandle, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(&FreeLibrary), module, 0, nullptr);
    WaitForSingleObject(th, INFINITE);
    CloseHandle(th);
  }*/
  CloseHandle(pHandle);
  pHandle = nullptr;
}

void FuelInjector::Send(const char* data, size_t size) {
  DWORD w;
  WriteFile(wPipe, data, size, &w, nullptr);
}
