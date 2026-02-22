#pragma once

#include "common.hpp"
#include "atlas.hpp"

class compositor final {
public:
  struct entry {
    std::string_view atlas;
    int index;
    float x, y, scale, cosr, sinr;
    uint8_t alpha;
  };

  compositor();
  ~compositor() = default;

  void submit(const entry& entry);
  void submit(std::span<const entry> entries);
  void draw();

private:
  std::unordered_map<std::string, class atlas, transparent_hash, std::equal_to<>> _atlases;
  std::vector<entry> _entries;
  std::vector<SDL_Vertex> _vertices;
  std::vector<int> _indices;
};
