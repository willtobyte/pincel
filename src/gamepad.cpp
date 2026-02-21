#include "gamepad.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <string_view>

static constexpr float DEADZONE_THRESHOLD = 0.1f;

static float deadzone(Sint16 raw) {
  const auto value = static_cast<float>(raw) / 32767.0f;
  if (std::abs(value) < DEADZONE_THRESHOLD)
    return 0.0f;

  return value;
}

struct gamepadslot {
  int slot;
  std::unique_ptr<SDL_Gamepad, SDL_Deleter> gamepad{nullptr};

  bool valid() {
    if (gamepad) [[likely]] {
      if (SDL_GamepadConnected(gamepad.get())) [[likely]]
        return true;

      gamepad.reset();
    }

    if (!SDL_HasGamepad())
      return false;

    int count = 0;
    const auto gamepads = std::unique_ptr<SDL_JoystickID[], SDL_Deleter>(SDL_GetGamepads(&count));
    if (gamepads && slot < count) [[likely]]
      gamepad.reset(SDL_OpenGamepad(gamepads[static_cast<size_t>(slot)]));

    return gamepad != nullptr;
  }
};

static gamepadslot slots[4] = {{0}, {1}, {2}, {3}};

static const std::unordered_map<std::string_view, SDL_GamepadButton> button_mapping{
  {"south", SDL_GAMEPAD_BUTTON_SOUTH}, {"east", SDL_GAMEPAD_BUTTON_EAST},
  {"west", SDL_GAMEPAD_BUTTON_WEST}, {"north", SDL_GAMEPAD_BUTTON_NORTH},
  {"back", SDL_GAMEPAD_BUTTON_BACK}, {"guide", SDL_GAMEPAD_BUTTON_GUIDE},
  {"start", SDL_GAMEPAD_BUTTON_START}, {"leftstick", SDL_GAMEPAD_BUTTON_LEFT_STICK},
  {"rightstick", SDL_GAMEPAD_BUTTON_RIGHT_STICK}, {"leftshoulder", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER},
  {"rightshoulder", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER}, {"up", SDL_GAMEPAD_BUTTON_DPAD_UP},
  {"down", SDL_GAMEPAD_BUTTON_DPAD_DOWN}, {"left", SDL_GAMEPAD_BUTTON_DPAD_LEFT},
  {"right", SDL_GAMEPAD_BUTTON_DPAD_RIGHT}
};

static const std::unordered_map<std::string_view, SDL_GamepadAxis> axis_mapping{
  {"leftx", SDL_GAMEPAD_AXIS_LEFTX}, {"lefty", SDL_GAMEPAD_AXIS_LEFTY},
  {"rightx", SDL_GAMEPAD_AXIS_RIGHTX}, {"righty", SDL_GAMEPAD_AXIS_RIGHTY},
  {"triggerleft", SDL_GAMEPAD_AXIS_LEFT_TRIGGER}, {"triggerright", SDL_GAMEPAD_AXIS_RIGHT_TRIGGER}
};

static int gamepadslot_index(lua_State *L) {
  auto **ptr = static_cast<gamepadslot **>(luaL_checkudata(L, 1, "GamepadSlot"));
  auto *self = *ptr;
  const std::string_view name = luaL_checkstring(L, 2);

  if (name == "connected")
    return lua_pushboolean(L, self->valid()), 1;

  if (name == "name") {
    if (self->valid()) [[likely]]
      lua_pushstring(L, SDL_GetGamepadName(self->gamepad.get()));
    else
      lua_pushnil(L);
    return 1;
  }

  if (name == "leftstick") {
    const auto x = SDL_GetGamepadAxis(self->gamepad.get(), SDL_GAMEPAD_AXIS_LEFTX);
    const auto y = SDL_GetGamepadAxis(self->gamepad.get(), SDL_GAMEPAD_AXIS_LEFTY);
    lua_pushnumber(L, deadzone(x));
    lua_pushnumber(L, deadzone(y));
    return 2;
  }

  if (name == "rightstick") {
    const auto x = SDL_GetGamepadAxis(self->gamepad.get(), SDL_GAMEPAD_AXIS_RIGHTX);
    const auto y = SDL_GetGamepadAxis(self->gamepad.get(), SDL_GAMEPAD_AXIS_RIGHTY);
    lua_pushnumber(L, deadzone(x));
    lua_pushnumber(L, deadzone(y));
    return 2;
  }

  if (name == "triggers") {
    const auto left = SDL_GetGamepadAxis(self->gamepad.get(), SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
    const auto right = SDL_GetGamepadAxis(self->gamepad.get(), SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
    lua_pushnumber(L, deadzone(left));
    lua_pushnumber(L, deadzone(right));
    return 2;
  }

  {
    const auto it = button_mapping.find(name);
    if (it != button_mapping.end()) [[likely]] {
      if (self->valid()) [[likely]]
        lua_pushboolean(L, SDL_GetGamepadButton(self->gamepad.get(), it->second));
      else
        lua_pushboolean(L, false);
      return 1;
    }
  }

  {
    const auto it = axis_mapping.find(name);
    if (it != axis_mapping.end()) {
      if (self->valid()) [[likely]]
        lua_pushnumber(L, deadzone(SDL_GetGamepadAxis(self->gamepad.get(), it->second)));
      else
        lua_pushnumber(L, 0);
      return 1;
    }
  }

  lua_pushnil(L);
  return 1;
}

static int gamepads_count(lua_State *L) {
  int total = 0;
  SDL_GetGamepads(&total);
  lua_pushinteger(L, std::min(total, 4));
  return 1;
}

static int gamepads_index(lua_State *L) {
  if (lua_type(L, 2) == LUA_TSTRING) {
    const std::string_view key = lua_tostring(L, 2);
    if (key == "count")
      return gamepads_count(L);

    lua_pushnil(L);
    return 1;
  }

  const auto slot = static_cast<int>(luaL_checkinteger(L, 2));
  const auto index = std::clamp(slot, 0, 3);

  auto **udata = static_cast<gamepadslot **>(lua_newuserdata(L, sizeof(gamepadslot *)));
  *udata = &slots[index];

  luaL_getmetatable(L, "GamepadSlot");
  lua_setmetatable(L, -2);

  return 1;
}

void gamepad::wire() {
  luaL_newmetatable(L, "GamepadSlot");
  lua_pushcfunction(L, gamepadslot_index);
  lua_setfield(L, -2, "__index");
  lua_pop(L, 1);

  lua_newuserdata(L, 1);

  luaL_newmetatable(L, "Gamepads");
  lua_pushcfunction(L, gamepads_index);
  lua_setfield(L, -2, "__index");

  lua_setmetatable(L, -2);
  lua_setglobal(L, "gamepads");
}
