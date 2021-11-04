#pragma once

#include "base_adapter.hpp"

#include <polyhook2/Detour/ADetour.hpp>

#include "headers/6.0.286.52/v8.h"
#include "../cmd_queue.hpp"

namespace Fuel {

class V8Adapter6 : public V8BaseAdapter {
public:

  void Setup(HMODULE mod) override;

  void Shutdown() override;

  void RunCode(Fuel::CmdQueue::List &list);

private:

  friend v8::MaybeLocal<v8::Value> __thiscall v8_Script_Run(v8::Script *_this, v8::Local<v8::Context> context);

  void createLibs();

  bool        isUp = false;
  PLH::Detour *detour;

  v8::Isolate *isolate = nullptr;
  v8::Persistent<v8::Context> _context;
  v8::Persistent<v8::Function> eval;
};

}
