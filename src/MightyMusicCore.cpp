#include "MightyMusicCore.h"
#include "TempoEngine.h"

class MightyMusicCore::Impl {
public:
#ifdef __ANDROID__
    TempoEngine engine;
#endif
};

MightyMusicCore::MightyMusicCore()
    : impl_(std::make_unique<Impl>()) {}

MightyMusicCore::~MightyMusicCore() {
    stop();
}

void MightyMusicCore::start() {
#ifdef __ANDROID__
    impl_->engine.onTick = onTick;
    impl_->engine.open();
#endif
}

void MightyMusicCore::stop() {
#ifdef __ANDROID__
    impl_->engine.close();
#endif
}

bool MightyMusicCore::isPlaying() const {
#ifdef __ANDROID__
    return impl_->engine.isRunning();
#else
    return false;
#endif
}

void MightyMusicCore::setBPM(double bpm) {
#ifdef __ANDROID__
    impl_->engine.setBPM(bpm);
#endif
}

double MightyMusicCore::getBPM() const {
#ifdef __ANDROID__
    return impl_->engine.getBPM();
#else
    return 120.0;
#endif
}
