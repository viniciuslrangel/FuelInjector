#pragma once

#include <string>
#include <vector>
#include <optional>
#include <algorithm>

struct FuncType {
  bool                     isConstructor;
  bool                     isConst;
  std::string              undecorated;
  std::string              symbol;
  std::string              className;
  std::string              methodName;
  std::string              modifier;
  std::string              name;
  std::string              ret;
  std::vector<std::string> args;
};

struct Filter {
  std::vector<std::pair<bool, std::string>> nameFilter;
  std::vector<std::pair<bool, std::string>> fullFilter;

  void Sort() {
    auto sort = [](auto &t) {
      std::sort(t.begin(), t.end(), [](const auto &a, const auto &b) { return a.second.size() < b.second.size(); });
    };
    sort(nameFilter);
    sort(fullFilter);
  }

  bool DoFilter(const FuncType &func) const {
    bool value = nameFilter.empty() && fullFilter.empty();

    for (const auto&[positive, item]: nameFilter) {
      std::string_view view(item.begin() + 1, item.end());
      if (func.name.find(view) != std::string_view::npos) {
        value = positive;
      }
    }

    for (const auto&[positive, item]: fullFilter) {
      std::string_view view(item.begin() + 1, item.end());
      if (func.undecorated.find(view) != std::string_view::npos) {
        value = positive;
      }
    }

    return value;
  }
};

std::optional<FuncType> parseFunctionName(const std::string &symbol, const Filter &filters);
