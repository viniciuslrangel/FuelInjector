#pragma once

#include <Windows.h>

#include "../utility.hpp"

namespace Fuel {

class V8BaseAdapter {
protected:
  HANDLE  thread{};
  CONTEXT context{};
  bool    paused = false;
public:

  static V8BaseAdapter* AutoSelectVersion(HMODULE mod = 0);

  static HMODULE FindModule(std::string_view name = "");

  virtual void Setup(HMODULE mod) = 0;

  virtual void Shutdown() = 0;

  virtual ~V8BaseAdapter() = 0;
};

}
