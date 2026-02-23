#pragma once

#include "common.hpp"

class compositor;

class scene final {
public:
  scene(std::string_view name, compositor& compositor);
  ~scene() noexcept;

  void on_enter();

  void on_loop(float delta);

  void on_draw();

  void on_leave();

private:
  compositor& _compositor;
  int _G;
  int _environment;
  int _pool;
  int _table;
  entt::registry _registry;
  b2WorldId _world;
  int16_t _next_z{};
};
