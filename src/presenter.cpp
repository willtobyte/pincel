#include "presenter.hpp"

void presenter::render(entt::registry& registry, atlasregistry& atlasregistry, compositor& compositor) {
  auto view = registry.view<transform, renderable, sorteable>();
  view.use<sorteable>();

  for (auto&& [entity, t, r, s] : view.each()) {
    if (!t.shown) [[unlikely]] continue;

    auto& a = atlasregistry.get(r.atlas);
    const auto& kf = a.keyframe_at(r.entry, r.current_frame);

    compositor.push(
      a,
      kf.sprite,
      t.x,
      t.y,
      t.scale,
      lcos(t.angle),
      lsin(t.angle),
      t.alpha
    );
  }
}
