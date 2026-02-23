#include "animator.hpp"

namespace {
  const animation* find(const animatable& a, entt::id_type name) {
    for (uint32_t i = 0; i < a.count; ++i) {
      if (a.animations[i].name == name) return &a.animations[i];
    }
    return nullptr;
  }
}

void animator::update(entt::registry& registry, float delta) {
  const auto ms = delta * 1000.0f;

  for (auto&& [entity, r, a] : registry.view<renderable, animatable>().each()) {
    const auto* anim = find(a, r.animation);
    if (!anim || anim->count == 0) [[unlikely]] continue;

    r.counter += ms;

    while (r.counter >= static_cast<float>(anim->keyframes[r.current_frame].duration)) {
      r.counter -= static_cast<float>(anim->keyframes[r.current_frame].duration);

      if (r.current_frame + 1 < anim->count) {
        ++r.current_frame;
      } else if (anim->next != 0) {
        r.animation = anim->next;
        r.current_frame = 0;
        r.counter = 0.0f;
        anim = find(a, r.animation);
        if (!anim || anim->count == 0) [[unlikely]] break;
      } else if (anim->once) {
        r.counter = 0.0f;
        break;
      } else {
        r.current_frame = 0;
      }
    }

    if (anim && anim->count > 0) [[likely]] {
      r.sprite = anim->keyframes[r.current_frame].sprite;
    }
  }
}
