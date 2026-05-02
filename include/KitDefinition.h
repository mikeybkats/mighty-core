#pragma once

// Kit defining: a metronome "kit" is a null-terminated list of sounds (max 12).
// The list ends at the first entry with `resourceName == nullptr` or empty string.

struct KitSoundEntry {
  /// Android `res/raw` basename without extension; ends the list when null or "".
  const char* resourceName;
  /// UTF-8 label for hosts (debug UI, optional Android row text). May be nullptr.
  const char* displayName;
};

struct KitDefinition {
  const char*     kitId;
  const KitSoundEntry* sounds;
};

/// Hard cap on how many sample slots a kit may enumerate (engine matches this).
constexpr int kKitMaxSounds = 12;

/// Counts entries until null/empty `resourceName` or `kKitMaxSounds`.
[[nodiscard]] inline int kitSoundCount(const KitDefinition& kit) {
  if (!kit.sounds) {
    return 0;
  }
  int n = 0;
  for (; n < kKitMaxSounds; ++n) {
    const char* r = kit.sounds[n].resourceName;
    if (r == nullptr || r[0] == '\0') {
      break;
    }
  }
  return n;
}

[[nodiscard]] inline const char* kitSoundResourceName(const KitDefinition& kit, int index) {
  if (index < 0 || index >= kitSoundCount(kit)) {
    return nullptr;
  }
  return kit.sounds[index].resourceName;
}

[[nodiscard]] inline const char* kitSoundDisplayName(const KitDefinition& kit, int index) {
  if (index < 0 || index >= kitSoundCount(kit)) {
    return nullptr;
  }
  const char* label = kit.sounds[index].displayName;
  return label ? label : kit.sounds[index].resourceName;
}
