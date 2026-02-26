#include "soundsystem.hpp"

namespace {
  int sound_play(lua_State* state) {
    auto* proxy = static_cast<soundproxy*>(luaL_checkudata(state, 1, "Sound"));
    proxy->fx->play();
    return 0;
  }

  int sound_stop(lua_State* state) {
    auto* proxy = static_cast<soundproxy*>(luaL_checkudata(state, 1, "Sound"));
    proxy->fx->stop();
    return 0;
  }

  int sound_index(lua_State* state) {
    auto* proxy = static_cast<soundproxy*>(luaL_checkudata(state, 1, "Sound"));
    const std::string_view key = luaL_checkstring(state, 2);

    if (key == "volume") {
      lua_pushnumber(state, static_cast<double>(proxy->fx->volume()));
      return 1;
    }

    if (key == "loop") {
      lua_pushboolean(state, proxy->fx->loop());
      return 1;
    }

    if (key == "play") {
      lua_pushcfunction(state, sound_play);
      return 1;
    }

    if (key == "stop") {
      lua_pushcfunction(state, sound_stop);
      return 1;
    }

    if (key == "on_start") {
      if (proxy->on_start != LUA_NOREF)
        lua_rawgeti(state, LUA_REGISTRYINDEX, proxy->on_start);
      else
        lua_pushnil(state);
      return 1;
    }

    if (key == "on_end") {
      if (proxy->on_end != LUA_NOREF)
        lua_rawgeti(state, LUA_REGISTRYINDEX, proxy->on_end);
      else
        lua_pushnil(state);
      return 1;
    }

    return lua_pushnil(state), 1;
  }

  int sound_newindex(lua_State* state) {
    auto* proxy = static_cast<soundproxy*>(luaL_checkudata(state, 1, "Sound"));
    const std::string_view key = luaL_checkstring(state, 2);

    if (key == "volume") {
      proxy->fx->set_volume(static_cast<float>(luaL_checknumber(state, 3)));
      return 0;
    }

    if (key == "loop") {
      proxy->fx->set_loop(lua_toboolean(state, 3) != 0);
      return 0;
    }

    if (key == "on_start") {
      if (proxy->on_start != LUA_NOREF)
        luaL_unref(state, LUA_REGISTRYINDEX, proxy->on_start);

      if (lua_isfunction(state, 3)) {
        lua_pushvalue(state, 3);
        proxy->on_start = luaL_ref(state, LUA_REGISTRYINDEX);
      } else {
        proxy->on_start = LUA_NOREF;
      }
      return 0;
    }

    if (key == "on_end") {
      if (proxy->on_end != LUA_NOREF)
        luaL_unref(state, LUA_REGISTRYINDEX, proxy->on_end);

      if (lua_isfunction(state, 3)) {
        lua_pushvalue(state, 3);
        proxy->on_end = luaL_ref(state, LUA_REGISTRYINDEX);
      } else {
        proxy->on_end = LUA_NOREF;
      }
      return 0;
    }

    return 0;
  }

  int sound_gc(lua_State* state) {
    auto* proxy = static_cast<soundproxy*>(luaL_checkudata(state, 1, "Sound"));

    if (proxy->on_start != LUA_NOREF)
      luaL_unref(state, LUA_REGISTRYINDEX, proxy->on_start);
    if (proxy->on_end != LUA_NOREF)
      luaL_unref(state, LUA_REGISTRYINDEX, proxy->on_end);

    proxy->~soundproxy();
    return 0;
  }
}

static void init() {
  luaL_newmetatable(L, "Sound");

  lua_pushcfunction(L, sound_index);
  lua_setfield(L, -2, "__index");

  lua_pushcfunction(L, sound_newindex);
  lua_setfield(L, -2, "__newindex");

  lua_pushcfunction(L, sound_gc);
  lua_setfield(L, -2, "__gc");

  lua_pop(L, 1);
}

void soundsystem::wire() {
  static std::once_flag once;
  std::call_once(once, init);
}

void soundsystem::dispatch(int pool, const std::vector<std::string>& names, soundregistry& registry) {
  for (const auto& name : names) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, pool);
    lua_getfield(L, -1, name.c_str());

    if (!lua_isuserdata(L, -1)) {
      lua_pop(L, 2);
      continue;
    }

    auto* proxy = static_cast<soundproxy*>(lua_touserdata(L, -1));

    const auto did_start = proxy->fx->started();
    const auto did_end = proxy->fx->ended();

    if (!did_start && !did_end) {
      lua_pop(L, 2);
      continue;
    }

    if (did_start && proxy->on_start != LUA_NOREF) {
      lua_rawgeti(L, LUA_REGISTRYINDEX, proxy->on_start);
      lua_pushvalue(L, -2);
      if (lua_pcall(L, 1, 0, 0) != 0) {
        std::string error = lua_tostring(L, -1);
        lua_pop(L, 3);
        throw std::runtime_error(error);
      }
    }

    if (did_end && proxy->on_end != LUA_NOREF) {
      lua_rawgeti(L, LUA_REGISTRYINDEX, proxy->on_end);
      lua_pushvalue(L, -2);
      if (lua_pcall(L, 1, 0, 0) != 0) {
        std::string error = lua_tostring(L, -1);
        lua_pop(L, 3);
        throw std::runtime_error(error);
      }
    }

    lua_pop(L, 2);
  }
}
