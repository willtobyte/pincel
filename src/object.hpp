#pragma once

#include "common.hpp"

extern std::unordered_map<entt::id_type, std::string> lookup;

namespace object {
  void register_metatable();
  void connect_signals(entt::registry& registry);

  void create(
    entt::registry& registry,
    b2WorldId world,
    compositor& compositor,
    int pool,
    int16_t z,
    std::string_view name,
    std::string_view kind,
    float x,
    float y,
    std::string_view animation
  );

  void sync_collision(entt::registry& registry, compositor& compositor);
}
