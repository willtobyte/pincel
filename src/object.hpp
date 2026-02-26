#pragma once

#include "common.hpp"

namespace object {
  void setup(entt::registry& registry);

  void create(
    entt::registry& registry,
    b2WorldId world,
    atlasregistry& atlasregistry,
    int pool,
    int16_t z,
    std::string_view name,
    std::string_view kind,
    float x,
    float y,
    std::string_view initial_animation
  );

  void destroy(entt::registry& registry, int pool, std::string_view name);

  void update(entt::registry& registry, atlasregistry& atlasregistry);
}
