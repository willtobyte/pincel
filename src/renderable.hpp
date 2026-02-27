#pragma once

#include "common.hpp"

struct mapping final {
  entt::id_type name{};
  entt::id_type atlas{};
  entt::id_type entry{};
};

static_assert(std::is_trivially_copyable_v<mapping>);

struct mappable final {
  std::array<mapping, 16> mappings{};
  uint32_t count{};
};

static_assert(std::is_trivially_copyable_v<mappable>);

struct renderable final {
  entt::id_type atlas{};
  entt::id_type entry{};
  float counter{};
  uint32_t current_frame{};
};

static_assert(std::is_trivially_copyable_v<renderable>);
