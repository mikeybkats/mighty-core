#pragma once

#include <atomic>
#include <cstdint>
#include <memory>

#include "engine/SynthSoundTypes.h"

// -----------------------------------------------------------------------------
// Synthesizer — DaisySP voice pool (render only).
//
// Patch definitions live in Sound::createSound(). Supports subtractive
// (2 VCO + SVF + LFO) and plucked (Karplus) engines plus a shared FX bus.
//
// Voice 0 = metronome click. Voices 1..15 = pool for notes.
// -----------------------------------------------------------------------------

class Synthesizer {
 public:
  /// Default synthesized click length when computing clickDurationSamples_.
  static constexpr int kDefaultClickMs = 8;

  /// Total voices: one click + fifteen assignable note voices.
  static constexpr int kMaxVoices = 16;

  /// Reserved voice for metronome clicks (not returned by allocateVoice()).
  static constexpr int kClickVoiceIndex = 0;

  /// First index returned by allocateVoice().
  static constexpr int kFirstAssignableVoice = 1;

  /// Initializes the voice pool at the default sample rate.
  Synthesizer();

  ~Synthesizer();

  Synthesizer(const Synthesizer&) = delete;
  Synthesizer& operator=(const Synthesizer&) = delete;

  /// Reconfigures all voices and recomputes clickDurationSamples_ (control thread).
  void setSampleRate(int32_t sampleRate);

  /// Current audio sample rate in Hz.
  [[nodiscard]] int32_t sampleRate() const;

  /// Synthesized click length in samples at the current sample rate.
  [[nodiscard]] int32_t clickDurationSamples() const;

  /// Renders a segment of the synthesized click into @p buf (voice 0).
  void renderClick(float* buf, int32_t frameOffset, int32_t clickSample, int32_t count,
                   bool softenOffbeat, float pairGainMul);

  /// Claims an empty pool slot (does not configure timbre — call applySoundSpec next).
  /// @return Voice index in [kFirstAssignableVoice, kMaxVoices), or -1 if full.
  int allocateVoice();

  /// Marks a pool voice free; safe if the index is already idle.
  void releaseVoice(int voiceIndex);

  /// Loads a patch from Sound::createSound() onto a pool voice (control thread).
  void applySoundSpec(int voiceIndex, const SynthSoundSpec& spec);

  /// Note-on: sets pitch, velocity, and retriggers envelopes on a pool voice.
  void triggerNote(int voiceIndex, int midiNote, float velocity);

  /// Note-off: enters release on a pool voice.
  void releaseNote(int voiceIndex);

  /// Sums all sounding pool voices into @p buf (audio thread).
  void renderVoices(float* buf, int32_t frameOffset, int32_t count);

 private:
  /// Pimpl holding ClickVoice + PatchVoice instances (DaisySP objects).
  struct VoiceStorage;

  /// Voice pool storage (click + subtractive voices).
  std::unique_ptr<VoiceStorage> voices_;

  /// Audio sample rate in Hz.
  int32_t sampleRate_{48000};

  /// Cached click length in samples (derived from kDefaultClickMs and sampleRate_).
  int32_t clickDurationSamples_{0};

  /// Bit mask of pool voices currently active (audio-thread read/write).
  std::atomic<uint32_t> voiceMask_{0};
};
