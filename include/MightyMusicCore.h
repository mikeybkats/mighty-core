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
  /// Maximum PCM click slots (must match `kKitMaxSounds` policy cap in practice).
  static constexpr int kMaxTickSoundSlots = 12;
  static constexpr int kTickSoundSine     = -1;

  MightyMusicCore();
  ~MightyMusicCore();

  void start();
  void stop();
  [[nodiscard]] bool isPlaying() const;

  void setBPM(double bpm);
  [[nodiscard]] double getBPM() const;

  // Preload mono float PCM for a tick slot. Safe before start(); resampled when
  // the device rate is known. Index must be in [0, kMaxTickSoundSlots).
  void setTickSoundPcm(int index, std::vector<float> samples,
                       int32_t sourceSampleRate);
  /// `kTickSoundSine` or 0 .. kMaxTickSoundSlots-1 for a PCM slot (empty slot → sine).
  void setTickSound(int soundIndex);

  void startListening();
  void stopListening();

  std::function<void(int beatNumber)> onTick;

private:
  friend class Metronome;
  void setTwoBeatMeasureInternal(bool enabled);
  void setSwingFractionInternal(double fraction);

  class Impl;
  std::unique_ptr<Impl> impl_;
};
