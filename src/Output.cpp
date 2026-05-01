#include "Output.h"

#include "TempoEngine.h"

#include <utility>

class Output::Impl {
public:
    TempoEngine engine;
};

Output::Output() : impl_(std::make_unique<Impl>()) {}
Output::~Output() { stop(); }

void Output::start(const std::function<void(int beatNumber)>& onTick) {
    // Snapshot callback at start so callers can change it between sessions.
    impl_->engine.onTick = onTick;
    impl_->engine.open();
}

void Output::stop() {
    impl_->engine.close();
}

bool Output::isPlaying() const {
    return impl_->engine.isRunning();
}

void Output::setBPM(double bpm) {
    impl_->engine.setBPM(bpm);
}

double Output::getBPM() const {
    return impl_->engine.getBPM();
}

void Output::setTwoBeatMeasure(bool enabled) {
    impl_->engine.setTwoBeatMeasure(enabled);
}

void Output::setSwingFraction(double fraction) {
    impl_->engine.setSwingFraction(fraction);
}

void Output::setTickSoundPcm(int index, std::vector<float> samples, int32_t sourceSampleRate) {
    impl_->engine.setTickSoundPcm(index, std::move(samples), sourceSampleRate);
}

void Output::setTickSound(int soundIndex) {
    impl_->engine.setTickSound(soundIndex);
}
