#pragma once

#include "common.hpp"

struct keyframe final {
  uint32_t sprite{};
  uint32_t duration{};
};

static_assert(std::is_trivially_copyable_v<keyframe>);

struct animation final {
  std::array<keyframe, 16> keyframes{};
  entt::id_type name{};
  entt::id_type atlas{};
  entt::id_type next{};
  uint32_t count{};
  bool once{};
};

static_assert(std::is_trivially_copyable_v<animation>);

struct animatable final {
  std::array<animation, 16> animations{};
  uint32_t count{};
};

static_assert(std::is_trivially_copyable_v<animatable>);

struct renderable final {
  entt::id_type atlas{};
  entt::id_type animation{};
  float counter{};
  uint32_t current_frame{};
  uint32_t sprite{};
};

static_assert(std::is_trivially_copyable_v<renderable>);
