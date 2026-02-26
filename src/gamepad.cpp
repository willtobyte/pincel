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

static int gamepad_rumble(lua_State *state);

static int left_index(lua_State *state) {
  const std::string_view name = luaL_checkstring(state, 2);
  const bool ok = valid();

  if (name == "x") {
    if (ok) [[likely]]
      return lua_pushnumber(state, static_cast<double>(deadzone(SDL_GetGamepadAxis(pad.get(), SDL_GAMEPAD_AXIS_LEFTX)))), 1;
    return lua_pushnumber(state, 0), 1;
  }

  if (name == "y") {
    if (ok) [[likely]]
      return lua_pushnumber(state, static_cast<double>(deadzone(SDL_GetGamepadAxis(pad.get(), SDL_GAMEPAD_AXIS_LEFTY)))), 1;
    return lua_pushnumber(state, 0), 1;
  }

  return lua_pushnil(state), 1;
}

static int right_index(lua_State *state) {
  const std::string_view name = luaL_checkstring(state, 2);
  const bool ok = valid();

  if (name == "x") {
    if (ok) [[likely]]
      return lua_pushnumber(state, static_cast<double>(deadzone(SDL_GetGamepadAxis(pad.get(), SDL_GAMEPAD_AXIS_RIGHTX)))), 1;
    return lua_pushnumber(state, 0), 1;
  }

  if (name == "y") {
    if (ok) [[likely]]
      return lua_pushnumber(state, static_cast<double>(deadzone(SDL_GetGamepadAxis(pad.get(), SDL_GAMEPAD_AXIS_RIGHTY)))), 1;
    return lua_pushnumber(state, 0), 1;
  }

  return lua_pushnil(state), 1;
}

static int trigger_index(lua_State *state) {
  const std::string_view name = luaL_checkstring(state, 2);
  const bool ok = valid();

  if (name == "left") {
    if (ok) [[likely]]
      return lua_pushnumber(state, static_cast<double>(deadzone(SDL_GetGamepadAxis(pad.get(), SDL_GAMEPAD_AXIS_LEFT_TRIGGER)))), 1;
    return lua_pushnumber(state, 0), 1;
  }

  if (name == "right") {
    if (ok) [[likely]]
      return lua_pushnumber(state, static_cast<double>(deadzone(SDL_GetGamepadAxis(pad.get(), SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)))), 1;
    return lua_pushnumber(state, 0), 1;
  }

  return lua_pushnil(state), 1;
}

static int shoulder_index(lua_State *state) {
  const std::string_view name = luaL_checkstring(state, 2);
  const bool ok = valid();

  if (name == "left") {
    if (ok) [[likely]]
      return lua_pushboolean(state, SDL_GetGamepadButton(pad.get(), SDL_GAMEPAD_BUTTON_LEFT_SHOULDER)), 1;
    return lua_pushboolean(state, false), 1;
  }

  if (name == "right") {
    if (ok) [[likely]]
      return lua_pushboolean(state, SDL_GetGamepadButton(pad.get(), SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)), 1;
    return lua_pushboolean(state, false), 1;
  }

  return lua_pushnil(state), 1;
}

static int stick_index(lua_State *state) {
  const std::string_view name = luaL_checkstring(state, 2);
  const bool ok = valid();

  if (name == "left") {
    if (ok) [[likely]]
      return lua_pushboolean(state, SDL_GetGamepadButton(pad.get(), SDL_GAMEPAD_BUTTON_LEFT_STICK)), 1;
    return lua_pushboolean(state, false), 1;
  }

  if (name == "right") {
    if (ok) [[likely]]
      return lua_pushboolean(state, SDL_GetGamepadButton(pad.get(), SDL_GAMEPAD_BUTTON_RIGHT_STICK)), 1;
    return lua_pushboolean(state, false), 1;
  }

  return lua_pushnil(state), 1;
}

static int dpad_index(lua_State *state) {
  const std::string_view name = luaL_checkstring(state, 2);
  const bool ok = valid();

  static const std::unordered_map<std::string_view, SDL_GamepadButton> mapping{
    {"up", SDL_GAMEPAD_BUTTON_DPAD_UP}, {"down", SDL_GAMEPAD_BUTTON_DPAD_DOWN},
    {"left", SDL_GAMEPAD_BUTTON_DPAD_LEFT}, {"right", SDL_GAMEPAD_BUTTON_DPAD_RIGHT}
  };

  const auto it = mapping.find(name);
  if (it != mapping.end()) {
    if (ok) [[likely]]
      return lua_pushboolean(state, SDL_GetGamepadButton(pad.get(), it->second)), 1;
    return lua_pushboolean(state, false), 1;
  }

  return lua_pushnil(state), 1;
}

static void push_sub_component(lua_State *state, const char *metaname, lua_CFunction index_fn) {
  lua_newuserdata(state, 1);

  if (luaL_newmetatable(state, metaname)) {
    lua_pushcfunction(state, index_fn);
    lua_setfield(state, -2, "__index");
  }

  lua_setmetatable(state, -2);
}

static const std::unordered_map<std::string_view, SDL_GamepadButton> button_mapping{
  {"south", SDL_GAMEPAD_BUTTON_SOUTH}, {"east", SDL_GAMEPAD_BUTTON_EAST},
  {"west", SDL_GAMEPAD_BUTTON_WEST}, {"north", SDL_GAMEPAD_BUTTON_NORTH},
  {"back", SDL_GAMEPAD_BUTTON_BACK}, {"guide", SDL_GAMEPAD_BUTTON_GUIDE},
  {"start", SDL_GAMEPAD_BUTTON_START}
};

static int gamepad_index(lua_State *state) {
  const std::string_view name = luaL_checkstring(state, 2);
  const bool ok = valid();

  if (name == "connected")
    return lua_pushboolean(state, ok), 1;

  if (name == "name") {
    if (ok) [[likely]]
      return lua_pushstring(state, SDL_GetGamepadName(pad.get())), 1;
    return lua_pushstring(state, ""), 1;
  }

  if (name == "rumble")
    return lua_pushcfunction(state, gamepad_rumble), 1;

  if (name == "left")
    return push_sub_component(state, "GamepadLeft", left_index), 1;

  if (name == "right")
    return push_sub_component(state, "GamepadRight", right_index), 1;

  if (name == "trigger")
    return push_sub_component(state, "GamepadTrigger", trigger_index), 1;

  if (name == "shoulder")
    return push_sub_component(state, "GamepadShoulder", shoulder_index), 1;

  if (name == "stick")
    return push_sub_component(state, "GamepadStick", stick_index), 1;

  if (name == "dpad")
    return push_sub_component(state, "GamepadDpad", dpad_index), 1;

  {
    const auto it = button_mapping.find(name);
    if (it != button_mapping.end()) [[likely]] {
      if (ok) [[likely]]
        return lua_pushboolean(state, SDL_GetGamepadButton(pad.get(), it->second)), 1;
      return lua_pushboolean(state, false), 1;
    }
  }

  lua_pushnil(state);
  return 1;
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

void gamepad::wire() {
  lua_newuserdata(L, 1);

  luaL_newmetatable(L, "Gamepad");
  lua_pushcfunction(L, gamepad_index);
  lua_setfield(L, -2, "__index");

  lua_setmetatable(L, -2);
  lua_setglobal(L, "gamepad");
}
