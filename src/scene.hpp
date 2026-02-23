#pragma once

#include "common.hpp"

class scene final {
public:
  explicit scene(std::string_view name);
  ~scene() noexcept = default;

  void on_enter();

  void on_loop();

  void on_leave();

private:
  entt::registry _registry;
  b2WorldId _world;
  // sol::enviroment
};
