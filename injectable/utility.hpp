#pragma once

#include <Windows.h>

#include <functional>
#include <utility>
#include <mutex>

#define CONCAT(x, y) x##y
#define CONCAT2(x, y) CONCAT(x, y)

#define DEFER(x) \
  ::Fuel::Defer CONCAT2(_defer_, __COUNTER__)([&]() { \
    x            \
  })

#ifdef FUEL_DEBUG

#define DEBUG(x) x
#define NON_DEBUG(x)

#else

#define DEBUG(x)
#define NON_DEBUG(x) x

#endif

namespace Fuel {

template<typename R, typename T>
R forcedCast(T t)
{
  union
  {
    T t;
    R r;
  } v;
  v.t = t;
  return v.r;
}

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
