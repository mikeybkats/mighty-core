#include "Metronome.h"

#include <algorithm>
#include <utility>

#include "BuiltInKits.h"
#include "MightyMusicCore.h"

static_assert(kKitMaxSounds == MightyMusicCore::kMaxTickSoundSlots);

namespace {

const KitDefinition* definitionForKit(Metronome::KitId id) {
  switch (id) {
    case Metronome::KitId::MetronomeGentle:
    default:
      return &BuiltInKits::kMetronomeGentle;
  }
}

}  // namespace

void Metronome::start() {
  core_.onTick = onTick;
  syncPolicyToCore();
  core_.start();
}

void Metronome::stop() {
  core_.stop();
}

bool Metronome::isPlaying() const {
  return core_.isPlaying();
}

bool Metronome::openPlayback() {
  return core_.openPlayback();
}

void Metronome::closePlayback() {
  core_.closePlayback();
}

bool Metronome::isPlaybackOpen() const {
  return core_.isPlaybackOpen();
}

void Metronome::setBPM(double bpm) {
  core_.setBPM(bpm);
}

double Metronome::getBPM() const {
  return core_.getBPM();
}

void Metronome::loadKit(KitId id) {
  loadedKit_ = definitionForKit(id);
  loadedKitId_ = id;
  const int n = loadedKitSoundCount();
  core_.setTickSound(n > 0 ? 0 : MightyMusicCore::kTickSoundSine);
}

int Metronome::loadedKitSoundCount() const {
  if (!loadedKit_) {
    return 0;
  }
  return kitSoundCount(*loadedKit_);
}

const char* Metronome::loadedKitSoundResourceName(int index) const {
  if (!loadedKit_) {
    return nullptr;
  }
  return kitSoundResourceName(*loadedKit_, index);
}

const char* Metronome::loadedKitSoundDisplayName(int index) const {
  if (!loadedKit_) {
    return nullptr;
  }
  return kitSoundDisplayName(*loadedKit_, index);
}

void Metronome::setTickSoundPcm(int slotIndex, std::vector<float> samples,
                                int32_t sourceSampleRate) {
  core_.setTickSoundPcm(slotIndex, std::move(samples), sourceSampleRate);
}

void Metronome::setActiveTickSound(int slotIndex) {
  core_.setTickSound(slotIndex);
}

void Metronome::setTwoBeatMeasure(bool enabled) {
  twoBeatMeasure_ = enabled;
  core_.setTwoBeatMeasureInternal(twoBeatMeasure_);
}

bool Metronome::getTwoBeatMeasure() const {
  return twoBeatMeasure_;
}

void Metronome::setSwingFraction(double fraction) {
  swingFraction_ = std::clamp(fraction, 0.0, 0.5);
  core_.setSwingFractionInternal(swingFraction_);
}

double Metronome::getSwingFraction() const {
  return swingFraction_;
}

void Metronome::startListening() {
  core_.startListening();
}

void Metronome::stopListening() {
  core_.stopListening();
}

bool Metronome::isListening() const {
  return core_.isListening();
}

bool Metronome::hasDetectedInputSignal() const {
  return core_.hasDetectedInputSignal();
}

int Metronome::lastDetectedMidiNote() const {
  return core_.lastDetectedMidiNote();
}

int Metronome::synthPatchCount() const {
  return MightyMusicCore::kSynthPatchCount;
}

const char* Metronome::synthPatchName(int patchIndex) const {
  return core_.synthPatchName(patchIndex);
}

void Metronome::triggerSynthNote(int patchIndex, int midiNote, float velocity) {
  core_.triggerSynthNote(patchIndex, midiNote, velocity);
}

void Metronome::releaseSynthGate() {
  core_.releaseSynthGate();
}

void Metronome::syncPolicyToCore() {
  core_.setTwoBeatMeasureInternal(twoBeatMeasure_);
  core_.setSwingFractionInternal(swingFraction_);
}
