#pragma once

#include "common.hpp"

struct alignas(64) transform final {
  float x{};
  float y{};
  float scale{1.0f};
  float angle{};
  uint8_t alpha{255};
  bool visible{true};
};

static_assert(std::is_trivially_copyable_v<transform>);
