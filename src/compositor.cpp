#include "compositor.hpp"

compositor::compositor() {
  const auto entries = io::enumerate("blobs/atlas");

  for (const auto& filename : entries) {
    if (!filename.ends_with(".png")) continue;
    auto name = std::filesystem::path{filename}.stem().string();
    const auto id = entt::hashed_string::value(name.c_str(), name.size());
    _atlases.emplace(id, atlas{name});
  }

  _entries.reserve(4096);
  _vertices.reserve(4096 * 4);
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
}

void compositor::submit(const entry& entry) {
  _entries.push_back(entry);
}

void compositor::submit(std::span<const entry> entries) {
  _entries.insert(_entries.end(), entries.begin(), entries.end());
}

const atlas::sprite* compositor::get_sprite(entt::id_type atlas_id, int index) const {
  const auto it = _atlases.find(atlas_id);
  if (it == _atlases.end()) return nullptr;

  const auto& a = it->second;
  if (index < 0 || index >= static_cast<int>(a._sprites.size())) return nullptr;

  return &a._sprites[static_cast<size_t>(index)];
}

void compositor::draw() {
  draw(0);
  _entries.clear();
}

void compositor::draw(size_t i) {
  if (i >= _entries.size()) return;

  const auto atlas_id = _entries[i].atlas;
  const auto it = _atlases.find(atlas_id);
  assert(it != _atlases.end() && "atlas not found");
  const auto& atlas = it->second;

  _vertices.clear();

  while (i < _entries.size() && _entries[i].atlas == atlas_id) {
    const auto& e = _entries[i];
    assert(e.index >= 0 && e.index < static_cast<int>(atlas._sprites.size()) && "sprite index out of bounds");
    const auto& sprite = atlas._sprites[static_cast<size_t>(e.index)];

    const auto hw = sprite.w * e.scale * 0.5f;
    const auto hh = sprite.h * e.scale * 0.5f;
    const auto color = SDL_FColor{1.0f, 1.0f, 1.0f, static_cast<float>(e.alpha) / 255.0f};

    _vertices.push_back(SDL_Vertex{
      {-hw * e.cosr - -hh * e.sinr + e.x, -hw * e.sinr + -hh * e.cosr + e.y}, color, {sprite.u0, sprite.v0}});
    _vertices.push_back(SDL_Vertex{
      {+hw * e.cosr - -hh * e.sinr + e.x, +hw * e.sinr + -hh * e.cosr + e.y}, color, {sprite.u1, sprite.v0}});
    _vertices.push_back(SDL_Vertex{
      {+hw * e.cosr - +hh * e.sinr + e.x, +hw * e.sinr + +hh * e.cosr + e.y}, color, {sprite.u1, sprite.v1}});
    _vertices.push_back(SDL_Vertex{
      {-hw * e.cosr - +hh * e.sinr + e.x, -hw * e.sinr + +hh * e.cosr + e.y}, color, {sprite.u0, sprite.v1}});

    ++i;
  }

  SDL_RenderGeometry(
    renderer,
    atlas._texture.get(),
    _vertices.data(),
    static_cast<int>(_vertices.size()),
    _indices.data(),
    static_cast<int>((_vertices.size() / 4) * 6)
  );

  draw(i);
}
