#include "compat.hpp"

#ifdef HAS_LUAJIT

void compat_pushglobaltable(lua_State* L) {
  lua_pushvalue(L, LUA_GLOBALSINDEX);
}

void compat_replaceglobaltable(lua_State* L) {
  lua_replace(L, LUA_GLOBALSINDEX);
}

void compat_setfenv(lua_State* L, int idx) {
  lua_setfenv(L, idx);
}

void compat_getfield_global(lua_State* L, const char* name) {
  lua_getfield(L, LUA_GLOBALSINDEX, name);
}

void compat_setfield_global(lua_State* L, const char* name) {
  lua_setfield(L, LUA_GLOBALSINDEX, name);
}

#else

void compat_pushglobaltable(lua_State* L) {
  lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
}

void compat_replaceglobaltable(lua_State* L) {
  lua_rawseti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
}

void compat_setfenv(lua_State* L, int idx) {
  const char* name = lua_setupvalue(L, idx, 1);
  (void)name;
}

void compat_getfield_global(lua_State* L, const char* name) {
  lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
  lua_getfield(L, -1, name);
  lua_remove(L, -2);
}

void compat_setfield_global(lua_State* L, const char* name) {
  lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
  lua_insert(L, -2);
  lua_setfield(L, -2, name);
  lua_pop(L, 1);
}

#endif
