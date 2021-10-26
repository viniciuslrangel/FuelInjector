#include "v8_adapter_v6.hpp"

#include "../cmd_queue.hpp"

#include <polyhook2/ZydisDisassembler.hpp>

#if defined(__x86_64__) || defined(_WIN64)
#define ARCH(_32, _64) _64

#include <polyhook2/Detour/x64Detour.hpp>

#elif defined(__i386__) || defined(_WIN32)
#define ARCH(_32, _64) _32

#include <polyhook2/Detour/x86Detour.hpp>

#else
#error Arch not supported
#endif

#include "headers/6.0.286.52/v8.h"

HMODULE v8Module;

#define getProc(typ, name) \
  static auto target = forcedCast<decltype(&typ), void*>((void*)GetProcAddress(v8Module, name))

v8::Isolate *v8::Context::GetIsolate() {
  getProc(v8::Context::GetIsolate, "?GetIsolate@Context@v8@@QAEPAVIsolate@2@XZ");
  return (this->*target)();
}

v8::MaybeLocal<v8::String> v8::String::NewFromUtf8(v8::Isolate *isolate, const char *data, v8::NewStringType type, int length) {
  getProc(v8::String::NewFromUtf8, "?NewFromUtf8@String@v8@@SA?AV?$MaybeLocal@VString@v8@@@2@PAVIsolate@2@PBDW4NewStringType@2@H@Z");
  return target(isolate, data, type, length);
}

void v8::V8::ToLocalEmpty() {
  getProc(v8::V8::ToLocalEmpty, "?ToLocalEmpty@V8@v8@@CAXXZ");
  return target();
}

v8::MaybeLocal<v8::Script> v8::Script::Compile(v8::Local<v8::Context> context, v8::Local<v8::String> source, v8::ScriptOrigin *origin) {
  getProc(v8::Script::Compile,
          "?Compile@Script@v8@@SA?AV?$MaybeLocal@VScript@v8@@@2@V?$Local@VContext@v8@@@2@V?$Local@VString@v8@@@2@PAVScriptOrigin@2@@Z");
  return target(context, source, origin);
}

/*HOOK_DEF(v8::MaybeLocal<v8::Value>, V8ScriptRun, ARCH(__thiscall, ), v8::Script *this_, v8::Local<v8::Context> context);*/
decltype(&v8::Script::Run) v8ScriptRun_Original;
v8::MaybeLocal<v8::Value> v8::Script::Run(Local<Context> context) {
  {
    auto list = Fuel::CmdQueue::GetCmdList();

    static int count = 0;
    std::cout << "RUN " << count++ << std::endl;
    if (count > 25)
      for (const auto &code: *list) {
        const auto &str = v8::String::NewFromUtf8(context->GetIsolate(), code.c_str(), v8::NewStringType::kNormal);

        v8::Local<v8::String> src;
        v8::Local<v8::Script> script;
        if (str.ToLocal(&src) && v8::Script::Compile(context, src).ToLocal(&script)) {
          (this->*v8ScriptRun_Original)(context);
        }
      }
    list->clear();
  }

  return (this->*v8ScriptRun_Original)(context);
}

void Fuel::V8Adapter6::Setup(HMODULE mod) {
  if (isUp) {
    return;
  }
  isUp = true;

  v8Module = mod;

  PLH::Log::registerLogger(std::make_shared<PLH::ErrorLog>());

  // Hooking v8::Script::Run(v8::Context);
  auto runAddr = (decltype(&v8::Script::Run) *) GetProcAddress(
      mod,
      "?Run@Script@v8@@QAE?AV?$MaybeLocal@VValue@v8@@@2@V?$Local@VContext@v8@@@2@@Z"
  );

  if (runAddr == nullptr) {
    Shutdown();
    return;
  }

  std::cout << "\nAddress: " << std::hex << reinterpret_cast<size_t>(runAddr) << std::dec << std::endl;

  PLH::ZydisDisassembler dis(PLH::Mode::ARCH(x86, x64));
  detour = new ARCH(PLH::x86Detour, PLH::x64Detour)(
      reinterpret_cast<const char *>(runAddr),
      forcedCast<const char *>(&v8::Script::Run),
      forcedCast<uint64_t *>(&v8ScriptRun_Original),
      reinterpret_cast<PLH::ADisassembler &>(dis)
  );
  auto ok = detour->hook();
  std::cout << "Hooking status: " << ok << std::endl;
}

void Fuel::V8Adapter6::Shutdown() {
  if (!isUp) {
    return;
  }
  isUp = false;
  if (detour != nullptr) {
    detour->unHook();
    delete detour;
  }
}
