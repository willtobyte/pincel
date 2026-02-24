#pragma once

#include "common.hpp"

struct alignas(64) scriptable final {
  int on_spawn{LUA_NOREF};
  int on_loop{LUA_NOREF};
  int on_animation_end{LUA_NOREF};
  int self_ref{LUA_NOREF};
};

static_assert(std::is_trivially_copyable_v<scriptable>);
