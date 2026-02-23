#include "object.hpp"
#include "io.hpp"

namespace {
  entt::id_type hash(std::string_view sv) {
    return entt::hashed_string::value(sv.data(), sv.size());
  }
}

entt::entity object::create(entt::registry& registry, int16_t z, std::string_view name, float x, float y, std::string_view animation) {
  const auto entity = registry.create();
  registry.emplace<sorteable>(entity, sorteable{z});
  auto& r = registry.emplace<renderable>(entity);
  registry.emplace<transform>(entity, x, y);

  if (!animation.empty())
    r.animation = hash(animation);

  const auto path = std::format("objects/{}.meta", name);
  const auto buffer = io::read(path);
  const auto content = std::string_view(
    reinterpret_cast<const char*>(buffer.data()), buffer.size());
  animatable an{};

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

    struct animation a{};
    a.name = hash(key);

    auto parts = value;
    if (const auto arrow = value.find('>'); arrow != std::string_view::npos) {
      parts = value.substr(0, arrow);
      a.next = hash(value.substr(arrow + 1));
    } else if (value.ends_with('!')) {
      a.once = true;
      parts.remove_suffix(1);
    }

    auto cursor = parts.data();
    const auto* const fence = parts.data() + parts.size();

    while (cursor < fence && a.count < a.keyframes.size()) {
      auto& f = a.keyframes[a.count];

      const auto [p1, e1] = std::from_chars(cursor, fence, f.sprite);
      assert(e1 == std::errc{} && *p1 == ':' && "failed to parse sprite index");

      const auto [p2, e2] = std::from_chars(p1 + 1, fence, f.duration);
      assert(e2 == std::errc{} && "failed to parse frame duration");

      ++a.count;
      cursor = p2;
      if (cursor < fence && *cursor == ',') ++cursor;
    }

    assert(an.count < an.animations.size() && "too many animations");
    an.animations[an.count++] = a;
  }

  registry.emplace<animatable>(entity, an.animations, an.count);
  registry.emplace<object>(entity);

  return entity;
}
