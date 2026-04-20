#include "TempoEngine.h"

#include <algorithm>
#include <cmath>
#include <cstring>

static constexpr float TWO_PI        = 6.28318530718f;
static constexpr float CLICK_FREQ_HZ = 1000.0f;
static constexpr float CLICK_DECAY   = 8.0f;
static constexpr float CLICK_GAIN    = 0.7f;
static constexpr float CLICK_MS      = 8.0f;

// ---------------------------------------------------------------------------
// Shared audio processing (audio thread)
// ---------------------------------------------------------------------------

void TempoEngine::processAudio(float* output, int32_t numFrames) {
    std::memset(output, 0, numFrames * sizeof(float));

    // Continue click that started in a previous callback
    if (clickPhase_ < clickDuration_) {
        int32_t remaining = clickDuration_ - clickPhase_;
        int32_t toRender  = std::min(remaining, numFrames);
        renderClick(output, 0, clickPhase_, toRender);
        clickPhase_ += toRender;
    }

    // Advance scheduler and render each tick that falls in this buffer
    AdvanceResult ticks = scheduler_.advance(numFrames);
    for (int i = 0; i < ticks.count; ++i) {
        const int32_t tickFrame = ticks.ticks[i].frameOffset;
        const int     beat      = ticks.ticks[i].beatNumber;

        clickPhase_      = 0;
        int32_t toRender = std::min(clickDuration_, numFrames - tickFrame);
        renderClick(output, tickFrame, 0, toRender);
        clickPhase_ = toRender;

        if (onTick) onTick(beat); // audio thread — callers must not block
    }
}

void TempoEngine::renderClick(
    float* buf, int32_t frameOffset, int32_t clickSample, int32_t count)
{
    const int32_t sr = scheduler_.getSampleRate();
    for (int32_t i = 0; i < count; ++i) {
        int32_t s      = clickSample + i;
        float   t      = static_cast<float>(s) / static_cast<float>(clickDuration_);
        float   env    = std::exp(-CLICK_DECAY * t);
        float   phase  = TWO_PI * CLICK_FREQ_HZ * s / static_cast<float>(sr);
        buf[frameOffset + i] += CLICK_GAIN * env * std::sin(phase);
    }
}

void TempoEngine::setBPM(double bpm) { scheduler_.setBPM(bpm); }
double TempoEngine::getBPM() const   { return scheduler_.getBPM(); }

// ---------------------------------------------------------------------------
// Android / Oboe backend
// ---------------------------------------------------------------------------
#ifdef __ANDROID__

#include <android/log.h>
#include <cinttypes>
#define LOG_TAG "TempoEngine"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

TempoEngine::TempoEngine()  = default;
TempoEngine::~TempoEngine() { close(); }

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

    const int32_t sr = stream_->getSampleRate();
    scheduler_.setSampleRate(sr);
    scheduler_.reset();
    clickDuration_ = static_cast<int32_t>(sr * CLICK_MS / 1000.0f);
    clickPhase_    = clickDuration_; // silent until first tick

    result = stream_->start();
    if (result != oboe::Result::OK) {
        LOGD("Failed to start stream: %s", oboe::convertToText(result));
        return false;
    }
    LOGD("Stream opened: %d Hz", sr);
    return true;
}

void TempoEngine::close() {
    if (stream_) {
        stream_->stop();
        stream_->close();
        stream_.reset();
    }
}

bool TempoEngine::isRunning() const {
    return stream_ && stream_->getState() == oboe::StreamState::Started;
}

oboe::DataCallbackResult TempoEngine::onAudioReady(
    oboe::AudioStream*, void* audioData, int32_t numFrames)
{
    processAudio(static_cast<float*>(audioData), numFrames);
    return oboe::DataCallbackResult::Continue;
}

// ---------------------------------------------------------------------------
// Desktop / miniaudio backend
// ---------------------------------------------------------------------------
#else

#define MINIAUDIO_IMPLEMENTATION
#include "../third_party/miniaudio.h"

struct TempoEngine::DesktopImpl {
    ma_device device;
    bool      running = false;
};

static void maDataCallback(
    ma_device* device, void* output, const void* /*input*/, ma_uint32 frameCount)
{
    static_cast<TempoEngine*>(device->pUserData)
        ->processAudio(static_cast<float*>(output), static_cast<int32_t>(frameCount));
}

TempoEngine::TempoEngine()  : desktop_(std::make_unique<DesktopImpl>()) {}
TempoEngine::~TempoEngine() { close(); }

bool TempoEngine::open() {
    ma_device_config config  = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32;
    config.playback.channels = 1;
    config.sampleRate        = 48000;
    config.dataCallback      = maDataCallback;
    config.pUserData         = this;

    if (ma_device_init(nullptr, &config, &desktop_->device) != MA_SUCCESS)
        return false;

    const int32_t sr = static_cast<int32_t>(desktop_->device.sampleRate);
    scheduler_.setSampleRate(sr);
    scheduler_.reset();
    clickDuration_ = static_cast<int32_t>(sr * CLICK_MS / 1000.0f);
    clickPhase_    = clickDuration_;

    if (ma_device_start(&desktop_->device) != MA_SUCCESS) {
        ma_device_uninit(&desktop_->device);
        return false;
    }

    desktop_->running = true;
    return true;
}

void TempoEngine::close() {
    if (desktop_ && desktop_->running) {
        ma_device_uninit(&desktop_->device);
        desktop_->running = false;
    }
}

bool TempoEngine::isRunning() const {
    return desktop_ && desktop_->running;
}

#endif // __ANDROID__
