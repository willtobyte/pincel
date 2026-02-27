#pragma once

#include "common.hpp"

class atlasregistry;
class compositor;

class atlas final {
public:
  struct alignas(8) sprite final {
    float u0, v0, u1, v1;
    float w, h;
    float hx{}, hy{}, hw{}, hh{};
  };

  struct keyframe final {
    sprite sprite;
    uint32_t duration{};
  };

  struct animation final {
    std::vector<keyframe> keyframes;
    entt::id_type next{};
    bool once{};
  };

  atlas() = delete;
  explicit atlas(std::string_view name);
  ~atlas() noexcept = default;

  atlas(atlas&&) noexcept = default;
  atlas& operator=(atlas&&) noexcept = default;

  const animation* find(entt::id_type entry) const;
  const keyframe& keyframe_at(entt::id_type entry, uint32_t frame) const;

private:
  friend class ::atlasregistry;
  friend class ::compositor;

  std::unique_ptr<SDL_Texture, SDL_Deleter> _texture;
  std::unordered_map<entt::id_type, animation> _entries;
  std::vector<SDL_Vertex> _vertices;
};
