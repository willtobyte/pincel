#pragma once

#include "common.hpp"
#include "compositor.hpp"
#include "scene.hpp"

class manager final {
public:
  manager();
  ~manager();

  void set(std::string_view name);

  void update();

  void draw();

private:
  compositor _compositor;
  std::unordered_map<std::string, std::unique_ptr<scene>, transparent_hash, std::equal_to<>> _scenes;
  scene* _active{nullptr};
};
