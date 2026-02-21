#pragma once

constexpr auto MAX_DELTA = 0.05f;

class engine final {
public:
  engine();
  ~engine() = default;

  void run();

protected:
  void loop();

private:
  bool _running{true};
};
