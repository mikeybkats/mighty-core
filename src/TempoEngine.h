#pragma once

#ifdef __ANDROID__
#include <oboe/Oboe.h>
#endif

#include <functional>
#include <memory>
#include "Scheduler.h"

class TempoEngine
#ifdef __ANDROID__
    : public oboe::AudioStreamDataCallback
#endif
{
public:
    TempoEngine();
    ~TempoEngine();

    std::function<void(int beatNumber)> onTick;

    bool open();
    void close();
    bool isRunning() const;

    void   setBPM(double bpm);
    double getBPM() const;

#ifdef __ANDROID__
    oboe::DataCallbackResult onAudioReady(
        oboe::AudioStream*, void*, int32_t) override;
#endif

    // Called by platform audio callbacks (Oboe / miniaudio). Not part of the public MMC API.
    void processAudio(float* output, int32_t numFrames);

private:
    void renderClick(float* buf, int32_t frameOffset, int32_t clickSample, int32_t count);

    Scheduler scheduler_;
    int32_t   clickPhase_{0};
    int32_t   clickDuration_{0};

#ifdef __ANDROID__
    std::shared_ptr<oboe::AudioStream> stream_;
#else
    struct DesktopImpl;
    std::unique_ptr<DesktopImpl> desktop_;
#endif
};
