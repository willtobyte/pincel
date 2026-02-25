#pragma once

#include "common.hpp"

struct sorteable final {
  int16_t z{};
};

static_assert(std::is_trivially_copyable_v<sorteable>);
