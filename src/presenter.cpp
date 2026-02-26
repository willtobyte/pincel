#include "presenter.hpp"

void presenter::render(entt::registry& registry, compositor& compositor) {
  auto view = registry.view<transform, renderable, sorteable>();
  view.use<sorteable>();

  for (auto&& [entity, t, r, s] : view.each()) {
    if (!t.shown || r.animation == 0) [[unlikely]] continue;

    compositor.push(
      r.atlas,
      static_cast<int>(r.sprite),
      t.x,
      t.y,
      t.scale,
      lcos(t.angle),
      lsin(t.angle),
      t.alpha
    );
  }
}
