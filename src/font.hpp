#pragma once

#include "common.hpp"

struct alignas(32) glyphprops final {
  float u0, v0, u1, v1;
  float sw, sh;
  float w;
  bool valid;
};

class font final {
public:
  font() = delete;

  explicit font(std::string_view family);

  ~font() = default;

  font(font&&) noexcept = default;
  font& operator=(font&&) noexcept = default;

  void enqueue(std::string_view text, SDL_FPoint position);
  void draw() const noexcept;
  void clear();

  std::string_view glyphs() const noexcept;

private:
  std::unique_ptr<SDL_Texture, SDL_Deleter> _texture;
  int _width{0};
  int _height{0};
  int16_t _spacing{0};
  int16_t _leading{0};
  float _fontheight{0.f};
  float _scale{1.f};
  std::string _glyphs;
  std::array<glyphprops, 256> _props{};
  std::vector<SDL_Vertex> _vertices;
  std::vector<int> _indices;
};
