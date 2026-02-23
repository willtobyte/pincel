#include "scriptable.hpp"

scriptable::~scriptable() noexcept {
  if (on_loop != LUA_NOREF) {
    luaL_unref(L, LUA_REGISTRYINDEX, on_loop);
  }
}
