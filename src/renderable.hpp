#pragma once

namespace {
  constexpr auto max_keyframes = 16uz;
  constexpr auto max_animations = 16uz;
}

struct keyframe final {
  uint32_t sprite{};
  uint32_t duration{};
};

struct animation final {
  entt::id_type name{};
  std::array<keyframe, max_keyframes> keyframes{};
  uint32_t count{};
  entt::id_type next{};
  bool once{};
};

struct animatable final {
  std::array<animation, max_animations> animations{};
  uint32_t count{};
};

struct renderable final {
  entt::id_type atlas{};
  entt::id_type animation{};
  float counter{};
  uint32_t current_frame{};
  uint32_t sprite{};
};
