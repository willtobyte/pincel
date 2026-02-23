#pragma once

#include "common.hpp"
#include "atlas.hpp"

class compositor final {
public:
  struct entry {
    entt::id_type atlas{};
    int index;
    float x, y, scale, cosr, sinr;
    uint8_t alpha;
    int16_t z{};
  };

  compositor();
  ~compositor() = default;

  void submit(const entry& entry);
  void submit(std::span<const entry> entries);
  void draw();

private:
  std::unordered_map<entt::id_type, class atlas> _atlases;
  std::vector<entry> _entries;
  std::vector<SDL_Vertex> _vertices;
  std::vector<int> _indices;
};
