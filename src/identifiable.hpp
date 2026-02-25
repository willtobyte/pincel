#pragma once

#include "common.hpp"

struct identifiable final {
  entt::id_type kind{};
  entt::id_type name{};
};

static_assert(std::is_trivially_copyable_v<identifiable>);
