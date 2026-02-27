#pragma once

#include "common.hpp"

class compositor;

using atlas_id = entt::id_type;

class atlasregistry final {
public:
  atlasregistry();
  ~atlasregistry() = default;

  atlas& get(atlas_id id);
  const atlas& get(atlas_id id) const;

private:
  friend class ::compositor;

  std::unordered_map<atlas_id, class atlas> _atlases;
};
