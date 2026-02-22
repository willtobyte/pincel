#pragma once

#include "common.hpp"
#include "atlas.hpp"
#include "font.hpp"

class compositor final {
public:
  enum class kind : uint8_t { sprite, text };

  struct entry {
    kind type;
    union {
      struct {
        int atlas;
        int index;
        float x, y;
        float scale;
        float cosr, sinr;
        uint8_t alpha;
      } sprite;
      struct {
        int font;
        std::string_view content;
        SDL_FPoint position;
      } text;
    };
  };

  compositor();
  ~compositor() = default;

  void submit(const entry& entry);
  void submit(std::span<const entry> entries);
  void update();
  void draw() const;

private:
  struct step {
    kind type;
    int index;
  };

  std::vector<class atlas> _atlases;
  std::vector<class font> _fonts;
  std::vector<entry> _entries;
  std::vector<atlas::command> _commands;
  std::vector<step> _sequence;
};
