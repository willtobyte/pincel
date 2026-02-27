#pragma once

#include "common.hpp"

class compositor final {
public:
  explicit compositor(atlasregistry& registry);
  ~compositor() = default;

  void push(atlas& atlas, const atlas::sprite& sprite, float x, float y, float scale, float cosr, float sinr, uint8_t alpha);

  void draw();

private:
  atlasregistry& _registry;
  std::vector<int> _indices;
};
