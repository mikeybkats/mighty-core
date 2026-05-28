#pragma once

#include <cstdint>

/// Runtime-adjustable synth parameter ids for MightyMusicCore::queueSynthParamChange().
enum class SynthRealtimeParamId : int32_t {
  MasterVolume = 1,
  FilterCutoffHz = 2,
  FilterResonance = 3,
  FilterDrive = 4,
  AmpAttackSec = 5,
  AmpDecaySec = 6,
  AmpSustain = 7,
  AmpReleaseSec = 8,
  LfoRateHz = 9,
  EffectsWet = 10,
  EffectsDelayMs = 11,
  EffectsDelayFeedback = 12,
  EffectsDelayMix = 13,
  PluckBrightness = 14,
  PluckDamping = 15,
  PluckStructure = 16,
  PluckAccent = 17,
  GlideSec = 18,
  Osc1RangeFeet = 19,
  Osc1Sync = 20,
  Osc1SubOsc = 21,
  Osc1Waveform = 22,
  Osc1PulseWidth = 23,
  Osc2RangeFeet = 24,
  Osc2Waveform = 25,
  Osc2EnvMod = 26,
  Osc2EnvAmountSemis = 27,
  PluckMode = 28,
  Osc1Level = 29,
  Osc2Level = 30,
  LfoDepth = 31,
};
