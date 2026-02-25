#pragma once

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <numbers>
#include <optional>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <variant>

#include <box2d/box2d.h>
#include <entt/entt.hpp>
#include <lua.hpp>
#include <lauxlib.h>
#include <luajit.h>
#include <miniaudio.h>
#include <opusfile.h>
#include <physfs.h>
#include <SDL3/SDL.h>
#include <spng.h>

class compositor;

extern lua_State* L;
extern SDL_Renderer* renderer;
extern ma_engine* audioengine;

struct viewport {
  float width;
  float height;
};

extern struct viewport viewport;

constexpr auto MAX_DELTA = 0.05f;

#include "helper.hpp"
