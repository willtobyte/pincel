#include "manager.hpp"
#include "io.hpp"

manager::manager() {
  const auto entries = io::enumerate("scenes");

  for (const auto& entry : entries) {
    if (!entry.ends_with(".lua")) continue;

    auto name = std::filesystem::path{entry}.stem().string();
    _scenes.emplace(name, std::make_unique<scene>(name, _compositor));
  }
}

manager::~manager() {
  if (_active) {
    _active->on_leave();
    _active = nullptr;
  }
}

void manager::set(std::string_view name) {
  const auto it = _scenes.find(name);
  assert(it != _scenes.end() && "scene not found");

  auto* const next = it->second.get();

  if (_active == next) return;

  if (_active) {
    _active->on_leave();
  }

  _active = next;
  _active->on_enter();
}

void manager::update(float delta) {
  if (!_active) return;

  _active->on_loop(delta);
}

void manager::draw() {
  if (!_active) return;

  _active->on_draw();
  _compositor.draw();
}
