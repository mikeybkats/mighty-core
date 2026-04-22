#include "TempoEngine.h"

#include <algorithm>
#include <cmath>
#include <cstring>

// Click timbre (sine fallback when a tick slot has no PCM loaded).
static constexpr float TWO_PI        = 6.28318530718f;
static constexpr float CLICK_FREQ_HZ = 1000.0f;
static constexpr float CLICK_DECAY   = 8.0f;
static constexpr float CLICK_GAIN    = 0.7f;
static constexpr float CLICK_MS      = 8.0f;

static constexpr float SAMPLE_CLICK_GAIN = 0.85f;

namespace {

std::vector<float> resampleLinear(
    const std::vector<float>& in, int32_t fromRate, int32_t toRate)
{
    if (in.empty() || fromRate <= 0 || toRate <= 0 || fromRate == toRate)
        return in;

    const size_t outCount = static_cast<size_t>(
        (static_cast<int64_t>(in.size()) * toRate + fromRate / 2) / fromRate);
    if (outCount == 0)
        return {};

    std::vector<float> out(outCount);
    for (size_t i = 0; i < outCount; ++i) {
        const double srcPos = static_cast<double>(i) * static_cast<double>(fromRate)
            / static_cast<double>(toRate);
        const size_t   i0   = static_cast<size_t>(srcPos);
        const double   frac = srcPos - static_cast<double>(i0);
        const float    s0   = in[std::min(i0, in.size() - 1)];
        const float    s1   = in[std::min(i0 + 1, in.size() - 1)];
        out[i]              = static_cast<float>(s0 + (s1 - s0) * frac);
    }
    return out;
}

} // namespace

// ---------------------------------------------------------------------------
// Shared audio processing (audio thread)
// ---------------------------------------------------------------------------

void TempoEngine::prepareDeviceTickBuffers(int32_t deviceSampleRate) {
    tickDeviceRatePrepared_ = deviceSampleRate;
    for (int i = 0; i < kTickSlotCount; ++i) {
        tickDevice_[i].clear();
        if (tickSources_[i].pcm.empty() || tickSources_[i].sampleRate <= 0)
            continue;
        if (tickSources_[i].sampleRate == deviceSampleRate) {
            tickDevice_[i] = tickSources_[i].pcm;
        } else {
            tickDevice_[i] = resampleLinear(
                tickSources_[i].pcm, tickSources_[i].sampleRate, deviceSampleRate);
        }
    }
}

void TempoEngine::setTickSoundPcm(
    int index, std::vector<float> samples, int32_t sourceSampleRate)
{
    if (index < 0 || index >= kTickSlotCount)
        return;
    tickSources_[static_cast<size_t>(index)].pcm         = std::move(samples);
    tickSources_[static_cast<size_t>(index)].sampleRate = sourceSampleRate;
    if (tickDeviceRatePrepared_ != 0)
        prepareDeviceTickBuffers(tickDeviceRatePrepared_);
}

void TempoEngine::setTickSoundIndex(int index) {
    const int clamped = std::clamp(index, 0, kTickSlotCount - 1);
    tickSoundIndex_.store(clamped, std::memory_order_relaxed);
}

void TempoEngine::processAudio(float* output, int32_t numFrames) {
    std::memset(output, 0, numFrames * sizeof(float));

    // Continue click that started in a previous callback
    if (clickPhase_ < activeClickTotalLength_) {
        int32_t remaining = activeClickTotalLength_ - clickPhase_;
        int32_t toRender  = std::min(remaining, numFrames);
        if (activeTickSlotForClick_ >= 0) {
            renderClickSample(
                output, 0, clickPhase_, toRender,
                tickDevice_[static_cast<size_t>(activeTickSlotForClick_)]);
        } else {
            renderClickSine(output, 0, clickPhase_, toRender);
        }
        clickPhase_ += toRender;
    }

    // Advance scheduler and render each tick that falls in this buffer
    AdvanceResult ticks = scheduler_.advance(numFrames);
    for (int i = 0; i < ticks.count; ++i) {
        const int32_t tickFrame = ticks.ticks[i].frameOffset;
        const int     beat      = ticks.ticks[i].beatNumber;

        const int idx = tickSoundIndex_.load(std::memory_order_relaxed);
        if (idx >= 0 && idx < kTickSlotCount && !tickDevice_[static_cast<size_t>(idx)].empty()) {
            activeTickSlotForClick_ = idx;
            activeClickTotalLength_ =
                static_cast<int32_t>(tickDevice_[static_cast<size_t>(idx)].size());
        } else {
            activeTickSlotForClick_ = -1;
            activeClickTotalLength_ = sineClickDuration_;
        }

        clickPhase_      = 0;
        int32_t toRender = std::min(activeClickTotalLength_, numFrames - tickFrame);
        if (activeTickSlotForClick_ >= 0) {
            renderClickSample(
                output, tickFrame, 0, toRender,
                tickDevice_[static_cast<size_t>(activeTickSlotForClick_)]);
        } else {
            renderClickSine(output, tickFrame, 0, toRender);
        }
        clickPhase_ = toRender;

        if (onTick) onTick(beat); // audio thread — callers must not block
    }
}

void TempoEngine::renderClickSine(
    float* buf, int32_t frameOffset, int32_t clickSample, int32_t count)
{
    const int32_t sr = scheduler_.getSampleRate();
    for (int32_t i = 0; i < count; ++i) {
        int32_t s      = clickSample + i;
        float   t      = static_cast<float>(s) / static_cast<float>(sineClickDuration_);
        float   env    = std::exp(-CLICK_DECAY * t);
        float   phase  = TWO_PI * CLICK_FREQ_HZ * s / static_cast<float>(sr);
        buf[frameOffset + i] += CLICK_GAIN * env * std::sin(phase);
    }
}

void TempoEngine::renderClickSample(
    float* buf, int32_t frameOffset, int32_t clickSample, int32_t count,
    const std::vector<float>& data)
{
    const int32_t n = static_cast<int32_t>(data.size());
    for (int32_t i = 0; i < count; ++i) {
        const int32_t p = clickSample + i;
        if (p >= n)
            break;
        buf[frameOffset + i] += SAMPLE_CLICK_GAIN * data[static_cast<size_t>(p)];
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
    // Low-latency mono float output; Oboe picks AAudio or OpenSL ES as appropriate.
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
    sineClickDuration_ = static_cast<int32_t>(sr * CLICK_MS / 1000.0f);
    prepareDeviceTickBuffers(sr);
    activeClickTotalLength_ = sineClickDuration_;
    // clickPhase_ == activeClickTotalLength_ means idle until first scheduled tick.
    clickPhase_             = activeClickTotalLength_;

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
    tickDeviceRatePrepared_ = 0;
    for (auto& d : tickDevice_)
        d.clear();
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
// Used by macOS/Linux dev hosts (mighty-core-host-check, mighty-core-ui).
// Same processAudio() path as Android; only the device open/close differs.
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
    // Fixed 48 kHz for the debug hosts; Android uses the device’s native rate from Oboe.
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
    sineClickDuration_ = static_cast<int32_t>(sr * CLICK_MS / 1000.0f);
    prepareDeviceTickBuffers(sr);
    activeClickTotalLength_ = sineClickDuration_;
    clickPhase_             = activeClickTotalLength_;

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
    tickDeviceRatePrepared_ = 0;
    for (auto& d : tickDevice_)
        d.clear();
}

bool TempoEngine::isRunning() const {
    return desktop_ && desktop_->running;
}

#endif // __ANDROID__
