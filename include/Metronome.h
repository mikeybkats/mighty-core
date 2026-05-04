#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include "KitDefinition.h"
#include "MightyMusicCore.h"

// App/domain metronome policy layer.
// Kit defining: static `KitDefinition` tables (BuiltInKits, etc.).
// Kit loading: `loadKit` selects one kit; hosts iterate `loadedKitSoundCount()` and
// fill PCM with `setTickSoundPcm` using `loadedKitSoundResourceName(i)` until the
// kit list ends (see `KitDefinition`).

class Metronome {
 public:
  enum class KitId : int32_t {
    MetronomeGentle = 0,
  };

  Metronome() = default;

  void start();
  void stop();
  [[nodiscard]] bool isPlaying() const;

  void setBPM(double bpm);
  [[nodiscard]] double getBPM() const;

  /// Selects built-in kit data (non-owning pointer). Resets active tick to the first
  /// kit sound, or sine when the kit lists no samples.
  void loadKit(KitId id);

  [[nodiscard]] KitId loadedKitId() const {
    return loadedKitId_;
  }
  [[nodiscard]] const KitDefinition* loadedKit() const {
    return loadedKit_;
  }
  [[nodiscard]] int loadedKitSoundCount() const;
  [[nodiscard]] const char* loadedKitSoundResourceName(int index) const;
  [[nodiscard]] const char* loadedKitSoundDisplayName(int index) const;

  /// PCM slot index must be in [0, MightyMusicCore::kMaxTickSoundSlots).
  void setTickSoundPcm(int slotIndex, std::vector<float> samples, int32_t sourceSampleRate);
  /// -1 = sine; otherwise slot index (typically 0 .. loadedKitSoundCount()-1).
  void setActiveTickSound(int slotIndex);

  void setTwoBeatMeasure(bool enabled);
  [[nodiscard]] bool getTwoBeatMeasure() const;

  void setSwingFraction(double fraction);
  [[nodiscard]] double getSwingFraction() const;

  std::function<void(int beatNumber)> onTick;

 private:
  void syncPolicyToCore();

  MightyMusicCore core_{};
  bool twoBeatMeasure_{false};
  double swingFraction_{0.0};
  const KitDefinition* loadedKit_{nullptr};
  KitId loadedKitId_{KitId::MetronomeGentle};
};
