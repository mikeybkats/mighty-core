#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include "MightyMusicCore.h"

// App/domain metronome policy layer.
// Owns rhythm behavior (swing, two-beat feel) and drives MightyMusicCore primitives.
class Metronome {
public:
  using TickSound = MightyMusicCore::TickSound;
  enum class PaletteSound : int32_t {
    SineClassic = -1,
    MetKickGentle = 0,
    MetSnareGentle = 1,
    MetDigiGentle = 2,
    MetRimshotGentle = 3,
  };
  enum class Kit : int32_t {
    Metronome = 0,
  };

  Metronome() = default;

  void start();
  void stop();
  [[nodiscard]] bool isPlaying() const;

  void setBPM(double bpm);
  [[nodiscard]] double getBPM() const;

  void setTickSoundPcm(int index, std::vector<float> samples,
                       int32_t sourceSampleRate);
  void setTickSound(TickSound sound);
  void setPaletteSoundPcm(PaletteSound sound, std::vector<float> samples,
                          int32_t sourceSampleRate);
  void setPaletteSound(PaletteSound sound);

  void setKit(Kit kit);
  [[nodiscard]] Kit getKit() const;
  [[nodiscard]] static std::vector<PaletteSound> getKitPalette(Kit kit);
  [[nodiscard]] static const char* getPaletteResourceName(PaletteSound sound);

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
  Kit activeKit_{Kit::Metronome};
};
