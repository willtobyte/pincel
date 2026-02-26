#include "soundregistry.hpp"

soundfx& soundregistry::get_or_load(std::string_view name) {
  const auto it = _sounds.find(name);
  if (it != _sounds.end())
    return *it->second;

  auto filepath = std::format("blobs/sounds/{}.opus", name);
  auto sound = std::make_unique<soundfx>(filepath);
  auto& reference = *sound;
  _sounds.emplace(std::filesystem::path{filepath}.stem().string(), std::move(sound));
  return reference;
}
