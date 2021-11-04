#include "v8_adapter_v6.hpp"

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

HMODULE v8Module;

namespace Js {

static const char *ToCString(const v8::String::Utf8Value &value) {
  return *value ? *value : "<string conversion failed>";
}

static v8::MaybeLocal<v8::String> FromCString(v8::Isolate *isolate, const char *data) {
  return v8::String::NewFromUtf8(isolate, data, v8::NewStringType::kNormal);
}

static const char *ValueToString(v8::Local<v8::Value> value) {
  v8::String::Utf8Value str(value);
  return ToCString(str);
}

static void Print(const v8::FunctionCallbackInfo<v8::Value> &args) {
  bool     first = true;
  for (int i     = 0; i < args.Length(); i++) {
    v8::HandleScope handle_scope(args.GetIsolate());
    if (first) {
      first = false;
    } else {
      std::cout << " ";
    }
    const char *cstr = ValueToString(args[i]);
    std::cout << cstr;
  }
  std::cout << '\n' << std::endl;
}

}

Fuel::V8Adapter6 *instance = nullptr;

decltype(&v8::Script::Run) v8_Script_Run_Tramp;
v8::MaybeLocal<v8::Value> __thiscall Fuel::v8_Script_Run(v8::Script *_this, v8::Local<v8::Context> context) {

  DEFER(
      if (instance != nullptr && (*instance).isolate == nullptr) {
        instance->isolate = context->GetIsolate();
        instance->_context.Reset(context->GetIsolate(), context);
        instance->createLibs();
        instance->detour->unHook();
        delete instance->detour;
      }
  );

  return (_this->*v8_Script_Run_Tramp)(context);
}

void Fuel::V8Adapter6::Setup(HMODULE mod) {
  if (isUp) {
    return;
  }
  isUp = true;

  v8Module = mod;
  instance = this;

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
      forcedCast<const char *>(&v8_Script_Run),
      forcedCast<uint64_t *>(&v8_Script_Run_Tramp),
      reinterpret_cast<PLH::ADisassembler &>(dis)
  );
  auto ok = detour->hook();
  std::cout << "Hooking status: " << ok << std::endl;

  Fuel::CmdQueue::Register([this](auto &&l) { this->RunCode(l); });
}

void Fuel::V8Adapter6::Shutdown() {
  if (!isUp) {
    return;
  }
  isUp = false;

  Fuel::CmdQueue::Unregister();
  _context.Reset();
  if (detour != nullptr) {
    detour->unHook();
    delete detour;
  }
}

void Fuel::V8Adapter6::RunCode(Fuel::CmdQueue::List &list) {
  if (isolate == nullptr) {
    return;
  }
  std::string result;
  DEFER(
      if (!result.empty()) {
        Fuel::SendBack(result);
      }
  );

  v8::Locker locker(isolate);

  v8::Isolate::Scope     isolateScope(isolate);
  v8::HandleScope        handle_scope(isolate);
  v8::Local<v8::Context> context = this->_context.Get(isolate);
  v8::Context::Scope     scope(context);

  auto begin = list.begin();
  while (begin != list.end()) {
    std::string code = std::move(*begin);
    begin = list.erase(begin);

    v8::TryCatch tryCatch(isolate);

    v8::Local<v8::String> src    = Js::FromCString(isolate, code.c_str()).ToLocalChecked();
    v8::Local<v8::Script> script = v8::Script::Compile(context, src).ToLocalChecked();

    auto ret = script->Run(context);
    if (tryCatch.HasCaught()) {
      v8::Local<v8::Value>      exception  = tryCatch.Exception();
      v8::Local<v8::Message>    message    = tryCatch.Message();
      v8::Local<v8::StackTrace> stackTrace = message->GetStackTrace();

      std::stringstream ss;
      ss << "Uncaught " << Js::ValueToString(exception) << '\n';
      for (int i = 0; i < stackTrace->GetFrameCount(); i++) {
        v8::Local<v8::StackFrame> frame = stackTrace->GetFrame(i);
        ss << std::format(
            "  at {} ({}:{}:{})\n",
            Js::ValueToString(frame->GetFunctionName()),
            Js::ValueToString(frame->GetScriptNameOrSourceURL()),
            frame->GetLineNumber(),
            frame->GetColumn()
        );
      }
      result = ss.str();
    } else if (ret.IsEmpty()) {
      result = "<empty>";
    } else {
      result = Js::ValueToString(ret.ToLocalChecked());
    }
  }
}

void Fuel::V8Adapter6::createLibs() {
  v8::HandleScope        handle_scope(isolate);
  v8::Local<v8::Context> context = _context.Get(isolate);
  v8::Local<v8::Object>  global  = context->Global();

  global->Set(
      context,
      Js::FromCString(isolate, "fuel_print").ToLocalChecked(),
      v8::Function::New(isolate, &Js::Print)
  );
}
