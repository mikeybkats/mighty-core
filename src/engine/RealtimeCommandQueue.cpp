#include "engine/RealtimeCommandQueue.h"

bool RealtimeCommandQueue::tryPush(const RealtimeCommand& cmd) {
  const size_t head = head_.load(std::memory_order_relaxed);
  const size_t next = (head + 1) % kCapacity;
  const size_t tail = tail_.load(std::memory_order_acquire);
  if (next == tail) {
    return false;
  }

  buffer_[head] = cmd;
  head_.store(next, std::memory_order_release);
  return true;
}

size_t RealtimeCommandQueue::popMany(RealtimeCommand* out, size_t maxCount) {
  if (!out || maxCount == 0) return 0;

  size_t popped = 0;
  size_t tail = tail_.load(std::memory_order_relaxed);
  const size_t head = head_.load(std::memory_order_acquire);
  while (tail != head && popped < maxCount) {
    out[popped++] = buffer_[tail];
    tail = (tail + 1) % kCapacity;
  }

  tail_.store(tail, std::memory_order_release);
  return popped;
}

size_t RealtimeCommandQueue::sizeApprox() const {
  const size_t head = head_.load(std::memory_order_acquire);
  const size_t tail = tail_.load(std::memory_order_acquire);
  return (head + kCapacity - tail) % kCapacity;
}
