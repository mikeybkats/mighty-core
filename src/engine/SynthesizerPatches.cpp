#include "engine/SynthesizerPatches.h"

namespace SynthesizerPatches {

const std::array<PatchDefinition, kBuiltInPatchCount>& builtInPatches() {
  static const std::array<PatchDefinition, kBuiltInPatchCount> kPatches = {{
      {
          "Default Square",
          {
              .engine = VoiceEngine::Subtractive,
              .osc1 =
                  {
                      .range = OscFootage::Foot8,
                      .wave = OscWave::Square,
                      .pulseWidth = 0.5f,
                      .level = 0.6f,
                  },
              .osc2 =
                  {
                      .range = OscFootage::Foot8,
                      .wave = OscWave::Square,
                      .pulseWidth = 0.5f,
                      .level = 0.4f,
                  },
              .filter =
                  {
                      .cutoffHz = 1200.f,
                      .resonance = 0.2f,
                      .envDepth = 0.0f,
                      .keyTrack = 0.0f,
                      .mode = FilterMode::Low,
                      .drive = 0.2f,
                  },
              .ampEnv =
                  {
                      .attackSec = 0.01f,
                      .decaySec = 0.2f,
                      .sustain = 0.8f,
                      .releaseSec = 0.25f,
                  },
              .filterEnv =
                  {
                      .attackSec = 0.01f,
                      .decaySec = 0.2f,
                      .sustain = 0.0f,
                      .releaseSec = 0.25f,
                  },
              .mixer =
                  {
                      .osc1 = 0.6f,
                      .osc2 = 0.4f,
                      .noise = 0.0f,
                      .ringMod = 0.0f,
                  },
              .effects =
                  {
                      .wet = 0.0f,
                  },
              .masterVolume = 0.7f,
              .glideSec = 0.0f,
              .osc2EnvMod = false,
              .osc2EnvAmountSemis = 0.0f,
          },
      },
      {
          "Guitar",
          {
              .engine = VoiceEngine::Plucked,
              .pluck =
                  {
                      .structure = 0.14f,
                      .brightness = 0.58f,
                      .damping = 0.5f,
                      .accent = 0.92f,
                      .level = 0.88f,
                  },
              .ampEnv =
                  {
                      .attackSec = 0.001f,
                      .decaySec = 0.45f,
                      .sustain = 0.08f,
                      .releaseSec = 0.22f,
                  },
              .effects =
                  {
                      .wet = 0.1f,
                      .chorusDepth = 0.25f,
                      .chorusRateHz = 0.4f,
                      .delayMs = 55.f,
                      .delayFeedback = 0.18f,
                      .delayMix = 0.35f,
                  },
              .masterVolume = 0.54f,
          },
      },
      {
          "Bass",
          {
              .engine = VoiceEngine::Plucked,
              .pluck =
                  {
                      .structure = 0.06f,
                      .brightness = 0.24f,
                      .damping = 0.82f,
                      .accent = 0.9f,
                      .level = 0.92f,
                  },
              .ampEnv =
                  {
                      .attackSec = 0.002f,
                      .decaySec = 0.28f,
                      .sustain = 0.55f,
                      .releaseSec = 0.1f,
                  },
              .effects =
                  {
                      .wet = 0.06f,
                      .chorusDepth = 0.12f,
                      .delayMs = 40.f,
                      .delayFeedback = 0.12f,
                      .delayMix = 0.1f,
                  },
              .masterVolume = 0.6f,
          },
      },
      {
          "Piano",
          {
              .osc1 =
                  {
                      .range = OscFootage::Foot8,
                      .wave = OscWave::Tri,
                      .level = 0.52f,
                  },
              .osc2 =
                  {
                      .range = OscFootage::Foot4,
                      .wave = OscWave::Tri,
                      .tuneSemis = 0.03f,
                      .level = 0.38f,
                  },
              .filter =
                  {
                      .cutoffHz = 3200.f,
                      .resonance = 0.12f,
                      .envDepth = 0.4f,
                      .keyTrack = 0.5f,
                      .drive = 0.35f,
                  },
              .ampEnv =
                  {
                      .attackSec = 0.001f,
                      .decaySec = 0.55f,
                      .sustain = 0.35f,
                      .releaseSec = 0.3f,
                  },
              .filterEnv =
                  {
                      .attackSec = 0.001f,
                      .decaySec = 0.4f,
                      .sustain = 0.f,
                      .releaseSec = 0.25f,
                  },
              .mixer =
                  {
                      .osc1 = 0.52f,
                      .osc2 = 0.38f,
                  },
              .effects =
                  {
                      .wet = 0.08f,
                      .chorusDepth = 0.1f,
                      .delayMs = 50.f,
                      .delayFeedback = 0.15f,
                      .delayMix = 0.4f,
                  },
              .masterVolume = 0.45f,
          },
      },
      {
          "Kick drum",
          {
              .osc1 =
                  {
                      .range = OscFootage::Foot32,
                      .wave = OscWave::PolySquare,
                      .level = 0.85f,
                  },
              .osc2 =
                  {
                      .enabled = false,
                  },
              .filter =
                  {
                      .cutoffHz = 120.f,
                      .resonance = 0.45f,
                      .envDepth = 0.9f,
                      .mode = FilterMode::Band,
                      .drive = 0.65f,
                  },
              .ampEnv =
                  {
                      .attackSec = 0.001f,
                      .decaySec = 0.12f,
                      .sustain = 0.f,
                      .releaseSec = 0.05f,
                  },
              .filterEnv =
                  {
                      .attackSec = 0.001f,
                      .decaySec = 0.08f,
                      .sustain = 0.f,
                      .releaseSec = 0.04f,
                  },
              .mixer =
                  {
                      .osc1 = 0.75f,
                      .noise = 0.2f,
                  },
              .masterVolume = 0.65f,
          },
      },
  }};
  return kPatches;
}

}  // namespace SynthesizerPatches
