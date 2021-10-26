#pragma once

#include "utility.hpp"

#include <deque>
#include <mutex>
#include <string>

namespace Fuel {
class CmdQueue {
public:
  typedef std::deque<std::string> List;
  static ThreadGuard<List> GetCmdList() {
    static List       list;
    static std::mutex m;
    return { &list, m };
  }
};
}