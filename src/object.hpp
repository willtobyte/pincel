#pragma once

struct object final {
  static entt::entity create(entt::registry& registry, int16_t z, std::string_view name);
};
