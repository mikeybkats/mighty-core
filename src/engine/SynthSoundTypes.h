#pragma once

#include <cstdint>

#include "SynthRealtimeParams.h"

// -----------------------------------------------------------------------------
// SynthSoundTypes — POD patch data for Sound::createSound().
//
// VoiceEngine::Subtractive — dual VCO, ladder VCF, dual envelope, LFO, ring mod.
// VoiceEngine::Plucked     — Karplus–Strong string (guitar/harp/piano plucks).
//
// Master delay/chorus are configured per patch (EffectsSpec); the wet mix is
// applied on the summed voice bus when notes play.
// -----------------------------------------------------------------------------

enum class OscFootage : int {
  Foot32 = 32,
  Foot16 = 16,
  Foot8 = 8,
  Foot4 = 4,
  Foot2 = 2,
  Low = 0,
};

enum class OscWave : uint8_t {
  Sine,
  Tri,
  Saw,
  Square,
  PolySaw,
  PolySquare,
};

enum class FilterMode : uint8_t {
  LowPass24,  ///< Main VCF: 4-pole ladder lowpass (default).
  LowPass12,  ///< Main VCF: 2-pole ladder lowpass.
  /// Single-stage bandpass only (e.g. kick); bypasses the post-LP highpass path.
  BandPass24,
  BandPass12,
};

enum class VoiceEngine : uint8_t {
  Subtractive,
  Plucked,
};

struct VcoSpec {
  OscFootage range = OscFootage::Foot8;
  OscWave wave = OscWave::PolySaw;
  float tuneSemis = 0.f;
  float pulseWidth = 0.5f;
  float level = 0.5f;
  /// Osc hard-sync style lock (UI-facing; currently keeps osc2 pitch locked to osc1 base).
  bool sync = false;
  /// Adds a one-octave-down square sub oscillator for this VCO when supported.
  bool subOsc = false;
  bool enabled = true;
};

struct VcfSpec {
  float cutoffHz = 800.f;
  float resonance = 0.3f;
  /// Filter envelope depth 0..1 (octaves of cutoff sweep at full env).
  float envDepth = 0.5f;
  /// Keyboard tracking 0..1 (1 = cutoff follows pitch, ref MIDI 60).
  float keyTrack = 0.33f;
  FilterMode mode = FilterMode::LowPass24;
  /// Post-LP highpass cutoff (Hz). 0 = HP stage off.
  float highpassCutoffHz = 0.f;
  /// Post-LP highpass resonance 0..1.8 (self-oscillates at high values).
  float highpassResonance = 0.2f;
};

struct AdsrSpec {
  float attackSec = 0.01f;
  float decaySec = 0.2f;
  float sustain = 0.6f;
  float releaseSec = 0.3f;
};

enum class LfoTarget : uint8_t {
  Pitch = 0,
  PulseWidth = 1,
};

/// Per-oscillator LFO (bipolar sine at rateHz).
struct OscLfoSpec {
  float rateHz = 5.f;
  /// Modulation amount 0..1 (maps to pitch semitones or PWM swing).
  float depth = 0.f;
  LfoTarget target = LfoTarget::Pitch;
  /// When true, LFO positive half-cycle opens amp/filter gate (requires main gate on).
  bool gateTrigger = false;
};

struct MixerSpec {
  float osc1 = 0.6f;
  float osc2 = 0.4f;
  float noise = 0.f;
  /// Ring mod amount; product of leveled osc1 × leveled osc2 (after mixer osc levels).
  float ringMod = 0.f;
};

/// DaisySP StringVoice (Rings-style Karplus). See stringvoice.h / DaisySP docs.
/// https://electro-smith.github.io/DaisySP/classdaisysp_1_1_string_voice.html
struct PluckSpec {
  /// String type: 0–0.26 curved bridge (guitar/bass), 0.26–1 dispersion (piano/harp).
  float structure = 0.12f;
  /// Timbre + strike noise density (0..1).
  float brightness = 0.5f;
  /// Decay; full damping needs accent near 1.
  float damping = 0.55f;
  /// Strike hardness; boosts brightness and damping on the attack.
  float accent = 0.85f;
  /// Continuous dust excitation (bowed); leave false for plucks.
  bool sustain = false;
  float level = 1.f;
};

/// Post-voice bus: chorus + feedback delay (applied in Synthesizer::renderVoices).
struct EffectsSpec {
  /// Overall wet amount 0..1 for this patch.
  float wet = 0.f;
  float chorusDepth = 0.35f;
  float chorusRateHz = 0.6f;
  float delayMs = 180.f;
  float delayFeedback = 0.38f;
  /// Balance inside wet: 0 = all chorus, 1 = all delay.
  float delayMix = 0.45f;
};

struct SynthSoundSpec {
  VoiceEngine engine = VoiceEngine::Subtractive;

  VcoSpec osc1{};
  VcoSpec osc2{};
  VcfSpec filter{};
  AdsrSpec ampEnv{};
  AdsrSpec filterEnv{};
  OscLfoSpec osc1Lfo{};
  OscLfoSpec osc2Lfo{};
  MixerSpec mixer{};
  PluckSpec pluck{};
  EffectsSpec effects{};

  float masterVolume = 0.75f;
  /// Portamento time in seconds (0 = instant pitch).
  float glideSec = 0.f;
  /// Enables envelope modulation on osc2 pitch.
  bool osc2EnvMod = false;
  /// Osc2 env amount in semitones when osc2EnvMod is enabled.
  float osc2EnvAmountSemis = 0.f;
};
