#include "object.hpp"
#include "collidable.hpp"
#include "identifiable.hpp"
#include "renderable.hpp"
#include "scriptable.hpp"
#include "sorteable.hpp"
#include "transform.hpp"
#include "dirtable.hpp"
#include "atlasregistry.hpp"

namespace {
  constexpr std::size_t lookup_capacity = 1024;

  constexpr int field_sprite = 1;
  constexpr int field_duration = 2;

  struct objectproxy final {
    entt::registry* registry;
    entt::entity entity;
    int object_ref{LUA_NOREF};
    entt::id_type name{};

    objectproxy(entt::registry& registry, entt::entity entity, int object_ref, entt::id_type name) noexcept
        : registry(&registry), entity(entity), object_ref(object_ref), name(name) {}

    ~objectproxy() noexcept = default;
  };

  entt::id_type hash(std::string_view value) {
    return entt::hashed_string::value(value.data(), value.size());
  }

  void detach_shape(collidable& c) {
    if (b2Shape_IsValid(c.shape)) {
      b2DestroyShape(c.shape, false);
      c.shape = b2ShapeId{};
    }
    c.hx = {};
    c.hy = {};
    c.hw = {};
    c.hh = {};
  }

  int object_index(lua_State* state) {
    auto* proxy = static_cast<objectproxy*>(luaL_checkudata(state, 1, "Object"));
    const std::string_view key = luaL_checkstring(state, 2);
    auto& registry = *proxy->registry;
    const auto entity = proxy->entity;

    if (key == "x") {
      lua_pushnumber(state, static_cast<double>(registry.get<transform>(entity).x));
      return 1;
    }

    if (key == "y") {
      lua_pushnumber(state, static_cast<double>(registry.get<transform>(entity).y));
      return 1;
    }

    if (key == "z") {
      lua_pushnumber(state, static_cast<double>(registry.get<sorteable>(entity).z));
      return 1;
    }

    if (key == "scale") {
      lua_pushnumber(state, static_cast<double>(registry.get<transform>(entity).scale));
      return 1;
    }

    if (key == "angle") {
      lua_pushnumber(state, static_cast<double>(registry.get<transform>(entity).angle));
      return 1;
    }

    if (key == "alpha") {
      lua_pushnumber(state, static_cast<double>(registry.get<transform>(entity).alpha));
      return 1;
    }

    if (key == "shown") {
      lua_pushboolean(state, registry.get<transform>(entity).shown);
      return 1;
    }

    if (key == "animation") {
      const auto& r = registry.get<renderable>(entity);
      const auto it = lookup.find(r.animation);
      if (it == lookup.end())
        return lua_pushnil(state), 1;
      lua_pushstring(state, it->second.c_str());
      return 1;
    }

    if (key == "name") {
      const auto& id = registry.get<identifiable>(entity);
      const auto it = lookup.find(id.name);
      if (it == lookup.end())
        return lua_pushnil(state), 1;
      lua_pushstring(state, it->second.c_str());
      return 1;
    }

    if (key == "kind") {
      const auto& id = registry.get<identifiable>(entity);
      const auto it = lookup.find(id.kind);
      if (it == lookup.end())
        return lua_pushnil(state), 1;
      lua_pushstring(state, it->second.c_str());
      return 1;
    }

    assert(proxy->object_ref != LUA_NOREF && "object must have an object ref");

    lua_rawgeti(state, LUA_REGISTRYINDEX, proxy->object_ref);
    lua_getfield(state, -1, key.data());
    lua_remove(state, -2);
    if (!lua_isnil(state, -1))
      return 1;
    lua_pop(state, 1);

    lua_rawgeti(state, LUA_REGISTRYINDEX, proxy->object_ref);
    static char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "on_%.*s", static_cast<int>(key.size()), key.data());
    lua_getfield(state, -1, buffer);
    lua_remove(state, -2);
    if (lua_isfunction(state, -1))
      return 1;
    lua_pop(state, 1);

    return lua_pushnil(state), 1;
  }

  int object_newindex(lua_State* state) {
    auto* proxy = static_cast<objectproxy*>(luaL_checkudata(state, 1, "Object"));
    const std::string_view key = luaL_checkstring(state, 2);
    auto& registry = *proxy->registry;
    const auto entity = proxy->entity;

    if (key == "x") {
      auto& t = registry.get<transform>(entity);
      t.x = static_cast<float>(luaL_checknumber(state, 3));

      auto* c = registry.try_get<collidable>(entity);
      if (c && b2Body_IsValid(c->body))
        b2Body_SetTransform(c->body, {t.x + c->ox, t.y + c->oy}, b2Rot_identity);

      return 0;
    }

    if (key == "y") {
      auto& t = registry.get<transform>(entity);
      t.y = static_cast<float>(luaL_checknumber(state, 3));

      auto* c = registry.try_get<collidable>(entity);
      if (c && b2Body_IsValid(c->body))
        b2Body_SetTransform(c->body, {t.x + c->ox, t.y + c->oy}, b2Rot_identity);

      return 0;
    }

    if (key == "z") {
      auto& sorteable = registry.get<::sorteable>(entity);
      const auto value = static_cast<int16_t>(luaL_checknumber(state, 3));
      if (sorteable.z != value) {
        sorteable.z = value;
        registry.ctx().get<dirtable>().mark(dirtable::sort);
      }
      return 0;
    }

    if (key == "scale") {
      registry.get<transform>(entity).scale = static_cast<float>(luaL_checknumber(state, 3));
      return 0;
    }

    if (key == "angle") {
      registry.get<transform>(entity).angle = static_cast<float>(luaL_checknumber(state, 3));
      return 0;
    }

    if (key == "alpha") {
      registry.get<transform>(entity).alpha = static_cast<uint8_t>(luaL_checknumber(state, 3));
      return 0;
    }

    if (key == "shown") {
      registry.get<transform>(entity).shown = lua_toboolean(state, 3) != 0;
      return 0;
    }

    if (key == "animation") {
      auto& r = registry.get<renderable>(entity);
      const auto value = luaL_checkstring(state, 3);
      const auto id = hash(value);
      r.animation = id;
      lookup.emplace(id, value);
      r.current_frame = 0;
      r.counter = 0;

      const auto& a = registry.get<animatable>(entity);
      for (uint32_t i = 0; i < a.count; ++i) {
        if (a.animations[i].name != id) continue;
        r.atlas = a.animations[i].atlas;
        break;
      }

      return 0;
    }

    assert(proxy->object_ref != LUA_NOREF && "object must have an object ref");

    lua_rawgeti(state, LUA_REGISTRYINDEX, proxy->object_ref);
    lua_pushvalue(state, 3);
    lua_setfield(state, -2, key.data());
    lua_pop(state, 1);
    return 0;
  }

  int object_gc(lua_State* state) {
    auto* proxy = static_cast<objectproxy*>(luaL_checkudata(state, 1, "Object"));
    if (proxy->object_ref != LUA_NOREF)
      luaL_unref(state, LUA_REGISTRYINDEX, proxy->object_ref);
    proxy->~objectproxy();
    return 0;
  }

  void on_destroy_scriptable(entt::registry& registry, entt::entity entity) {
    auto& s = registry.get<::scriptable>(entity);
    if (s.on_spawn != LUA_NOREF)
      luaL_unref(L, LUA_REGISTRYINDEX, s.on_spawn);
    if (s.on_loop != LUA_NOREF)
      luaL_unref(L, LUA_REGISTRYINDEX, s.on_loop);
    if (s.on_animation_end != LUA_NOREF)
      luaL_unref(L, LUA_REGISTRYINDEX, s.on_animation_end);
    if (s.on_collision != LUA_NOREF)
      luaL_unref(L, LUA_REGISTRYINDEX, s.on_collision);
    if (s.on_collision_end != LUA_NOREF)
      luaL_unref(L, LUA_REGISTRYINDEX, s.on_collision_end);
    if (s.on_screen_exit != LUA_NOREF)
      luaL_unref(L, LUA_REGISTRYINDEX, s.on_screen_exit);
    if (s.on_screen_enter != LUA_NOREF)
      luaL_unref(L, LUA_REGISTRYINDEX, s.on_screen_enter);
    if (s.self_ref != LUA_NOREF)
      luaL_unref(L, LUA_REGISTRYINDEX, s.self_ref);

    auto* c = registry.try_get<collidable>(entity);
    if (c)
      b2DestroyBody(c->body);
  }

  void wire() {
    luaL_newmetatable(L, "Object");

    lua_pushcfunction(L, object_index);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, object_newindex);
    lua_setfield(L, -2, "__newindex");

    lua_pushcfunction(L, object_gc);
    lua_setfield(L, -2, "__gc");

    lua_pop(L, 1);

    lookup.reserve(lookup_capacity);
  }
}

std::unordered_map<entt::id_type, std::string> lookup;

void object::setup(entt::registry& registry) {
  static std::once_flag once;
  std::call_once(once, wire);

  registry.on_destroy<scriptable>().connect<&on_destroy_scriptable>();
}

void object::create(
  entt::registry& registry,
  b2WorldId world,
  atlasregistry& atlasregistry,
  int pool,
  int16_t z,
  std::string_view name,
  std::string_view kind,
  float x,
  float y,
  std::string_view initial_animation
) {
  const auto entity = registry.create();
  registry.emplace<sorteable>(entity, sorteable{z});
  auto& r = registry.emplace<renderable>(entity);
  registry.emplace<transform>(entity, x, y);

  assert(!initial_animation.empty() && "object must have an initial animation");
  r.animation = hash(initial_animation);

  static char filename[128];
  std::snprintf(filename, sizeof(filename), "objects/%.*s.lua", static_cast<int>(kind.size()), kind.data());
  const auto buffer = io::read(filename);
  const auto* data = reinterpret_cast<const char*>(buffer.data());
  const auto size = buffer.size();
  static char label[136];
  std::snprintf(label, sizeof(label), "@%s", filename);

  luaL_loadbuffer(L, data, size, label);
  if (lua_pcall(L, 0, 1, 0) != 0) {
    std::string error = lua_tostring(L, -1);
    lua_pop(L, 1);
    throw std::runtime_error(error);
  }

  animatable animation{};

  lua_getfield(L, -1, "animations");
  assert(lua_istable(L, -1) && "object must have animations");
  lua_pushnil(L);
  while (lua_next(L, -2) != 0) {
    const std::string_view key = lua_tostring(L, -2);

    struct ::animation a{};
    a.name = hash(key);
    lookup.emplace(a.name, key);

    lua_getfield(L, -1, "atlas");
    assert(lua_isstring(L, -1) && "animation must have an atlas");
    a.atlas = hash(lua_tostring(L, -1));
    lua_pop(L, 1);

    const auto length = static_cast<uint32_t>(lua_objlen(L, -1));
    assert(length > 0 && "animation must have at least one keyframe");
    for (uint32_t i = 1; i <= length && a.count < a.keyframes.size(); ++i) {
      lua_rawgeti(L, -1, static_cast<int>(i));
      assert(lua_istable(L, -1) && "keyframe must be a table");

      lua_rawgeti(L, -1, field_sprite);
      a.keyframes[a.count].sprite = static_cast<uint32_t>(lua_tonumber(L, -1));
      lua_pop(L, 1);

      lua_rawgeti(L, -1, field_duration);
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

    assert(animation.count < animation.animations.size() && "too many animations");
    animation.animations[animation.count++] = a;

    lua_pop(L, 1);
  }
  lua_pop(L, 1);

  auto on_spawn_ref = LUA_NOREF;
  auto on_loop_ref = LUA_NOREF;
  auto on_animation_end_ref = LUA_NOREF;
  auto on_collision_ref = LUA_NOREF;
  auto on_collision_end_ref = LUA_NOREF;
  auto on_screen_exit_ref = LUA_NOREF;
  auto on_screen_enter_ref = LUA_NOREF;

  lua_getfield(L, -1, "on_spawn");
  if (lua_isfunction(L, -1))
    on_spawn_ref = luaL_ref(L, LUA_REGISTRYINDEX);
  else
    lua_pop(L, 1);

  lua_getfield(L, -1, "on_loop");
  if (lua_isfunction(L, -1))
    on_loop_ref = luaL_ref(L, LUA_REGISTRYINDEX);
  else
    lua_pop(L, 1);

  lua_getfield(L, -1, "on_animation_end");
  if (lua_isfunction(L, -1))
    on_animation_end_ref = luaL_ref(L, LUA_REGISTRYINDEX);
  else
    lua_pop(L, 1);

  lua_getfield(L, -1, "on_collision");
  if (lua_isfunction(L, -1))
    on_collision_ref = luaL_ref(L, LUA_REGISTRYINDEX);
  else
    lua_pop(L, 1);

  lua_getfield(L, -1, "on_collision_end");
  if (lua_isfunction(L, -1))
    on_collision_end_ref = luaL_ref(L, LUA_REGISTRYINDEX);
  else
    lua_pop(L, 1);

  lua_getfield(L, -1, "on_screen_exit");
  if (lua_isfunction(L, -1))
    on_screen_exit_ref = luaL_ref(L, LUA_REGISTRYINDEX);
  else
    lua_pop(L, 1);

  lua_getfield(L, -1, "on_screen_enter");
  if (lua_isfunction(L, -1))
    on_screen_enter_ref = luaL_ref(L, LUA_REGISTRYINDEX);
  else
    lua_pop(L, 1);

  const auto object_ref = luaL_ref(L, LUA_REGISTRYINDEX);

  registry.emplace<animatable>(entity, animation.animations, animation.count);

  for (uint32_t i = 0; i < animation.count; ++i) {
    if (animation.animations[i].name == r.animation) {
      r.atlas = animation.animations[i].atlas;
      break;
    }
  }

  registry.emplace<identifiable>(entity, hash(kind), hash(name));
  auto& scriptable = registry.emplace<::scriptable>(entity);
  scriptable.on_spawn = on_spawn_ref;
  scriptable.on_loop = on_loop_ref;
  scriptable.on_animation_end = on_animation_end_ref;
  scriptable.on_collision = on_collision_ref;
  scriptable.on_collision_end = on_collision_end_ref;
  scriptable.on_screen_exit = on_screen_exit_ref;
  scriptable.on_screen_enter = on_screen_enter_ref;

  const auto& sprite = atlasregistry.sprite(r.atlas, static_cast<int>(r.sprite));
  if (sprite.hw > 0 && sprite.hh > 0) {
    auto def = b2DefaultBodyDef();
    def.type = b2_dynamicBody;
    def.fixedRotation = true;
    def.gravityScale = .0f;
    def.position = {x, y};
    def.userData = reinterpret_cast<void*>(static_cast<std::uintptr_t>(entity));

    auto& c = registry.emplace<collidable>(entity);
    c.body = b2CreateBody(world, &def);
  }

  const auto kind_id = hash(kind);
  lookup.emplace(kind_id, kind);

  const auto name_id = hash(name);
  lookup.emplace(name_id, name);

  lua_rawgeti(L, LUA_REGISTRYINDEX, pool);
  auto* memory = lua_newuserdata(L, sizeof(objectproxy));
  new (memory) objectproxy(registry, entity, object_ref, name_id);
  luaL_getmetatable(L, "Object");
  lua_setmetatable(L, -2);
  lua_pushvalue(L, -1);
  scriptable.self_ref = luaL_ref(L, LUA_REGISTRYINDEX);
  lua_setfield(L, -2, name.data());
  lua_pop(L, 1);

  if (on_spawn_ref != LUA_NOREF) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, on_spawn_ref);
    lua_rawgeti(L, LUA_REGISTRYINDEX, scriptable.self_ref);
    if (lua_pcall(L, 1, 0, 0) != 0) {
      std::string error = lua_tostring(L, -1);
      lua_pop(L, 1);
      throw std::runtime_error(error);
    }
  }
}

void object::update(entt::registry& registry, atlasregistry& atlasregistry) {
  for (auto&& [entity, t, r, c] : registry.view<transform, renderable, collidable>().each()) {
    const auto& sprite = atlasregistry.sprite(r.atlas, static_cast<int>(r.sprite));

    if (sprite.hw == 0 || sprite.hh == 0 || t.alpha == 0) [[unlikely]] {
      detach_shape(c);
      continue;
    }

    const auto shx = sprite.hx * t.scale;
    const auto shy = sprite.hy * t.scale;
    const auto shw = sprite.hw * t.scale;
    const auto shh = sprite.hh * t.scale;

    const auto sw = sprite.w * t.scale;
    const auto sh = sprite.h * t.scale;
    const auto ox = -sw * .5f + shx + shw * .5f;
    const auto oy = -sh * .5f + shy + shh * .5f;

    if (shw != c.hw || shh != c.hh || shx != c.hx || shy != c.hy) {
      const auto poly = b2MakeBox(shw * .5f, shh * .5f);
      if (b2Shape_IsValid(c.shape)) {
        b2Shape_SetPolygon(c.shape, &poly);
      } else {
        auto def = b2DefaultShapeDef();
        def.isSensor = true;
        def.enableSensorEvents = true;
        def.userData = reinterpret_cast<void*>(static_cast<std::uintptr_t>(entity));
        c.shape = b2CreatePolygonShape(c.body, &def, &poly);
      }
      c.hx = shx;
      c.hy = shy;
      c.hw = shw;
      c.hh = shh;
    }

    c.ox = ox;
    c.oy = oy;
    b2Body_SetTransform(c.body, {t.x + ox, t.y + oy}, b2Rot_identity);
  }
}
