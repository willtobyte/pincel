#include "presenter.hpp"
#include "compositor.hpp"
#include "transform.hpp"
#include "renderable.hpp"
#include "sorteable.hpp"
#include "trigonometry.hpp"

void presenter::update(entt::registry& registry, compositor& compositor) {
  for (auto&& [entity, t, r, s] : registry.view<transform, renderable, sorteable>().each()) {
    if (!t.visible) [[unlikely]] continue;

    compositor.submit({
      .atlas = r.atlas,
      .index = static_cast<int>(r.sprite),
      .x = t.x,
      .y = t.y,
      .scale = t.scale,
      .cosr = lcos(t.angle),
      .sinr = lsin(t.angle),
      .alpha = t.alpha,
      .z = s.z,
    });
  }
}
