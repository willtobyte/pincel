#include "trigonometry.hpp"

namespace {
constexpr auto resolution = 3600;
constexpr auto quarter = resolution / 4;
constexpr auto degress_to_index = static_cast<float>(resolution) / 360.0f;
}

static const auto table = [] {
  std::array<float, resolution> t{};
  for (auto i = 0uz; i < resolution; ++i) {
    const auto radians = static_cast<float>(i) * (std::numbers::pi_v<float> * 2.0f / resolution);
    t[i] = std::sin(radians);
  }
  return t;
}();

static auto to_index(float degrees) -> size_t {
  const auto index = static_cast<int>(degrees * degress_to_index);
  return static_cast<size_t>(((index % resolution) + resolution) % resolution);
}

float lsin(float degrees) {
  return table[to_index(degrees)];
}

float lcos(float degrees) {
  return table[(to_index(degrees) + quarter) % resolution];
}
