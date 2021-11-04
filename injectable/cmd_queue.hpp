#pragma once

#include "utility.hpp"

#include <deque>
#include <mutex>
#include <string>

namespace Fuel {
  extern std::function<void(const std::string&)> SendBack;

class CmdQueue {
public:
  using List = std::deque<std::string>;
  using Callback = std::function<void(List &)>;

private:
  static Callback registeredCallback;

public:

  static ThreadGuard <List> GetCmdList() {
    static List       list;
    static std::mutex m;
    return {&list, m};
  }

  static void Unregister() {
    registeredCallback = {};
  }

  static void Register(Callback callback) {
    registeredCallback = std::move(callback);
  }

  static void Publish(std::string str) {
    auto list = GetCmdList();
    list->emplace_back(std::move(str));
    registeredCallback(*list);
  }
};
}