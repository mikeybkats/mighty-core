#include "engine/Output.h"

#include <utility>

#include "engine/Sound.h"
#include "engine/Synthesizer.h"
#include "engine/TempoEngine.h"

class Output::Impl {
 public:
  TempoEngine engine;
};

Output::Output() : impl_(std::make_unique<Impl>()) {
}

Output::~Output() {
  stopMetronome();
  closePlayback();
}

bool Output::openPlayback() {
  return impl_->engine.open();
}

void Output::closePlayback() {
  impl_->engine.close();
}

bool Output::isPlaybackOpen() const {
  return impl_->engine.isRunning();
}

void Output::startMetronome(const std::function<void(int beatNumber)>& onTick) {
  impl_->engine.onTick = onTick;
  if (!impl_->engine.isRunning()) {
    openPlayback();
  }
  impl_->engine.startMetronome();
}

void Output::stopMetronome() {
  impl_->engine.stopMetronome();
}

bool Output::isMetronomeRunning() const {
  return impl_->engine.isMetronomeRunning();
}

void Output::start(const std::function<void(int beatNumber)>& onTick) {
  openPlayback();
  startMetronome(onTick);
}

void Output::stop() {
  stopMetronome();
}

bool Output::isPlaying() const {
  return isMetronomeRunning();
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

bool Output::queueCommand(const RealtimeCommand& command) {
  return impl_->engine.queueCommand(command);
}

Sound& Output::sound() {
  return impl_->engine.sound();
}

const Sound& Output::sound() const {
  return impl_->engine.sound();
}

Synthesizer& Output::synthesizer() {
  return impl_->engine.synthesizer();
}

const Synthesizer& Output::synthesizer() const {
  return impl_->engine.synthesizer();
}
