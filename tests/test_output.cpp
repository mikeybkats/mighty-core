// Integration tests against the real audio backend used by Output.
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

#include "engine/Output.h"

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

TEST(Output, NotPlayingAfterConstruction) {
  Output output;
  EXPECT_FALSE(output.isPlaying());
}

TEST(Output, IsPlayingAfterStart) {
  Output output;
  output.setBPM(120.0);
  output.start(nullptr);
  EXPECT_TRUE(output.isPlaying());
  output.stop();
}

TEST(Output, NotPlayingAfterStop) {
  Output output;
  output.start(nullptr);
  output.stop();
  EXPECT_FALSE(output.isPlaying());
}

TEST(Output, StopWithoutStartIsHarmless) {
  Output output;
  EXPECT_NO_THROW(output.stop());
}

// ---------------------------------------------------------------------------
// BPM
// ---------------------------------------------------------------------------

TEST(Output, DefaultBPM) {
  Output output;
  EXPECT_DOUBLE_EQ(output.getBPM(), 120.0);
}

TEST(Output, SetBPMBeforeStart) {
  Output output;
  output.setBPM(90.0);
  EXPECT_DOUBLE_EQ(output.getBPM(), 90.0);
}

TEST(Output, SetBPMWhilePlaying) {
  Output output;
  output.start(nullptr);
  output.setBPM(180.0);
  EXPECT_DOUBLE_EQ(output.getBPM(), 180.0);
  output.stop();
}

// ---------------------------------------------------------------------------
// Tick callback integration (real audio, timing-tolerant)
// ---------------------------------------------------------------------------

// At 120 BPM, expect 2 ticks/second. Run for 1.5 s -> expect 2-4 ticks.
TEST(Output, TickCallbackFires) {
  Output output;
  std::atomic<int> tickCount{0};

  output.setBPM(120.0);
  output.start([&](int) { tickCount.fetch_add(1, std::memory_order_relaxed); });
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));
  output.stop();

  EXPECT_GE(tickCount.load(), 2);
  EXPECT_LE(tickCount.load(), 4);
}

// Verify beat numbers are monotonically increasing.
TEST(Output, BeatNumbersAreSequential) {
  Output output;
  std::atomic<int> lastBeat{-1};
  std::atomic<bool> outOfOrder{false};

  output.start([&](int beat) {
    int prev = lastBeat.exchange(beat, std::memory_order_relaxed);
    if (beat != prev + 1 && prev != -1) {
      outOfOrder.store(true, std::memory_order_relaxed);
    }
  });

  output.setBPM(240.0);  // fast - 4 ticks/sec
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));
  output.stop();

  EXPECT_FALSE(outOfOrder.load());
  EXPECT_GE(lastBeat.load(), 3);
}
