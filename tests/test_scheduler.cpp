// Unit tests for Scheduler math only (no audio device).
#include "Scheduler.h"
#include <gtest/gtest.h>

// ---------------------------------------------------------------------------
// BPM get/set
// ---------------------------------------------------------------------------

TEST(Scheduler, DefaultBPMIs120) {
    Scheduler s;
    EXPECT_DOUBLE_EQ(s.getBPM(), 120.0);
}

TEST(Scheduler, SetAndGetBPM) {
    Scheduler s;
    s.setBPM(90.0);
    EXPECT_DOUBLE_EQ(s.getBPM(), 90.0);
}

TEST(Scheduler, BPMClampedAtMinimum) {
    Scheduler s;
    s.setBPM(10.0);
    EXPECT_DOUBLE_EQ(s.getBPM(), 20.0);
}

TEST(Scheduler, BPMClampedAtMaximum) {
    Scheduler s;
    s.setBPM(500.0);
    EXPECT_DOUBLE_EQ(s.getBPM(), 300.0);
}

// ---------------------------------------------------------------------------
// First tick fires at frame 0
// ---------------------------------------------------------------------------

TEST(Scheduler, FirstTickAtFrameZero) {
    Scheduler s(48000);
    s.setBPM(120.0);

    auto result = s.advance(256);
    ASSERT_EQ(result.count, 1);
    EXPECT_EQ(result.ticks[0].frameOffset, 0);
    EXPECT_EQ(result.ticks[0].beatNumber, 0);
}

TEST(Scheduler, BeatNumbersIncrement) {
    Scheduler s(48000);
    s.setBPM(120.0);

    // 120 BPM @ 48 kHz → 24000 samples/beat → collect 3 beats
    int beatsFound = 0;
    for (int buf = 0; buf < 500 && beatsFound < 3; ++buf) {
        auto r = s.advance(256);
        for (int i = 0; i < r.count; ++i)
            EXPECT_EQ(r.ticks[i].beatNumber, beatsFound++);
    }
    EXPECT_EQ(beatsFound, 3);
}

// ---------------------------------------------------------------------------
// Tick interval accuracy
// ---------------------------------------------------------------------------

// At 120 BPM, 48 kHz: exactly 24000 samples between beats.
TEST(Scheduler, TickIntervalIsExact_120BPM_48kHz) {
    Scheduler s(48000);
    s.setBPM(120.0);

    int64_t absoluteFrame = 0;
    int64_t prevTickFrame = -1;
    int     tickCount     = 0;

    // Run for enough buffers to collect 5 ticks (5 * 24000 / 256 ≈ 469 buffers)
    for (int buf = 0; buf < 500; ++buf) {
        auto r = s.advance(256);
        for (int i = 0; i < r.count; ++i) {
            int64_t tickFrame = absoluteFrame + r.ticks[i].frameOffset;
            if (prevTickFrame >= 0) {
                int64_t interval = tickFrame - prevTickFrame;
                // Allow ±1 sample due to integer truncation of the exact position
                EXPECT_NEAR(interval, 24000, 1)
                    << "interval wrong at tick " << tickCount;
            }
            prevTickFrame = tickFrame;
            ++tickCount;
        }
        absoluteFrame += 256;
    }
    EXPECT_GE(tickCount, 4);
}

TEST(Scheduler, TickIntervalIsExact_60BPM_44100Hz) {
    Scheduler s(44100);
    s.setBPM(60.0);
    // 60 BPM @ 44100 Hz → 44100 samples/beat

    int64_t absoluteFrame = 0;
    int64_t prevTickFrame = -1;
    int     tickCount     = 0;

    for (int buf = 0; buf < 700; ++buf) {
        auto r = s.advance(256);
        for (int i = 0; i < r.count; ++i) {
            int64_t tickFrame = absoluteFrame + r.ticks[i].frameOffset;
            if (prevTickFrame >= 0)
                EXPECT_NEAR(tickFrame - prevTickFrame, 44100, 1);
            prevTickFrame = tickFrame;
            ++tickCount;
        }
        absoluteFrame += 256;
    }
    EXPECT_GE(tickCount, 3);
}

// ---------------------------------------------------------------------------
// Reset
// ---------------------------------------------------------------------------

TEST(Scheduler, ResetRestartsBeatCounter) {
    Scheduler s(48000);
    s.setBPM(120.0);

    // Advance past the first tick
    s.advance(512);
    s.reset();

    auto r = s.advance(256);
    ASSERT_GE(r.count, 1);
    EXPECT_EQ(r.ticks[0].beatNumber, 0);    // beat counter restarted
    EXPECT_EQ(r.ticks[0].frameOffset, 0);   // tick fires immediately after reset
}

// ---------------------------------------------------------------------------
// BPM change mid-playback
// ---------------------------------------------------------------------------

// BPM changes don't retcon the already-scheduled next tick; they take effect on
// the interval *after* that tick fires. Verified by checking the gap between the
// second and third ticks.
TEST(Scheduler, BPMChangeTakesEffectOnNextInterval) {
    Scheduler s(48000);
    s.setBPM(120.0); // 24000 samples/beat

    // Consume first tick (fires at frame 0)
    s.advance(256);

    // Change to 60 BPM → next interval should become 48000 samples
    s.setBPM(60.0);

    int64_t absoluteFrame   = 256;
    int64_t secondTickFrame = -1;
    int64_t thirdTickFrame  = -1;

    for (int buf = 0; buf < 1200 && thirdTickFrame < 0; ++buf) {
        auto r = s.advance(256);
        for (int i = 0; i < r.count; ++i) {
            int64_t tf = absoluteFrame + r.ticks[i].frameOffset;
            if (r.ticks[i].beatNumber == 1) secondTickFrame = tf;
            if (r.ticks[i].beatNumber == 2) thirdTickFrame  = tf;
        }
        absoluteFrame += 256;
    }

    ASSERT_GE(secondTickFrame, 0) << "second tick never fired";
    ASSERT_GE(thirdTickFrame,  0) << "third tick never fired";

    // Second tick still fires at the 120-BPM-scheduled position (24000)
    EXPECT_NEAR(secondTickFrame, 24000, 1);

    // Interval from second → third tick now uses the new 60 BPM pace (48000)
    EXPECT_NEAR(thirdTickFrame - secondTickFrame, 48000, 1);
}
