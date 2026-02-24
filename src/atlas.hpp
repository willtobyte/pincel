#pragma once

#include "common.hpp"

class atlas final {
public:
  struct alignas(8) sprite final {
    float u0, v0, u1, v1;
    float w, h;
  };

  atlas() = delete;
  explicit atlas(std::string_view name);
  ~atlas() noexcept = default;

  atlas(atlas&&) noexcept = default;
  atlas& operator=(atlas&&) noexcept = default;

private:
  friend class ::compositor;

  std::unique_ptr<SDL_Texture, SDL_Deleter> _texture;
  std::array<sprite, 4096> _sprites{};
  std::size_t _sprite_count{0};
};
