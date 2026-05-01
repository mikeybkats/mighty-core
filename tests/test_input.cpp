// Input tests focus on lifecycle safety. StartListening is hardware/permission
// dependent on each platform, so we keep this suite deterministic.
#include "Input.h"
#include <gtest/gtest.h>

TEST(Input, StopListeningWithoutStartIsHarmless) {
    Input input;
    EXPECT_NO_THROW(input.stopListening());
}

TEST(Input, RepeatedStopListeningIsHarmless) {
    Input input;
    input.stopListening();
    EXPECT_NO_THROW(input.stopListening());
}
