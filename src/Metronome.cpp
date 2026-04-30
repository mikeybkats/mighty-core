#include "Metronome.h"

#include <algorithm>
#include <utility>

void Metronome::start() {
  core_.onTick = onTick;
  syncPolicyToCore();
  core_.start();
}

void Metronome::stop() { core_.stop(); }

bool Metronome::isPlaying() const { return core_.isPlaying(); }

void Metronome::setBPM(double bpm) { core_.setBPM(bpm); }

double Metronome::getBPM() const { return core_.getBPM(); }

void Metronome::setTickSoundPcm(int index, std::vector<float> samples,
                                int32_t sourceSampleRate) {
  core_.setTickSoundPcm(index, std::move(samples), sourceSampleRate);
}

void Metronome::setTickSound(TickSound sound) { core_.setTickSound(sound); }

void Metronome::setTwoBeatMeasure(bool enabled) {
  twoBeatMeasure_ = enabled;
  core_.setTwoBeatMeasureInternal(twoBeatMeasure_);
}

bool Metronome::getTwoBeatMeasure() const { return twoBeatMeasure_; }

void Metronome::setSwingFraction(double fraction) {
  swingFraction_ = std::clamp(fraction, 0.0, 0.5);
  core_.setSwingFractionInternal(swingFraction_);
}

double Metronome::getSwingFraction() const { return swingFraction_; }

void Metronome::syncPolicyToCore() {
  core_.setTwoBeatMeasureInternal(twoBeatMeasure_);
  core_.setSwingFractionInternal(swingFraction_);
}
