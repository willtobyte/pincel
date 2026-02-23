#include "object.hpp"
#include "io.hpp"
#include "scriptable.hpp"

namespace {
  entt::id_type hash(std::string_view sv) {
    return entt::hashed_string::value(sv.data(), sv.size());
  }
}

entt::entity object::create(entt::registry& registry, int16_t z, std::string_view name, float x, float y, std::string_view animation) {
  const auto entity = registry.create();
  registry.emplace<sorteable>(entity, sorteable{z});
  auto& r = registry.emplace<renderable>(entity);
  registry.emplace<transform>(entity, x, y);

  if (!animation.empty())
    r.animation = hash(animation);

  const auto filename = std::format("objects/{}.lua", name);
  const auto buffer = io::read(filename);
  const auto* data = reinterpret_cast<const char*>(buffer.data());
  const auto size = buffer.size();
  const auto label = std::format("@{}", filename);

  luaL_loadbuffer(L, data, size, label.c_str());
  lua_pcall(L, 0, 1, 0);
  assert(lua_istable(L, -1) && "objects lua must return a table");

  animatable an{};

  lua_pushnil(L);
  while (lua_next(L, -2) != 0) {
    const auto* key = lua_tostring(L, -2);

    if (std::strcmp(key, "atlas") == 0) {
      r.atlas = hash(lua_tostring(L, -1));
      lua_pop(L, 1);
      continue;
    }

    if (!lua_istable(L, -1)) {
      lua_pop(L, 1);
      continue;
    }

    struct animation a{};
    a.name = hash(key);

    const auto lenght = static_cast<uint32_t>(lua_objlen(L, -1));
    for (uint32_t i = 1; i <= lenght && a.count < a.keyframes.size(); ++i) {
      lua_rawgeti(L, -1, static_cast<int>(i));
      assert(lua_istable(L, -1) && "keyframe must be a table");

      lua_rawgeti(L, -1, 1);
      a.keyframes[a.count].sprite = static_cast<uint32_t>(lua_tonumber(L, -1));
      lua_pop(L, 1);

      lua_rawgeti(L, -1, 2);
      a.keyframes[a.count].duration = static_cast<uint32_t>(lua_tonumber(L, -1));
      lua_pop(L, 1);

      ++a.count;
      lua_pop(L, 1);
    }

    lua_getfield(L, -1, "next");
    if (lua_isstring(L, -1))
      a.next = hash(lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "once");
    if (lua_isboolean(L, -1))
      a.once = lua_toboolean(L, -1) != 0;
    lua_pop(L, 1);

    assert(an.count < an.animations.size() && "too many animations");
    an.animations[an.count++] = a;

    lua_pop(L, 1);
  }

  auto on_loop_ref = LUA_NOREF;

  lua_getfield(L, -1, "on_loop");
  if (lua_isfunction(L, -1))
    on_loop_ref = luaL_ref(L, LUA_REGISTRYINDEX);
  else
    lua_pop(L, 1);

  lua_pop(L, 1);

  registry.emplace<animatable>(entity, an.animations, an.count);
  auto& s = registry.emplace<scriptable>(entity);
  s.on_loop = on_loop_ref;
  registry.emplace<object>(entity);

  return entity;
}
