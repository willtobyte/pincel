#pragma once

#include "common.hpp"

class atlasregistry;
class compositor;

namespace presenter {
  void render(entt::registry& registry, atlasregistry& atlasregistry, compositor& compositor);
}
