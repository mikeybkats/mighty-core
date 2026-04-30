#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

// Mighty Music Core (MMC) — platform-agnostic metronome engine.
// Android apps link this as a static library and bridge JNI to these calls.
// Internally: TempoEngine opens the OS audio device, Scheduler decides beat
// times in samples, and the audio callback mixes the tick (WAV or sine) +
// invokes onTick.
class MightyMusicCore {
public:
  enum class TickSound {
    Sine = -1,
    Slot0 = 0,
    Slot1 = 1,
    Slot2 = 2,
    Slot3 = 3,
  };

  MightyMusicCore();
  ~MightyMusicCore();

  // Opens the default output stream and begins audio callbacks (Oboe on
  // Android, miniaudio on desktop hosts). Copies the current onTick handler
  // into the engine.
  void start();
  void stop();
  [[nodiscard]] bool isPlaying() const;

  // BPM is stored atomically inside Scheduler; safe to call from UI / JNI while
  // playing.
  void setBPM(double bpm);
  [[nodiscard]] double getBPM() const;

  // Preload mono float PCM for one of four tick slots (typically 48 kHz from
  // decoded WAV). Safe to call before start(); if the engine is already open at
  // a known device rate, buffers are resampled to the device immediately. Index
  // must be in [0, 3].
  void setTickSoundPcm(int index, std::vector<float> samples,
                       int32_t sourceSampleRate);
  // Selects synthesized sine click or one of the preloaded tick PCM slots.
  void setTickSound(TickSound sound);

  // void startListening()
  // void stopListening()

  // Fired from the audio thread when a beat boundary is crossed — do not block,
  // allocate, or take locks. Platform code often forwards this to another
  // thread for UI.
  std::function<void(int beatNumber)> onTick;

private:
  friend class Metronome;
  // Policy hooks used by the app/domain metronome layer.
  void setTwoBeatMeasureInternal(bool enabled);
  void setSwingFractionInternal(double fraction);

  class Impl;
  std::unique_ptr<Impl> impl_;
};
