#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

// Mighty Music Core (MMC) — platform-agnostic metronome engine.
// Android apps link this as a static library and bridge JNI to these calls.
// Internally: TempoEngine opens the OS audio device, Scheduler decides beat
// times in samples, and the audio callback mixes the tick (WAV or sine) + invokes onTick.
class MightyMusicCore {
public:
    MightyMusicCore();
    ~MightyMusicCore();

    // Opens the default output stream and begins audio callbacks (Oboe on Android,
    // miniaudio on desktop hosts). Copies the current onTick handler into the engine.
    void start();
    void stop();
    bool isPlaying() const;

    // BPM is stored atomically inside Scheduler; safe to call from UI / JNI while playing.
    void setBPM(double bpm);
    double getBPM() const;

    // Preload mono float PCM for one of four tick slots (typically 48 kHz from decoded WAV).
    // Safe to call before start(); if the engine is already open at a known device rate,
    // buffers are resampled to the device immediately. Index must be in [0, 3].
    void setTickSoundPcm(int index, std::vector<float> samples, int32_t sourceSampleRate);
    // Which preloaded slot to mix on each beat (read on the audio thread). Use -1 for sine click.
    void setTickSoundIndex(int index);

    // Fired from the audio thread when a beat boundary is crossed — do not block,
    // allocate, or take locks. Platform code often forwards this to another thread for UI.
    std::function<void(int beatNumber)> onTick;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
