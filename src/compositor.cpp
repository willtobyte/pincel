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
}

void compositor::submit(const entry& entry) {
  _entries.emplace_back(entry);
}

void compositor::submit(std::span<const entry> entries) {
  _entries.append_range(entries);
}

void compositor::draw() {
  if (_entries.empty()) [[unlikely]] return;

  const auto size = _entries.size();
  _commands.reserve(size);

  for (auto i = 0uz; i < size;) {
    const auto atlas = _entries[i].atlas;
    assert(atlas >= 0 && atlas < static_cast<int>(_atlases.size()) && "atlas index out of bounds");

    _commands.clear();
    for (; i < size && _entries[i].atlas == atlas; ++i) {
      _commands.push_back(atlas::command{
        .x = _entries[i].x,
        .y = _entries[i].y,
        .scale = _entries[i].scale,
        .rotation = _entries[i].rotation,
        .index = _entries[i].index,
        .alpha = _entries[i].alpha,
      });
    }

    _atlases[atlas].enqueue(_commands);
    _atlases[atlas].draw();
  }

  _entries.clear();
}
