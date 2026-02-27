#include "compositor.hpp"

namespace {
  constexpr size_t quads = 4096;
}

compositor::compositor(atlasregistry& registry)
  : _registry(registry) {

  _indices.resize(quads * 6);

  for (auto q = 0uz; q < quads; ++q) {
    const auto base = static_cast<int>(q * 4);
    const auto i = q * 6;
    _indices[i + 0] = base;
    _indices[i + 1] = base + 1;
    _indices[i + 2] = base + 2;
    _indices[i + 3] = base;
    _indices[i + 4] = base + 2;
    _indices[i + 5] = base + 3;
  }
}

void compositor::push(atlas& a, const atlas::sprite& sprite, float x, float y, float scale, float cosr, float sinr, uint8_t alpha) {
  const auto hw = sprite.w * scale * 0.5f;
  const auto hh = sprite.h * scale * 0.5f;
  const auto color = SDL_FColor{1.0f, 1.0f, 1.0f, static_cast<float>(alpha) / 255.0f};

  const auto size = a._vertices.size();
  assert(size / 4 < quads && "quad limit exceeded");

  a._vertices.resize(size + 4);
  auto* v = a._vertices.data() + size;

  v[0] = {{-hw * cosr + hh * sinr + x, -hw * sinr - hh * cosr + y}, color, {sprite.u0, sprite.v0}};
  v[1] = {{+hw * cosr + hh * sinr + x, +hw * sinr - hh * cosr + y}, color, {sprite.u1, sprite.v0}};
  v[2] = {{+hw * cosr - hh * sinr + x, +hw * sinr + hh * cosr + y}, color, {sprite.u1, sprite.v1}};
  v[3] = {{-hw * cosr - hh * sinr + x, -hw * sinr + hh * cosr + y}, color, {sprite.u0, sprite.v1}};
}

void compositor::draw() {
  for (auto& [id, a] : _registry._atlases) {
    if (a._vertices.empty()) continue;

    const auto count = static_cast<int>(a._vertices.size());
    assert(count % 4 == 0 && "vertex count must be a multiple of 4");

    SDL_RenderGeometry(
      renderer,
      a._texture.get(),
      a._vertices.data(),
      count,
      _indices.data(),
      count / 4 * 6
    );

    a._vertices.clear();
  }
}
