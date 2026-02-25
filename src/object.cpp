#include "object.hpp"
#include "collidable.hpp"
#include "renderable.hpp"
#include "scriptable.hpp"
#include "sorteable.hpp"
#include "transform.hpp"
#include "dirtable.hpp"
#include "compositor.hpp"
#include "atlas.hpp"

namespace {
  constexpr std::size_t lookup_capacity = 1024;

  constexpr int keyframe_sprite = 1;
  constexpr int keyframe_duration = 2;

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

  b2BodyType to_b2_body_type(body_type type) {
    switch (type) {
      case body_type::stationary: return b2_staticBody;
      case body_type::kinematic: return b2_kinematicBody;
      case body_type::sensor: return b2_kinematicBody;
      default: return b2_kinematicBody;
    }
  }

  bool is_sensor(body_type type) {
    return type == body_type::sensor;
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

    assert(proxy->object_ref != LUA_NOREF && "object must have an object ref");

    lua_rawgeti(state, LUA_REGISTRYINDEX, proxy->object_ref);
    lua_getfield(state, -1, key.data());
    lua_remove(state, -2);
    if (!lua_isnil(state, -1))
      return 1;
    lua_pop(state, 1);

    lua_rawgeti(state, LUA_REGISTRYINDEX, proxy->object_ref);
    const auto method = std::format("on_{}", key);
    lua_getfield(state, -1, method.c_str());
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
      registry.ctx().get<dirtable>().mark(dirtable::physics);
      return 0;
    }

    if (key == "angle") {
      registry.get<transform>(entity).angle = static_cast<float>(luaL_checknumber(state, 3));
      return 0;
    }

    if (key == "alpha") {
      registry.get<transform>(entity).alpha = static_cast<uint8_t>(luaL_checknumber(state, 3));
      registry.ctx().get<dirtable>().mark(dirtable::physics);
      return 0;
    }

    if (key == "shown") {
      registry.get<transform>(entity).shown = lua_toboolean(state, 3) != 0;
      return 0;
    }

    if (key == "animation") {
      auto& r = registry.get<renderable>(entity);
      const auto name = luaL_checkstring(state, 3);
      const auto id = hash(name);
      r.animation = id;
      lookup.emplace(id, name);
      r.current_frame = 0;
      r.counter = 0;
      registry.ctx().get<dirtable>().mark(dirtable::physics);
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
    if (s.self_ref != LUA_NOREF)
      luaL_unref(L, LUA_REGISTRYINDEX, s.self_ref);

    auto* c = registry.try_get<collidable>(entity);
    if (c)
      b2DestroyBody(c->body);
  }
}

std::unordered_map<entt::id_type, std::string> lookup;

void object::register_metatable() {
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

void object::connect_signals(entt::registry& registry) {
  registry.on_destroy<scriptable>().connect<&on_destroy_scriptable>();
}

void object::create(
  entt::registry& registry,
  b2WorldId world,
  compositor& compositor,
  int pool,
  int16_t z,
  std::string_view name,
  std::string_view kind,
  float x,
  float y,
  std::string_view animation
) {
  const auto entity = registry.create();
  registry.emplace<sorteable>(entity, sorteable{z});
  auto& r = registry.emplace<renderable>(entity);
  registry.emplace<transform>(entity, x, y);

  if (!animation.empty())
    r.animation = hash(animation);

  const auto filename = std::format("objects/{}.lua", kind);
  const auto buffer = io::read(filename);
  const auto* data = reinterpret_cast<const char*>(buffer.data());
  const auto size = buffer.size();
  const auto label = std::format("@{}", filename);

  luaL_loadbuffer(L, data, size, label.c_str());
  if (lua_pcall(L, 0, 1, 0) != 0) {
    std::string error = lua_tostring(L, -1);
    lua_pop(L, 1);
    throw std::runtime_error(error);
  }

  animatable anim{};

  lua_getfield(L, -1, "atlas");
  if (lua_isstring(L, -1))
    r.atlas = hash(lua_tostring(L, -1));
  lua_pop(L, 1);

  lua_getfield(L, -1, "animations");
  if (lua_istable(L, -1)) {
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
      const std::string_view key = lua_tostring(L, -2);

      struct animation a{};
      a.name = hash(key);
      lookup.emplace(a.name, key);

      const auto length = static_cast<uint32_t>(lua_objlen(L, -1));
      for (uint32_t i = 1; i <= length && a.count < a.keyframes.size(); ++i) {
        lua_rawgeti(L, -1, static_cast<int>(i));
        assert(lua_istable(L, -1) && "keyframe must be a table");

        lua_rawgeti(L, -1, keyframe_sprite);
        a.keyframes[a.count].sprite = static_cast<uint32_t>(lua_tonumber(L, -1));
        lua_pop(L, 1);

        lua_rawgeti(L, -1, keyframe_duration);
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

      assert(anim.count < anim.animations.size() && "too many animations");
      anim.animations[anim.count++] = a;

      lua_pop(L, 1);
    }
  }
  lua_pop(L, 1);

  auto on_spawn_ref = LUA_NOREF;
  auto on_loop_ref = LUA_NOREF;
  auto on_animation_end_ref = LUA_NOREF;

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

  const auto object_ref = luaL_ref(L, LUA_REGISTRYINDEX);

  registry.emplace<animatable>(entity, anim.animations, anim.count);
  auto& scriptable = registry.emplace<::scriptable>(entity);
  scriptable.on_spawn = on_spawn_ref;
  scriptable.on_loop = on_loop_ref;
  scriptable.on_animation_end = on_animation_end_ref;

  if (compositor.has_hitbox(r.atlas)) {
    auto def = b2DefaultBodyDef();
    def.type = b2_kinematicBody;
    def.position = {x, y};
    def.userData = reinterpret_cast<void*>(static_cast<std::uintptr_t>(entity));

    for (uint32_t i = 0; i < anim.count; ++i) {
      for (uint32_t j = 0; j < anim.animations[i].count; ++j) {
        const auto* sprite = compositor.get_sprite(r.atlas, static_cast<int>(anim.animations[i].keyframes[j].sprite));
        if (sprite && sprite->type != body_type::none) {
          def.type = to_b2_body_type(sprite->type);
          goto found;
        }
      }
    }
    found:

    auto& c = registry.emplace<collidable>(entity);
    c.body = b2CreateBody(world, &def);
  }

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

void object::sync_collision(entt::registry& registry, compositor& compositor) {
  for (auto&& [entity, t, r, c] : registry.view<transform, renderable, collidable>().each()) {
    const auto* sprite = compositor.get_sprite(r.atlas, static_cast<int>(r.sprite));

    if (!sprite || sprite->type == body_type::none || t.alpha == 0) [[unlikely]] {
      detach_shape(c);
      return;
    }

    const auto shx = sprite->hx * t.scale;
    const auto shy = sprite->hy * t.scale;
    const auto shw = sprite->hw * t.scale;
    const auto shh = sprite->hh * t.scale;

    const auto sw = sprite->w * t.scale;
    const auto sh = sprite->h * t.scale;
    const auto ox = -sw * .5f + shx + shw * .5f;
    const auto oy = -sh * .5f + shy + shh * .5f;

    if (shw != c.hw || shh != c.hh || shx != c.hx || shy != c.hy) {
      detach_shape(c);

      const auto poly = b2MakeBox(shw * .5f, shh * .5f);
      auto shape_def = b2DefaultShapeDef();
      shape_def.isSensor = is_sensor(sprite->type);
      shape_def.enableSensorEvents = shape_def.isSensor;
      shape_def.userData = reinterpret_cast<void*>(static_cast<std::uintptr_t>(entity));
      c.shape = b2CreatePolygonShape(c.body, &shape_def, &poly);
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
