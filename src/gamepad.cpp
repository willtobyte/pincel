#include "gamepad.hpp"

static constexpr float DEADZONE_THRESHOLD = 0.1f;

static float deadzone(Sint16 raw) {
  const auto value = static_cast<float>(raw) / 32767.0f;
  if (std::abs(value) < DEADZONE_THRESHOLD)
    return .0f;

  return value;
}

static std::unique_ptr<SDL_Gamepad, SDL_Deleter> pad{nullptr};

static bool valid() {
  if (pad) [[likely]] {
    if (SDL_GamepadConnected(pad.get())) [[likely]]
      return true;

    pad.reset();
  }

  if (!SDL_HasGamepad()) [[unlikely]]
    return false;

  auto count = 0;
  const auto mapping = std::unique_ptr<SDL_JoystickID[], SDL_Deleter>(SDL_GetGamepads(&count));
  if (!mapping) [[unlikely]]
    return false;

  for (auto i = 0; i < count; ++i) {
    pad.reset(SDL_OpenGamepad(mapping[static_cast<size_t>(i)]));
    if (pad) [[likely]]
      return true;
  }

  return false;
}

static int gamepad_rumble(lua_State *state) {
  const auto low = std::clamp(static_cast<float>(luaL_checknumber(state, 2)), .0f, 1.0f);
  const auto high = std::clamp(static_cast<float>(luaL_checknumber(state, 3)), .0f, 1.0f);
  const auto duration = static_cast<Uint32>(luaL_checkinteger(state, 4));

  const auto low16 = static_cast<Uint16>(low * 65535.0f);
  const auto high16 = static_cast<Uint16>(high * 65535.0f);

  if (!valid() || !pad) [[unlikely]]
    return lua_pushboolean(state, false), 1;

  return lua_pushboolean(state, SDL_RumbleGamepad(pad.get(), low16, high16, duration)), 1;
}

static int push_axis(lua_State *state, bool ok, SDL_GamepadAxis a) {
  if (ok) [[likely]]
    return lua_pushnumber(state, static_cast<double>(deadzone(SDL_GetGamepadAxis(pad.get(), a)))), 1;
  return lua_pushnumber(state, 0), 1;
}

static int push_button(lua_State *state, bool ok, SDL_GamepadButton b) {
  if (ok) [[likely]]
    return lua_pushboolean(state, SDL_GetGamepadButton(pad.get(), b)), 1;
  return lua_pushboolean(state, false), 1;
}

static int gamepad_index(lua_State *state) {
  const std::string_view name = luaL_checkstring(state, 2);
  const bool ok = valid();

  if (name == "connected")      return lua_pushboolean(state, ok), 1;
  if (name == "rumble")          return lua_pushcfunction(state, gamepad_rumble), 1;

  if (name == "name") {
    if (ok) [[likely]]
      return lua_pushstring(state, SDL_GetGamepadName(pad.get())), 1;
    return lua_pushstring(state, ""), 1;
  }

  if (name == "left_x")         return push_axis(state, ok, SDL_GAMEPAD_AXIS_LEFTX);
  if (name == "left_y")         return push_axis(state, ok, SDL_GAMEPAD_AXIS_LEFTY);
  if (name == "right_x")        return push_axis(state, ok, SDL_GAMEPAD_AXIS_RIGHTX);
  if (name == "right_y")        return push_axis(state, ok, SDL_GAMEPAD_AXIS_RIGHTY);
  if (name == "trigger_left")   return push_axis(state, ok, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
  if (name == "trigger_right")  return push_axis(state, ok, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);

  if (name == "south")          return push_button(state, ok, SDL_GAMEPAD_BUTTON_SOUTH);
  if (name == "east")           return push_button(state, ok, SDL_GAMEPAD_BUTTON_EAST);
  if (name == "west")           return push_button(state, ok, SDL_GAMEPAD_BUTTON_WEST);
  if (name == "north")          return push_button(state, ok, SDL_GAMEPAD_BUTTON_NORTH);
  if (name == "back")           return push_button(state, ok, SDL_GAMEPAD_BUTTON_BACK);
  if (name == "guide")          return push_button(state, ok, SDL_GAMEPAD_BUTTON_GUIDE);
  if (name == "start")          return push_button(state, ok, SDL_GAMEPAD_BUTTON_START);
  if (name == "shoulder_left")  return push_button(state, ok, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
  if (name == "shoulder_right") return push_button(state, ok, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
  if (name == "stick_left")     return push_button(state, ok, SDL_GAMEPAD_BUTTON_LEFT_STICK);
  if (name == "stick_right")    return push_button(state, ok, SDL_GAMEPAD_BUTTON_RIGHT_STICK);
  if (name == "dpad_up")        return push_button(state, ok, SDL_GAMEPAD_BUTTON_DPAD_UP);
  if (name == "dpad_down")      return push_button(state, ok, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
  if (name == "dpad_left")      return push_button(state, ok, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
  if (name == "dpad_right")     return push_button(state, ok, SDL_GAMEPAD_BUTTON_DPAD_RIGHT);

  return lua_pushnil(state), 1;
}

void gamepad::wire() {
  lua_newuserdata(L, 1);

  luaL_newmetatable(L, "Gamepad");
  lua_pushcfunction(L, gamepad_index);
  lua_setfield(L, -2, "__index");

  lua_setmetatable(L, -2);
  lua_setglobal(L, "gamepad");
}
