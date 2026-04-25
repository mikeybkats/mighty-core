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

void Scheduler::setTwoBeatMeasure(bool enabled) {
    twoBeatMeasure_.store(enabled, std::memory_order_relaxed);
}

void Scheduler::setSwingFraction(double fraction) {
    swingFraction_.store(
        std::clamp(fraction, 0.0, 0.5), std::memory_order_relaxed);
}

bool Scheduler::getTwoBeatMeasure() const {
    return twoBeatMeasure_.load(std::memory_order_relaxed);
}

double Scheduler::intervalForEmittedBeat(int beatIndex) const {
    const double spb = samplesPerBeat();
    if (!twoBeatMeasure_.load(std::memory_order_relaxed))
        return spb;
    const double swing = swingFraction_.load(std::memory_order_relaxed);
    const double halfSpb = spb * 0.5;
    // Double-time in two-beat mode: emit twice as many clicks as straight mode.
    // Swing redistributes each click pair while keeping the pair total at one straight beat.
    return (beatIndex % 2 == 0) ? halfSpb * (1.0 + swing) : halfSpb * (1.0 - swing);
}

AdvanceResult Scheduler::advance(int32_t numFrames) {
    AdvanceResult result;
    const int64_t bufferEnd = samplePosition_ + numFrames;

    // Emit every beat whose exact sample time falls inside [samplePosition_, bufferEnd).
    while (static_cast<int64_t>(nextTickExact_) < bufferEnd
           && result.count < AdvanceResult::kMaxTicks)
    {
        auto frameOffset = static_cast<int32_t>(
            static_cast<int64_t>(nextTickExact_) - samplePosition_);
        frameOffset = std::clamp(frameOffset, 0, numFrames - 1);

        const int thisBeat = beatNumber_;
        result.ticks[result.count++] = { frameOffset, thisBeat };
        ++beatNumber_;
        nextTickExact_ += intervalForEmittedBeat(thisBeat);
    }

    samplePosition_ += numFrames;
    return result;
}
