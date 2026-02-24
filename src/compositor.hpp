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

private:
  std::unordered_map<entt::id_type, class atlas> _atlases;
  std::array<entry, 4096> _entries{};
  std::size_t _entry_count{0};
  std::array<SDL_Vertex, 4096 * 4> _vertices{};
  std::size_t _vertex_count{0};
  std::array<int, 4096 * 6> _indices{};
};
