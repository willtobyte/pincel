#pragma once

#include "common.hpp"

class compositor final {
public:
  struct entry {
    entt::id_type atlas{};
    int index;
    float x, y, scale, cosr, sinr;
    uint8_t alpha;
  };

  compositor();
  ~compositor() = default;

  void submit(const entry& entry);
  void submit(std::span<const entry> entries);
  void draw();

  const atlas::sprite* get_sprite(entt::id_type atlas_id, int index) const;

private:
  void draw(size_t i);

  std::unordered_map<entt::id_type, class atlas> _atlases;
  std::vector<entry> _entries;
  std::vector<SDL_Vertex> _vertices;
  std::vector<int> _indices;
};
