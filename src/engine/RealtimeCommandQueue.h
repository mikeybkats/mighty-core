#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

/// Shared command envelope for real-time-safe control updates.
/// Single producer (control thread) -> single consumer (audio thread).
struct RealtimeCommand {
  enum class Domain : uint8_t {
    Synth = 1,
  };

  enum class SynthOp : uint8_t {
    SetParam = 1,
  };

  Domain domain = Domain::Synth;
  uint8_t op = static_cast<uint8_t>(SynthOp::SetParam);
  uint16_t reserved = 0;
  int32_t target = -1;     // Meaning depends on domain (Synth: voice index, -1 = all active voices).
  int32_t paramId = 0;     // Domain-specific parameter id.
  float value = 0.0f;      // Domain-specific scalar value.
  int32_t frameOffset = 0; // Optional scheduling within callback; 0 = apply immediately.
};

class RealtimeCommandQueue {
 public:
  static constexpr size_t kCapacity = 1024;

  /// Pushes a command from the control thread. Returns false when full.
  bool tryPush(const RealtimeCommand& cmd);

  /// Pops up to @p maxCount commands on the audio thread.
  size_t popMany(RealtimeCommand* out, size_t maxCount);

  /// Approximate queued command count (for diagnostics only).
  [[nodiscard]] size_t sizeApprox() const;

 private:
  std::array<RealtimeCommand, kCapacity> buffer_{};
  std::atomic<size_t> head_{0};  // write position (producer)
  std::atomic<size_t> tail_{0};  // read position (consumer)
};
