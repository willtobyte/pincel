#pragma once

#include "common.hpp"

namespace object {
  void setup(entt::registry& registry);

  void create(
    class stage& stage,
    int16_t z,
    std::string_view name,
    std::string_view kind,
    float x,
    float y,
    std::string_view initial_animation
  );

  void update(entt::registry& registry, atlasregistry& atlasregistry);
}
