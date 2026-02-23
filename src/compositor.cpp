#include "compositor.hpp"
#include "io.hpp"

compositor::compositor() {
  const auto entries = io::enumerate("blobs/atlas");

  for (const auto& filename : entries) {
    if (!filename.ends_with(".png")) continue;
    auto name = std::filesystem::path{filename}.stem().string();
    const auto id = entt::hashed_string::value(name.c_str(), name.size());
    _atlases.emplace(id, atlas{name});
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

  std::sort(_entries.begin(), _entries.begin() + static_cast<std::ptrdiff_t>(_entry_count),
    [](const entry& a, const entry& b) {
      return a.z < b.z || (a.z == b.z && a.atlas < b.atlas);
    });

  for (auto i = 0uz; i < _entry_count;) {
    const auto atlas_id = _entries[i].atlas;
    const auto it = _atlases.find(atlas_id);
    assert(it != _atlases.end() && "atlas not found");

    const auto& a = it->second;

    _vertex_count = 0;
    _index_count = 0;

    const auto z = _entries[i].z;

    for (; i < _entry_count && _entries[i].z == z && _entries[i].atlas == atlas_id; ++i) {
      const auto& e = _entries[i];
      assert(e.index >= 0 && e.index < static_cast<int>(a._sprite_count) && "sprite index out of bounds");
      const auto& s = a._sprites[static_cast<size_t>(e.index)];

      const auto hw = s.w * e.scale * 0.5f;
      const auto hh = s.h * e.scale * 0.5f;
      const auto color = SDL_FColor{1.0f, 1.0f, 1.0f, static_cast<float>(e.alpha) / 255.0f};
      const auto base = static_cast<int>(_vertex_count);

      assert(_vertex_count + 4 <= max_compositor_vertices && "vertex count exceeds maximum");
      _vertices[_vertex_count++] = SDL_Vertex{
        {-hw * e.cosr - -hh * e.sinr + e.x, -hw * e.sinr + -hh * e.cosr + e.y}, color, {s.u0, s.v0}};
      _vertices[_vertex_count++] = SDL_Vertex{
        {+hw * e.cosr - -hh * e.sinr + e.x, +hw * e.sinr + -hh * e.cosr + e.y}, color, {s.u1, s.v0}};
      _vertices[_vertex_count++] = SDL_Vertex{
        {+hw * e.cosr - +hh * e.sinr + e.x, +hw * e.sinr + +hh * e.cosr + e.y}, color, {s.u1, s.v1}};
      _vertices[_vertex_count++] = SDL_Vertex{
        {-hw * e.cosr - +hh * e.sinr + e.x, -hw * e.sinr + +hh * e.cosr + e.y}, color, {s.u0, s.v1}};

      assert(_index_count + 6 <= max_compositor_indices && "index count exceeds maximum");
      _indices[_index_count++] = base;
      _indices[_index_count++] = base + 1;
      _indices[_index_count++] = base + 2;
      _indices[_index_count++] = base;
      _indices[_index_count++] = base + 2;
      _indices[_index_count++] = base + 3;
    }

    SDL_RenderGeometry(
      renderer,
      a._texture.get(),
      _vertices.data(),
      static_cast<int>(_vertex_count),
      _indices.data(),
      static_cast<int>(_index_count)
    );
  }

  _entry_count = 0;
}
