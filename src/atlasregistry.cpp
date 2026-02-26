#include "atlasregistry.hpp"

atlasregistry::atlasregistry() {
  const auto entries = io::enumerate("blobs/atlas");

  for (const auto& filename : entries) {
    if (!filename.ends_with(".png")) continue;
    auto name = std::filesystem::path{filename}.stem().string();
    const auto id = entt::hashed_string::value(name.c_str(), name.size());
    _atlases.emplace(id, atlas{name});
  }
}

const atlas::sprite& atlasregistry::sprite(atlas_id id, int index) const {
  const auto it = _atlases.find(id);
  assert(it != _atlases.end() && "atlas not found");

  const auto& a = it->second;
  assert(index >= 0 && index < static_cast<int>(a._sprites.size()) && "sprite index out of bounds");

  return a._sprites[static_cast<size_t>(index)];
}
