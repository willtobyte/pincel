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

atlas& atlasregistry::get(atlas_id id) {
  const auto it = _atlases.find(id);
  assert(it != _atlases.end() && "atlas not found");
  return it->second;
}

const atlas& atlasregistry::get(atlas_id id) const {
  const auto it = _atlases.find(id);
  assert(it != _atlases.end() && "atlas not found");
  return it->second;
}
