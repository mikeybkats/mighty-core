// MightyMusicCore tests verify high-level API/orchestration behavior.
#include <gtest/gtest.h>

#include "MightyMusicCore.h"

TEST(MightyMusicCore, StopWithoutStartIsHarmless) {
  MightyMusicCore core;
  EXPECT_NO_THROW(core.stop());
}

TEST(MightyMusicCore, ListeningStopWithoutStartIsHarmless) {
  MightyMusicCore core;
  EXPECT_NO_THROW(core.stopListening());
}

TEST(MightyMusicCore, DefaultBPMIsAccessibleFromCoreApi) {
  MightyMusicCore core;
  EXPECT_DOUBLE_EQ(core.getBPM(), 120.0);
}

TEST(MightyMusicCore, SetBPMThroughCoreApi) {
  MightyMusicCore core;
  core.setBPM(100.0);
  EXPECT_DOUBLE_EQ(core.getBPM(), 100.0);
}
