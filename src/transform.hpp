#pragma once

#include "common.hpp"

struct transform final {
  float x{};
  float y{};
  float scale{1.0f};
  float angle{};
  uint8_t alpha{255};
  bool visible{true};
};
