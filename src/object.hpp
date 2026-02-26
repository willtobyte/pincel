#pragma once

#include "common.hpp"

struct lookupable final {
  entt::dense_map<entt::id_type, std::string> names{};
};

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

  void update(entt::registry& registry, atlasregistry& atlasregistry);
}
