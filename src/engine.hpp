#pragma once

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
