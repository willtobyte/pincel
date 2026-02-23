#include "scene.hpp"

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
  const auto lbuf = io::read(filename);
  const auto *data = reinterpret_cast<const char *>(lbuf.data());
  const auto size = lbuf.size();
  const auto label = std::format("@{}", filename);

  luaL_loadbuffer(L, data, size, label.c_str());

  lua_rawgeti(L, LUA_REGISTRYINDEX, _environment);
  lua_setfenv(L, -2);

  lua_pcall(L, 0, 1, 0);
  _table = luaL_ref(L, LUA_REGISTRYINDEX);

  const auto obuf = io::read(std::format("scenes/{}.objects", name));
  const auto content = std::string_view(
    reinterpret_cast<const char*>(obuf.data()), obuf.size());

  auto cursor = content.data();
  const auto* const fence = content.data() + content.size();

  while (cursor < fence) {
    auto eol = std::find(cursor, fence, '\n');
    auto line = std::string_view(cursor, eol);
    cursor = eol < fence ? eol + 1 : fence;

    while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t'))
      line.remove_suffix(1);

    if (line.empty()) continue;

    const auto eq = line.find('=');
    assert(eq != std::string_view::npos && "missing '=' separator in objects entry");

    const auto oname = line.substr(0, eq);
    const auto values = line.substr(eq + 1);

    const auto first_comma = values.find(',');
    assert(first_comma != std::string_view::npos && "missing ',' after x coordinate");

    float x{}, y{};
    const auto x_str = values.substr(0, first_comma);
    const auto [px, ex] = std::from_chars(x_str.data(), x_str.data() + x_str.size(), x);
    assert(ex == std::errc{} && "failed to parse x coordinate");

    const auto after_x = values.substr(first_comma + 1);
    const auto separator = after_x.find(',');

    const auto y_str = after_x.substr(0, separator);
    const auto [py, ey] = std::from_chars(y_str.data(), y_str.data() + y_str.size(), y);
    assert(ey == std::errc{} && "failed to parse y coordinate");

    const auto animation = separator != std::string_view::npos
      ? after_x.substr(separator + 1) : std::string_view{};

    object::create(_registry, _next_z++, oname, x, y, animation);
  }
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
