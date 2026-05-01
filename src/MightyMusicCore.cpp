#include "MightyMusicCore.h"
#include "Input.h"
#include "Output.h"

// PIMPL keeps the public header free of TempoEngine / Oboe / miniaudio
// includes.
class MightyMusicCore::Impl {
public:
  Input input;
  Output output;
};

MightyMusicCore::MightyMusicCore() : impl_(std::make_unique<Impl>()) {
}
// Ensures both output playback and input listening are closed when the core is
// destroyed.
MightyMusicCore::~MightyMusicCore() {
  stopListening();
  stop();
}

void MightyMusicCore::start() {
  impl_->output.start(onTick);
}

// Stops the metronome output stream.
void MightyMusicCore::stop() {
  impl_->output.stop();
}
// Returns whether the metronome output engine is currently running.
bool MightyMusicCore::isPlaying() const {
  return impl_->output.isPlaying();
}
// Updates metronome tempo in BPM (Beats Per Minute).
void MightyMusicCore::setBPM(double bpm) {
  impl_->output.setBPM(bpm);
}
// Reads current tempo in BPM.
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
  // Stores user-provided PCM (Pulse-Code Modulation) tick audio.
  impl_->output.setTickSoundPcm(index, std::move(samples), sourceSampleRate);
}

void MightyMusicCore::setTickSound(MightyMusicCore::TickSound sound) {
  // Chooses synthesized click or one of the preloaded PCM slots.
  impl_->output.setTickSound(static_cast<int>(sound));
}

// Starts microphone capture + monophonic note detection.
// This method resets per-session buffers so each listen period is independent.
void MightyMusicCore::startListening() {
  impl_->input.startListening();
}

// Stops capture, snapshots the note history, clears in-memory session state,
// and writes a CSV log that can be used for debugging and model evaluation.
void MightyMusicCore::stopListening() {
  impl_->input.stopListening();
}
