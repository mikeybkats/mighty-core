#pragma once

#ifdef __ANDROID__

#include <oboe/Oboe.h>
#include <atomic>
#include <functional>
#include <cstdint>

class TempoEngine : public oboe::AudioStreamDataCallback {
public:
    std::function<void(int beatNumber)> onTick;

    bool open();
    void close();
    bool isRunning() const;

    void setBPM(double bpm);
    double getBPM() const;

    // oboe::AudioStreamDataCallback
    oboe::DataCallbackResult onAudioReady(
        oboe::AudioStream* stream,
        void* audioData,
        int32_t numFrames) override;

private:
    void renderClick(float* buf, int32_t frameOffset, int32_t clickSample, int32_t count);

    std::atomic<double> bpm_{120.0};
    int32_t sampleRate_{48000};

    // Beat scheduling: all in samples, audio-clock based
    int64_t samplePosition_{0};   // total frames output since start
    double nextTickExact_{0.0};   // exact fractional sample of next tick
    int beatNumber_{0};

    // Click playback state
    int32_t clickPhase_{0};          // samples into current click (>= clickDuration_ = silent)
    int32_t clickDuration_{0};       // samples, set on open()

    std::shared_ptr<oboe::AudioStream> stream_;
};

#endif // __ANDROID__
