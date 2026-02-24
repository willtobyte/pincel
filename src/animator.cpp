#include "animator.hpp"
#include "scriptable.hpp"

namespace {
  const animation* find(const animatable& a, entt::id_type name) {
    for (uint32_t i = 0; i < a.count; ++i) {
      if (a.animations[i].name == name) return &a.animations[i];
    }

    return nullptr;
  }

  void dispatch_animation_end(entt::registry& registry, entt::entity entity, entt::id_type name) {
    const auto* s = registry.try_get<scriptable>(entity);
    if (!s || s->on_animation_end == LUA_NOREF) return;

    const auto it = lookup.find(name);
    if (it == lookup.end()) return;

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

void animator::update(entt::registry& registry, float delta) {
  for (auto&& [entity, r, a] : registry.view<renderable, animatable>().each()) {
    const auto* animation = find(a, r.animation);
    if (!animation || animation->count == 0) [[unlikely]] continue;

    r.counter += delta * 1000.f;

    while (r.counter >= static_cast<float>(animation->keyframes[r.current_frame].duration)) {
      r.counter -= static_cast<float>(animation->keyframes[r.current_frame].duration);

      if (r.current_frame + 1 < animation->count) {
        ++r.current_frame;
      } else if (animation->next != 0) {
        const auto* finished = animation;
        r.animation = animation->next;
        r.current_frame = 0;
        r.counter = .0f;
        animation = find(a, r.animation);
        dispatch_animation_end(registry, entity, finished->name);
        if (!animation || animation->count == 0) [[unlikely]] break;
      } else if (animation->once) {
        r.counter = .0f;
        dispatch_animation_end(registry, entity, animation->name);
        break;
      } else {
        r.current_frame = 0;
        dispatch_animation_end(registry, entity, animation->name);
      }
    }

    if (animation && animation->count > 0) [[likely]] {
      r.sprite = animation->keyframes[r.current_frame].sprite;
    }
  }
}
