#include "atlas.hpp"

namespace {
  constexpr int field_x = 1;
  constexpr int field_y = 2;
  constexpr int field_w = 3;
  constexpr int field_h = 4;
  constexpr int field_hx = 5;
  constexpr int field_hy = 6;
  constexpr int field_hw = 7;
  constexpr int field_hh = 8;
  constexpr uint32_t fields_with_hitbox = 8;
}

atlas::atlas(std::string_view name) {
  const auto png = io::read(std::format("blobs/atlas/{}.png", name));

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

  const auto fw = static_cast<float>(tw);
  const auto fh = static_cast<float>(th);

  const auto filename = std::format("blobs/atlas/{}.lua", name);
  const auto buffer = io::read(filename);
  const auto* data = reinterpret_cast<const char*>(buffer.data());
  const auto size = buffer.size();
  const auto label = std::format("@{}", filename);

  luaL_loadbuffer(L, data, size, label.c_str());
  if (lua_pcall(L, 0, 1, 0) != 0) {
    std::string error = lua_tostring(L, -1);
    lua_pop(L, 1);
    throw std::runtime_error(error);
  }

  const auto count = static_cast<uint32_t>(lua_objlen(L, -1));
  for (uint32_t i = 1; i <= count; ++i) {
    lua_rawgeti(L, -1, static_cast<int>(i));
    assert(lua_istable(L, -1) && "sprite entry must be a table");

    lua_rawgeti(L, -1, field_x);
    const auto x = static_cast<float>(lua_tonumber(L, -1));
    lua_pop(L, 1);

    lua_rawgeti(L, -1, field_y);
    const auto y = static_cast<float>(lua_tonumber(L, -1));
    lua_pop(L, 1);

    lua_rawgeti(L, -1, field_w);
    const auto w = static_cast<float>(lua_tonumber(L, -1));
    lua_pop(L, 1);

    lua_rawgeti(L, -1, field_h);
    const auto h = static_cast<float>(lua_tonumber(L, -1));
    lua_pop(L, 1);

    auto& s = _sprites.emplace_back();
    s.u0 = x / fw;
    s.v0 = y / fh;
    s.u1 = (x + w) / fw;
    s.v1 = (y + h) / fh;
    s.w = w;
    s.h = h;

    const auto fields = static_cast<uint32_t>(lua_objlen(L, -1));
    if (fields >= fields_with_hitbox) {
      lua_rawgeti(L, -1, field_hx);
      s.hx = static_cast<float>(lua_tonumber(L, -1));
      lua_pop(L, 1);

      lua_rawgeti(L, -1, field_hy);
      s.hy = static_cast<float>(lua_tonumber(L, -1));
      lua_pop(L, 1);

      lua_rawgeti(L, -1, field_hw);
      s.hw = static_cast<float>(lua_tonumber(L, -1));
      lua_pop(L, 1);

      lua_rawgeti(L, -1, field_hh);
      s.hh = static_cast<float>(lua_tonumber(L, -1));
      lua_pop(L, 1);

    }

    lua_pop(L, 1);
  }

  lua_pop(L, 1);
}
