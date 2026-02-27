#include "atlas.hpp"

namespace {
  constexpr int field_x = 1;
  constexpr int field_y = 2;
  constexpr int field_w = 3;
  constexpr int field_h = 4;

  entt::id_type hash(std::string_view value) {
    return entt::hashed_string::value(value.data(), value.size());
  }

  atlas::sprite parse_sprite(float fw, float fh, uint32_t fields) {
    atlas::sprite s{};

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

    s.u0 = x / fw;
    s.v0 = y / fh;
    s.u1 = (x + w) / fw;
    s.v1 = (y + h) / fh;
    s.w = w;
    s.h = h;

    if (fields >= 8) {
      lua_rawgeti(L, -1, 5);
      s.hx = static_cast<float>(lua_tonumber(L, -1));
      lua_pop(L, 1);

      lua_rawgeti(L, -1, 6);
      s.hy = static_cast<float>(lua_tonumber(L, -1));
      lua_pop(L, 1);

      lua_rawgeti(L, -1, 7);
      s.hw = static_cast<float>(lua_tonumber(L, -1));
      lua_pop(L, 1);

      lua_rawgeti(L, -1, 8);
      s.hh = static_cast<float>(lua_tonumber(L, -1));
      lua_pop(L, 1);
    }

    return s;
  }
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

  assert(lua_istable(L, -1) && "atlas lua must return a table");

  lua_pushnil(L);
  while (lua_next(L, -2) != 0) {
    if (!lua_isstring(L, -2)) {
      lua_pop(L, 1);
      continue;
    }

    const std::string_view key = lua_tostring(L, -2);
    const auto entry_id = hash(key);

    assert(lua_istable(L, -1) && "atlas entry must be a table");

    lua_rawgeti(L, -1, 1);
    const bool is_animation = lua_istable(L, -1);
    lua_pop(L, 1);

    animation anim{};

    if (is_animation) {
      const auto frame_count = static_cast<uint32_t>(lua_objlen(L, -1));
      anim.keyframes.reserve(frame_count);

      for (uint32_t i = 1; i <= frame_count; ++i) {
        lua_rawgeti(L, -1, static_cast<int>(i));
        assert(lua_istable(L, -1) && "animation frame must be a table");

        const auto fields = static_cast<uint32_t>(lua_objlen(L, -1));
        assert(fields >= 5 && "animation frame must have at least x, y, w, h, duration");

        const auto duration_field = static_cast<int>(fields);
        auto s = parse_sprite(fw, fh, fields - 1);

        lua_rawgeti(L, -1, duration_field);
        const auto duration = static_cast<uint32_t>(lua_tonumber(L, -1));
        lua_pop(L, 1);

        anim.keyframes.push_back({s, duration});
        lua_pop(L, 1);
      }

      lua_getfield(L, -1, "next");
      if (lua_isstring(L, -1))
        anim.next = hash(lua_tostring(L, -1));
      lua_pop(L, 1);

      lua_getfield(L, -1, "once");
      if (lua_isboolean(L, -1))
        anim.once = lua_toboolean(L, -1) != 0;
      lua_pop(L, 1);

    } else {
      const auto fields = static_cast<uint32_t>(lua_objlen(L, -1));
      assert(fields >= 4 && "sprite must have at least x, y, w, h");

      auto s = parse_sprite(fw, fh, fields);
      anim.keyframes.push_back({s, 0});
    }

    _entries.emplace(entry_id, std::move(anim));
    lua_pop(L, 1);
  }

  lua_pop(L, 1);
}

const atlas::animation* atlas::find(entt::id_type entry) const {
  const auto it = _entries.find(entry);
  if (it == _entries.end()) return nullptr;
  return &it->second;
}

const atlas::keyframe& atlas::keyframe_at(entt::id_type entry, uint32_t frame) const {
  const auto it = _entries.find(entry);
  assert(it != _entries.end() && "entry not found in atlas");
  assert(frame < it->second.keyframes.size() && "frame index out of bounds");
  return it->second.keyframes[frame];
}
