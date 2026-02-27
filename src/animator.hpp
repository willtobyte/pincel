#pragma once

#include "common.hpp"

class atlasregistry;

namespace animator {
  void update(entt::registry& registry, atlasregistry& atlasregistry, float delta);
}
