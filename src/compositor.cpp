#include "compositor.hpp"
#include "io.hpp"

compositor::compositor() {
  const auto entries = io::enumerate("blobs/atlas");

  for (const auto& filename : entries) {
    if (!filename.ends_with(".png")) continue;

    int id;
    const auto [ptr, ec] = std::from_chars(filename.data(), filename.data() + filename.size() - 4, id);
    assert(ec == std::errc{} && "failed to parse atlas id from filename");

    _atlases.emplace_back(id);
  }

  const auto fonts = io::enumerate("fonts");

  for (const auto& filename : fonts) {
    if (!filename.ends_with(".font")) continue;

    _fonts.emplace_back(filename.substr(0, filename.size() - 5));
  }
}

void compositor::submit(const entry& entry) {
  _entries.emplace_back(entry);
}

void compositor::submit(std::span<const entry> entries) {
  _entries.append_range(entries);
}

void compositor::update() {
  _sequence.clear();

  for (auto& f : _fonts) f.clear();

  if (_entries.empty()) [[unlikely]] return;

  const auto size = _entries.size();
  _commands.reserve(size);

  for (auto i = 0uz; i < size;) {
    switch (_entries[i].type) {
      case kind::sprite: {
        const auto id = _entries[i].sprite.atlas;
        assert(id >= 0 && id < static_cast<int>(_atlases.size()) && "atlas index out of bounds");

        _commands.clear();
        for (; i < size && _entries[i].type == kind::sprite && _entries[i].sprite.atlas == id; ++i) {
          const auto& s = _entries[i].sprite;
          _commands.emplace_back(atlas::command{
            .x = s.x,
            .y = s.y,
            .scale = s.scale,
            .cosr = s.cosr,
            .sinr = s.sinr,
            .index = s.index,
            .alpha = s.alpha,
          });
        }

        _atlases[static_cast<size_t>(id)].enqueue(_commands);
        _sequence.emplace_back(kind::sprite, id);
      } break;

      case kind::text: {
        const auto id = _entries[i].text.font;
        assert(id >= 0 && id < static_cast<int>(_fonts.size()) && "font index out of bounds");

        for (; i < size && _entries[i].type == kind::text && _entries[i].text.font == id; ++i) {
          const auto& t = _entries[i].text;
          _fonts[static_cast<size_t>(id)].enqueue(t.content, t.position);
        }

        _sequence.emplace_back(kind::text, id);
      } break;
    }
  }

  _entries.clear();
}

void compositor::draw() const {
  for (const auto& s : _sequence) {
    switch (s.type) {
      case kind::sprite: {
        _atlases[static_cast<size_t>(s.index)].draw();
      } break;

      case kind::text: {
        _fonts[static_cast<size_t>(s.index)].draw();
      } break;
    }
  }
}
