#pragma once

#include "common.hpp"
#include "atlas.hpp"

class compositor final {
public:
  struct entry {
    int atlas;
    int index;
    float x, y;
    float scale;
    float rotation;
    uint8_t alpha;
  };

  compositor();
  ~compositor() = default;

  void submit(const entry& entry);
  void submit(std::span<const entry> entries);
  void draw();

private:
  std::vector<class atlas> _atlases;
  std::vector<entry> _entries;
  std::vector<atlas::command> _commands;
};
