#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

#include "engine/SynthSoundTypes.h"

class Synthesizer;

// -----------------------------------------------------------------------------
// Sound — preset catalog (createSound) + metronome click playback.
//
// * createSound(spec)  → SoundId (patch in the library, like a synth program)
// * allocateVoice(id)  → runtime pool index in Synthesizer (1..15)
// * triggerNote(...)   → play a MIDI note on that pool voice
//
// Metronome clicks are separate (PCM kit slots or synthesized click on voice 0).
// Composer / Orchestrator choose *when* to play; Sound owns *timbre* definitions.
// -----------------------------------------------------------------------------

/// Handle returned by createSound(). Invalid if createSound failed.
using SoundId = int;

class Sound {
 public:
  /// Sentinel returned when createSound() fails or the id is otherwise unusable.
  static constexpr SoundId kInvalidSound = -1;

  /// Max kit WAV slots for metronome (matches MightyMusicCore / BuiltInKits).
  static constexpr int kSlotCount = 12;

  /// selection() / playbackSlot value: use Synthesizer voice 0 instead of PCM.
  static constexpr int kUseSynthesized = -1;

  /// Max registered patch presets (guitar, bass, drums, etc.).
  static constexpr int kMaxSoundPresets = 32;

  /// Registers built-in presets and retains @p synthesizer for voice/click rendering.
  explicit Sound(Synthesizer& synthesizer);

  // ---- Sound library (patch definitions) --------------------------------------

  /// Registers a subtractive patch (VCO1/VCO2/VCF/ENV/mixer). Returns SoundId or kInvalidSound.
  /// Call on the control thread during setup — not from the audio callback.
  SoundId createSound(const SynthSoundSpec& spec);

  /// True when @p id refers to a preset stored in soundPresets_.
  [[nodiscard]] bool isValidSound(SoundId id) const;

  /// Patch definition for @p id, or nullptr if invalid.
  [[nodiscard]] const SynthSoundSpec* getSoundSpec(SoundId id) const;

  /// Built-in SoundIds (registered in the constructor). Use with allocateVoice().
  [[nodiscard]] SoundId guitarSound() const { return guitarSound_; }
  [[nodiscard]] SoundId bassSound() const { return bassSound_; }
  [[nodiscard]] SoundId pianoSound() const { return pianoSound_; }
  [[nodiscard]] SoundId kickDrumSound() const { return kickDrumSound_; }

  // ---- Runtime voice pool (playing notes) -------------------------------------

  /// Reserves a Synthesizer pool voice and loads the preset for @p soundId.
  /// @return Pool voice index (1..15), or -1 if the pool or preset is unavailable.
  int allocateVoice(SoundId soundId);

  /// Returns a pool voice to the free list without stopping an in-flight note abruptly.
  void releaseVoice(int voiceIndex);

  /// Starts or retriggers a MIDI note on an allocated pool voice.
  void triggerNote(int voiceIndex, int midiNote, float velocity);

  /// Sends note-off on a pool voice (release envelope).
  void releaseNote(int voiceIndex);

  // ---- Metronome PCM / click selection ----------------------------------------

  /// Stores kit WAV @p samples for metronome slot @p index (control thread).
  void setPcmSlot(int index, std::vector<float> samples, int32_t sourceSampleRate);

  /// Selects which kit slot plays on the next click; @p index may be kUseSynthesized.
  void setSelection(int index);

  /// Currently selected kit slot, or kUseSynthesized.
  [[nodiscard]] int selection() const;

  /// Resamples loaded kit slots to @p deviceSampleRate for playback in renderPartial.
  void prepareDeviceRate(int32_t deviceSampleRate);

  /// Sample rate used by devicePcm_ after the last prepareDeviceRate(); 0 if unprepared.
  [[nodiscard]] int32_t preparedDeviceRate() const;

  /// Drops resampled device buffers (e.g. on stream stop or sample-rate change).
  void clearDeviceBuffers();

  /// Length in samples of the built-in synthesized click at the prepared device rate.
  [[nodiscard]] int32_t synthesizedClickLength() const;

  /// Click length for the current selection (PCM slot or synthesized).
  [[nodiscard]] int32_t clickLengthForSelection() const;

  /// Slot index actually used for playback (falls back to kUseSynthesized if PCM missing).
  [[nodiscard]] int resolvePlaybackSlot() const;

  /// Per-callback state when a click spans multiple audio buffers.
  struct ClickPlaybackState {
    /// Sample offset into the current click (0 .. totalLength).
    int32_t phase = 0;
    /// Total click length in samples for this trigger.
    int32_t totalLength = 0;
    /// Resolved kit slot or kUseSynthesized (set in triggerAt).
    int playbackSlot = kUseSynthesized;
    /// Low-pass the click on weak beats when true.
    bool softenOffbeat = false;
    /// Pairing gain applied to this tick (e.g. downbeat emphasis).
    float pairGainMul = 1.f;
  };

  /// Renders the tail of an in-progress click from @p state into @p buf.
  void renderPartial(float* buf, int32_t frameOffset, int32_t numFrames, ClickPlaybackState& state);

  /// Starts a click at @p tickFrame and renders the first segment into @p buf.
  void triggerAt(float* buf, int32_t tickFrame, int32_t numFrames, bool softenOffbeat,
                 float pairGainMul, ClickPlaybackState& state);

  /// True when no click is active (phase >= totalLength).
  [[nodiscard]] bool isIdle(const ClickPlaybackState& state) const;

 private:
  /// Registers guitar, bass, piano, and kick presets during construction.
  void registerBuiltInSounds();

  /// Renders @p count samples of one click segment (PCM or synthesized).
  void renderSegment(float* buf, int32_t frameOffset, int32_t clickSample, int32_t count,
                     int playbackSlot, bool softenOffbeat, float pairGainMul,
                     ClickPlaybackState& state);

  /// DaisySP voice pool used for notes and synthesized clicks.
  Synthesizer& synthesizer_;

  /// All patches created via createSound() (indexed by SoundId).
  std::vector<SynthSoundSpec> soundPresets_;

  /// SoundIds for built-in instruments (set in registerBuiltInSounds).
  SoundId guitarSound_{kInvalidSound};
  SoundId bassSound_{kInvalidSound};
  SoundId pianoSound_{kInvalidSound};
  SoundId kickDrumSound_{kInvalidSound};

  /// Raw kit WAV per slot, at original file sample rate.
  std::array<std::vector<float>, kSlotCount> sourcePcm_{};
  /// Source sample rate per sourcePcm_ slot.
  std::array<int32_t, kSlotCount> sourceRates_{};
  /// sourcePcm_ resampled to deviceRatePrepared_ for the audio callback.
  std::array<std::vector<float>, kSlotCount> devicePcm_{};
  /// Device rate used to build devicePcm_; 0 when buffers are cleared.
  int32_t deviceRatePrepared_{0};
  /// Active metronome kit selection (atomic for control vs audio thread).
  std::atomic<int> selection_{kUseSynthesized};
};
