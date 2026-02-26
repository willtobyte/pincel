#pragma once

#include "common.hpp"

class scene;
class soundregistry;

class manager final {
public:
  manager();
  ~manager();

  void set(std::string_view name);

  void update(float delta);

  void draw();

private:
  std::unique_ptr<atlasregistry> _atlasregistry;
  std::unique_ptr<compositor> _compositor;
  std::unique_ptr<soundregistry> _soundregistry;
  std::unordered_map<std::string, std::unique_ptr<scene>, transparent_hash, std::equal_to<>> _scenes;
  scene* _active{nullptr};
};
