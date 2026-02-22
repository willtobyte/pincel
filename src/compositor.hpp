#pragma once

#include "common.hpp"
#include "atlas.hpp"
#include "font.hpp"

enum category : uint8_t { sprite, text };

class compositor final {
public:
  struct entry {
    category kind;
    union {
      struct {
        int atlas;
        int index;
        float x, y;
        float scale;
        float cosr, sinr;
        uint8_t alpha;
      } sprite;
      struct {
        int font;
        std::string_view content;
        float x, y;
      } text;
    };
  };

  compositor();
  ~compositor() = default;

  void submit(const entry& entry);
  void submit(std::span<const entry> entries);
  void draw();

private:
  std::vector<class atlas> _atlases;
  std::vector<class font> _fonts;
  std::vector<entry> _entries;
  std::vector<SDL_Vertex> _vertices;
  std::vector<int> _indices;
};
