#include "TempoEngine.h"

#ifdef __ANDROID__

#include <android/log.h>
#include <cinttypes>
#include <cmath>
#include <cstring>
#include <algorithm>

#define LOG_TAG "TempoEngine"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

static constexpr float TWO_PI         = 6.28318530718f;
static constexpr float CLICK_FREQ_HZ  = 1000.0f;
static constexpr float CLICK_DECAY    = 8.0f;   // envelope steepness
static constexpr float CLICK_GAIN     = 0.7f;
static constexpr float CLICK_MS       = 8.0f;   // click length in milliseconds

bool TempoEngine::open() {
    oboe::AudioStreamBuilder builder;
    oboe::Result result = builder
        .setDirection(oboe::Direction::Output)
        ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
        ->setSharingMode(oboe::SharingMode::Exclusive)
        ->setFormat(oboe::AudioFormat::Float)
        ->setChannelCount(oboe::ChannelCount::Mono)
        ->setDataCallback(this)
        ->openStream(stream_);

    if (result != oboe::Result::OK) {
        LOGD("Failed to open stream: %s", oboe::convertToText(result));
        return false;
    }

    sampleRate_    = stream_->getSampleRate();
    clickDuration_ = static_cast<int32_t>(sampleRate_ * CLICK_MS / 1000.0f);
    clickPhase_    = clickDuration_; // silent until first tick

    // Schedule first tick at frame 0
    samplePosition_ = 0;
    nextTickExact_  = 0.0;
    beatNumber_     = 0;

    result = stream_->start();
    if (result != oboe::Result::OK) {
        LOGD("Failed to start stream: %s", oboe::convertToText(result));
        return false;
    }

    LOGD("Stream opened: %d Hz, bufferSize=%d", sampleRate_, stream_->getBufferSizeInFrames());
    return true;
}

void TempoEngine::close() {
    if (stream_) {
        stream_->stop();
        stream_->close();
        stream_.reset();
        LOGD("Stream closed");
    }
}

bool TempoEngine::isRunning() const {
    return stream_ && stream_->getState() == oboe::StreamState::Started;
}

void TempoEngine::setBPM(double bpm) {
    bpm_.store(std::clamp(bpm, 20.0, 300.0), std::memory_order_relaxed);
}

double TempoEngine::getBPM() const {
    return bpm_.load(std::memory_order_relaxed);
}

oboe::DataCallbackResult TempoEngine::onAudioReady(
    oboe::AudioStream* /*stream*/,
    void* audioData,
    int32_t numFrames)
{
    float* output = static_cast<float*>(audioData);
    std::memset(output, 0, numFrames * sizeof(float));

    // Continue any click started in a previous callback
    if (clickPhase_ < clickDuration_) {
        int32_t remaining = clickDuration_ - clickPhase_;
        int32_t toRender  = std::min(remaining, numFrames);
        renderClick(output, 0, clickPhase_, toRender);
        clickPhase_ += toRender;
    }

    // Dispatch all ticks whose exact sample falls within this buffer
    const double bpm           = bpm_.load(std::memory_order_relaxed);
    const double samplesPerBeat = sampleRate_ * 60.0 / bpm;
    const int64_t bufferEnd     = samplePosition_ + numFrames;

    while (static_cast<int64_t>(nextTickExact_) < bufferEnd) {
        int32_t tickFrame = static_cast<int32_t>(
            static_cast<int64_t>(nextTickExact_) - samplePosition_);
        tickFrame = std::clamp(tickFrame, 0, numFrames - 1);

        // Start click at tickFrame; render what fits in this buffer
        clickPhase_       = 0;
        int32_t toRender  = std::min(clickDuration_, numFrames - tickFrame);
        renderClick(output, tickFrame, 0, toRender);
        clickPhase_       = toRender;

        // Fire callback — audio thread; callers must not block
        if (onTick) {
            onTick(beatNumber_);
        }
        LOGD("Tick %d at sample %" PRId64, beatNumber_, static_cast<int64_t>(nextTickExact_));
        ++beatNumber_;

        nextTickExact_ += samplesPerBeat;
    }

    samplePosition_ += numFrames;
    return oboe::DataCallbackResult::Continue;
}

void TempoEngine::renderClick(
    float* buf,
    int32_t frameOffset,
    int32_t clickSample,
    int32_t count)
{
    for (int32_t i = 0; i < count; ++i) {
        int32_t s = clickSample + i;
        float t       = static_cast<float>(s) / static_cast<float>(clickDuration_);
        float envelope = std::exp(-CLICK_DECAY * t);
        float phase    = TWO_PI * CLICK_FREQ_HZ * s / static_cast<float>(sampleRate_);
        buf[frameOffset + i] += CLICK_GAIN * envelope * std::sin(phase);
    }
}

#endif // __ANDROID__
