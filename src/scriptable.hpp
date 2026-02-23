#pragma once

#include "common.hpp"

struct alignas(64) scriptable final {
  int on_loop{LUA_NOREF};
};

static_assert(std::is_trivially_copyable_v<scriptable>);
