#include "compositor.hpp"

compositor::compositor() {
  const auto entries = io::enumerate("blobs/atlas");

  for (const auto& filename : entries) {
    if (!filename.ends_with(".png")) continue;
    auto name = std::filesystem::path{filename}.stem().string();
    const auto id = entt::hashed_string::value(name.c_str(), name.size());
    _atlases.emplace(id, atlas{name});
  }

  for (auto q = 0uz; q < max_compositor_vertices / 4; ++q) {
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

void compositor::submit(const entry& entry) {
  assert(_entry_count < max_entries && "entry count exceeds maximum");
  _entries[_entry_count++] = entry;
}

void compositor::submit(std::span<const entry> entries) {
  assert(_entry_count + entries.size() <= max_entries && "entry count exceeds maximum");
  std::copy(entries.begin(), entries.end(), _entries.begin() + static_cast<std::ptrdiff_t>(_entry_count));
  _entry_count += entries.size();
}

void compositor::draw() {
  if (_entry_count == 0) [[unlikely]] return;

  for (auto i = 0uz; i < _entry_count;) {
    const auto atlas_id = _entries[i].atlas;
    const auto it = _atlases.find(atlas_id);
    assert(it != _atlases.end() && "atlas not found");

    const auto& atlas = it->second;

    _vertex_count = 0;

    for (; i < _entry_count && _entries[i].atlas == atlas_id; ++i) {
      const auto& entry = _entries[i];
      assert(entry.index >= 0 && entry.index < static_cast<int>(atlas._sprite_count) && "sprite index out of bounds");
      const auto& sprite = atlas._sprites[static_cast<size_t>(entry.index)];

      const auto hw = sprite.w * entry.scale * 0.5f;
      const auto hh = sprite.h * entry.scale * 0.5f;
      const auto color = SDL_FColor{1.0f, 1.0f, 1.0f, static_cast<float>(entry.alpha) / 255.0f};

      assert(_vertex_count + 4 <= max_compositor_vertices && "vertex count exceeds maximum");
      _vertices[_vertex_count++] = SDL_Vertex{
        {-hw * entry.cosr - -hh * entry.sinr + entry.x, -hw * entry.sinr + -hh * entry.cosr + entry.y}, color, {sprite.u0, sprite.v0}};
      _vertices[_vertex_count++] = SDL_Vertex{
        {+hw * entry.cosr - -hh * entry.sinr + entry.x, +hw * entry.sinr + -hh * entry.cosr + entry.y}, color, {sprite.u1, sprite.v0}};
      _vertices[_vertex_count++] = SDL_Vertex{
        {+hw * entry.cosr - +hh * entry.sinr + entry.x, +hw * entry.sinr + +hh * entry.cosr + entry.y}, color, {sprite.u1, sprite.v1}};
      _vertices[_vertex_count++] = SDL_Vertex{
        {-hw * entry.cosr - +hh * entry.sinr + entry.x, -hw * entry.sinr + +hh * entry.cosr + entry.y}, color, {sprite.u0, sprite.v1}};
    }

    const auto index_count = (_vertex_count / 4) * 6;

    SDL_RenderGeometry(
      renderer,
      atlas._texture.get(),
      _vertices.data(),
      static_cast<int>(_vertex_count),
      _indices.data(),
      static_cast<int>(index_count)
    );
  }

  _entry_count = 0;
}
