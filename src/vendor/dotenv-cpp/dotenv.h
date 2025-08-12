// Copyright 2025 WildSpark Authors

#pragma once

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <string>
#include <utility>  // Added for std::pair

namespace dotenv {

enum class Flags {
  None = 0,
  Preserve = 1 << 0,
  Expand = 1 << 1,
};

inline Flags operator|(const Flags& f1, const Flags& f2) {
  return static_cast<Flags>(static_cast<int>(f1) | static_cast<int>(f2));
}

inline bool operator&(const Flags& f1, const Flags& f2) {
  return (static_cast<int>(f1) & static_cast<int>(f2)) != 0;
}

namespace _detail {

inline std::string trim(const std::string& s) {
  const auto front = std::find_if_not(s.begin(), s.end(), ::isspace);
  const auto back = std::find_if_not(s.rbegin(), s.rend(), ::isspace);
  return std::string(front, back.base());
}

inline bool is_comment(const std::string& s) {
  return s.rfind("#", 0) == 0;
}

inline bool is_empty(const std::string& s) {
  return s.empty();
}

inline bool is_key_val(const std::string& s) {
  return s.find("=") != std::string::npos;
}

inline std::pair<std::string, std::string> parse_key_val(std::string s, const Flags& flags) {
  const auto pos = s.find("=");
  auto key = s.substr(0, pos);
  auto val = s.substr(pos + 1);

  if (!(flags & Flags::Preserve)) {
    key = trim(key);
    val = trim(val);
  }

  if (val.front() == '"' && val.back() == '"') {
    val = val.substr(1, val.length() - 2);
  } else if (val.front() == '\'' && val.back() == '\'') {
    val = val.substr(1, val.length() - 2);
  }

  return {key, val};
}

inline void set_env(const std::string& key, const std::string& value) {
#ifdef _MSC_VER
  _putenv_s(key.c_str(), value.c_str());
#else
  setenv(key.c_str(), value.c_str(), 1);
#endif
}

inline std::string resolve_variables(std::string val, const Flags& flags) {
  if (!(flags & Flags::Expand)) {
    return val;
  }

  size_t pos = 0;
  while ((pos = val.find('$', pos)) != std::string::npos) {
    if (pos > 0 && val[pos - 1] == '\\') {
      val.replace(pos - 1, 2, "$");
      continue;
    }

    std::string var_name;
    size_t start_pos;

    if (val[pos + 1] == '{') {
      start_pos = pos + 2;
      size_t end_pos = val.find('}', start_pos);
      if (end_pos == std::string::npos) {
        pos++;
        continue;
      }
      var_name = val.substr(start_pos, end_pos - start_pos);
      val.replace(pos, end_pos - pos + 1, std::getenv(var_name.c_str()) ? std::getenv(var_name.c_str()) : "");
    } else {
      start_pos = pos + 1;
      size_t end_pos = start_pos;
      while (end_pos < val.length() && (std::isalnum(val[end_pos]) || val[end_pos] == '_')) {
        end_pos++;
      }
      var_name = val.substr(start_pos, end_pos - start_pos);
      val.replace(pos, end_pos - pos, std::getenv(var_name.c_str()) ? std::getenv(var_name.c_str()) : "");
    }
  }
  return val;
}

}  // namespace _detail

inline void init(const std::string& path = ".env", const Flags& flags = Flags::None) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return;
  }

  std::string line;
  while (std::getline(file, line)) {
    line = _detail::trim(line);

    if (_detail::is_empty(line) || _detail::is_comment(line)) {
      continue;
    }

    if (_detail::is_key_val(line)) {
      auto [key, val] = _detail::parse_key_val(line, flags);
      if (!(flags & Flags::Preserve) || !std::getenv(key.c_str())) {
        val = _detail::resolve_variables(val, flags | Flags::Expand);
        _detail::set_env(key, val);
      }
    }
  }
}

inline void init(const Flags& flags) {  // Overload for just flags
  init(".env", flags);
}

inline std::string getenv(const char* name, const std::string& default_value = "") {  // Wrapper for std::getenv
  const char* value = std::getenv(name);
  return value ? value : default_value;
}

}  // namespace dotenv
