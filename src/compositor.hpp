#pragma once

#include "common.hpp"
#include "atlas.hpp"

namespace {
  constexpr auto max_entries = 2048uz;
  constexpr auto max_compositor_vertices = 8192uz;
  constexpr auto max_compositor_indices = 12288uz;
}

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
  std::array<entry, max_entries> _entries{};
  std::size_t _entry_count{0};
  std::array<SDL_Vertex, max_compositor_vertices> _vertices{};
  std::size_t _vertex_count{0};
  std::array<int, max_compositor_indices> _indices{};
  std::size_t _index_count{0};
};
