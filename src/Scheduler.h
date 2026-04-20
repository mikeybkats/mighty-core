#pragma once

#include <atomic>
#include <cstdint>

struct TickEvent {
    int32_t frameOffset; // frame within the current buffer where the tick fires
    int     beatNumber;
};

// Fixed-size result — no heap allocation on the audio thread.
struct AdvanceResult {
    static constexpr int kMaxTicks = 8; // never exceeded at sane BPM + buffer sizes
    int       count = 0;
    TickEvent ticks[kMaxTicks];
};

// Pure tick-scheduling math. No audio I/O, no callbacks.
// Drive it by calling advance() with each audio buffer's frame count.
// BPM is atomic so it can be set from any thread while advance() runs on the audio thread.
class Scheduler {
public:
    explicit Scheduler(int32_t sampleRate = 48000);

    void    reset();
    void    setSampleRate(int32_t sampleRate);
    int32_t getSampleRate() const;

    void   setBPM(double bpm);   // clamped to [20, 300]
    double getBPM() const;

    // Called once per audio buffer from the audio thread.
    AdvanceResult advance(int32_t numFrames);

private:
    double samplesPerBeat() const;

    int32_t             sampleRate_;
    std::atomic<double> bpm_{120.0};
    int64_t             samplePosition_{0};
    double              nextTickExact_{0.0};
    int                 beatNumber_{0};
};
