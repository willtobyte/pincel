#include "atlas.hpp"

atlas::atlas() {
  const auto entries = io::enumerate("blobs/atlas");
  _textures.reserve(entries.size());
  for (const auto& entry : entries) {
    const auto buffer = io::read(entry);
  
    auto spng =
      std::unique_ptr<spng_ctx, SPNG_Deleter>(spng_ctx_new(SPNG_CTX_IGNORE_ADLER32));
  
    spng_set_crc_action(spng.get(), SPNG_CRC_USE, SPNG_CRC_USE);
    spng_set_png_buffer(spng.get(), buffer.data(), buffer.size());
  
    spng_ihdr ihdr;
    spng_get_ihdr(spng.get(), &ihdr);
  
    const auto width = static_cast<int>(ihdr.width);
    const auto height = static_cast<int>(ihdr.height);

    [[maybe_unused]] const auto max = static_cast<int>(SDL_GetNumberProperty(
      SDL_GetRendererProperties(renderer), SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER, 0));
    assert(width <= max && "texture width exceeds GPU maximum texture size");
    assert(height <= max && "texture height exceeds GPU maximum texture size");
  
    size_t length;
    spng_decoded_image_size(spng.get(), SPNG_FMT_RGBA8, &length);
  
    auto pixels = std::make_unique_for_overwrite<uint8_t[]>(length);
    spng_decode_image(spng.get(), pixels.get(), length, SPNG_FMT_RGBA8, SPNG_DECODE_TRNS);
  
    auto& texture = _textures.emplace_back(
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, width, height));
  
    SDL_UpdateTexture(texture.get(), nullptr, pixels.get(), width * SDL_BYTESPERPIXEL(SDL_PIXELFORMAT_RGBA32));
    SDL_SetTextureScaleMode(texture.get(), SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_BLEND);
  }
}