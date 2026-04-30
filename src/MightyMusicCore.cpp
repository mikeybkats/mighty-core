#include "MightyMusicCore.h"
#include "TempoEngine.h"

// PIMPL keeps the public header free of TempoEngine / Oboe / miniaudio includes.
class MightyMusicCore::Impl {
public:
    TempoEngine engine;
};

MightyMusicCore::MightyMusicCore()  : impl_(std::make_unique<Impl>()) {}
MightyMusicCore::~MightyMusicCore() { stop(); }

void MightyMusicCore::start() {
    // Snapshot the callback each start so callers can replace std::function between sessions.
    impl_->engine.onTick = onTick;
    impl_->engine.open();
}

void MightyMusicCore::stop()             { impl_->engine.close(); }
bool MightyMusicCore::isPlaying() const  { return impl_->engine.isRunning(); }
void MightyMusicCore::setBPM(double bpm) { impl_->engine.setBPM(bpm); }
double MightyMusicCore::getBPM() const   { return impl_->engine.getBPM(); }

void MightyMusicCore::setTwoBeatMeasureInternal(bool enabled) {
    impl_->engine.setTwoBeatMeasure(enabled);
}

void MightyMusicCore::setSwingFractionInternal(double fraction) {
    impl_->engine.setSwingFraction(fraction);
}

void MightyMusicCore::setTickSoundPcm(
    int index, std::vector<float> samples, int32_t sourceSampleRate)
{
    impl_->engine.setTickSoundPcm(index, std::move(samples), sourceSampleRate);
}

void MightyMusicCore::setTickSound(MightyMusicCore::TickSound sound) {
    impl_->engine.setTickSound(static_cast<int>(sound));
}
