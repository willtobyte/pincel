#pragma once

#include "common.hpp"

struct collidable final {
  b2BodyId body{};
  b2ShapeId shape{};
  float hx{}, hy{}, hw{}, hh{};
  float ox{}, oy{};
};
