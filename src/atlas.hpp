#pragma once

#include "common.hpp"

class atlas final {
public:
  struct alignas(8) sprite final {
    float u0, v0, u1, v1;
    float w, h;
    float hx{}, hy{}, hw{}, hh{};
    bool has_hitbox() const { return hw > 0 && hh > 0; }
  };

  atlas() = delete;
  explicit atlas(std::string_view name);
  ~atlas() noexcept = default;

  atlas(atlas&&) noexcept = default;
  atlas& operator=(atlas&&) noexcept = default;

private:
  friend class ::compositor;

  std::unique_ptr<SDL_Texture, SDL_Deleter> _texture;
  std::vector<sprite> _sprites;
};
