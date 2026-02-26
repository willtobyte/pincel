#include "compositor.hpp"

compositor::compositor(atlasregistry& registry)
  : _registry(registry) {

  _indices.resize(4096 * 6);

  for (auto q = 0uz; q < 4096; ++q) {
    const auto base = static_cast<int>(q * 4);
    const auto idx = q * 6;
    _indices[idx + 0] = base;
    _indices[idx + 1] = base + 1;
    _indices[idx + 2] = base + 2;
    _indices[idx + 3] = base;
    _indices[idx + 4] = base + 2;
    _indices[idx + 5] = base + 3;
  }

  _draw_order.reserve(_registry._atlases.size());
  for (auto& [id, a] : _registry._atlases) {
    _draw_order.push_back(&a);
  }
  std::sort(_draw_order.begin(), _draw_order.end(),
    [](const atlas* a, const atlas* b) { return a->_layer < b->_layer; });
}

void compositor::push(atlas_id atlas, int index, float x, float y, float scale, float cosr, float sinr, uint8_t alpha) {
  const auto it = _registry._atlases.find(atlas);
  assert(it != _registry._atlases.end() && "atlas not found");
  auto& a = it->second;

  assert(index >= 0 && index < static_cast<int>(a._sprites.size()) && "sprite index out of bounds");
  const auto& sprite = a._sprites[static_cast<size_t>(index)];

  const auto hw = sprite.w * scale * 0.5f;
  const auto hh = sprite.h * scale * 0.5f;
  const auto color = SDL_FColor{1.0f, 1.0f, 1.0f, static_cast<float>(alpha) / 255.0f};

  a._vertices.push_back(SDL_Vertex{
    {-hw * cosr - -hh * sinr + x, -hw * sinr + -hh * cosr + y}, color, {sprite.u0, sprite.v0}});
  a._vertices.push_back(SDL_Vertex{
    {+hw * cosr - -hh * sinr + x, +hw * sinr + -hh * cosr + y}, color, {sprite.u1, sprite.v0}});
  a._vertices.push_back(SDL_Vertex{
    {+hw * cosr - +hh * sinr + x, +hw * sinr + +hh * cosr + y}, color, {sprite.u1, sprite.v1}});
  a._vertices.push_back(SDL_Vertex{
    {-hw * cosr - +hh * sinr + x, -hw * sinr + +hh * cosr + y}, color, {sprite.u0, sprite.v1}});
}

void compositor::draw() {
  for (auto* a : _draw_order) {
    if (a->_vertices.empty()) continue;

    const auto quad_count = a->_vertices.size() / 4;
    const auto index_count = quad_count * 6;

    if (index_count > static_cast<size_t>(_indices.size())) {
      const auto old_size = _indices.size() / 6;
      const auto new_size = quad_count;
      _indices.resize(new_size * 6);
      for (auto q = old_size; q < new_size; ++q) {
        const auto base = static_cast<int>(q * 4);
        const auto idx = q * 6;
        _indices[idx + 0] = base;
        _indices[idx + 1] = base + 1;
        _indices[idx + 2] = base + 2;
        _indices[idx + 3] = base;
        _indices[idx + 4] = base + 2;
        _indices[idx + 5] = base + 3;
      }
    }

    SDL_RenderGeometry(
      renderer,
      a->_texture.get(),
      a->_vertices.data(),
      static_cast<int>(a->_vertices.size()),
      _indices.data(),
      static_cast<int>(index_count)
    );

    a->_vertices.clear();
  }
}
