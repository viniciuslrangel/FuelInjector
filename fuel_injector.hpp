#pragma once

#ifndef BASETYPES
typedef unsigned long DWORD;
typedef void          *HANDLE;
#endif

class FuelInjector {
  DWORD  pid;
  HANDLE handle = nullptr;
public:
  FuelInjector(DWORD pid);

  ~FuelInjector();

  bool Bind();

  void Unbind();
};
