#pragma once

#include <functional>

#include "MightyMusicCore.h"

// App/domain metronome policy layer.
// Owns rhythm behavior (swing, two-beat feel) and drives MightyMusicCore primitives.
class Metronome {
public:
  using TickSound = MightyMusicCore::TickSound;

  Metronome() = default;

  void start();
  void stop();
  [[nodiscard]] bool isPlaying() const;

  void setBPM(double bpm);
  [[nodiscard]] double getBPM() const;

  void setTickSoundPcm(int index, std::vector<float> samples,
                       int32_t sourceSampleRate);
  void setTickSound(TickSound sound);

  void setTwoBeatMeasure(bool enabled);
  [[nodiscard]] bool getTwoBeatMeasure() const;

  // 0..0.5, where 0.5 is the strongest swing.
  void setSwingFraction(double fraction);
  [[nodiscard]] double getSwingFraction() const;

  // Called from audio thread via MightyMusicCore.
  std::function<void(int beatNumber)> onTick;

private:
  void syncPolicyToCore();

  MightyMusicCore core_{};
  bool twoBeatMeasure_{false};
  double swingFraction_{0.0};
};
