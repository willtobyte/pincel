#pragma once

#include <charconv>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <print>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <variant>

#include <entt/entt.hpp>
#include <lua.hpp>
#include <lauxlib.h>
#include <luajit.h>
#include <miniaudio.h>
#include <opusfile.h>
#include <physfs.h>
#include <SDL3/SDL.h>
#include <spng.h>

extern lua_State* L;
extern SDL_Renderer* renderer;
extern ma_engine* audioengine;

#include "helper.hpp"
