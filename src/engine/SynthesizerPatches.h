#pragma once

#include <array>

#include "engine/SynthSoundTypes.h"

namespace SynthesizerPatches {

struct PatchDefinition {
  const char* name;
  SynthSoundSpec spec;
};

constexpr int kBuiltInPatchCount = 5;

const std::array<PatchDefinition, kBuiltInPatchCount>& builtInPatches();

}  // namespace SynthesizerPatches
