#pragma once

#include "common.hpp"
#include "manager.hpp"

class engine final {
public:
  engine();
  ~engine() = default;

  void run();

protected:
  void loop();

private:
  bool _running{true};
  manager _manager;
};
