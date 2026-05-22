#include "engine/Sound.h"

#include <algorithm>

#include "engine/Synthesizer.h"

namespace {

constexpr float kSampleClickGain = 0.85f;

std::vector<float> resampleLinear(const std::vector<float>& in, int32_t fromRate, int32_t toRate) {
  if (in.empty() || fromRate <= 0 || toRate <= 0 || fromRate == toRate) return in;

  const auto outCount =
      static_cast<size_t>((static_cast<int64_t>(in.size()) * toRate + fromRate / 2) / fromRate);
  if (outCount == 0) return {};

  std::vector<float> out(outCount);
  for (size_t i = 0; i < outCount; ++i) {
    const double srcPos =
        static_cast<double>(i) * static_cast<double>(fromRate) / static_cast<double>(toRate);
    const auto i0 = static_cast<size_t>(srcPos);
    const auto frac = srcPos - static_cast<double>(i0);
    const auto s0 = in[std::min(i0, in.size() - 1)];
    const auto s1 = in[std::min(i0 + 1, in.size() - 1)];
    out[i] = static_cast<float>(s0 + (s1 - s0) * frac);
  }
  return out;
}

}  // namespace

Sound::Sound(Synthesizer& synthesizer) : synthesizer_(synthesizer) {
  soundPresets_.reserve(kMaxSoundPresets);
  registerBuiltInSounds();
}

SoundId Sound::createSound(const SynthSoundSpec& spec) {
  if (soundPresets_.size() >= static_cast<size_t>(kMaxSoundPresets)) {
    return kInvalidSound;
  }
  soundPresets_.push_back(spec);
  return static_cast<SoundId>(soundPresets_.size() - 1);
}

bool Sound::isValidSound(SoundId id) const {
  return id >= 0 && static_cast<size_t>(id) < soundPresets_.size();
}

const SynthSoundSpec* Sound::getSoundSpec(SoundId id) const {
  if (!isValidSound(id)) return nullptr;
  return &soundPresets_[static_cast<size_t>(id)];
}

int Sound::allocateVoice(SoundId soundId) {
  if (!isValidSound(soundId)) return -1;

  const int voiceIndex = synthesizer_.allocateVoice();
  if (voiceIndex < 0) return -1;

  synthesizer_.applySoundSpec(voiceIndex, soundPresets_[static_cast<size_t>(soundId)]);
  return voiceIndex;
}

void Sound::releaseVoice(int voiceIndex) {
  synthesizer_.releaseVoice(voiceIndex);
}

void Sound::triggerNote(int voiceIndex, int midiNote, float velocity) {
  synthesizer_.triggerNote(voiceIndex, midiNote, velocity);
}

void Sound::releaseNote(int voiceIndex) {
  synthesizer_.releaseNote(voiceIndex);
}

void Sound::registerBuiltInSounds() {
  // Guitar: StringVoice curved bridge (structure < 0.26), pick attack, chorus.
  guitarSound_ = createSound({
      .engine = VoiceEngine::Plucked,
      .pluck =
          {
              .structure = 0.14f,
              .brightness = 0.58f,
              .damping = 0.5f,
              .accent = 0.92f,
              .level = 0.88f,
          },
      .ampEnv =
          {
              .attackSec = 0.001f,
              .decaySec = 0.45f,
              .sustain = 0.08f,
              .releaseSec = 0.22f,
          },
      .effects =
          {
              .wet = 0.1f,
              .chorusDepth = 0.25f,
              .chorusRateHz = 0.4f,
              .delayMs = 55.f,
              .delayFeedback = 0.18f,
              .delayMix = 0.35f,
          },
      .masterVolume = 0.54f,
  });

  // Bass: low bridge structure, dark brightness, heavy damping, dry FX.
  bassSound_ = createSound({
      .engine = VoiceEngine::Plucked,
      .pluck =
          {
              .structure = 0.06f,
              .brightness = 0.24f,
              .damping = 0.82f,
              .accent = 0.9f,
              .level = 0.92f,
          },
      .ampEnv =
          {
              .attackSec = 0.002f,
              .decaySec = 0.28f,
              .sustain = 0.55f,
              .releaseSec = 0.1f,
          },
      .effects =
          {
              .wet = 0.06f,
              .chorusDepth = 0.12f,
              .delayMs = 40.f,
              .delayFeedback = 0.12f,
              .delayMix = 0.1f,
          },
      .masterVolume = 0.6f,
  });

  // Piano: subtractive dual-triangle, soft hammer envelope — not plucked Karplus.
  pianoSound_ = createSound({
      .osc1 =
          {
              .range = OscFootage::Foot8,
              .wave = OscWave::Tri,
              .level = 0.52f,
          },
      .osc2 =
          {
              .range = OscFootage::Foot4,
              .wave = OscWave::Tri,
              .tuneSemis = 0.03f,
              .level = 0.38f,
          },
      .filter =
          {
              .cutoffHz = 3200.f,
              .resonance = 0.12f,
              .envDepth = 0.4f,
              .keyTrack = 0.5f,
              .drive = 0.35f,
          },
      .ampEnv =
          {
              .attackSec = 0.001f,
              .decaySec = 0.55f,
              .sustain = 0.35f,
              .releaseSec = 0.3f,
          },
      .filterEnv =
          {
              .attackSec = 0.001f,
              .decaySec = 0.4f,
              .sustain = 0.f,
              .releaseSec = 0.25f,
          },
      .mixer =
          {
              .osc1 = 0.52f,
              .osc2 = 0.38f,
          },
      .effects =
          {
              .wet = 0.08f,
              .chorusDepth = 0.1f,
              .delayMs = 50.f,
              .delayFeedback = 0.15f,
              .delayMix = 0.4f,
          },
      .masterVolume = 0.45f,
  });

  // Kick: low square-ish osc + noise burst, fast filter sweep (drum placeholder).
  kickDrumSound_ = createSound({
      .osc1 =
          {
              .range = OscFootage::Foot32,
              .wave = OscWave::PolySquare,
              .level = 0.85f,
          },
      .osc2 =
          {
              .enabled = false,
          },
      .filter =
          {
              .cutoffHz = 120.f,
              .resonance = 0.45f,
              .envDepth = 0.9f,
              .mode = FilterMode::Band,
              .drive = 0.65f,
          },
      .ampEnv =
          {
              .attackSec = 0.001f,
              .decaySec = 0.12f,
              .sustain = 0.f,
              .releaseSec = 0.05f,
          },
      .filterEnv =
          {
              .attackSec = 0.001f,
              .decaySec = 0.08f,
              .sustain = 0.f,
              .releaseSec = 0.04f,
          },
      .mixer =
          {
              .osc1 = 0.75f,
              .noise = 0.2f,
          },
      .masterVolume = 0.65f,
  });
}

void Sound::setPcmSlot(int index, std::vector<float> samples, int32_t sourceSampleRate) {
  if (index < 0 || index >= kSlotCount) return;
  sourcePcm_[static_cast<size_t>(index)] = std::move(samples);
  sourceRates_[static_cast<size_t>(index)] = sourceSampleRate;
  if (deviceRatePrepared_ != 0) prepareDeviceRate(deviceRatePrepared_);
}

void Sound::setSelection(int index) {
  if (index < 0) {
    selection_.store(kUseSynthesized, std::memory_order_relaxed);
    return;
  }
  selection_.store(std::clamp(index, 0, kSlotCount - 1), std::memory_order_relaxed);
}

int Sound::selection() const {
  return selection_.load(std::memory_order_relaxed);
}

void Sound::prepareDeviceRate(int32_t deviceSampleRate) {
  deviceRatePrepared_ = deviceSampleRate;
  synthesizer_.setSampleRate(deviceSampleRate);

  for (int i = 0; i < kSlotCount; ++i) {
    devicePcm_[static_cast<size_t>(i)].clear();
    const auto& src = sourcePcm_[static_cast<size_t>(i)];
    const int32_t srcRate = sourceRates_[static_cast<size_t>(i)];
    if (src.empty() || srcRate <= 0) continue;
    if (srcRate == deviceSampleRate) {
      devicePcm_[static_cast<size_t>(i)] = src;
    } else {
      devicePcm_[static_cast<size_t>(i)] = resampleLinear(src, srcRate, deviceSampleRate);
    }
  }
}

int32_t Sound::preparedDeviceRate() const {
  return deviceRatePrepared_;
}

void Sound::clearDeviceBuffers() {
  deviceRatePrepared_ = 0;
  for (auto& d : devicePcm_) d.clear();
}

int32_t Sound::synthesizedClickLength() const {
  return synthesizer_.clickDurationSamples();
}

int32_t Sound::clickLengthForSelection() const {
  const int slot = resolvePlaybackSlot();
  if (slot >= 0) {
    return static_cast<int32_t>(devicePcm_[static_cast<size_t>(slot)].size());
  }
  return synthesizedClickLength();
}

int Sound::resolvePlaybackSlot() const {
  const int idx = selection();
  if (idx >= 0 && idx < kSlotCount && !devicePcm_[static_cast<size_t>(idx)].empty()) {
    return idx;
  }
  return kUseSynthesized;
}

bool Sound::isIdle(const ClickPlaybackState& state) const {
  return state.phase >= state.totalLength;
}

void Sound::renderPartial(float* buf, int32_t frameOffset, int32_t numFrames,
                          ClickPlaybackState& state) {
  if (isIdle(state)) return;

  const int32_t remaining = state.totalLength - state.phase;
  const int32_t count = std::min(remaining, numFrames - frameOffset);
  renderSegment(buf, frameOffset, state.phase, count, state.playbackSlot, state.softenOffbeat,
                state.pairGainMul, state);
  state.phase += count;
}

void Sound::triggerAt(float* buf, int32_t tickFrame, int32_t numFrames, bool softenOffbeat,
                      float pairGainMul, ClickPlaybackState& state) {
  state.softenOffbeat = softenOffbeat;
  state.pairGainMul = pairGainMul;
  state.playbackSlot = resolvePlaybackSlot();
  state.totalLength = clickLengthForSelection();
  state.phase = 0;

  const int32_t count = std::min(state.totalLength, numFrames - tickFrame);
  renderSegment(buf, tickFrame, 0, count, state.playbackSlot, softenOffbeat, pairGainMul, state);
  state.phase = count;
}

void Sound::renderSegment(float* buf, int32_t frameOffset, int32_t clickSample, int32_t count,
                          int playbackSlot, bool softenOffbeat, float pairGainMul,
                          ClickPlaybackState& /*state*/) {
  if (playbackSlot >= 0) {
    const auto& data = devicePcm_[static_cast<size_t>(playbackSlot)];
    float offbeatLp = 0.f;
    const auto n = static_cast<int32_t>(data.size());
    for (int32_t i = 0; i < count; ++i) {
      const int32_t p = clickSample + i;
      if (p >= n) break;
      float sample = kSampleClickGain * data[static_cast<size_t>(p)];
      if (softenOffbeat) {
        offbeatLp += 0.65f * (sample - offbeatLp);
        sample = 0.91f * sample + 0.09f * offbeatLp;
      }
      buf[frameOffset + i] += pairGainMul * sample;
    }
    return;
  }

  synthesizer_.renderClick(buf, frameOffset, clickSample, count, softenOffbeat, pairGainMul);
}
