#include "scene.hpp"

namespace {
  struct entityproxy final {
    entt::registry* registry;
    entt::entity entity;
    int object_ref{LUA_NOREF};
    char name[48];

    entityproxy(entt::registry& registry, entt::entity entity, int object_ref, std::string_view name) noexcept
        : registry(&registry), entity(entity), object_ref(object_ref), name{} {
      const auto length = std::min(name.size(), sizeof(this->name) - 1);
      std::copy_n(name.data(), length, this->name);
      this->name[length] = '\0';
    }

    ~entityproxy() noexcept = default;
  };

  entt::id_type hash(std::string_view value) {
    return entt::hashed_string::value(value.data(), value.size());
  }

  int entity_index(lua_State* state) {
    auto* proxy = static_cast<entityproxy*>(luaL_checkudata(state, 1, "Object"));
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

    assert(proxy->object_ref != LUA_NOREF && "entity must have an object ref");

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

  int entity_newindex(lua_State* state) {
    auto* proxy = static_cast<entityproxy*>(luaL_checkudata(state, 1, "Object"));
    const std::string_view key = luaL_checkstring(state, 2);
    auto& registry = *proxy->registry;
    const auto entity = proxy->entity;

    if (key == "x") {
      registry.get<transform>(entity).x = static_cast<float>(luaL_checknumber(state, 3));
      return 0;
    }

    if (key == "y") {
      registry.get<transform>(entity).y = static_cast<float>(luaL_checknumber(state, 3));
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

    assert(proxy->object_ref != LUA_NOREF && "entity must have an object ref");

    lua_rawgeti(state, LUA_REGISTRYINDEX, proxy->object_ref);
    lua_pushvalue(state, 3);
    lua_setfield(state, -2, key.data());
    lua_pop(state, 1);
    return 0;
  }

  int entity_gc(lua_State* state) {
    auto* proxy = static_cast<entityproxy*>(luaL_checkudata(state, 1, "Object"));
    if (proxy->object_ref != LUA_NOREF)
      luaL_unref(state, LUA_REGISTRYINDEX, proxy->object_ref);
    proxy->~entityproxy();
    return 0;
  }

  bool by_depth(const sorteable& a, const sorteable& b) {
    return a.z < b.z;
  }

  void create_object(
    entt::registry& registry,
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
    auto& renderable = registry.emplace<::renderable>(entity);
    registry.emplace<transform>(entity, x, y);

    if (!animation.empty())
      renderable.animation = hash(animation);

    const auto filename = std::format("objects/{}.lua", kind);
    const auto buffer = io::read(filename);
    const auto* data = reinterpret_cast<const char*>(buffer.data());
    const auto size = buffer.size();
    const auto label = std::format("@{}", filename);

    luaL_loadbuffer(L, data, size, label.c_str());
    lua_pcall(L, 0, 1, 0);
    assert(lua_istable(L, -1) && "objects lua must return a table");

    animatable animatable{};

    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
      const std::string_view key = lua_tostring(L, -2);

      if (key == "atlas") {
        renderable.atlas = hash(lua_tostring(L, -1));
        lua_pop(L, 1);
        continue;
      }

      if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        continue;
      }

      struct animation a{};
      a.name = hash(key);

      const auto length = static_cast<uint32_t>(lua_objlen(L, -1));
      for (uint32_t i = 1; i <= length && a.count < a.keyframes.size(); ++i) {
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

      assert(animatable.count < animatable.animations.size() && "too many animations");
      animatable.animations[animatable.count++] = a;

      lua_pop(L, 1);
    }

    auto on_loop_ref = LUA_NOREF;

    lua_getfield(L, -1, "on_loop");
    if (lua_isfunction(L, -1))
      on_loop_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    else
      lua_pop(L, 1);

    const auto object_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    registry.emplace<::animatable>(entity, animatable.animations, animatable.count);
    auto& scriptable = registry.emplace<::scriptable>(entity);
    scriptable.on_loop = on_loop_ref;

    lua_rawgeti(L, LUA_REGISTRYINDEX, pool);
    auto* memory = lua_newuserdata(L, sizeof(entityproxy));
    new (memory) entityproxy(registry, entity, object_ref, name);
    luaL_getmetatable(L, "Object");
    lua_setmetatable(L, -2);
    lua_setfield(L, -2, name.data());
    lua_pop(L, 1);
  }

  void on_destroy_scriptable(entt::registry& registry, entt::entity entity) {
    auto& scriptable = registry.get<::scriptable>(entity);
    if (scriptable.on_loop != LUA_NOREF)
      luaL_unref(L, LUA_REGISTRYINDEX, scriptable.on_loop);
  }
}

scene::scene(std::string_view name, compositor& compositor)
    : _compositor(compositor) {
  _registry.on_destroy<scriptable>().connect<&on_destroy_scriptable>();
  _registry.ctx().emplace<dirtable>();

  luaL_newmetatable(L, "Object");

  lua_pushcfunction(L, entity_index);
  lua_setfield(L, -2, "__index");

  lua_pushcfunction(L, entity_newindex);
  lua_setfield(L, -2, "__newindex");

  lua_pushcfunction(L, entity_gc);
  lua_setfield(L, -2, "__gc");

  lua_pop(L, 1);

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
  assert(lua_istable(L, -1) && "scene lua must return a table");

  const auto count = static_cast<int>(lua_objlen(L, -1));
  for (int i = 1; i <= count; ++i) {
    lua_rawgeti(L, -1, i);
    assert(lua_istable(L, -1) && "scene entry must be a table");

    lua_getfield(L, -1, "kind");
    const std::string_view kind = luaL_checkstring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "name");
    const std::string_view entry_name = luaL_checkstring(L, -1);
    lua_pop(L, 1);

    float x = 0, y = 0;

    lua_getfield(L, -1, "x");
    if (lua_isnumber(L, -1)) x = static_cast<float>(lua_tonumber(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "y");
    if (lua_isnumber(L, -1)) y = static_cast<float>(lua_tonumber(L, -1));
    lua_pop(L, 1);

    std::string_view animation{};
    lua_getfield(L, -1, "animation");
    if (lua_isstring(L, -1)) animation = lua_tostring(L, -1);
    lua_pop(L, 1);

    create_object(_registry, _pool, _next_z++, entry_name, kind, x, y, animation);

    lua_pop(L, 1);
  }

  _table = luaL_ref(L, LUA_REGISTRYINDEX);
}

scene::~scene() noexcept {
  luaL_unref(L, LUA_REGISTRYINDEX, _table);
  luaL_unref(L, LUA_REGISTRYINDEX, _pool);
  luaL_unref(L, LUA_REGISTRYINDEX, _environment);
  luaL_unref(L, LUA_REGISTRYINDEX, _G);
}

void scene::on_enter() {
  lua_rawgeti(L, LUA_REGISTRYINDEX, _environment);
  lua_replace(L, LUA_GLOBALSINDEX);

  lua_rawgeti(L, LUA_REGISTRYINDEX, _table);
  lua_getfield(L, -1, "on_enter");
  if (lua_isfunction(L, -1)) {
    if (lua_pcall(L, 0, 0, 0) != 0)
      lua_pop(L, 1);
  } else {
    lua_pop(L, 1);
  }
  lua_pop(L, 1);
}

void scene::on_loop(float delta) {
  animator::update(_registry, delta);
  scripting::update(_registry, delta);

  auto& d = _registry.ctx().get<dirtable>();
  if (d.is(dirtable::sort)) {
    _registry.sort<sorteable>(by_depth, entt::insertion_sort{});

    d.clear(dirtable::sort);
  }

  lua_rawgeti(L, LUA_REGISTRYINDEX, _table);
  lua_getfield(L, -1, "on_loop");
  if (lua_isfunction(L, -1)) {
    lua_pushnumber(L, static_cast<double>(delta));
    if (lua_pcall(L, 1, 0, 0) != 0)
      lua_pop(L, 1);
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
    if (lua_pcall(L, 0, 0, 0) != 0)
      lua_pop(L, 1);
  } else {
    lua_pop(L, 1);
  }
  lua_pop(L, 1);

  lua_rawgeti(L, LUA_REGISTRYINDEX, _G);
  lua_replace(L, LUA_GLOBALSINDEX);
}
