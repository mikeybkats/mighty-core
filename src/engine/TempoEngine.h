#pragma once

#ifdef __ANDROID__
#include <oboe/Oboe.h>
#endif

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>
#include "scheduler/Scheduler.h"

// Owns sample-accurate beat scheduling + click synthesis + platform I/O.
// __ANDROID__: Oboe output stream; this class is the DataCallback.
// Desktop: miniaudio playback device (see TempoEngine.cpp).
class TempoEngine
#ifdef __ANDROID__
    : public oboe::AudioStreamDataCallback
#endif
{
public:
    /// Must match `MightyMusicCore::kMaxTickSoundSlots` / `kKitMaxSounds`.
    static constexpr int kTickSlotCount = 12;

    TempoEngine();
#ifdef __ANDROID__
    ~TempoEngine() override;
#else
    ~TempoEngine();
#endif

    std::function<void(int beatNumber)> onTick;

    bool open();
    void close();
    bool isRunning() const;

    void   setBPM(double bpm);
    double getBPM() const;

    void setTwoBeatMeasure(bool enabled);
    void setSwingFraction(double fraction); // 0 .. 0.5

    void setTickSoundPcm(int index, std::vector<float> samples, int32_t sourceSampleRate);
    // -1 = synthesized sine click; 0..kTickSlotCount-1 = sample slot (empty slot falls back to sine).
    void setTickSound(int index);

#ifdef __ANDROID__
    // Oboe invokes this on a high-priority audio thread for each output buffer.
    oboe::DataCallbackResult onAudioReady(
        oboe::AudioStream*, void*, int32_t) override;
#endif

    // Shared path: zero buffer, extend any in-flight click, run scheduler, render new ticks.
    // Called by platform audio callbacks (Oboe / miniaudio); not part of the public MMC API.
    void processAudio(float* output, int32_t numFrames);

private:
    void prepareDeviceTickBuffers(int32_t deviceSampleRate);

    void renderClickSine(
        float* buf, int32_t frameOffset, int32_t clickSample, int32_t count, bool softenOffbeat,
        float pairGainMul);
    void renderClickSample(
        float* buf, int32_t frameOffset, int32_t clickSample, int32_t count,
        const std::vector<float>& data, bool softenOffbeat, float pairGainMul);

    Scheduler scheduler_;

    // Sub-sample position within the current click (0 .. activeClickTotalLength_).
    int32_t clickPhase_{0};
    // Length of the synthesized click at the current device sample rate.
    int32_t sineClickDuration_{0};
    // Length of the click currently being rendered (sine or sample); idle when clickPhase_ >= this.
    int32_t activeClickTotalLength_{0};
    // -1 = sine click; 0..kTickSlotCount-1 = sample slot for the current partial click.
    int activeTickSlotForClick_{-1};
    // Second beat of each pair in two-beat mode: mild low-pass on the tick timbre.
    bool activeClickOffbeatSoft_{false};
    // In two-beat mode, backbeats (odd beat index) are quieter so 1–2 grouping is obvious even with swing at 0.
    float activeClickPairGainMul_{1.f};
    // Separate one-pole states for click voicing and two-beat offbeat softening.
    float synthToneZ_{0.f};
    float offbeatToneZ_{0.f};

    struct TickSource {
        std::vector<float> pcm;
        int32_t            sampleRate = 0;
    };
    std::array<TickSource, kTickSlotCount>           tickSources_{};
    std::array<std::vector<float>, kTickSlotCount> tickDevice_{};
    int32_t tickDeviceRatePrepared_{0};
    std::atomic<int> tickSoundIndex_{-1};

#ifdef __ANDROID__
    std::shared_ptr<oboe::AudioStream> stream_;
#else
    struct DesktopImpl;
    std::unique_ptr<DesktopImpl> desktop_;
#endif
};
