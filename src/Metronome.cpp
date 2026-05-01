#include "Metronome.h"

#include <algorithm>
#include <utility>

namespace {
using PaletteSound = Metronome::PaletteSound;
using TickSound = Metronome::TickSound;
using Kit = Metronome::Kit;

TickSound tickSoundForPalette(PaletteSound sound) {
  switch (sound) {
    case PaletteSound::MetKickGentle:
      return TickSound::Slot0;
    case PaletteSound::MetSnareGentle:
      return TickSound::Slot1;
    case PaletteSound::MetDigiGentle:
      return TickSound::Slot2;
    case PaletteSound::MetRimshotGentle:
      return TickSound::Slot3;
    case PaletteSound::SineClassic:
    default:
      return TickSound::Sine;
  }
}

int slotForPalette(PaletteSound sound) {
  switch (sound) {
    case PaletteSound::MetKickGentle:
      return 0;
    case PaletteSound::MetSnareGentle:
      return 1;
    case PaletteSound::MetDigiGentle:
      return 2;
    case PaletteSound::MetRimshotGentle:
      return 3;
    case PaletteSound::SineClassic:
    default:
      return -1;
  }
}
} // namespace

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

void Metronome::setPaletteSoundPcm(PaletteSound sound, std::vector<float> samples,
                                   int32_t sourceSampleRate) {
  const int slot = slotForPalette(sound);
  if (slot < 0) {
    return;
  }
  core_.setTickSoundPcm(slot, std::move(samples), sourceSampleRate);
}

void Metronome::setPaletteSound(PaletteSound sound) {
  core_.setTickSound(tickSoundForPalette(sound));
}

void Metronome::setKit(Kit kit) {
  activeKit_ = kit;
  const auto palette = getKitPalette(activeKit_);
  if (!palette.empty()) {
    setPaletteSound(palette.front());
  } else {
    setPaletteSound(PaletteSound::SineClassic);
  }
}

Metronome::Kit Metronome::getKit() const { return activeKit_; }

std::vector<Metronome::PaletteSound> Metronome::getKitPalette(Kit kit) {
  switch (kit) {
    case Kit::Metronome:
    default:
      return {
          PaletteSound::MetKickGentle,
          PaletteSound::MetSnareGentle,
          PaletteSound::MetDigiGentle,
          PaletteSound::MetRimshotGentle,
      };
  }
}

const char* Metronome::getPaletteResourceName(PaletteSound sound) {
  switch (sound) {
    case PaletteSound::MetKickGentle:
      return "metkick_gentle";
    case PaletteSound::MetSnareGentle:
      return "metsnare_gentle";
    case PaletteSound::MetDigiGentle:
      return "metdigi_gentle";
    case PaletteSound::MetRimshotGentle:
      return "metrimshot_gentle";
    case PaletteSound::SineClassic:
    default:
      return "";
  }
}

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
