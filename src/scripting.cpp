#include "scripting.hpp"

void scripting::update(entt::registry& registry, float delta) {
  for (auto&& [entity, s] : registry.view<scriptable>().each()) {
    if (s.on_loop == LUA_NOREF) continue;

    lua_rawgeti(L, LUA_REGISTRYINDEX, s.on_loop);
    lua_pushnumber(L, static_cast<double>(delta));
    if (lua_pcall(L, 1, 0, 0) != 0) {
      std::string error = lua_tostring(L, -1);
      lua_pop(L, 1);
      throw std::runtime_error(error);
    }
  }
}
