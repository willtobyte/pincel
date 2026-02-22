#pragma once

#include "common.hpp"

class atlas final {
public:
  struct sprite {
    float u0, v0, u1, v1;
    float w, h;
  };

  struct command {
    int index;
    float x, y;
    float scale;
    float rotation;
    uint8_t alpha;
  };

  atlas() = delete;
  explicit atlas(std::string_view name);
  ~atlas() noexcept = default;

  void enqueue(std::span<const command> commands);
  void draw() const noexcept;

private:
  std::unique_ptr<SDL_Texture, SDL_Deleter> _texture;
  std::vector<sprite> _sprites;
  std::vector<SDL_Vertex> _vertices;
  std::vector<int> _indices;
};
