#pragma once

#ifndef BASETYPES
typedef unsigned long DWORD;

typedef void *HANDLE;
typedef void *HINTANCE;
typedef void *HMODULE;
#endif

namespace Fuel {

class Injector {
  DWORD   pid;
  HANDLE  remoteThread{}, pHandle{};
  HANDLE  rPipe{}, wPipe{};
  HMODULE module{};

  bool opened;
  bool ownHandle;

  struct {
    HANDLE rPipe{}, wPipe{};
  }    remote{};
public:
  explicit Injector() : Injector(DWORD(0)) {}

  explicit Injector(DWORD pid);

  explicit Injector(HANDLE process, DWORD pid = 0);

  ~Injector();

  bool Bind();

  void Unbind();

  bool IsOpen() const;

  operator bool() const;

  void Send(const char *data, size_t size);

  size_t Read(char *output, size_t maxSize, DWORD &err);
};

}
