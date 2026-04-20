#include "MightyMusicCore.h"
#include "TempoEngine.h"

class MightyMusicCore::Impl {
public:
    TempoEngine engine;
};

MightyMusicCore::MightyMusicCore()  : impl_(std::make_unique<Impl>()) {}
MightyMusicCore::~MightyMusicCore() { stop(); }

void MightyMusicCore::start() {
    impl_->engine.onTick = onTick;
    impl_->engine.open();
}

void MightyMusicCore::stop()             { impl_->engine.close(); }
bool MightyMusicCore::isPlaying() const  { return impl_->engine.isRunning(); }
void MightyMusicCore::setBPM(double bpm) { impl_->engine.setBPM(bpm); }
double MightyMusicCore::getBPM() const   { return impl_->engine.getBPM(); }
