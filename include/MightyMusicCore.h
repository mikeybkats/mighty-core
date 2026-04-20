#pragma once

#include <functional>
#include <memory>

class MightyMusicCore {
public:
    MightyMusicCore();
    ~MightyMusicCore();

    void start();
    void stop();
    bool isPlaying() const;

    void setBPM(double bpm);
    double getBPM() const;

    // Fired from the audio thread — callers must not block.
    std::function<void(int beatNumber)> onTick;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
