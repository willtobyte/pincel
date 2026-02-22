#pragma once

class atlas {
public:
  atlas();
  ~atlas() = default;

private:
  std::vector<std::unique_ptr<SDL_Texture, SDL_Deleter>> _textures;
};
