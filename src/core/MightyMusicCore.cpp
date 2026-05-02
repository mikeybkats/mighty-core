#include "MightyMusicCore.h"
#include "engine/Input.h"
#include "engine/Output.h"

class MightyMusicCore::Impl {
public:
  Input input;
  Output output;
};

MightyMusicCore::MightyMusicCore() : impl_(std::make_unique<Impl>()) {
}
MightyMusicCore::~MightyMusicCore() {
  stopListening();
  stop();
}

void MightyMusicCore::start() {
  impl_->output.start(onTick);
}

void MightyMusicCore::stop() {
  impl_->output.stop();
}

bool MightyMusicCore::isPlaying() const {
  return impl_->output.isPlaying();
}

void MightyMusicCore::setBPM(double bpm) {
  impl_->output.setBPM(bpm);
}

double MightyMusicCore::getBPM() const {
  return impl_->output.getBPM();
}

void MightyMusicCore::setTwoBeatMeasureInternal(bool enabled) {
  impl_->output.setTwoBeatMeasure(enabled);
}

void MightyMusicCore::setSwingFractionInternal(double fraction) {
  impl_->output.setSwingFraction(fraction);
}

void MightyMusicCore::setTickSoundPcm(int index, std::vector<float> samples,
                                      int32_t sourceSampleRate) {
  impl_->output.setTickSoundPcm(index, std::move(samples), sourceSampleRate);
}

void MightyMusicCore::setTickSound(int soundIndex) {
  impl_->output.setTickSound(soundIndex);
}

void MightyMusicCore::startListening() {
  impl_->input.startListening();
}

void MightyMusicCore::stopListening() {
  impl_->input.stopListening();
}
