#pragma once

class atlas {
public:
  atlas();
  ~atlas() = default;

  static constexpr auto CAPACITY = 8uz;

private:
  std::unique_ptr<SDL_Texture, SDL_Deleter> _textures[CAPACITY];
};
