#include "scene.hpp"
#include "object.hpp"
#include "collidable.hpp"
#include "identifiable.hpp"
#include "scriptable.hpp"
#include "dirtable.hpp"
#include "sorteable.hpp"

namespace {
  constexpr auto fixed_timestep = 1.0f / 60.0f;
  constexpr auto world_substeps = 4;

  constexpr std::string_view directions[] = {"left", "right", "top", "bottom"};

  bool by_depth(const sorteable& a, const sorteable& b) {
    return a.z < b.z;
  }

  void dispatch_sensor_events(b2WorldId world, entt::registry& registry) {
    const auto events = b2World_GetSensorEvents(world);

    for (auto i = 0; i < events.beginCount; ++i) {
      const auto& e = events.beginEvents[i];
      assert(b2Shape_IsValid(e.sensorShapeId) && "sensor shape must be valid");
      assert(b2Shape_IsValid(e.visitorShapeId) && "visitor shape must be valid");

      const auto a = static_cast<entt::entity>(reinterpret_cast<std::uintptr_t>(b2Shape_GetUserData(e.sensorShapeId)));
      const auto b = static_cast<entt::entity>(reinterpret_cast<std::uintptr_t>(b2Shape_GetUserData(e.visitorShapeId)));

      const auto& id_b = registry.get<identifiable>(b);
      const auto& s_a = registry.get<scriptable>(a);

      const auto& name_b = lookup.at(id_b.name);
      const auto& kind_b = lookup.at(id_b.kind);

      if (s_a.on_collision != LUA_NOREF) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, s_a.on_collision);
        lua_rawgeti(L, LUA_REGISTRYINDEX, s_a.self_ref);
        lua_pushstring(L, name_b.c_str());
        lua_pushstring(L, kind_b.c_str());
        if (lua_pcall(L, 3, 0, 0) != 0) {
          std::string error = lua_tostring(L, -1);
          lua_pop(L, 1);
          throw std::runtime_error(error);
        }
      }
    }

    for (int i = 0; i < events.endCount; ++i) {
      const auto& e = events.endEvents[i];
      if (!b2Shape_IsValid(e.sensorShapeId) || !b2Shape_IsValid(e.visitorShapeId))
        continue;

      const auto a = static_cast<entt::entity>(reinterpret_cast<std::uintptr_t>(b2Shape_GetUserData(e.sensorShapeId)));
      const auto b = static_cast<entt::entity>(reinterpret_cast<std::uintptr_t>(b2Shape_GetUserData(e.visitorShapeId)));

      const auto& id_b = registry.get<identifiable>(b);
      const auto& s_a = registry.get<scriptable>(a);

      const auto& name_b = lookup.at(id_b.name);
      const auto& kind_b = lookup.at(id_b.kind);

      if (s_a.on_collision_end != LUA_NOREF) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, s_a.on_collision_end);
        lua_rawgeti(L, LUA_REGISTRYINDEX, s_a.self_ref);
        lua_pushstring(L, name_b.c_str());
        lua_pushstring(L, kind_b.c_str());
        if (lua_pcall(L, 3, 0, 0) != 0) {
          std::string error = lua_tostring(L, -1);
          lua_pop(L, 1);
          throw std::runtime_error(error);
        }
      }
    }
  }
}

scene::scene(std::string_view name, compositor& compositor)
    : _compositor(compositor) {
  b2WorldDef def = b2DefaultWorldDef();
  def.gravity = {0.0f, 0.0f};
  _world = b2CreateWorld(&def);

  object::register_metatable();
  object::connect_signals(_registry);
  _registry.ctx().emplace<dirtable>();

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

  if (lua_pcall(L, 0, 1, 0) != 0) {
    std::string error = lua_tostring(L, -1);
    lua_pop(L, 1);
    throw std::runtime_error(error);
  }

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

    object::create(_registry, _world, _compositor, _pool, _next_z++, entry_name, kind, x, y, animation);

    lua_pop(L, 1);
  }

  _table = luaL_ref(L, LUA_REGISTRYINDEX);
}

scene::~scene() noexcept {
  b2DestroyWorld(_world);

  luaL_unref(L, LUA_REGISTRYINDEX, _table);
  luaL_unref(L, LUA_REGISTRYINDEX, _pool);
  luaL_unref(L, LUA_REGISTRYINDEX, _environment);
  luaL_unref(L, LUA_REGISTRYINDEX, _G);

  lookup.clear();
}

void scene::on_enter() {
  lua_rawgeti(L, LUA_REGISTRYINDEX, _environment);
  lua_replace(L, LUA_GLOBALSINDEX);

  lua_rawgeti(L, LUA_REGISTRYINDEX, _table);
  lua_getfield(L, -1, "on_enter");
  if (lua_isfunction(L, -1)) {
    if (lua_pcall(L, 0, 0, 0) != 0) {
      std::string error = lua_tostring(L, -1);
      lua_pop(L, 2);
      throw std::runtime_error(error);
    }
  } else {
    lua_pop(L, 1);
  }
  lua_pop(L, 1);
}

void scene::on_loop(float delta) {
  _accumulator += delta;
  while (_accumulator >= fixed_timestep) {
    b2World_Step(_world, fixed_timestep, world_substeps);
    dispatch_sensor_events(_world, _registry);
    _accumulator -= fixed_timestep;
  }

  animator::update(_registry, delta);
  object::sync_collision(_registry, _compositor);
  scripting::update(_registry, delta);

  for (auto&& [entity, s, c] : _registry.view<scriptable, collidable>().each()) {
    if (s.on_screen_exit == LUA_NOREF && s.on_screen_enter == LUA_NOREF)
      continue;

    if (!b2Shape_IsValid(c.shape))
      continue;

    const auto aabb = b2Shape_GetAABB(c.shape);

    uint8_t current = 0;
    if (aabb.upperBound.x < 0)               current |= scriptable::screen_left;
    if (aabb.lowerBound.x > viewport.width)   current |= scriptable::screen_right;
    if (aabb.upperBound.y < 0)                current |= scriptable::screen_top;
    if (aabb.lowerBound.y > viewport.height)  current |= scriptable::screen_bottom;

    const auto exited  = static_cast<uint8_t>(current & ~s.screen_previous);
    const auto entered = static_cast<uint8_t>(s.screen_previous & ~current);

    for (uint8_t bit = 0; bit < 4; ++bit) {
      const auto mask = static_cast<uint8_t>(1u << bit);

      if ((exited & mask) && s.on_screen_exit != LUA_NOREF) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, s.on_screen_exit);
        lua_rawgeti(L, LUA_REGISTRYINDEX, s.self_ref);
        lua_pushstring(L, directions[bit].data());
        if (lua_pcall(L, 2, 0, 0) != 0) {
          std::string error = lua_tostring(L, -1);
          lua_pop(L, 1);
          throw std::runtime_error(error);
        }
      }

      if ((entered & mask) && s.on_screen_enter != LUA_NOREF) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, s.on_screen_enter);
        lua_rawgeti(L, LUA_REGISTRYINDEX, s.self_ref);
        lua_pushstring(L, directions[bit].data());
        if (lua_pcall(L, 2, 0, 0) != 0) {
          std::string error = lua_tostring(L, -1);
          lua_pop(L, 1);
          throw std::runtime_error(error);
        }
      }
    }

    s.screen_previous = current;
  }

  auto& d = _registry.ctx().get<dirtable>();
  if (d.is(dirtable::sort)) {
    _registry.sort<sorteable>(by_depth, entt::insertion_sort{});
    d.clear(dirtable::sort);
  }

  lua_rawgeti(L, LUA_REGISTRYINDEX, _table);
  lua_getfield(L, -1, "on_loop");
  if (lua_isfunction(L, -1)) {
    lua_pushnumber(L, static_cast<double>(delta));
    if (lua_pcall(L, 1, 0, 0) != 0) {
      std::string error = lua_tostring(L, -1);
      lua_pop(L, 2);
      throw std::runtime_error(error);
    }
  } else {
    lua_pop(L, 1);
  }
  lua_pop(L, 1);
}

void scene::on_draw() {
  presenter::render(_registry, _compositor);

#ifdef DEVELOPMENT
  SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);

  const b2AABB aabb = {{0, 0}, {viewport.width, viewport.height}};
  const b2QueryFilter filter = b2DefaultQueryFilter();

  b2World_OverlapAABB(_world, aabb, filter, [](b2ShapeId shape, void*) -> bool {
    const auto box = b2Shape_GetAABB(shape);
    const SDL_FRect r{
      box.lowerBound.x,
      box.lowerBound.y,
      box.upperBound.x - box.lowerBound.x,
      box.upperBound.y - box.lowerBound.y
    };
    SDL_RenderRect(renderer, &r);
    return true;
  }, nullptr);

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
#endif
}

void scene::on_leave() {
  lua_rawgeti(L, LUA_REGISTRYINDEX, _table);
  lua_getfield(L, -1, "on_leave");
  if (lua_isfunction(L, -1)) {
    if (lua_pcall(L, 0, 0, 0) != 0) {
      std::string error = lua_tostring(L, -1);
      lua_pop(L, 2);
      throw std::runtime_error(error);
    }
  } else {
    lua_pop(L, 1);
  }
  lua_pop(L, 1);

  lua_rawgeti(L, LUA_REGISTRYINDEX, _G);
  lua_replace(L, LUA_GLOBALSINDEX);
}
