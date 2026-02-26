#pragma once

#include "common.hpp"

struct lookupable final {
  entt::dense_map<entt::id_type, std::string> names{};
};
