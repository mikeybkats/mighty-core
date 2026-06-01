#include <gtest/gtest.h>

#include "engine/RealtimeCommandQueue.h"

TEST(RealtimeCommandQueue, PushAndPopSingleCommand) {
  RealtimeCommandQueue q;
  RealtimeCommand in{};
  in.domain = RealtimeCommand::Domain::Synth;
  in.op = static_cast<uint8_t>(RealtimeCommand::SynthOp::SetParam);
  in.target = 3;
  in.paramId = 2;
  in.value = 1200.0f;

  EXPECT_TRUE(q.tryPush(in));

  RealtimeCommand out[1];
  const size_t popped = q.popMany(out, 1);
  ASSERT_EQ(popped, 1u);
  EXPECT_EQ(out[0].target, 3);
  EXPECT_EQ(out[0].paramId, 2);
  EXPECT_FLOAT_EQ(out[0].value, 1200.0f);
}

TEST(RealtimeCommandQueue, ReturnsFalseWhenQueueIsFull) {
  RealtimeCommandQueue q;
  RealtimeCommand cmd{};

  size_t pushed = 0;
  while (q.tryPush(cmd)) {
    ++pushed;
  }

  EXPECT_EQ(pushed, RealtimeCommandQueue::kCapacity - 1);
  EXPECT_FALSE(q.tryPush(cmd));
}
