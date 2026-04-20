#include "MightyMusicCore.h"
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

TEST(MightyMusicCore, NotPlayingAfterConstruction) {
    MightyMusicCore core;
    EXPECT_FALSE(core.isPlaying());
}

TEST(MightyMusicCore, IsPlayingAfterStart) {
    MightyMusicCore core;
    core.setBPM(120.0);
    core.start();
    EXPECT_TRUE(core.isPlaying());
    core.stop();
}

TEST(MightyMusicCore, NotPlayingAfterStop) {
    MightyMusicCore core;
    core.start();
    core.stop();
    EXPECT_FALSE(core.isPlaying());
}

TEST(MightyMusicCore, StopWithoutStartIsHarmless) {
    MightyMusicCore core;
    EXPECT_NO_THROW(core.stop());
}

// ---------------------------------------------------------------------------
// BPM
// ---------------------------------------------------------------------------

TEST(MightyMusicCore, DefaultBPM) {
    MightyMusicCore core;
    EXPECT_DOUBLE_EQ(core.getBPM(), 120.0);
}

TEST(MightyMusicCore, SetBPMBeforeStart) {
    MightyMusicCore core;
    core.setBPM(90.0);
    EXPECT_DOUBLE_EQ(core.getBPM(), 90.0);
}

TEST(MightyMusicCore, SetBPMWhilePlaying) {
    MightyMusicCore core;
    core.start();
    core.setBPM(180.0);
    EXPECT_DOUBLE_EQ(core.getBPM(), 180.0);
    core.stop();
}

// ---------------------------------------------------------------------------
// onTick integration (real audio, timing-tolerant)
// ---------------------------------------------------------------------------

// At 120 BPM, expect 2 ticks/second. Run for 1.5 s → expect 2–4 ticks.
TEST(MightyMusicCore, TickCallbackFires) {
    MightyMusicCore core;
    std::atomic<int> tickCount{0};
    core.onTick = [&](int) { tickCount.fetch_add(1, std::memory_order_relaxed); };

    core.setBPM(120.0);
    core.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    core.stop();

    EXPECT_GE(tickCount.load(), 2);
    EXPECT_LE(tickCount.load(), 4);
}

// Verify the beat number passed to onTick is monotonically increasing.
TEST(MightyMusicCore, BeatNumbersAreSequential) {
    MightyMusicCore core;
    std::atomic<int> lastBeat{-1};
    std::atomic<bool> outOfOrder{false};

    core.onTick = [&](int beat) {
        int prev = lastBeat.exchange(beat, std::memory_order_relaxed);
        if (beat != prev + 1 && prev != -1)
            outOfOrder.store(true, std::memory_order_relaxed);
    };

    core.setBPM(240.0); // fast — 4 ticks/sec
    core.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    core.stop();

    EXPECT_FALSE(outOfOrder.load());
    EXPECT_GE(lastBeat.load(), 3);
}
