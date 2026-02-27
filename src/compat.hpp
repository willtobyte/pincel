#pragma once

#include <lua.hpp>
#include <lauxlib.h>

#ifdef HAS_LUAJIT
#include <luajit.h>
#else
#define lua_objlen(L, i) lua_rawlen(L, i)
#endif

void compat_pushglobaltable(lua_State* L);
void compat_replaceglobaltable(lua_State* L);
void compat_setfenv(lua_State* L, int idx);
void compat_getfield_global(lua_State* L, const char* name);
void compat_setfield_global(lua_State* L, const char* name);
