#include "trigonometry.hpp"

constexpr auto RESOLUTION = 3600;
constexpr auto QUARTER = RESOLUTION / 4;
constexpr auto DEGREES_TO_INDEX = static_cast<float>(RESOLUTION) / 360.0f;

static const auto table = [] {
  std::array<float, RESOLUTION> t{};
  for (auto i = 0uz; i < RESOLUTION; ++i) {
    const auto radians = static_cast<float>(i) * (std::numbers::pi_v<float> * 2.0f / RESOLUTION);
    t[i] = std::sin(radians);
  }
  return t;
}();

static auto to_index(float degrees) -> size_t {
  const auto index = static_cast<int>(degrees * DEGREES_TO_INDEX);
  return static_cast<size_t>(((index % RESOLUTION) + RESOLUTION) % RESOLUTION);
}

float lsin(float degrees) {
  return table[to_index(degrees)];
}

float lcos(float degrees) {
  return table[(to_index(degrees) + QUARTER) % RESOLUTION];
}
