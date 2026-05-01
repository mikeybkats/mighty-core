#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

// Output owns playback lifecycle, click/tick sound selection, and tempo controls.
class Output {
public:
    Output();
    ~Output();

    // Starts output playback and installs the current beat callback.
    void start(const std::function<void(int beatNumber)>& onTick);
    void stop();
    [[nodiscard]] bool isPlaying() const;

    void setBPM(double bpm);
    [[nodiscard]] double getBPM() const;

    void setTwoBeatMeasure(bool enabled);
    void setSwingFraction(double fraction);

    // Index must be in [0, 3].
    void setTickSoundPcm(int index, std::vector<float> samples, int32_t sourceSampleRate);
    // -1 = synthesized sine click, 0..3 = preloaded sample slot.
    void setTickSound(int soundIndex);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
