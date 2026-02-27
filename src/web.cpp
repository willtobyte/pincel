#include "web.hpp"

static int openurl(lua_State *state) {
  const auto *url = luaL_checkstring(state, 1);

#ifdef EMSCRIPTEN
  const auto script = std::format(R"javascript(window.open('{}', '_blank', 'noopener,noreferrer');)javascript", url);
  emscripten_run_script(script.c_str());
#else
  SDL_OpenURL(url);
#endif

  return 0;
}

static int queryparam(lua_State *state) {
  const auto *key = luaL_checkstring(state, 1);
  const auto *defval = luaL_optstring(state, 2, "");

#ifdef EMSCRIPTEN
  const auto script = std::format(
      R"javascript(
        new URLSearchParams(location.search).get("{}") ?? "{}"
      )javascript",
      key,
      defval
  );

  if (const auto *result = emscripten_run_script_string(script.c_str()); result && *result) {
    lua_pushstring(state, result);
    return 1;
  }
#else
  std::array<char, 64> uppercase_key{};
  const auto len = std::min(std::strlen(key), uppercase_key.size() - 1);
  for (std::size_t i = 0; i < len; ++i) {
    uppercase_key[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(key[i])));
  }

  if (const auto *value = std::getenv(uppercase_key.data()); value && *value) {
    lua_pushstring(state, value);
    return 1;
  }
#endif

  lua_pushstring(state, defval);
  return 1;
}

void web::wire() {
  lua_pushcfunction(L, openurl);
  lua_setglobal(L, "openurl");

  lua_pushcfunction(L, queryparam);
  lua_setglobal(L, "queryparam");
}
