#pragma once

#include "common.hpp"

class soundregistry final {
public:
  soundregistry() = default;
  ~soundregistry() = default;

  soundfx& get_or_load(std::string_view name);

private:
  std::unordered_map<std::string, std::unique_ptr<soundfx>, transparent_hash, std::equal_to<>> _sounds;
};
