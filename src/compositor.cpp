#include "compositor.hpp"
#include "io.hpp"

namespace {
  constexpr auto initial_quadrilateral_capacity = 1024uz;
  constexpr auto vertices_per_quadrilateral = 4uz;
  constexpr auto indices_per_quadrilateral = 6uz;
}

compositor::compositor() {
  const auto entries = io::enumerate("blobs/atlas");

  for (const auto& filename : entries) {
    if (!filename.ends_with(".png")) continue;
    auto name = std::filesystem::path{filename}.stem().string();
    _atlases.emplace(name, atlas{name});
  }

  _vertices.reserve(initial_quadrilateral_capacity * vertices_per_quadrilateral);
  _indices.reserve(initial_quadrilateral_capacity * indices_per_quadrilateral);
}

void compositor::submit(const entry& entry) {
  _entries.emplace_back(entry);
}

void compositor::submit(std::span<const entry> entries) {
  _entries.append_range(entries);
}

void compositor::draw() {
  if (_entries.empty()) [[unlikely]] return;

  const auto size = _entries.size();

  for (auto i = 0uz; i < size;) {
    const auto name = _entries[i].atlas;
    const auto it = _atlases.find(name);
    assert(it != _atlases.end() && "atlas not found");

    const auto& a = it->second;

    _vertices.clear();
    _indices.clear();

    for (; i < size && _entries[i].atlas == name; ++i) {
      const auto& e = _entries[i];
      assert(e.index >= 0 && e.index < static_cast<int>(a._sprites.size()) && "sprite index out of bounds");
      const auto& s = a._sprites[static_cast<size_t>(e.index)];

      const auto hw = s.w * e.scale * 0.5f;
      const auto hh = s.h * e.scale * 0.5f;
      const auto color = SDL_FColor{1.0f, 1.0f, 1.0f, static_cast<float>(e.alpha) / 255.0f};
      const auto base = static_cast<int>(_vertices.size());

      _vertices.emplace_back(SDL_Vertex{
        {-hw * e.cosr - -hh * e.sinr + e.x, -hw * e.sinr + -hh * e.cosr + e.y}, color, {s.u0, s.v0}});
      _vertices.emplace_back(SDL_Vertex{
        {+hw * e.cosr - -hh * e.sinr + e.x, +hw * e.sinr + -hh * e.cosr + e.y}, color, {s.u1, s.v0}});
      _vertices.emplace_back(SDL_Vertex{
        {+hw * e.cosr - +hh * e.sinr + e.x, +hw * e.sinr + +hh * e.cosr + e.y}, color, {s.u1, s.v1}});
      _vertices.emplace_back(SDL_Vertex{
        {-hw * e.cosr - +hh * e.sinr + e.x, -hw * e.sinr + +hh * e.cosr + e.y}, color, {s.u0, s.v1}});

      _indices.emplace_back(base);
      _indices.emplace_back(base + 1);
      _indices.emplace_back(base + 2);
      _indices.emplace_back(base);
      _indices.emplace_back(base + 2);
      _indices.emplace_back(base + 3);
    }

    SDL_RenderGeometry(
      renderer,
      a._texture.get(),
      _vertices.data(),
      static_cast<int>(_vertices.size()),
      _indices.data(),
      static_cast<int>(_indices.size())
    );
  }

  _entries.clear();
}
