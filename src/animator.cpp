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
  for (auto&& [entity, r, a] : registry.view<renderable, animatable>().each()) {
    const auto* animation = find(a, r.animation);
    if (!animation || animation->count == 0) [[unlikely]] continue;

    r.counter += delta * 1000.0f;

    while (r.counter >= static_cast<float>(animation->keyframes[r.current_frame].duration)) {
      r.counter -= static_cast<float>(animation->keyframes[r.current_frame].duration);

      if (r.current_frame + 1 < animation->count) {
        ++r.current_frame;
      } else if (animation->next != 0) {
        r.animation = animation->next;
        r.current_frame = 0;
        r.counter = 0.0f;
        animation = find(a, r.animation);
        if (!animation || animation->count == 0) [[unlikely]] break;
      } else if (animation->once) {
        r.counter = 0.0f;
        break;
      } else {
        r.current_frame = 0;
      }
    }

    if (animation && animation->count > 0) [[likely]] {
      r.sprite = animation->keyframes[r.current_frame].sprite;
    }
  }
}
