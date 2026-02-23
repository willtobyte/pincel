#include "scene.hpp"
#include "animator.hpp"
#include "presenter.hpp"

scene::scene(std::string_view name, compositor& compositor)
  : _compositor(compositor) {
  lua_pushvalue(L, LUA_GLOBALSINDEX);
  _G = luaL_ref(L, LUA_REGISTRYINDEX);

  lua_newtable(L);
  lua_newtable(L);
  lua_pushvalue(L, LUA_GLOBALSINDEX);
  lua_setfield(L, -2, "__index");
  lua_setmetatable(L, -2);
  _environment = luaL_ref(L, LUA_REGISTRYINDEX);

  lua_newtable(L);
  _pool = luaL_ref(L, LUA_REGISTRYINDEX);

  lua_rawgeti(L, LUA_REGISTRYINDEX, _environment);
  lua_rawgeti(L, LUA_REGISTRYINDEX, _pool);
  lua_setfield(L, -2, "pool");
  lua_pop(L, 1);

  const auto filename = std::format("scenes/{}.lua", name);
  const auto buffer = io::read(filename);
  const auto *data = reinterpret_cast<const char *>(buffer.data());
  const auto size = buffer.size();
  const auto label = std::format("@{}", filename);

  luaL_loadbuffer(L, data, size, label.c_str());

  lua_rawgeti(L, LUA_REGISTRYINDEX, _environment);
  lua_setfenv(L, -2);

  lua_pcall(L, 0, 1, 0);
  _table = luaL_ref(L, LUA_REGISTRYINDEX);
}

scene::~scene() noexcept {
  luaL_unref(L, LUA_REGISTRYINDEX, _table);
  luaL_unref(L, LUA_REGISTRYINDEX, _pool);
  luaL_unref(L, LUA_REGISTRYINDEX, _environment);
  luaL_unref(L, LUA_REGISTRYINDEX, _G);
}

void scene::on_enter() {
  object::create(_registry, _next_z++, "char");

  lua_rawgeti(L, LUA_REGISTRYINDEX, _environment);
  lua_replace(L, LUA_GLOBALSINDEX);

  lua_rawgeti(L, LUA_REGISTRYINDEX, _table);
  lua_getfield(L, -1, "on_enter");
  if (lua_isfunction(L, -1)) {
    lua_pcall(L, 0, 0, 0);
  } else {
    lua_pop(L, 1);
  }
  lua_pop(L, 1);
}

void scene::on_loop(float delta) {
  animator::update(_registry, delta);

  lua_rawgeti(L, LUA_REGISTRYINDEX, _table);
  lua_getfield(L, -1, "on_loop");
  if (lua_isfunction(L, -1)) {
    lua_pushnumber(L, static_cast<double>(delta));
    lua_pcall(L, 1, 0, 0);
  } else {
    lua_pop(L, 1);
  }
  lua_pop(L, 1);
}

void scene::on_draw() {
  presenter::update(_registry, _compositor);
}

void scene::on_leave() {
  lua_rawgeti(L, LUA_REGISTRYINDEX, _table);
  lua_getfield(L, -1, "on_leave");
  if (lua_isfunction(L, -1)) {
    lua_pcall(L, 0, 0, 0);
  } else {
    lua_pop(L, 1);
  }
  lua_pop(L, 1);

  lua_rawgeti(L, LUA_REGISTRYINDEX, _G);
  lua_replace(L, LUA_GLOBALSINDEX);
}
