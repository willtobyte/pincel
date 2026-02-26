#include "soundfx.hpp"

soundfx::soundfx(std::string_view filename) {
  int channels = 0;

  {
    const auto buffer = io::read(filename);

    auto error = 0;
    const std::unique_ptr<OggOpusFile, decltype(&op_free)> codec(
      op_open_memory(buffer.data(), buffer.size(), &error),
      &op_free
    );

    assert((error == 0) && "[op_open_memory] failed to decode");

    channels = op_channel_count(codec.get(), -1);
    const auto nsamples = op_pcm_total(codec.get(), -1);
    const auto total = static_cast<size_t>(nsamples) * static_cast<size_t>(channels);

    auto samples = std::make_unique_for_overwrite<float[]>(total);

    size_t offset = 0;
    while (offset < total) {
      const auto read = op_read_float(
        codec.get(),
        samples.get() + offset,
        static_cast<int>(total - offset),
        nullptr
      );

      if (read == OP_HOLE) {
        continue;
      }

      assert((read >= 0) && "[op_read_float] failed to decode");

      if (read == 0) {
        break;
      }

      offset += static_cast<size_t>(read) * static_cast<size_t>(channels);
    }

    auto config = ma_audio_buffer_config_init(
      ma_format_f32,
      static_cast<ma_uint32>(channels),
      offset / static_cast<size_t>(channels),
      samples.get(),
      nullptr
    );
    config.sampleRate = 48000;

    ma_audio_buffer_init_copy(&config, &_buffer);
  }

  ma_sound_init_from_data_source(
    audioengine,
    &_buffer,
    MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_NO_PITCH,
    nullptr,
    &_sound
  );

  ma_sound_set_end_callback(&_sound, [](void* ptr, ma_sound*) {
    static_cast<soundfx*>(ptr)->_ended.store(true, std::memory_order_release);
  }, this);
}

soundfx::~soundfx() {
  ma_sound_uninit(&_sound);
  ma_audio_buffer_uninit(&_buffer);
}

void soundfx::play() {
  ma_sound_seek_to_pcm_frame(&_sound, 0);
  ma_sound_start(&_sound);
  _started = true;
}

void soundfx::stop() noexcept {
  ma_sound_stop(&_sound);
}

void soundfx::set_volume(float gain) noexcept {
  ma_sound_set_volume(&_sound, std::clamp(gain, .0f, 1.0f));
}

float soundfx::volume() const noexcept {
  return ma_sound_get_volume(&_sound);
}

void soundfx::set_loop(bool loop) noexcept {
  ma_sound_set_looping(&_sound, loop ? MA_TRUE : MA_FALSE);
}

bool soundfx::loop() const noexcept {
  return ma_sound_is_looping(&_sound) == MA_TRUE;
}

bool soundfx::started() {
  return std::exchange(_started, false);
}

bool soundfx::ended() {
  return _ended.exchange(false, std::memory_order_acquire);
}
