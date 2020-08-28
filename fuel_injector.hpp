#pragma once

#ifndef BASETYPES
typedef unsigned long DWORD;

typedef void *HANDLE;
typedef void *HINTANCE;
typedef void *HMODULE;
#endif

class FuelInjector {
  DWORD   pid;
  HANDLE  remoteThread{}, pHandle{};
  HANDLE  rPipe{}, wPipe{};
  HMODULE module{};

  struct {
    HANDLE rPipe{}, wPipe{};
  }       remote{};
public:
  explicit FuelInjector(DWORD pid);

  ~FuelInjector();

  bool Bind();

  void Unbind();

  void Send(const char *data, size_t size);
};
