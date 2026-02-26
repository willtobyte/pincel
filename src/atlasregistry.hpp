#pragma once

#include "common.hpp"

using atlas_id = entt::id_type;

class atlasregistry final {
public:
  atlasregistry();
  ~atlasregistry() = default;

  const atlas::sprite& sprite(atlas_id id, int index) const;

private:
  friend class ::compositor;

  std::unordered_map<atlas_id, class atlas> _atlases;
};
