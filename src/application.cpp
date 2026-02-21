#include "application.hpp"

#include "scriptengine.hpp"

int application::run() {
  try {
    const auto* const rom = std::getenv("CARTRIDGE");

    filesystem::mount(rom ? rom : "cartridge.rom", "/");

    auto se = scriptengine();
    se.run();
  } catch (const std::exception& e) {
    const auto* const error = e.what();

    std::fprintf(stderr, "%s\n", error);

    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Ink Spill Disaster", error, nullptr);

    return 1;
  }

  return 0;
}
