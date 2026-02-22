#pragma once

#include "common.hpp"
#include "font.hpp"

class engine final {
public:
  engine();
  ~engine() = default;

  void run();

protected:
  void loop();

private:
  bool _running{true};
  std::optional<font> _font;
};
