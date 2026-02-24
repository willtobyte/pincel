#pragma once

#include "common.hpp"

struct dirtable final {
  uint8_t flags{0xff};

  static constexpr uint8_t sort = 1 << 0;

  void mark(uint8_t flag) noexcept { flags |= flag; }
  void clear(uint8_t flag) noexcept { flags &= static_cast<uint8_t>(~flag); }
  [[nodiscard]] bool is(uint8_t flag) const noexcept { return (flags & flag) != 0; }
};

static_assert(std::is_trivially_copyable_v<dirtable>);
