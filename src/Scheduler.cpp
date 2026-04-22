// Implementation of sample-accurate beat scheduling (see Scheduler.h).
#include "Scheduler.h"

#include <algorithm>

Scheduler::Scheduler(int32_t sampleRate)
    : sampleRate_(sampleRate) {}

void Scheduler::reset() {
    samplePosition_ = 0;
    nextTickExact_  = 0.0;
    beatNumber_     = 0;
}

void Scheduler::setSampleRate(int32_t sampleRate) {
    sampleRate_ = sampleRate;
}

int32_t Scheduler::getSampleRate() const {
    return sampleRate_;
}

void Scheduler::setBPM(double bpm) {
    bpm_.store(std::clamp(bpm, 20.0, 300.0), std::memory_order_relaxed);
}

double Scheduler::getBPM() const {
    return bpm_.load(std::memory_order_relaxed);
}

double Scheduler::samplesPerBeat() const {
    return sampleRate_ * 60.0 / bpm_.load(std::memory_order_relaxed);
}

AdvanceResult Scheduler::advance(int32_t numFrames) {
    AdvanceResult result;
    const double  spb       = samplesPerBeat();
    const int64_t bufferEnd = samplePosition_ + numFrames;

    // Emit every beat whose exact sample time falls inside [samplePosition_, bufferEnd).
    while (static_cast<int64_t>(nextTickExact_) < bufferEnd
           && result.count < AdvanceResult::kMaxTicks)
    {
        int32_t frameOffset = static_cast<int32_t>(
            static_cast<int64_t>(nextTickExact_) - samplePosition_);
        frameOffset = std::clamp(frameOffset, 0, numFrames - 1);

        result.ticks[result.count++] = { frameOffset, beatNumber_++ };
        nextTickExact_ += spb;
    }

    samplePosition_ += numFrames;
    return result;
}
