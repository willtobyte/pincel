#pragma once

#include "common.hpp"

class soundfx;
class soundregistry;

struct soundproxy final {
  soundfx* fx;
  int on_start{LUA_NOREF};
  int on_end{LUA_NOREF};
};

namespace soundsystem {
  void wire();
  void dispatch(int pool, const std::vector<std::string>& names, soundregistry& registry);
}
