#pragma once

#include "common.hpp"

class soundfx final {
public:
  explicit soundfx(std::string_view filename);
  ~soundfx();

  soundfx(const soundfx&) = delete;
  soundfx& operator=(const soundfx&) = delete;
  soundfx(soundfx&&) = delete;
  soundfx& operator=(soundfx&&) = delete;

  void play();
  void stop() noexcept;

  void set_volume(float gain) noexcept;
  float volume() const noexcept;

  void set_loop(bool loop) noexcept;
  bool loop() const noexcept;

  bool started();
  bool ended();

private:
  ma_audio_buffer _buffer{};
  ma_sound _sound{};
  std::atomic<bool> _ended{false};
  bool _started{false};
};
