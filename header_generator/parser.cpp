#include "parser.h"

#include <sstream>

#include <Windows.h>
#include <dbghelp.h>

#define GET_OR_ELSE(var, expr, r) \
    {                             \
        auto _v = (expr);         \
        if (!_v)                  \
        {                         \
            r;                    \
        }                         \
        (var) = *_v;              \
    }                             \
    (void)0

std::optional<std::string> parseIdentifier(const std::string &value) {
  if (value.empty()) {
    return std::nullopt;
  }

  size_t size = value.size();

  int modCount = 0;

  for (int i = 0; i < size; i++) {
    char c = value[i];
    if ((c == ' ' || c == ',') && modCount == 0) {
      auto v = value.substr(0, i);
      if (v == "class" || v == "struct" || v == "enum" || v == "unsigned") {
        continue;
      }
      if (i + 1 < size && (value[i + 1] == '*' || value[i + 1] == '&')) {
        i++;
        continue;
      }
      if (i + 6 < size && value.substr(i + 1, 6) == "const ") {
        i += 5;
        continue;
      }

      return std::make_optional(v);
    }
    if (c == '<' || c == '(') {
      modCount++;
      continue;
    }
    if (c == '>' || c == ')') {
      modCount--;
      continue;
    }
  }

  return std::make_optional(value);
}

std::optional<FuncType> parseFuncType(const std::string &value) {
  size_t pOpen                             = value.find_first_of('(');
  size_t pClose                            = value.find_last_of(')');
  if (pOpen == std::string::npos || pClose == std::string::npos) {
    return std::nullopt;
  }

  FuncType                 result;
  bool                     &isConstructor  = result.isConstructor;
  bool                     &isConst        = result.isConst;
  std::string              &modifier       = result.modifier;
  std::string              &fullIdentifier = result.name;
  std::string              &retType        = result.ret;
  std::vector<std::string> &args           = result.args;

  // Parse return type

  GET_OR_ELSE(retType, parseIdentifier(value.substr(0, pOpen)), return std::nullopt);

  bool paramSkip = false;

  if (retType == "__thiscall" || retType == "__cdecl") {
    isConstructor = true;
    std::swap(retType, modifier);
    GET_OR_ELSE(
        fullIdentifier,
        parseIdentifier(value.substr(modifier.size() + 1, pOpen - modifier.size() - 1)),
        return std::nullopt
    );
    paramSkip = true;
  }

  if (!paramSkip) {
    // Parse name
    size_t start = retType.size() + 1;
    GET_OR_ELSE(fullIdentifier, parseIdentifier(value.substr(start, pOpen - start)), return std::nullopt);
    if (fullIdentifier.size() + retType.size() == pOpen - 1) {
      paramSkip = true;
    }
  }

  if (!paramSkip) {
    std::swap(modifier, fullIdentifier);
    size_t start = retType.size() + modifier.size() + 2;
    GET_OR_ELSE(fullIdentifier, parseIdentifier(value.substr(start, pOpen - start)), return std::nullopt);
  }

  // Parse parameters

  size_t      length           = pClose;
  size_t      begin            = pOpen + 1;
  int         parenthesisCount = 0;
  for (size_t i                = begin; i < length; ++i) {
    char c = value[i];
    if (c == '<' || c == '(') {
      parenthesisCount++;
    } else if (c == '>' || c == ')') {
      parenthesisCount--;
    } else if (c == ',' && parenthesisCount == 0) {
      args.push_back(value.substr(begin, i - begin));
      i++;
      begin = i;
    }
  }
  if (begin < pClose - 1) {
    args.push_back(value.substr(begin, pClose - begin));
  }
  if (!args.empty() && args[0] == "void") {
    args.pop_back();
  }

  size_t separator = fullIdentifier.rfind("::");
  if (separator == std::string::npos) {
    return std::nullopt;
  }
  result.className  = fullIdentifier.substr(0, separator);
  result.methodName = fullIdentifier.substr(separator + 2);

  if (result.methodName.starts_with("operator") && !args.empty()) {
    std::string_view p0 = args[0];
    if (p0.starts_with("class") || p0.starts_with("struct") || p0.starts_with("enum")) {
      p0 = p0.substr(p0.find_first_of(' ') + 1);
    }
    if (p0.starts_with(result.className)) {
      return std::nullopt;
    }
  }

  isConst = value.ends_with("const");

  return std::make_optional(result);
}

std::optional<FuncType> parseFunctionName(const std::string &symbol, const Filter &filters) {

  if (symbol.empty()) {
    return std::nullopt;
  }

  char buf[512] = {0};
  UnDecorateSymbolName(symbol.c_str(), buf, sizeof(buf),
                       UNDNAME_NO_ACCESS_SPECIFIERS | UNDNAME_NO_MEMBER_TYPE);

  std::string undecorated = buf;
  const char* ws = " \t\n\r\f\v";
  undecorated.erase(undecorated.find_last_not_of(ws) + 1);
  undecorated.erase(0, undecorated.find_first_not_of(ws));

  if (undecorated.ends_with(']')) {
    return std::nullopt;
  }

  if (undecorated.find('(') == std::string::npos || !undecorated.find(')')) {
    return std::nullopt;
  }

  std::optional<FuncType> f = parseFuncType(undecorated);
  if (!f.has_value()) {
    return std::nullopt;
  }

  f->undecorated = std::move(undecorated);
  f->symbol      = symbol;

  bool ok = filters.DoFilter(*f);

  if (ok) {
    return f;
  }

  return std::nullopt;
}

