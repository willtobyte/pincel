#include "object.hpp"
#include "stage.hpp"

namespace {
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

  const mapping* find_mapping(const mappable& m, entt::id_type name) {
    for (uint32_t i = 0; i < m.count; ++i) {
      if (m.mappings[i].name == name) return &m.mappings[i];
    }
    return nullptr;
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

  int object_destroy(lua_State* state) {
    auto* proxy = static_cast<objectproxy*>(luaL_checkudata(state, 1, "Object"));
    if (!proxy->registry->valid(proxy->entity)) return 0;

    const auto& lu = proxy->registry->ctx().get<lookupable>();
    const auto it = lu.names.find(proxy->name);
    if (it != lu.names.end()) {
      lua_getfield(state, LUA_GLOBALSINDEX, "pool");
      lua_pushnil(state);
      lua_setfield(state, -2, it->second.c_str());
      lua_pop(state, 1);
    }

    proxy->registry->destroy(proxy->entity);
    return 0;
  }

  int object_index(lua_State* state) {
    auto* proxy = static_cast<objectproxy*>(luaL_checkudata(state, 1, "Object"));
    const std::string_view key = luaL_checkstring(state, 2);

    if (!proxy->registry->valid(proxy->entity))
      return lua_pushnil(state), 1;

    auto& registry = *proxy->registry;
    const auto entity = proxy->entity;

    if (key == "destroy") {
      lua_pushcfunction(state, object_destroy);
      return 1;
    }

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
      const auto& m = registry.get<mappable>(entity);
      for (uint32_t i = 0; i < m.count; ++i) {
        if (m.mappings[i].atlas == r.atlas && m.mappings[i].entry == r.entry) {
          const auto& lu = registry.ctx().get<lookupable>();
          const auto it = lu.names.find(m.mappings[i].name);
          if (it != lu.names.end()) {
            lua_pushstring(state, it->second.c_str());
            return 1;
          }
        }
      }
      return lua_pushnil(state), 1;
    }

    if (key == "name") {
      const auto& id = registry.get<identifiable>(entity);
      const auto& lu = registry.ctx().get<lookupable>();
      const auto it = lu.names.find(id.name);
      if (it == lu.names.end())
        return lua_pushnil(state), 1;
      lua_pushstring(state, it->second.c_str());
      return 1;
    }

    if (key == "kind") {
      const auto& id = registry.get<identifiable>(entity);
      const auto& lu = registry.ctx().get<lookupable>();
      const auto it = lu.names.find(id.kind);
      if (it == lu.names.end())
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

    if (!proxy->registry->valid(proxy->entity))
      return 0;

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
      const auto& m = registry.get<mappable>(entity);

      if (lua_istable(state, 3)) {
        lua_rawgeti(state, 3, 1);
        const auto atlas_name = luaL_checkstring(state, -1);
        lua_pop(state, 1);

        lua_rawgeti(state, 3, 2);
        const auto entry_name = luaL_checkstring(state, -1);
        lua_pop(state, 1);

        r.atlas = hash(atlas_name);
        r.entry = hash(entry_name);
      } else {
        const auto value = luaL_checkstring(state, 3);
        const auto id = hash(value);

        const auto* mp = find_mapping(m, id);
        assert(mp && "animation mapping not found");

        r.atlas = mp->atlas;
        r.entry = mp->entry;
      }

      r.current_frame = 0;
      r.counter = 0;

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

    if (s.self_ref != LUA_NOREF) {
      lua_rawgeti(L, LUA_REGISTRYINDEX, s.self_ref);
      auto* proxy = static_cast<objectproxy*>(lua_touserdata(L, -1));
      if (proxy && proxy->object_ref != LUA_NOREF) {
        luaL_unref(L, LUA_REGISTRYINDEX, proxy->object_ref);
        proxy->object_ref = LUA_NOREF;
      }

      lua_pop(L, 1);
      luaL_unref(L, LUA_REGISTRYINDEX, s.self_ref);
    }

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
  }
}

void object::setup(entt::registry& registry) {
  static std::once_flag once;
  std::call_once(once, wire);

  registry.on_destroy<scriptable>().connect<&on_destroy_scriptable>();
}

void object::create(
  stage& stage,
  int16_t z,
  std::string_view name,
  std::string_view kind,
  float x,
  float y,
  std::string_view initial_animation
) {
  auto& registry = stage._registry;
  auto& world = stage._world;
  auto& atlasregistry = stage._atlasregistry;
  auto& pool = stage._pool;

  const auto entity = registry.create();
  registry.emplace<sorteable>(entity, sorteable{z});
  registry.emplace<transform>(entity, x, y);
  auto& r = registry.emplace<renderable>(entity);

  assert(!initial_animation.empty() && "object must have an initial animation");
  const auto initial_id = hash(initial_animation);

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

  mappable m{};

  auto on_spawn_ref = LUA_NOREF;
  auto on_loop_ref = LUA_NOREF;
  auto on_animation_end_ref = LUA_NOREF;
  auto on_collision_ref = LUA_NOREF;
  auto on_collision_end_ref = LUA_NOREF;
  auto on_screen_exit_ref = LUA_NOREF;
  auto on_screen_enter_ref = LUA_NOREF;

  lua_pushnil(L);
  while (lua_next(L, -2) != 0) {
    if (!lua_isstring(L, -2)) {
      lua_pop(L, 1);
      continue;
    }

    const std::string_view field = lua_tostring(L, -2);

    if (lua_istable(L, -1)) {
      lua_rawgeti(L, -1, 1);
      const bool is_mapping = lua_isstring(L, -1);
      lua_pop(L, 1);

      if (is_mapping) {
        lua_rawgeti(L, -1, 1);
        const std::string_view atlas_name = lua_tostring(L, -1);
        lua_pop(L, 1);

        lua_rawgeti(L, -1, 2);
        const std::string_view entry_name = lua_tostring(L, -1);
        lua_pop(L, 1);

        assert(m.count < m.mappings.size() && "too many mappings");
        auto& mp = m.mappings[m.count++];
        mp.name = hash(field);
        mp.atlas = hash(atlas_name);
        mp.entry = hash(entry_name);

        registry.ctx().get<lookupable>().names.emplace(mp.name, field);
      }
    } else if (lua_isfunction(L, -1)) {
      if (field == "on_spawn") {
        on_spawn_ref = luaL_ref(L, LUA_REGISTRYINDEX);
        continue;
      } else if (field == "on_loop") {
        on_loop_ref = luaL_ref(L, LUA_REGISTRYINDEX);
        continue;
      } else if (field == "on_animation_end") {
        on_animation_end_ref = luaL_ref(L, LUA_REGISTRYINDEX);
        continue;
      } else if (field == "on_collision") {
        on_collision_ref = luaL_ref(L, LUA_REGISTRYINDEX);
        continue;
      } else if (field == "on_collision_end") {
        on_collision_end_ref = luaL_ref(L, LUA_REGISTRYINDEX);
        continue;
      } else if (field == "on_screen_exit") {
        on_screen_exit_ref = luaL_ref(L, LUA_REGISTRYINDEX);
        continue;
      } else if (field == "on_screen_enter") {
        on_screen_enter_ref = luaL_ref(L, LUA_REGISTRYINDEX);
        continue;
      }
    }

    lua_pop(L, 1);
  }

  const auto object_ref = luaL_ref(L, LUA_REGISTRYINDEX);

  registry.emplace<mappable>(entity, m.mappings, m.count);

  const auto* mp = find_mapping(m, initial_id);
  assert(mp && "initial animation mapping not found in object");
  r.atlas = mp->atlas;
  r.entry = mp->entry;

  registry.emplace<identifiable>(entity, hash(kind), hash(name));
  auto& scriptable = registry.emplace<::scriptable>(entity);
  scriptable.on_spawn = on_spawn_ref;
  scriptable.on_loop = on_loop_ref;
  scriptable.on_animation_end = on_animation_end_ref;
  scriptable.on_collision = on_collision_ref;
  scriptable.on_collision_end = on_collision_end_ref;
  scriptable.on_screen_exit = on_screen_exit_ref;
  scriptable.on_screen_enter = on_screen_enter_ref;

  const auto& kf = atlasregistry.get(r.atlas).keyframe_at(r.entry, 0);
  if (kf.sprite.hw > 0 && kf.sprite.hh > 0) {
    auto def = b2DefaultBodyDef();
    def.type = b2_dynamicBody;
    def.fixedRotation = true;
    def.gravityScale = .0f;
    def.position = {x, y};
    def.userData = reinterpret_cast<void*>(static_cast<std::uintptr_t>(entity));

    auto& c = registry.emplace<collidable>(entity);
    c.body = b2CreateBody(world, &def);
  }

  auto& lu = registry.ctx().get<lookupable>();

  const auto kind_id = hash(kind);
  lu.names.emplace(kind_id, kind);

  const auto name_id = hash(name);
  lu.names.emplace(name_id, name);

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
    const auto& kf = atlasregistry.get(r.atlas).keyframe_at(r.entry, r.current_frame);
    const auto& sprite = kf.sprite;

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
