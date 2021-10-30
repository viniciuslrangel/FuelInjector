#include <Windows.h>
#include <dbghelp.h>

#include <ranges>
#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <sstream>
#include <format>
#include <algorithm>
#include <functional>

#include "parser.h"

BOOL CALLBACK EnumSymProc(
    PSYMBOL_INFO pSymInfo,
    ULONG SymbolSize,
    PVOID UserContext) {
  UNREFERENCED_PARAMETER(SymbolSize);

  auto list = (std::list<std::string> *) UserContext;

  auto required = SYMFLAG_EXPORT | SYMFLAG_FUNCTION;
  if ((pSymInfo->Flags & required) != 0) {
    list->emplace_front(pSymInfo->Name);
  }

  return TRUE;
}

void formatFunc(const FuncType &func, std::stringstream &output) {
  // language=C++
  static std::string
      funcTemplate = "{} {{\n  static const auto target = ({}) GetProcAddress(v8Module, \"{}\");\n  {} target({});\n}}\n\n";

  std::string typList;
  std::string params;
  std::string args;

  if (func.modifier == "__thiscall") {
    bool empty = func.args.empty();
    typList += func.className;
    typList += "*";
    typList += empty ? "" : ", ";
    args += "this";
    args += empty ? "" : ", ";
  }

  for (int i = 0; i < func.args.size(); i++) {
    bool        isLast = i == func.args.size() - 1;
    std::string name   = func.args[i];
    std::string p      = "param_" + std::to_string(i);

    typList += name;
    typList += (isLast ? "" : ", ");

    size_t cFuncPos = name.find("*)(");
    if (cFuncPos != std::string::npos) {
      std::string_view f1(name.begin(), name.begin() + cFuncPos + 1);
      std::string_view f2(name.begin() + cFuncPos + 1, name.end());
      params += f1;
      params += p;
      params += f2;
    } else {
      params += name;
      params += " ";
      params += p;
    }
    params += (isLast ? "" : ", ");

    args += p;
    args += (isLast ? "" : ", ");
  }

  std::string definition = func.ret + " " + func.name + "(" + params + ")";
  std::string type       = (func.isConstructor ? "void" : func.ret) + " (" + func.modifier + "*)(" + typList + ")";

  const char *hasReturn = func.isConstructor ? "" : "return";

  output << std::format(funcTemplate, definition, type, func.symbol, hasReturn, args);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " dll_path [+/- filters...]" << std::endl;
    return 3;
  }

  std::string dll_path   = argv[1];
  std::string baseName   = dll_path.substr(dll_path.find_last_of("/\\") + 1);
  std::string outputPath = std::string("./") + baseName + ".cpp";

  Filter   filter;
  for (int i             = 2; i < argc; ++i) {
    std::string_view value(argv[i]);
    if (value.size() > 1) {
      char f1 = value[0];
      char f2 = value[0];
      if (f1 == '+' || f1 == '-') {
        filter.nameFilter.emplace_back(f1 == '+', std::string(value.substr(1)));
      } else if (f1 == 'f') {
        filter.fullFilter.emplace_back(f2 == '+', std::string(value.substr(1)));
      }
    }
  }
  filter.Sort();

  HANDLE hProcess = GetCurrentProcess();

  SymSetOptions(SYMOPT_DEFERRED_LOADS);
  bool status = SymInitialize(hProcess, nullptr, FALSE);
  if (status == FALSE) {
    std::cerr << "Could not initialize DbgHelp (" << GetLastError() << ")" << std::endl;
    return 1;
  }

  DWORD64 BaseOfDll = SymLoadModuleEx(hProcess,
                                      nullptr,
                                      dll_path.c_str(),
                                      nullptr,
                                      0,
                                      0,
                                      nullptr,
                                      0);

  if (BaseOfDll == 0) {
    SymCleanup(hProcess);
    std::cerr << "Could not open the DLL (" << GetLastError() << ")" << std::endl;
    return 1;
  }

  std::list<std::string> symbols;

  bool success = SymEnumSymbols(hProcess,     // Process handle from SymInitialize.
                                BaseOfDll,   // Base address of module.
                                "*",        // Name of symbols to match.
                                EnumSymProc, // Symbol handler procedure.
                                &symbols);

  if (!success) {
    std::cerr << "Could not enumerate the symbols (" << GetLastError() << ")" << std::endl;
    return 1;
  }

  SymCleanup(hProcess);

  auto optionalFilter    = [](auto &&entry) { return entry.has_value(); };
  auto transform = [&filter](auto &&PH1) { return parseFunctionName(std::forward<decltype(PH1)>(PH1), filter); };

  std::stringstream total;

  total << "// Generated by Fuel Injector header_generator\n";
  total << "#include <Windows.h>\n";
  total << "#include \"v8.h\"\n\n";
  total << "extern HMODULE v8Module;\n\n\n";

  for (const auto &item: symbols
      | std::views::transform(transform)
      | std::views::filter(optionalFilter)
      | std::views::transform([](auto &&p) { return p.value(); })) {
    formatFunc(item, total);
  }
  std::cout << total.str() << std::endl;

  return 0;
}
