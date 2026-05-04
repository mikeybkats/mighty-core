// Tests for kit defining: null-terminated sound lists and helper queries.
#include <gtest/gtest.h>

#include "KitDefinition.h"

namespace {

const KitSoundEntry kTerminatorOnly[] = {
    {nullptr, nullptr},
};

const KitSoundEntry kSingleSound[] = {
    {"tick_a", "Tick A"},
    {nullptr, nullptr},
};

const KitSoundEntry kStopsAtEmptyResource[] = {
    {"first", "First"},
    {"", "ignored"},
    {"never_seen", "Never"},
    {nullptr, nullptr},
};

const KitSoundEntry kNullDisplayFallsBackToResource[] = {
    {"res_only", nullptr},
    {nullptr, nullptr},
};

// Exactly twelve named resources, then terminator (exercises kKitMaxSounds cap).
const KitSoundEntry kTwelveSounds[] = {
    {"s00", "0"},  {"s01", "1"},  {"s02", "2"},       {"s03", "3"}, {"s04", "4"},
    {"s05", "5"},  {"s06", "6"},  {"s07", "7"},       {"s08", "8"}, {"s09", "9"},
    {"s10", "10"}, {"s11", "11"}, {nullptr, nullptr},
};

// Thirteen would-be entries: helpers must never report more than kKitMaxSounds.
const KitSoundEntry kThirteenSounds[] = {
    {"t00", nullptr},   {"t01", nullptr}, {"t02", nullptr}, {"t03", nullptr}, {"t04", nullptr},
    {"t05", nullptr},   {"t06", nullptr}, {"t07", nullptr}, {"t08", nullptr}, {"t09", nullptr},
    {"t10", nullptr},   {"t11", nullptr}, {"t12", "extra"},  // unreachable for count, but present
                                                             // in storage
    {nullptr, nullptr},
};

}  // namespace

TEST(KitDefinition, NullSoundsPointerYieldsZeroCount) {
  const KitDefinition kit{"empty", nullptr};
  EXPECT_EQ(kitSoundCount(kit), 0);
  EXPECT_EQ(kitSoundResourceName(kit, 0), nullptr);
  EXPECT_EQ(kitSoundDisplayName(kit, 0), nullptr);
}

TEST(KitDefinition, ImmediateTerminatorYieldsZeroCount) {
  const KitDefinition kit{"no_sounds", kTerminatorOnly};
  EXPECT_EQ(kitSoundCount(kit), 0);
}

TEST(KitDefinition, CountsSingleSoundBeforeTerminator) {
  const KitDefinition kit{"one", kSingleSound};
  EXPECT_EQ(kitSoundCount(kit), 1);
  EXPECT_STREQ(kitSoundResourceName(kit, 0), "tick_a");
  EXPECT_STREQ(kitSoundDisplayName(kit, 0), "Tick A");
}

TEST(KitDefinition, EmptyResourceNameTerminatesList) {
  const KitDefinition kit{"trunc", kStopsAtEmptyResource};
  EXPECT_EQ(kitSoundCount(kit), 1);
  EXPECT_STREQ(kitSoundResourceName(kit, 0), "first");
  EXPECT_EQ(kitSoundResourceName(kit, 1), nullptr);
}

TEST(KitDefinition, NullDisplayNameFallsBackToResourceName) {
  const KitDefinition kit{"fallback", kNullDisplayFallsBackToResource};
  EXPECT_EQ(kitSoundCount(kit), 1);
  EXPECT_STREQ(kitSoundDisplayName(kit, 0), "res_only");
}

TEST(KitDefinition, OutOfRangeQueriesReturnNull) {
  const KitDefinition kit{"one", kSingleSound};
  EXPECT_EQ(kitSoundResourceName(kit, -1), nullptr);
  EXPECT_EQ(kitSoundResourceName(kit, 1), nullptr);
  EXPECT_EQ(kitSoundDisplayName(kit, 99), nullptr);
}

TEST(KitDefinition, ExactlyMaxSoundsCountsTwelve) {
  const KitDefinition kit{"dozen", kTwelveSounds};
  EXPECT_EQ(kitSoundCount(kit), kKitMaxSounds);
  EXPECT_STREQ(kitSoundResourceName(kit, 0), "s00");
  EXPECT_STREQ(kitSoundResourceName(kit, 11), "s11");
  EXPECT_EQ(kitSoundResourceName(kit, 12), nullptr);
}

TEST(KitDefinition, MoreThanMaxEntriesStillCountsTwelve) {
  const KitDefinition kit{"overflow", kThirteenSounds};
  EXPECT_EQ(kitSoundCount(kit), kKitMaxSounds);
  EXPECT_STREQ(kitSoundResourceName(kit, 11), "t11");
  EXPECT_EQ(kitSoundResourceName(kit, 12), nullptr);
}

TEST(KitDefinition, MaxSoundsConstantIsTwelve) {
  EXPECT_EQ(kKitMaxSounds, 12);
}
