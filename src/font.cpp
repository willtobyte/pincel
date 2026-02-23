#include "font.hpp"

namespace {
  constexpr auto max_vertices = 1024uz;
  constexpr auto max_indices = 1536uz;
}

font::font(std::string_view family) {
  const auto meta = io::read(std::format("blobs/fonts/{}.meta", family));
  const auto content = std::string_view(reinterpret_cast<const char*>(meta.data()), meta.size());

  auto position = 0uz;
  while (position < content.size()) {
    auto end = content.find('\n', position);
    if (end == std::string_view::npos) end = content.size();

    auto line = content.substr(position, end - position);
    position = end + 1;

    while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t'))
      line.remove_suffix(1);
    while (!line.empty() && (line.front() == ' ' || line.front() == '\t'))
      line.remove_prefix(1);

    if (line.empty() || line.front() == '#') continue;

    const auto eq = line.find('=');
    if (eq == std::string_view::npos) continue;

    auto key = line.substr(0, eq);
    auto value = line.substr(eq + 1);

    while (!key.empty() && (key.back() == ' ' || key.back() == '\t'))
      key.remove_suffix(1);

    if (key == "glyphs") {
      _glyphs = value;
    } else {
      while (!value.empty() && (value.front() == ' ' || value.front() == '\t'))
        value.remove_prefix(1);

      if (key == "spacing") {
        int16_t v;
        std::from_chars(value.data(), value.data() + value.size(), v);
        _spacing = v;
      } else if (key == "leading") {
        int16_t v;
        std::from_chars(value.data(), value.data() + value.size(), v);
        _leading = v;
      } else if (key == "scale") {
        _scale = std::strtof(value.data(), nullptr);
      }
    }
  }

  const auto png = io::read(std::format("blobs/fonts/{}.png", family));

  auto spng = std::unique_ptr<spng_ctx, SPNG_Deleter>(spng_ctx_new(SPNG_CTX_IGNORE_ADLER32));

  spng_set_crc_action(spng.get(), SPNG_CRC_USE, SPNG_CRC_USE);
  spng_set_png_buffer(spng.get(), png.data(), png.size());

  spng_ihdr ihdr;
  spng_get_ihdr(spng.get(), &ihdr);

  _width = static_cast<int>(ihdr.width);
  _height = static_cast<int>(ihdr.height);

  size_t length;
  spng_decoded_image_size(spng.get(), SPNG_FMT_RGBA8, &length);

  auto decoded = std::make_unique_for_overwrite<uint8_t[]>(length);
  spng_decode_image(spng.get(), decoded.get(), length, SPNG_FMT_RGBA8, SPNG_DECODE_TRNS);

  _texture = std::unique_ptr<SDL_Texture, SDL_Deleter>(
    SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, _width, _height));

  SDL_UpdateTexture(_texture.get(), nullptr, decoded.get(), _width * SDL_BYTESPERPIXEL(SDL_PIXELFORMAT_RGBA32));
  SDL_SetTextureScaleMode(_texture.get(), SDL_SCALEMODE_NEAREST);
  SDL_SetTextureBlendMode(_texture.get(), SDL_BLENDMODE_BLEND);

  const auto* pixels = reinterpret_cast<const uint32_t*>(decoded.get());
  const auto separator = pixels[0];

  const auto iw = 1.0f / static_cast<float>(_width);
  const auto ih = 1.0f / static_cast<float>(_height);

  constexpr auto inset = .5f;

  auto x = 0, y = 0;
  auto first = true;
  for (char glyph : _glyphs) {
    while (x < _width && pixels[y * _width + x] == separator) {
      ++x;
    }

    assert(x < _width && "missing glyph in font texture");

    auto w = 0;
    while (x + w < _width && pixels[y * _width + x + w] != separator) {
      ++w;
    }

    auto h = 0;
    while (y + h < _height && pixels[(y + h) * _width + x] != separator) {
      ++h;
    }

    const auto fx = static_cast<float>(x);
    const auto fy = static_cast<float>(y);
    const auto fw = static_cast<float>(w);
    const auto fh = static_cast<float>(h);

    _props[static_cast<uint8_t>(glyph)] = {
      (fx + inset) * iw,
      (fy + inset) * ih,
      (fx + fw - inset) * iw,
      (fy + fh - inset) * ih,
      fw * _scale,
      fh * _scale,
      fw
    };

    if (first) {
      _fontheight = fh * _scale;
      first = false;
    }

    x += w;
  }

}

void font::draw(std::string_view text, float x, float y) const noexcept {
  if (text.empty()) [[unlikely]] return;

  _vertex_count = 0;
  _index_count = 0;

  constexpr auto color = SDL_FColor{1.f, 1.f, 1.f, 1.f};

  auto cx = x;
  auto cy = y;

  for (const auto ch : text) {
    if (ch == '\n') {
      cx = x;
      cy += _fontheight + static_cast<float>(_leading);
      continue;
    }

    if (_vertex_count + 4 > max_vertices) break;

    const auto& g = _props[static_cast<uint8_t>(ch)];
    const auto base = static_cast<int>(_vertex_count);

    _vertices[_vertex_count++] = SDL_Vertex{{cx,        cy},        color, {g.u0, g.v0}};
    _vertices[_vertex_count++] = SDL_Vertex{{cx + g.sw, cy},        color, {g.u1, g.v0}};
    _vertices[_vertex_count++] = SDL_Vertex{{cx + g.sw, cy + g.sh}, color, {g.u1, g.v1}};
    _vertices[_vertex_count++] = SDL_Vertex{{cx,        cy + g.sh}, color, {g.u0, g.v1}};

    _indices[_index_count++] = base;
    _indices[_index_count++] = base + 1;
    _indices[_index_count++] = base + 2;
    _indices[_index_count++] = base;
    _indices[_index_count++] = base + 2;
    _indices[_index_count++] = base + 3;

    cx += g.sw + static_cast<float>(_spacing);
  }

  if (_vertex_count == 0) [[unlikely]] return;

  SDL_RenderGeometry(
    renderer,
    _texture.get(),
    _vertices.data(),
    static_cast<int>(_vertex_count),
    _indices.data(),
    static_cast<int>(_index_count)
  );
}
