#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

// Mighty Music Core (MMC) — playback bus + metronome + synth preview API.
//
// Playback (openPlayback) is shared; metronome start/stop and synth trigger/release
// are independent. Android JNI bridges these calls.
class MightyMusicCore {
 public:
  /// Maximum PCM click slots (must match `kKitMaxSounds` policy cap in practice).
  static constexpr int kMaxTickSoundSlots = 12;
  static constexpr int kTickSoundSine = -1;

  MightyMusicCore();
  ~MightyMusicCore();

  /// Metronome transport (opens playback if needed).
  void start();
  void stop();
  [[nodiscard]] bool isPlaying() const;

  /// Shared output device — required for synth; independent of metronome.
  bool openPlayback();
  void closePlayback();
  [[nodiscard]] bool isPlaybackOpen() const;

  void setBPM(double bpm);
  [[nodiscard]] double getBPM() const;

  // Preload mono float PCM for a tick slot. Safe before start(); resampled when
  // the device rate is known. Index must be in [0, kMaxTickSoundSlots).
  void setTickSoundPcm(int index, std::vector<float> samples, int32_t sourceSampleRate);
  /// `kTickSoundSine` or 0 .. kMaxTickSoundSlots-1 for a PCM slot (empty slot → sine).
  void setTickSound(int soundIndex);

  void startListening();
  void stopListening();
  [[nodiscard]] bool isListening() const;
  [[nodiscard]] bool hasDetectedInputSignal() const;
  [[nodiscard]] int lastDetectedMidiNote() const;

  /// Built-in subtractive presets (Sound::guitarSound, etc.).
  static constexpr int kSynthPatchCount = 4;
  [[nodiscard]] const char* synthPatchName(int patchIndex) const;

  /// Note-on on a pooled voice; opens playback if needed (no metronome required).
  void triggerSynthNote(int patchIndex, int midiNote, float velocity);
  /// Note-off (amp envelope release); keeps the voice allocated for retrigger.
  void releaseSynthGate();

  std::function<void(int beatNumber)> onTick;

 private:
  friend class Metronome;
  void setTwoBeatMeasureInternal(bool enabled);
  void setSwingFractionInternal(double fraction);
  void resetSynthVoiceHeld();
  void ensurePlaybackOpen();

  class Impl;
  std::unique_ptr<Impl> impl_;
};
