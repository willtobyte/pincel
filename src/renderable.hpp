#pragma once

#include "common.hpp"

struct frame final {
  uint32_t sprite{};
  uint32_t duration{};
};

struct animation final {
  entt::id_type name{};
  std::array<frame, 16> frames{};
  uint32_t count{};
  entt::id_type next{};
};

struct renderable final {
  entt::id_type atlas{};
  entt::id_type animation{};
  uint32_t counter{};
  uint32_t current_frame{};
  uint32_t sprite{};
};
