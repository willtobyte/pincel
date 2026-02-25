#pragma once

#include "common.hpp"

struct alignas(64) scriptable final {
  int on_spawn{LUA_NOREF};
  int on_loop{LUA_NOREF};
  int on_animation_end{LUA_NOREF};
  int on_collision{LUA_NOREF};
  int on_collision_end{LUA_NOREF};
  int on_screen_exit{LUA_NOREF};
  int on_screen_enter{LUA_NOREF};
  int self_ref{LUA_NOREF};
  uint8_t screen_previous{0};

  static constexpr uint8_t screen_left   = 1 << 0;
  static constexpr uint8_t screen_right  = 1 << 1;
  static constexpr uint8_t screen_top    = 1 << 2;
  static constexpr uint8_t screen_bottom = 1 << 3;
};
