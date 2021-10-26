#pragma once

#include "base_adapter.hpp"

#include <polyhook2/Detour/ADetour.hpp>

namespace Fuel {

class V8Adapter6 : public V8BaseAdapter {
  bool isUp = false;
  PLH::Detour *detour;
public:
  void Setup(HMODULE mod) override;

  void Shutdown() override;
};

}
