#pragma once

#include "common.hpp"

class manager;

class engine final {
public:
  engine();
  ~engine() = default;

  void run();

protected:
  void loop();

private:
  bool _running{true};
  std::unique_ptr<manager> _manager;
};
