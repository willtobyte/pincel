#include "filesystem.hpp"

void filesystem::mount(std::string_view filename, std::string_view mountpoint) {
  const auto result = PHYSFS_mount(filename.data(), mountpoint.data(), true);

  const auto* const message = PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());

  if (!result) [[unlikely]]
    throw std::runtime_error(std::format("[PHYSFS_mount] failed to mount {} to {}. reason: {}",
      filename,
      mountpoint,
      message).c_str());
}
