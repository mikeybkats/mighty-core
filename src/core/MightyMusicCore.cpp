#include "MightyMusicCore.h"

#include "engine/Input.h"
#include "engine/Output.h"
#include "engine/Sound.h"

class MightyMusicCore::Impl {
 public:
  Input input;
  Output output;
  int synthVoiceIndex = -1;
  SoundId synthSoundId = Sound::kInvalidSound;
};

namespace {

SoundId soundIdForPatch(Sound& sound, int patchIndex) {
  switch (patchIndex) {
    case 0:
      return sound.guitarSound();
    case 1:
      return sound.bassSound();
    case 2:
      return sound.pianoSound();
    case 3:
      return sound.kickDrumSound();
    default:
      return Sound::kInvalidSound;
  }
}

}  // namespace

MightyMusicCore::MightyMusicCore() : impl_(std::make_unique<Impl>()) {
}
MightyMusicCore::~MightyMusicCore() {
  stopListening();
  stop();
  resetSynthVoiceHeld();
  impl_->output.closePlayback();
}

void MightyMusicCore::resetSynthVoiceHeld() {
  if (impl_->synthVoiceIndex >= 0) {
    impl_->output.sound().releaseVoice(impl_->synthVoiceIndex);
  }
  impl_->synthVoiceIndex = -1;
  impl_->synthSoundId = Sound::kInvalidSound;
}

void MightyMusicCore::ensurePlaybackOpen() {
  impl_->output.openPlayback();
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

bool MightyMusicCore::openPlayback() {
  return impl_->output.openPlayback();
}

void MightyMusicCore::closePlayback() {
  resetSynthVoiceHeld();
  impl_->output.closePlayback();
}

bool MightyMusicCore::isPlaybackOpen() const {
  return impl_->output.isPlaybackOpen();
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

bool MightyMusicCore::isListening() const {
  return impl_->input.isListening();
}

bool MightyMusicCore::hasDetectedInputSignal() const {
  return impl_->input.hasDetectedSignal();
}

int MightyMusicCore::lastDetectedMidiNote() const {
  return impl_->input.lastDetectedMidiNote();
}

const char* MightyMusicCore::synthPatchName(int patchIndex) const {
  static const char* kNames[kSynthPatchCount] = {"Guitar", "Bass", "Piano", "Kick drum"};
  if (patchIndex < 0 || patchIndex >= kSynthPatchCount) {
    return nullptr;
  }
  return kNames[patchIndex];
}

void MightyMusicCore::triggerSynthNote(int patchIndex, int midiNote, float velocity) {
  ensurePlaybackOpen();

  Sound& sound = impl_->output.sound();
  const SoundId id = soundIdForPatch(sound, patchIndex);
  if (!sound.isValidSound(id)) {
    return;
  }

  // Always re-allocate: opening playback calls setSampleRate(), which clears voiceMask_.
  resetSynthVoiceHeld();
  impl_->synthVoiceIndex = sound.allocateVoice(id);
  impl_->synthSoundId = id;
  if (impl_->synthVoiceIndex >= 0) {
    sound.triggerNote(impl_->synthVoiceIndex, midiNote, velocity);
  }
}

void MightyMusicCore::releaseSynthGate() {
  if (impl_->synthVoiceIndex < 0) {
    return;
  }
  impl_->output.sound().releaseNote(impl_->synthVoiceIndex);
}
