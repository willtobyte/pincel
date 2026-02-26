#include "manager.hpp"
#include "soundregistry.hpp"

manager::manager()
  : _atlasregistry(std::make_unique<atlasregistry>())
  , _compositor(std::make_unique<compositor>(*_atlasregistry))
  , _soundregistry(std::make_unique<soundregistry>()) {
  const auto entries = io::enumerate("stages");

  for (const auto& entry : entries) {
    if (!entry.ends_with(".lua")) continue;

    auto name = std::filesystem::path{entry}.stem().string();
    _stages.emplace(name, std::make_unique<stage>(name, *_atlasregistry, *_compositor, *_soundregistry));
  }
}

manager::~manager() {
  if (_active) {
    _active->on_leave();
    _active = nullptr;
  }
}

void manager::request(std::string_view name) {
  _pending = std::string(name);
}

const std::string& manager::current() const {
  return _current;
}

void manager::update(float delta) {
  if (_pending) {
    const auto it = _stages.find(*_pending);
    assert(it != _stages.end() && "stage not found");

    auto* const next = it->second.get();

    if (_active != next) {
      if (_active) {
        _active->on_leave();
      }

      _active = next;
      _current = std::move(*_pending);
      _active->on_enter();
    }

    _pending.reset();
  }

  if (!_active) return;

  _active->on_loop(delta);
}

void manager::draw() {
  if (!_active) return;

  _active->on_draw();
  _compositor->draw();
}
