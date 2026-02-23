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
  std::array<entry, 2048> _entries{};
  std::size_t _entry_count{0};
  std::array<SDL_Vertex, 8192> _vertices{};
  std::size_t _vertex_count{0};
  std::array<int, 12288> _indices{};
  std::size_t _index_count{0};
};
