#include <gtest/gtest.h>

#include "BuiltInKits.h"
#include "KitDefinition.h"
#include "Metronome.h"
#include "MightyMusicCore.h"

TEST(Metronome, BeforeLoadKitExposesNoKitData) {
  Metronome m;
  EXPECT_EQ(m.loadedKit(), nullptr);
  EXPECT_EQ(m.loadedKitSoundCount(), 0);
  EXPECT_EQ(m.loadedKitSoundResourceName(0), nullptr);
  EXPECT_EQ(m.loadedKitSoundDisplayName(0), nullptr);
}

TEST(Metronome, LoadKitCountsSoundsUntilNullTerminator) {
  Metronome m;
  m.loadKit(Metronome::KitId::MetronomeGentle);
  EXPECT_EQ(m.loadedKitSoundCount(), 4);
  EXPECT_STREQ(m.loadedKit()->kitId, "metronome_gentle");
  EXPECT_STREQ(m.loadedKitSoundResourceName(0), "metkick_gentle");
  EXPECT_STREQ(m.loadedKitSoundResourceName(3), "metrimshot_gentle");
  EXPECT_EQ(m.loadedKitSoundResourceName(4), nullptr);
}

TEST(Metronome, DisplayNamesMatchBuiltInKit) {
  Metronome m;
  m.loadKit(Metronome::KitId::MetronomeGentle);
  EXPECT_STREQ(m.loadedKitSoundDisplayName(0), "Kick (gentle)");
  EXPECT_STREQ(m.loadedKitSoundDisplayName(3), "Rim shot (gentle)");
}

TEST(Metronome, LoadedKitMatchesBuiltInDefinition) {
  Metronome m;
  m.loadKit(Metronome::KitId::MetronomeGentle);
  const KitDefinition* loaded = m.loadedKit();
  ASSERT_NE(loaded, nullptr);
  EXPECT_EQ(loaded, &BuiltInKits::kMetronomeGentle);
  EXPECT_EQ(kitSoundCount(*loaded), m.loadedKitSoundCount());
  EXPECT_EQ(m.loadedKitId(), Metronome::KitId::MetronomeGentle);
}

TEST(Metronome, BuiltInKitHasNullTerminatorAfterLastSound) {
  const KitDefinition& def = BuiltInKits::kMetronomeGentle;
  const int n = kitSoundCount(def);
  ASSERT_EQ(n, 4);
  EXPECT_EQ(def.sounds[n].resourceName, nullptr);
}

TEST(Metronome, LoadKitIsIdempotent) {
  Metronome m;
  m.loadKit(Metronome::KitId::MetronomeGentle);
  const KitDefinition* first = m.loadedKit();
  m.loadKit(Metronome::KitId::MetronomeGentle);
  EXPECT_EQ(m.loadedKit(), first);
  EXPECT_EQ(m.loadedKitSoundCount(), 4);
}

static_assert(kKitMaxSounds == MightyMusicCore::kMaxTickSoundSlots);
