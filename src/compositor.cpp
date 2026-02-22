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

  const auto fonts = io::enumerate("blobs/fonts");

  for (const auto& filename : fonts) {
    if (!filename.ends_with(".meta")) continue;

    _fonts.emplace_back(filename.substr(0, filename.size() - 5));
  }

  _sequence.reserve(64);
  _vertices.reserve(1024);
  _indices.reserve(1536);
}

void compositor::submit(const entry& entry) {
  _entries.emplace_back(entry);
}

void compositor::submit(std::span<const entry> entries) {
  _entries.append_range(entries);
}

void compositor::update() {
  _sequence.clear();
  _vertices.clear();
  _indices.clear();

  if (_entries.empty()) [[unlikely]] return;

  const auto size = _entries.size();
  _vertices.reserve(size * 4);
  _indices.reserve(size * 6);

  for (auto i = 0uz; i < size;) {
    switch (_entries[i].kind) {
      case sprite: {
        const auto id = _entries[i].sprite.atlas;
        assert(id >= 0 && id < static_cast<int>(_atlases.size()) && "atlas index out of bounds");

        const auto& a = _atlases[static_cast<size_t>(id)];
        const auto vbegin = static_cast<int>(_vertices.size());
        const auto ibegin = static_cast<int>(_indices.size());

        for (; i < size && _entries[i].kind == sprite && _entries[i].sprite.atlas == id; ++i) {
          const auto& e = _entries[i].sprite;
          assert(e.index >= 0 && e.index < static_cast<int>(a._sprites.size()) && "sprite index out of bounds");
          const auto& s = a._sprites[static_cast<size_t>(e.index)];

          const auto hw = s.w * e.scale * 0.5f;
          const auto hh = s.h * e.scale * 0.5f;
          const auto color = SDL_FColor{1.0f, 1.0f, 1.0f, static_cast<float>(e.alpha) / 255.0f};
          const auto base = static_cast<int>(_vertices.size());

          _vertices.emplace_back(SDL_Vertex{
            {-hw * e.cosr - -hh * e.sinr + e.x, -hw * e.sinr + -hh * e.cosr + e.y}, color, {s.u0, s.v0}});
          _vertices.emplace_back(SDL_Vertex{
            {+hw * e.cosr - -hh * e.sinr + e.x, +hw * e.sinr + -hh * e.cosr + e.y}, color, {s.u1, s.v0}});
          _vertices.emplace_back(SDL_Vertex{
            {+hw * e.cosr - +hh * e.sinr + e.x, +hw * e.sinr + +hh * e.cosr + e.y}, color, {s.u1, s.v1}});
          _vertices.emplace_back(SDL_Vertex{
            {-hw * e.cosr - +hh * e.sinr + e.x, -hw * e.sinr + +hh * e.cosr + e.y}, color, {s.u0, s.v1}});

          _indices.emplace_back(base);
          _indices.emplace_back(base + 1);
          _indices.emplace_back(base + 2);
          _indices.emplace_back(base);
          _indices.emplace_back(base + 2);
          _indices.emplace_back(base + 3);
        }

        _sequence.emplace_back(step{
          .texture = a._texture.get(),
          .vertex_begin = vbegin,
          .vertex_count = static_cast<int>(_vertices.size()) - vbegin,
          .index_begin = ibegin,
          .index_count = static_cast<int>(_indices.size()) - ibegin,
        });
      } break;

      case text: {
        const auto id = _entries[i].text.font;
        assert(id >= 0 && id < static_cast<int>(_fonts.size()) && "font index out of bounds");

        const auto& f = _fonts[static_cast<size_t>(id)];
        const auto vbegin = static_cast<int>(_vertices.size());
        const auto ibegin = static_cast<int>(_indices.size());

        for (; i < size && _entries[i].kind == text && _entries[i].text.font == id; ++i) {
          const auto& t = _entries[i].text;

          auto cx = t.x;
          auto cy = t.y;

          for (const auto ch : t.content) {
            if (ch == '\n') [[unlikely]] {
              cx = t.x;
              cy += f._fontheight + f._leading;
              continue;
            }

            const auto& g = f._props[static_cast<uint8_t>(ch)];
            if (!g.valid) [[unlikely]] continue;

            const auto hw = g.sw * 0.5f;
            const auto hh = g.sh * 0.5f;
            const auto gx = cx + hw;
            const auto gy = cy + hh;

            constexpr SDL_FColor color{1.f, 1.f, 1.f, 1.f};
            const auto base = static_cast<int>(_vertices.size());

            _vertices.emplace_back(SDL_Vertex{{gx - hw, gy - hh}, color, {g.u0, g.v0}});
            _vertices.emplace_back(SDL_Vertex{{gx + hw, gy - hh}, color, {g.u1, g.v0}});
            _vertices.emplace_back(SDL_Vertex{{gx + hw, gy + hh}, color, {g.u1, g.v1}});
            _vertices.emplace_back(SDL_Vertex{{gx - hw, gy + hh}, color, {g.u0, g.v1}});

            _indices.emplace_back(base);
            _indices.emplace_back(base + 1);
            _indices.emplace_back(base + 2);
            _indices.emplace_back(base);
            _indices.emplace_back(base + 2);
            _indices.emplace_back(base + 3);

            cx += g.w + f._spacing;
          }
        }

        _sequence.emplace_back(step{
          .texture = f._texture.get(),
          .vertex_begin = vbegin,
          .vertex_count = static_cast<int>(_vertices.size()) - vbegin,
          .index_begin = ibegin,
          .index_count = static_cast<int>(_indices.size()) - ibegin,
        });
      } break;
    }
  }

  _entries.clear();
}

void compositor::draw() const {
  for (const auto& s : _sequence) {
    SDL_RenderGeometry(
      renderer,
      s.texture,
      _vertices.data() + s.vertex_begin,
      s.vertex_count,
      _indices.data() + s.index_begin,
      s.index_count
    );
  }
}
