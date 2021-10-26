#include "base_adapter.hpp"

#include "v8_adapter_v6.hpp"

#include <tlhelp32.h>
#include <string>

namespace Fuel {

/**
Address=7A091653
Type=Export
Ordinal=4161
Symbol=?GetVersion@V8@v8@@SAPBDXZ
Symbol (undecorated)=public: static char const * __cdecl v8::V8::GetVersion(void)
 */

struct GetVersionInfo {
  using FuncType = const char *(__cdecl *)(void);
  static const char *SymbolName;
};
const char *GetVersionInfo::SymbolName = "?GetVersion@V8@v8@@SAPBDXZ";

V8BaseAdapter::~V8BaseAdapter() = default;

HMODULE V8BaseAdapter::FindModule(std::string_view name) {
  HMODULE mod;
  HANDLE  snap = INVALID_HANDLE_VALUE;
  snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
  if (snap == INVALID_HANDLE_VALUE) {
    return nullptr;
  }
  DEFER({
          CloseHandle(snap);
        });

  MODULEENTRY32 me;
  me.dwSize = sizeof(MODULEENTRY32);

  if (!Module32First(snap, &me)) {
    return nullptr;
  }

  if (name.length() == 0) {
    return me.hModule;
  }

  do {
    std::string_view moduleName(me.szModule);

    if (moduleName == name) {
      return me.hModule;
    }

  } while (Module32Next(snap, &me));

  return nullptr;
}

V8BaseAdapter *V8BaseAdapter::AutoSelectVersion(HMODULE mod) {
  auto GetVersion = (GetVersionInfo::FuncType) GetProcAddress(mod, GetVersionInfo::SymbolName);

  const std::string version(GetVersion());

  int majorVersion = std::stoi(version.substr(0, version.find_first_of('.')));

  switch (majorVersion) {
    case 6:
      return new V8Adapter6;
    default:
      return nullptr;
  }
}

}
