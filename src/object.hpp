#pragma once

#include "common.hpp"
#include "sorteable.hpp"
#include "renderable.hpp"

class object final {
public:
  object(entt::registry& registry, int16_t z, std::string_view name);
  ~object() noexcept;

  object(const object&) = delete;
  object& operator=(const object&) = delete;

  object(object&& other) noexcept;
  object& operator=(object&& other) noexcept;

  [[nodiscard]] int16_t z() const;
  void z(int16_t value);

private:
  entt::registry* _registry{};
  entt::entity _entity{entt::null};
  std::vector<animation> _animations;
};
