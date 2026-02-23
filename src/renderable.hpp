#pragma once

#include "common.hpp"

namespace {
  constexpr auto max_keyframes = 16uz;
  constexpr auto max_animations = 16uz;
}

struct alignas(8) keyframe final {
  uint32_t sprite{};
  uint32_t duration{};
};

static_assert(std::is_trivially_copyable_v<keyframe>);

struct alignas(64) animation final {
  std::array<keyframe, max_keyframes> keyframes{};
  entt::id_type name{};
  entt::id_type next{};
  uint32_t count{};
  bool once{};
};

static_assert(std::is_trivially_copyable_v<animation>);

struct alignas(64) animatable final {
  std::array<animation, max_animations> animations{};
  uint32_t count{};
};

static_assert(std::is_trivially_copyable_v<animatable>);

struct alignas(64) renderable final {
  entt::id_type atlas{};
  entt::id_type animation{};
  float counter{};
  uint32_t current_frame{};
  uint32_t sprite{};
};

static_assert(std::is_trivially_copyable_v<renderable>);
