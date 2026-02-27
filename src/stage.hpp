#pragma once

#include "common.hpp"

class soundregistry;
class stage;

namespace object {
  void create(stage&, int16_t, std::string_view, std::string_view, float, float, std::string_view);
}

class stage final {
  friend void object::create(stage&, int16_t, std::string_view, std::string_view, float, float, std::string_view);

public:
  stage(std::string_view name, atlasregistry& atlasregistry, compositor& compositor, soundregistry& soundregistry);
  ~stage() noexcept;

  void on_enter();

  void on_loop(float delta);

  void on_draw();

  void on_leave();

private:
  atlasregistry& _atlasregistry;
  compositor& _compositor;
  soundregistry& _soundregistry;
  int _G;
  int _environment;
  int _pool;
  int _table;
  std::vector<std::string> _sounds;
  entt::registry _registry;
  b2WorldId _world;
  float _accumulator{};
  int16_t _next_z{};
};
