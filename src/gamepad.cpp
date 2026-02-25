#include "gamepad.hpp"

static constexpr float DEADZONE_THRESHOLD = 0.1f;

static float deadzone(Sint16 raw) {
  const auto value = static_cast<float>(raw) / 32767.0f;
  if (std::abs(value) < DEADZONE_THRESHOLD)
    return 0.0f;

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

static int gamepad_rumble(lua_State *state);

static int gamepad_index(lua_State *state) {
  const std::string_view name = luaL_checkstring(state, 2);
  const bool ok = valid();

  if (name == "name") {
    if (ok) [[likely]]
      return lua_pushstring(state, SDL_GetGamepadName(pad.get())), 1;

    return lua_pushstring(state, ""), 1;
  }

  if (name == "rumble")
    return lua_pushcfunction(state, gamepad_rumble), 1;

  if (name == "leftaxis") {
    if (ok) [[likely]] {
      lua_pushnumber(state, static_cast<double>(deadzone(SDL_GetGamepadAxis(pad.get(), SDL_GAMEPAD_AXIS_LEFTX))));
      lua_pushnumber(state, static_cast<double>(deadzone(SDL_GetGamepadAxis(pad.get(), SDL_GAMEPAD_AXIS_LEFTY))));
      return 2;
    }

    return lua_pushnumber(state, 0), lua_pushnumber(state, 0), 2;
  }

  if (name == "rightaxis") {
    if (ok) [[likely]] {
      lua_pushnumber(state, static_cast<double>(deadzone(SDL_GetGamepadAxis(pad.get(), SDL_GAMEPAD_AXIS_RIGHTX))));
      lua_pushnumber(state, static_cast<double>(deadzone(SDL_GetGamepadAxis(pad.get(), SDL_GAMEPAD_AXIS_RIGHTY))));
      return 2;
    }

    return lua_pushnumber(state, 0), lua_pushnumber(state, 0), 2;
  }

  if (name == "triggers") {
    if (ok) [[likely]] {
      lua_pushnumber(state, static_cast<double>(deadzone(SDL_GetGamepadAxis(pad.get(), SDL_GAMEPAD_AXIS_LEFT_TRIGGER))));
      lua_pushnumber(state, static_cast<double>(deadzone(SDL_GetGamepadAxis(pad.get(), SDL_GAMEPAD_AXIS_RIGHT_TRIGGER))));
      return 2;
    }

    return lua_pushnumber(state, 0), lua_pushnumber(state, 0), 2;
  }

  {
    const auto it = button_mapping.find(name);
    if (it != button_mapping.end()) [[likely]] {
      if (ok) [[likely]]
        return lua_pushboolean(state, SDL_GetGamepadButton(pad.get(), it->second)), 1;

      return lua_pushboolean(state, false), 1;
    }
  }

  {
    const auto it = axis_mapping.find(name);
    if (it != axis_mapping.end()) {
      if (ok) [[likely]]
        return lua_pushnumber(state, static_cast<double>(deadzone(SDL_GetGamepadAxis(pad.get(), it->second)))), 1;

      return lua_pushnumber(state, 0), 1;
    }
  }

  lua_pushnil(state);
  return 1;
}

static int gamepad_rumble(lua_State *state) {
  const auto low = std::clamp(static_cast<float>(luaL_checknumber(state, 2)), 0.0f, 1.0f);
  const auto high = std::clamp(static_cast<float>(luaL_checknumber(state, 3)), 0.0f, 1.0f);
  const auto duration = static_cast<Uint32>(luaL_checkinteger(state, 4));

  const auto low16 = static_cast<Uint16>(low * 65535.0f);
  const auto high16 = static_cast<Uint16>(high * 65535.0f);

  if (valid() && pad) [[likely]]
    lua_pushboolean(state, SDL_RumbleGamepad(pad.get(), low16, high16, duration));
  else
    lua_pushboolean(state, false);

  return 1;
}

void gamepad::wire() {
  lua_newuserdata(L, 1);

  luaL_newmetatable(L, "Gamepad");
  lua_pushcfunction(L, gamepad_index);
  lua_setfield(L, -2, "__index");

  lua_setmetatable(L, -2);
  lua_setglobal(L, "gamepad");
}
