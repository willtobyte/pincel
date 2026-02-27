#include "animator.hpp"

namespace {
  void dispatch_animation_end(entt::registry& registry, entt::entity entity, entt::id_type name) {
    const auto* s = registry.try_get<scriptable>(entity);
    if (!s || s->on_animation_end == LUA_NOREF) return;

    const auto& lu = registry.ctx().get<lookupable>();
    const auto it = lu.names.find(name);
    if (it == lu.names.end()) return;

    lua_rawgeti(L, LUA_REGISTRYINDEX, s->on_animation_end);
    lua_rawgeti(L, LUA_REGISTRYINDEX, s->self_ref);
    lua_pushstring(L, it->second.c_str());
    if (lua_pcall(L, 2, 0, 0) != 0) {
      std::string error = lua_tostring(L, -1);
      lua_pop(L, 1);
      throw std::runtime_error(error);
    }
  }
}

void animator::update(entt::registry& registry, atlasregistry& atlasregistry, float delta) {
  for (auto&& [entity, r] : registry.view<renderable>().each()) {
    const auto* anim = atlasregistry.get(r.atlas).find(r.entry);
    if (!anim || anim->keyframes.empty()) [[unlikely]] continue;

    if (anim->keyframes.size() == 1 && anim->keyframes[0].duration == 0) [[unlikely]] continue;

    r.counter += delta * 1000.f;

    while (r.counter >= static_cast<float>(anim->keyframes[r.current_frame].duration)) {
      r.counter -= static_cast<float>(anim->keyframes[r.current_frame].duration);

      if (r.current_frame + 1 < static_cast<uint32_t>(anim->keyframes.size())) {
        ++r.current_frame;
      } else if (anim->next != 0) {
        const auto finished_entry = r.entry;
        r.entry = anim->next;
        r.current_frame = 0;
        r.counter = .0f;
        dispatch_animation_end(registry, entity, finished_entry);

        anim = atlasregistry.get(r.atlas).find(r.entry);
        if (!anim || anim->keyframes.empty()) [[unlikely]] break;
      } else if (anim->once) {
        r.counter = .0f;
        dispatch_animation_end(registry, entity, r.entry);
        break;
      } else {
        r.current_frame = 0;
        dispatch_animation_end(registry, entity, r.entry);
      }
    }
  }
}
