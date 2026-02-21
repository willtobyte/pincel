
#include "engine.hpp"

lua_State *L = nullptr;
ma_engine *audioengine = nullptr;
SDL_Renderer *renderer = nullptr;

engine::engine() {
  const auto buffer = io::read("scripts/main.lua");
  const auto *data = reinterpret_cast<const char *>(buffer.data());
  const auto size = buffer.size();

  luaL_loadbuffer(L, data, size, "@main.lua");
  lua_pcall(L, 0, 1, 0);

  assert(lua_istable(L, -1) && "scripts/main.lua must return a table");

  lua_getfield(L, -1, "title");
  const auto *title = lua_isstring(L, -1) ? lua_tostring(L, -1) : "Untitled";
  lua_pop(L, 1);

  lua_getfield(L, -1, "width");
  const auto width = lua_isnumber(L, -1) ? static_cast<int>(lua_tonumber(L, -1)) : 1920;
  lua_pop(L, 1);

  lua_getfield(L, -1, "height");
  const auto height = lua_isnumber(L, -1) ? static_cast<int>(lua_tonumber(L, -1)) : 1080;
  lua_pop(L, 1);

  lua_getfield(L, -1, "scale");
  const auto scale = lua_isnumber(L, -1) ? static_cast<float>(lua_tonumber(L, -1)) : 1.0f;
  lua_pop(L, 1);

  lua_getfield(L, -1, "fullscreen");
  const auto fullscreen = lua_isboolean(L, -1) ? lua_toboolean(L, -1) : 0;
  lua_pop(L, 1);

  lua_pop(L, 1);

  static const auto window = SDL_CreateWindow(
    title, width, height,
    fullscreen ? SDL_WINDOW_FULLSCREEN : 0
  );

  const auto vsync = std::getenv("NOVSYNC") ? 0 : 1;
  const auto properties = SDL_CreateProperties();
  SDL_SetPointerProperty(properties, SDL_PROP_RENDERER_CREATE_WINDOW_POINTER, window);
  SDL_SetNumberProperty(properties, SDL_PROP_RENDERER_CREATE_PRESENT_VSYNC_NUMBER, vsync);
  SDL_SetStringProperty(properties, SDL_PROP_RENDERER_CREATE_NAME_STRING, nullptr);

  renderer = SDL_CreateRendererWithProperties(properties);

  SDL_DestroyProperties(properties);

  SDL_SetRenderLogicalPresentation(renderer, width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
  SDL_SetRenderScale(renderer, scale, scale);

  SDL_RaiseWindow(window);

  lua_newtable(L);
  lua_pushinteger(L, width);
  lua_setfield(L, -2, "width");
  lua_pushinteger(L, height);
  lua_setfield(L, -2, "height");

  lua_newtable(L);
  lua_pushvalue(L, -2);
  lua_setfield(L, -2, "__index");

  lua_newtable(L);
  lua_pushvalue(L, -2);
  lua_setmetatable(L, -2);

  lua_setglobal(L, "viewport");

  lua_pop(L, 2);
}

void engine::run() {
  while (_running) [[likely]] {
    loop();
  }
}

void engine::loop() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_EVENT_QUIT: {
        _running = false;
      } break;

      case SDL_EVENT_KEY_UP: {
        switch (event.key.key) {
          case SDLK_F11: {
            auto *const window = SDL_GetRenderWindow(renderer);
            const auto fullscreen = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0;
            SDL_SetWindowFullscreen(window, !fullscreen);
          } break;

          default:
            break;
        }
      } break;

      default:
        break;
    }
  }

  const auto now = SDL_GetPerformanceCounter();
  static auto prior = now;
  static const auto frequency = static_cast<double>(SDL_GetPerformanceFrequency());
  const auto delta = std::min(static_cast<float>(static_cast<double>(now - prior) / frequency), MAX_DELTA);
  prior = now;

  SDL_RenderClear(renderer);

  // TODO

  SDL_RenderPresent(renderer);

  SteamAPI_RunCallbacks();
}
