#include "presenter.hpp"
#include "compositor.hpp"

void presenter::update(entt::registry& registry) {
  auto* compositor = registry.ctx().get<::compositor*>();
  auto view = registry.view<transform, renderable, sorteable>();
  view.use<sorteable>();

  for (auto&& [entity, t, r, s] : view.each()) {
    if (!t.shown || r.animation == 0) [[unlikely]] continue;

    compositor->submit({
      .atlas = r.atlas,
      .index = static_cast<int>(r.sprite),
      .x = t.x,
      .y = t.y,
      .scale = t.scale,
      .cosr = lcos(t.angle),
      .sinr = lsin(t.angle),
      .alpha = t.alpha,
    });
  }
}
