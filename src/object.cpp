#include "object.hpp"
#include "io.hpp"

namespace {
  entt::id_type hash(std::string_view sv) {
    return entt::hashed_string::value(sv.data(), sv.size());
  }
}

object::object(entt::registry& registry, int16_t z, std::string_view name)
    : _registry(&registry)
    , _entity(registry.create()) {
  _registry->emplace<sorteable>(_entity, sorteable{z});
  _registry->emplace<renderable>(_entity);

  const auto path = std::format("objects/{}.meta", name);
  const auto buffer = io::read(path);
  const auto content = std::string_view(
    reinterpret_cast<const char*>(buffer.data()), buffer.size());

  auto& r = _registry->get<renderable>(_entity);

  auto position = 0uz;
  while (position < content.size()) {
    auto end = content.find('\n', position);
    if (end == std::string_view::npos) end = content.size();

    auto line = content.substr(position, end - position);
    position = end + 1;

    if (const auto comment = line.find("--"); comment != std::string_view::npos)
      line = line.substr(0, comment);

    while (!line.empty() && (line.front() == ' ' || line.front() == '\t'))
      line.remove_prefix(1);
    while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r'))
      line.remove_suffix(1);

    if (line.empty()) continue;

    const auto eq = line.find('=');
    if (eq == std::string_view::npos) continue;

    const auto key = line.substr(0, eq);
    const auto value = line.substr(eq + 1);

    if (key == "atlas") {
      r.atlas = hash(value);
      continue;
    }

    animation animation{};
    animation.name = hash(key);

    auto frames_part = value;
    if (const auto arrow = value.find('>'); arrow != std::string_view::npos) {
      frames_part = value.substr(0, arrow);
      animation.next = hash(value.substr(arrow + 1));
    }

    auto cursor = frames_part.data();
    const auto* const fence = frames_part.data() + frames_part.size();

    while (cursor < fence && animation.count < animation.frames.size()) {
      auto& f = animation.frames[animation.count];

      const auto [p1, e1] = std::from_chars(cursor, fence, f.sprite);
      assert(e1 == std::errc{} && *p1 == ':' && "failed to parse sprite index");

      const auto [p2, e2] = std::from_chars(p1 + 1, fence, f.duration);
      assert(e2 == std::errc{} && "failed to parse frame duration");

      ++animation.count;
      cursor = p2;
      if (cursor < fence && *cursor == ',') ++cursor;
    }

    _animations.emplace_back(animation);

    if (_animations.size() == 1) {
      r.animation = animation.name;
      r.counter = 0;
      r.current_frame = 0;
      r.sprite = animation.frames[0].sprite;
    }
  }
}

object::~object() noexcept {
  if (_registry && _entity != entt::null) {
    _registry->destroy(_entity);
  }
}

object::object(object&& other) noexcept
  : _registry(other._registry)
  , _entity(other._entity)
  , _animations(std::move(other._animations)) {
  other._registry = nullptr;
  other._entity = entt::null;
}

object& object::operator=(object&& other) noexcept {
  if (this != &other) {
    if (_registry && _entity != entt::null) {
      _registry->destroy(_entity);
    }
    _registry = other._registry;
    _entity = other._entity;
    _animations = std::move(other._animations);
    other._registry = nullptr;
    other._entity = entt::null;
  }
  return *this;
}

int16_t object::z() const {
  return _registry->get<sorteable>(_entity).z;
}

void object::z(int16_t value) {
  _registry->get<sorteable>(_entity).z = value;
}
