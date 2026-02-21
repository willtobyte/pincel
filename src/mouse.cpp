#include "mouse.hpp"

static int mouse_x(lua_State *state) {
  float x, y;
  SDL_GetMouseState(&x, &y);
  SDL_RenderCoordinatesFromWindow(renderer, x, y, &x, &y);
  lua_pushnumber(state, static_cast<double>(x));
  return 1;
}

static int mouse_y(lua_State *state) {
  float x, y;
  SDL_GetMouseState(&x, &y);
  SDL_RenderCoordinatesFromWindow(renderer, x, y, &x, &y);
  lua_pushnumber(state, static_cast<double>(y));
  return 1;
}

static int mouse_xy(lua_State *state) {
  float x, y;
  SDL_GetMouseState(&x, &y);
  SDL_RenderCoordinatesFromWindow(renderer, x, y, &x, &y);
  lua_pushnumber(state, static_cast<double>(x));
  lua_pushnumber(state, static_cast<double>(y));
  return 2;
}

static int mouse_button(lua_State *state) {
  float x, y;
  const auto button = SDL_GetMouseState(&x, &y);
  if (button & SDL_BUTTON_MASK(SDL_BUTTON_LEFT))
    return lua_pushinteger(state, SDL_BUTTON_LEFT), 1;
  if (button & SDL_BUTTON_MASK(SDL_BUTTON_MIDDLE))
    return lua_pushinteger(state, SDL_BUTTON_MIDDLE), 1;
  if (button & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT))
    return lua_pushinteger(state, SDL_BUTTON_RIGHT), 1;

  lua_pushinteger(state, 0);
  return 1;
}

static const luaL_Reg functions[] = {
  {"x", mouse_x},
  {"y", mouse_y},
  {"xy", mouse_xy},
  {"button", mouse_button},
  {nullptr, nullptr}
};

void mouse::wire() {
  luaL_register(L, "mouse", functions);
  lua_pop(L, 1);
}
