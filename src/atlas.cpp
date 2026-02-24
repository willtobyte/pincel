#include "atlas.hpp"

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
  lua_pcall(L, 0, 1, 0);
  assert(lua_istable(L, -1) && "atlas lua must return a table");

  const auto count = static_cast<uint32_t>(lua_objlen(L, -1));
  for (uint32_t i = 1; i <= count; ++i) {
    lua_rawgeti(L, -1, static_cast<int>(i));
    assert(lua_istable(L, -1) && "sprite entry must be a table");

    lua_rawgeti(L, -1, 1);
    const auto x = static_cast<float>(lua_tonumber(L, -1));
    lua_pop(L, 1);

    lua_rawgeti(L, -1, 2);
    const auto y = static_cast<float>(lua_tonumber(L, -1));
    lua_pop(L, 1);

    lua_rawgeti(L, -1, 3);
    const auto w = static_cast<float>(lua_tonumber(L, -1));
    lua_pop(L, 1);

    lua_rawgeti(L, -1, 4);
    const auto h = static_cast<float>(lua_tonumber(L, -1));
    lua_pop(L, 1);

    _sprites[_sprite_count++] = sprite{
      .u0 = x / fw,
      .v0 = y / fh,
      .u1 = (x + w) / fw,
      .v1 = (y + h) / fh,
      .w = w,
      .h = h,
    };

    lua_pop(L, 1);
  }

  lua_pop(L, 1);
}
