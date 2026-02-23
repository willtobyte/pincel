#pragma once

#include "common.hpp"

struct scriptable final {
  int on_loop{LUA_NOREF};

  scriptable() noexcept = default;
  ~scriptable() noexcept;

  scriptable(const scriptable&) = delete;
  scriptable& operator=(const scriptable&) = delete;

  scriptable(scriptable&& other) noexcept : on_loop(other.on_loop) {
    other.on_loop = LUA_NOREF;
  }

  scriptable& operator=(scriptable&& other) noexcept {
    if (this != &other) {
      if (on_loop != LUA_NOREF)
        luaL_unref(L, LUA_REGISTRYINDEX, on_loop);
      on_loop = other.on_loop;
      other.on_loop = LUA_NOREF;
    }
    return *this;
  }
};
