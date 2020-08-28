#include <Windows.h>

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
