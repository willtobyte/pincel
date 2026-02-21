#include "cassette.hpp"

namespace {
constexpr std::string_view TYPE_NULL   = "null";
constexpr std::string_view TYPE_BOOL   = "bool";
constexpr std::string_view TYPE_INT64  = "int64";
constexpr std::string_view TYPE_UINT64 = "uint64";
constexpr std::string_view TYPE_DOUBLE = "double";
constexpr std::string_view TYPE_STRING = "string";

void encode_string_to(std::string_view str, std::string& out) {
  out.reserve(out.size() + str.size());
  for (const char c : str) {
    switch (c) {
      case '\n': out.append("\\n");  break;
      case '\r': out.append("\\r");  break;
      case '\\': out.append("\\\\"); break;
      case '\'': out.append("\\'");  break;
      default: out.push_back(c);   break;
    }
  }
}

std::string decode_string(std::string_view str) {
  if (str.find('\\') == std::string_view::npos) [[likely]] {
    return std::string(str);
  }

  std::string result;
  result.reserve(str.size());
  for (std::size_t index = 0; index < str.size(); ++index) {
    if (str[index] == '\\' && index + 1 < str.size()) {
      switch (str[index + 1]) {
        case 'n':  result += '\n'; ++index; break;
        case 'r':  result += '\r'; ++index; break;
        case '\\': result += '\\'; ++index; break;
        case '\'': result += '\''; ++index; break;
        default:   result += str[index];    break;
      }
    } else {
      result += str[index];
    }
  }

  return result;
}

bool parse(std::string_view line, std::string_view& type, std::string_view& key, std::string_view& value) {
  if (line.empty()) {
    return false;
  }

  const auto p = line.find(':');
  if (p == std::string_view::npos) {
    return false;
  }

  type = line.substr(0, p);

  const auto e = line.find('=', p + 1);
  if (e == std::string_view::npos) {
    return false;
  }

  key = line.substr(p + 1, e - p - 1);
  value = line.substr(e + 1);
  return !key.empty();
}
}

cassette::cassette() {
#ifdef EMSCRIPTEN
  const auto* const result = emscripten_run_script_string(std::format("localStorage.getItem('{}')", _storagekey).c_str());
  const std::string content{result ? result : ""};

  if (content.empty() || content == "null") {
    return;
  }
#else
  if (!std::filesystem::exists(_filename)) {
    return;
  }

  std::ifstream file(_filename, std::ios::binary | std::ios::ate);
  if (!file) {
    return;
  }

  const auto size = file.tellg();
  file.seekg(0);
  std::string content(static_cast<std::size_t>(size), '\0');
  file.read(content.data(), size);
#endif

  std::string_view remaining{content};
  while (!remaining.empty()) {
    const auto newline_pos = remaining.find('\n');
    const auto line = remaining.substr(0, newline_pos);
    remaining = (newline_pos == std::string_view::npos) ? std::string_view{} : remaining.substr(newline_pos + 1);

    if (line.empty()) {
      continue;
    }

    std::string_view type;
    std::string_view key, value;
    if (!parse(line, type, key, value)) {
      continue;
    }

    if (type == TYPE_NULL) {
      _data.emplace(std::string(key), nullptr);
    } else if (type == TYPE_BOOL) {
      _data.emplace(std::string(key), value == "1");
    } else if (type == TYPE_INT64) {
      int64_t v{};
      if (auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), v); ec == std::errc{}) {
        _data.emplace(std::string(key), v);
      }
    } else if (type == TYPE_UINT64) {
      uint64_t v{};
      if (auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), v); ec == std::errc{}) {
        _data.emplace(std::string(key), v);
      }
    } else if (type == TYPE_DOUBLE) {
      char* end{};
      const auto v = std::strtod(value.data(), &end);
      if (end != value.data() && end == value.data() + value.size()) {
        _data.emplace(std::string(key), v);
      }
    } else if (type == TYPE_STRING) {
      _data.emplace(std::string(key), decode_string(value));
    }
  }
}

void cassette::persist() const {
  std::string buffer;
  buffer.reserve(_data.size() * 64);

  for (const auto& [key, value] : _data) {
    std::visit([&key, &buffer](const auto& v) {
      using T = std::decay_t<decltype(v)>;

      if constexpr (std::is_same_v<T, std::nullptr_t>) {
        std::format_to(std::back_inserter(buffer), "{}:{}=\n", TYPE_NULL, key);
      } else if constexpr (std::is_same_v<T, bool>) {
        std::format_to(std::back_inserter(buffer), "{}:{}={}\n", TYPE_BOOL, key, v ? '1' : '0');
      } else if constexpr (std::is_same_v<T, int64_t>) {
        std::format_to(std::back_inserter(buffer), "{}:{}={}\n", TYPE_INT64, key, v);
      } else if constexpr (std::is_same_v<T, uint64_t>) {
        std::format_to(std::back_inserter(buffer), "{}:{}={}\n", TYPE_UINT64, key, v);
      } else if constexpr (std::is_same_v<T, double>) {
        std::format_to(std::back_inserter(buffer), "{}:{}={}\n", TYPE_DOUBLE, key, v);
      } else if constexpr (std::is_same_v<T, std::string>) {
        std::format_to(std::back_inserter(buffer), "{}:{}=", TYPE_STRING, key);
        encode_string_to(v, buffer);
        buffer.push_back('\n');
      }
    }, value);
  }

  std::ofstream file(_filename, std::ios::binary | std::ios::trunc);
  file.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
}

void cassette::clear(std::string_view key) {
  if (key.empty()) [[unlikely]] {
    return;
  }

  if (const auto it = _data.find(key); it != _data.end()) {
    _data.erase(it);
    persist();
  }
}

void cassette::clear() {
  _data.clear();
  persist();
}

std::optional<cassette::value_type> cassette::find(std::string_view key) const noexcept {
  const auto it = _data.find(key);
  if (it == _data.end()) {
    return std::nullopt;
  }

  return it->second;
}

static cassette instance;

static int cassette_set(lua_State *state) {
  const auto *key = luaL_checkstring(state, 2);

  switch (lua_type(state, 3)) {
    case LUA_TNIL:
      instance.set(key, nullptr);
      break;
    case LUA_TBOOLEAN:
      instance.set(key, lua_toboolean(state, 3) != 0);
      break;
    case LUA_TNUMBER: {
      const auto n = lua_tonumber(state, 3);
      const auto i = static_cast<int64_t>(n);
      if (static_cast<double>(i) == n) {
        instance.set(key, i);
      } else {
        instance.set(key, n);
      }
      break;
    }
    case LUA_TSTRING:
      instance.set<std::string_view>(key, lua_tostring(state, 3));
      break;
    default:
      return luaL_error(state, "cassette:set unsupported value type");
  }

  return 0;
}

static int cassette_get(lua_State *state) {
  const auto *key = luaL_checkstring(state, 2);

  const auto result = instance.find(key);
  if (!result) {
    lua_pushvalue(state, 3);
    return 1;
  }

  std::visit([state](const auto &v) {
    using T = std::decay_t<decltype(v)>;

    if constexpr (std::is_same_v<T, std::nullptr_t>) {
      lua_pushnil(state);
    } else if constexpr (std::is_same_v<T, bool>) {
      lua_pushboolean(state, v);
    } else if constexpr (std::is_same_v<T, int64_t>) {
      lua_pushinteger(state, static_cast<lua_Integer>(v));
    } else if constexpr (std::is_same_v<T, uint64_t>) {
      lua_pushinteger(state, static_cast<lua_Integer>(v));
    } else if constexpr (std::is_same_v<T, double>) {
      lua_pushnumber(state, v);
    } else if constexpr (std::is_same_v<T, std::string>) {
      lua_pushlstring(state, v.data(), v.size());
    }
  }, *result);

  return 1;
}

static int cassette_clear(lua_State *state) {
  if (lua_gettop(state) >= 2 && lua_type(state, 2) == LUA_TSTRING) {
    instance.clear(lua_tostring(state, 2));
  } else {
    instance.clear();
  }

  return 0;
}

static const luaL_Reg functions[] = {
  {"set", cassette_set},
  {"get", cassette_get},
  {"clear", cassette_clear},
  {nullptr, nullptr}
};

void cassette::wire() {
  luaL_register(L, "cassette", functions);
  lua_pop(L, 1);
}
