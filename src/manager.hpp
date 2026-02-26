#pragma once

#include "common.hpp"

class stage;
class soundregistry;

class manager final {
public:
  manager();
  ~manager();

  void set(std::string_view name);

  void request(std::string_view name);

  const std::string& current() const;

  void update(float delta);

  void draw();

private:
  std::unique_ptr<atlasregistry> _atlasregistry;
  std::unique_ptr<compositor> _compositor;
  std::unique_ptr<soundregistry> _soundregistry;
  std::unordered_map<std::string, std::unique_ptr<stage>, transparent_hash, std::equal_to<>> _stages;
  stage* _active{nullptr};
  std::optional<std::string> _pending;
  std::string _current;
};
