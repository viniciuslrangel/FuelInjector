#pragma once

#include <functional>
#include <utility>
#include <mutex>

namespace Fuel {

class Defer {
  typedef std::function<void()> F;

  F f;
public:

  explicit Defer(F f) : f(std::move(f)) {}

  ~Defer() { f(); }
};

template<typename T>
class ThreadGuard {
  T          *target;
  std::mutex &m;

public:
  ThreadGuard(T *target, std::mutex &m) : target(target), m(m) {
    m.lock();
  }

  ~ThreadGuard() {
    m.unlock();
  }

  T &operator*() const {
    return *target;
  }
  T *operator->() const {
    return target;
  }
};

}

#define CONCAT(x, y) x##y
#define CONCAT2(x, y) CONCAT(x, y)
#define HOOK_DEF(ret, name, conv, ...) \
ret (conv *CONCAT(name, _original))(__VA_ARGS__); \
ret name(__VA_ARGS__)

#define DEFER(x) \
  ::Fuel::Defer CONCAT2(_defer_, __COUNTER__)([&]() { \
    x            \
  });
