#include "scriptengine.hpp"

static int searcher(lua_State *L) {
  const auto *module = luaL_checkstring(L, 1);
  const auto filename = std::format("scripts/{}.lua", module);
  const auto buffer = io::read(filename);
  const auto *data = reinterpret_cast<const char *>(buffer.data());
  const auto size = buffer.size();
  const auto label = std::format("@{}", filename);

  luaL_loadbuffer(L, data, size, label.c_str());

  return 1;
}

void scriptengine::run() {
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "loaders");

  const auto len = static_cast<int>(lua_objlen(L, -1));
  lua_pushcfunction(L, searcher);
  lua_rawseti(L, -2, len + 1);

  lua_pop(L, 2);

  cassette::wire();
  gamepad::wire();
  keyboard::wire();
  mouse::wire();

  auto e = engine();
  e.run();
}
