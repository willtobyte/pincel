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
  void update();
  void draw() const;

private:
  struct step {
    SDL_Texture* texture;
    int vertex_begin;
    int vertex_count;
    int index_begin;
    int index_count;
  };

  std::vector<class atlas> _atlases;
  std::vector<class font> _fonts;
  std::vector<entry> _entries;
  std::vector<step> _sequence;
  std::vector<SDL_Vertex> _vertices;
  std::vector<int> _indices;
};
