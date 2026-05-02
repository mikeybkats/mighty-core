#include "BuiltInKits.h"

namespace BuiltInKits {

namespace {

static const KitSoundEntry kMetronomeGentleEntries[] = {
    {"metkick_gentle", "Kick (gentle)"},
    {"metsnare_gentle", "Snare (gentle)"},
    {"metdigi_gentle", "Digital (gentle)"},
    {"metrimshot_gentle", "Rim shot (gentle)"},
    {nullptr, nullptr},
};

} // namespace

const KitDefinition kMetronomeGentle = {
    "metronome_gentle",
    kMetronomeGentleEntries,
};

} // namespace BuiltInKits
