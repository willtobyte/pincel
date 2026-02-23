#include "object.hpp"
#include "io.hpp"

namespace {
  entt::id_type hash(std::string_view sv) {
    return entt::hashed_string::value(sv.data(), sv.size());
  }
}

entt::entity object::create(entt::registry& registry, int16_t z, std::string_view name) {
  const auto entity = registry.create();
  registry.emplace<sorteable>(entity, sorteable{z});
  auto& r = registry.emplace<renderable>(entity);
  registry.emplace<transform>(entity);

  const auto path = std::format("objects/{}.meta", name);
  const auto buffer = io::read(path);
  const auto content = std::string_view(
    reinterpret_cast<const char*>(buffer.data()), buffer.size());
  animatable a{};

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

    auto key = line.substr(0, eq);
    auto value = line.substr(eq + 1);

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
    } else if (value.ends_with('!')) {
      animation.once = true;
      frames_part.remove_suffix(1);
    }

    auto cursor = frames_part.data();
    const auto* const fence = frames_part.data() + frames_part.size();

    while (cursor < fence && animation.count < animation.keyframes.size()) {
      auto& f = animation.keyframes[animation.count];

      const auto [p1, e1] = std::from_chars(cursor, fence, f.sprite);
      assert(e1 == std::errc{} && *p1 == ':' && "failed to parse sprite index");

      const auto [p2, e2] = std::from_chars(p1 + 1, fence, f.duration);
      assert(e2 == std::errc{} && "failed to parse frame duration");

      ++animation.count;
      cursor = p2;
      if (cursor < fence && *cursor == ',') ++cursor;
    }

    assert(a.count < a.animations.size() && "too many animations");
    a.animations[a.count++] = animation;
  }

  registry.emplace<animatable>(entity, a.animations, a.count);
  registry.emplace<object>(entity);

  return entity;
}
