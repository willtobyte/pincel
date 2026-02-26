#pragma once

#include "common.hpp"

class compositor final {
public:
  explicit compositor(atlasregistry& registry);
  ~compositor() = default;

  void push(atlas_id atlas, int index, float x, float y, float scale, float cosr, float sinr, uint8_t alpha);

  void draw();

private:
  atlasregistry& _registry;
  std::vector<class atlas*> _order;
  std::vector<int> _indices;
};
