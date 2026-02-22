#include "atlas.hpp"
#include "io.hpp"

atlas::atlas(int id) {
  const auto png = io::read(std::format("blobs/atlas/{}.png", id));

  auto spng =
    std::unique_ptr<spng_ctx, SPNG_Deleter>(spng_ctx_new(SPNG_CTX_IGNORE_ADLER32));

  spng_set_crc_action(spng.get(), SPNG_CRC_USE, SPNG_CRC_USE);
  spng_set_png_buffer(spng.get(), png.data(), png.size());

  spng_ihdr ihdr;
  spng_get_ihdr(spng.get(), &ihdr);

  const auto tw = static_cast<int>(ihdr.width);
  const auto th = static_cast<int>(ihdr.height);

  [[maybe_unused]] const auto maximum = static_cast<int>(SDL_GetNumberProperty(
    SDL_GetRendererProperties(renderer), SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER, 0));
  assert(tw <= maximum && "texture width exceeds GPU maximum texture size");
  assert(th <= maximum && "texture height exceeds GPU maximum texture size");

  size_t length;
  spng_decoded_image_size(spng.get(), SPNG_FMT_RGBA8, &length);

  auto pixels = std::make_unique_for_overwrite<uint8_t[]>(length);
  spng_decode_image(spng.get(), pixels.get(), length, SPNG_FMT_RGBA8, SPNG_DECODE_TRNS);

  _texture = std::unique_ptr<SDL_Texture, SDL_Deleter>(
    SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, tw, th));

  SDL_UpdateTexture(_texture.get(), nullptr, pixels.get(), tw * SDL_BYTESPERPIXEL(SDL_PIXELFORMAT_RGBA32));
  SDL_SetTextureScaleMode(_texture.get(), SDL_SCALEMODE_NEAREST);
  SDL_SetTextureBlendMode(_texture.get(), SDL_BLENDMODE_BLEND);

  const auto text = io::read(std::format("blobs/atlas/{}.txt", id));
  const auto content = std::string_view(reinterpret_cast<const char*>(text.data()), text.size());

  const auto fw = static_cast<float>(tw);
  const auto fh = static_cast<float>(th);

  auto position = 0uz;
  while (position < content.size()) {
    auto end = content.find('\n', position);
    if (end == std::string_view::npos) end = content.size();

    auto line = content.substr(position, end - position);
    position = end + 1;

    if (const auto comment = line.find("--"); comment != std::string_view::npos)
      line = line.substr(0, comment);

    while (!line.empty() && (line.front() == ' ' || line.front() == '\t'))
      line.remove_prefix(1);
    while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r'))
      line.remove_suffix(1);

    if (line.empty()) continue;

    int x, y, w, h;
    auto cursor = line.data();
    const auto* const fence = line.data() + line.size();

    const auto [p1, e1] = std::from_chars(cursor, fence, x);
    assert(e1 == std::errc{} && *p1 == ',' && "failed to parse x in atlas entry");
    const auto [p2, e2] = std::from_chars(p1 + 1, fence, y);
    assert(e2 == std::errc{} && *p2 == ',' && "failed to parse y in atlas entry");
    const auto [p3, e3] = std::from_chars(p2 + 1, fence, w);
    assert(e3 == std::errc{} && *p3 == ',' && "failed to parse w in atlas entry");
    const auto [p4, e4] = std::from_chars(p3 + 1, fence, h);
    assert(e4 == std::errc{} && "failed to parse h in atlas entry");

    _sprites.emplace_back(
      sprite{
        .u0 = static_cast<float>(x) / fw,
        .v0 = static_cast<float>(y) / fh,
        .u1 = static_cast<float>(x + w) / fw,
        .v1 = static_cast<float>(y + h) / fh,
        .w = static_cast<float>(w),
        .h = static_cast<float>(h),
      });
  }

  _vertices.reserve(_sprites.size() * 4);
  _indices.reserve(_sprites.size() * 6);
}

void atlas::enqueue(std::span<const command> commands) {
  _vertices.clear();
  _indices.clear();

  for (const auto& command : commands) {
    assert(command.index >= 0 && command.index < static_cast<int>(_sprites.size()) && "sprite index out of bounds");
    const auto& s = _sprites[command.index];

    const auto hw = s.w * command.scale * 0.5f;
    const auto hh = s.h * command.scale * 0.5f;

    const auto radians = command.rotation * (std::numbers::pi_v<float> / 180.0f);
    const auto cosr = std::cos(radians);
    const auto sinr = std::sin(radians);

    const auto color = SDL_FColor{ 1.0f, 1.0f, 1.0f, static_cast<float>(command.alpha) / 255.0f };

    const float cx[] = { -hw, +hw, +hw, -hw };
    const float cy[] = { -hh, -hh, +hh, +hh };
    const float tu[] = { s.u0, s.u1, s.u1, s.u0 };
    const float tv[] = { s.v0, s.v0, s.v1, s.v1 };

    const auto base = static_cast<int>(_vertices.size());

    for (auto i = 0; i < 4; ++i) {
      _vertices.push_back(SDL_Vertex{
        .position = {
          cx[i] * cosr - cy[i] * sinr + command.x,
          cx[i] * sinr + cy[i] * cosr + command.y,
        },
        .color = color,
        .tex_coord = { tu[i], tv[i] },
      });
    }

    _indices.push_back(base + 0);
    _indices.push_back(base + 1);
    _indices.push_back(base + 2);
    _indices.push_back(base + 0);
    _indices.push_back(base + 2);
    _indices.push_back(base + 3);
  }
}

void atlas::draw() const noexcept {
  if (_vertices.empty()) [[unlikely]] return;

  SDL_RenderGeometry(
    renderer,
    _texture.get(),
    _vertices.data(),
    static_cast<int>(_vertices.size()),
    _indices.data(),
    static_cast<int>(_indices.size())
  );
}
