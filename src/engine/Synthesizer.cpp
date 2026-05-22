#include "engine/Synthesizer.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "Control/adenv.h"
#include "Control/adsr.h"
#include "Control/phasor.h"
#include "Effects/chorus.h"
#include "Filters/onepole.h"
#include "Filters/svf.h"
#include "Noise/whitenoise.h"
#include "PhysicalModeling/stringvoice.h"
#include "Synthesis/oscillator.h"

namespace {

constexpr float kClickFreqHz = 1000.0f;
constexpr float kClickGain = 0.7f;
constexpr float kOffbeatLpCutoff = 0.12f;
constexpr float kPi2 = 6.283185307179586f;

float midiToHz(int midiNote) {
  return 440.0f * std::pow(2.0f, (static_cast<float>(midiNote) - 69.0f) / 12.0f);
}

float footageToMultiplier(OscFootage foot) {
  if (foot == OscFootage::Low) return 0.25f;
  const int f = static_cast<int>(foot);
  if (f <= 0) return 1.f;
  return 8.0f / static_cast<float>(f);
}

uint8_t toDaisyWave(OscWave wave) {
  switch (wave) {
    case OscWave::Tri:
      return daisysp::Oscillator::WAVE_TRI;
    case OscWave::Saw:
      return daisysp::Oscillator::WAVE_SAW;
    case OscWave::Square:
      return daisysp::Oscillator::WAVE_SQUARE;
    case OscWave::PolySquare:
      return daisysp::Oscillator::WAVE_POLYBLEP_SQUARE;
    case OscWave::Sine:
      return daisysp::Oscillator::WAVE_SIN;
    case OscWave::PolySaw:
    default:
      return daisysp::Oscillator::WAVE_POLYBLEP_SAW;
  }
}

void configureOsc(daisysp::Oscillator& osc, float sampleRate, const VcoSpec& spec) {
  osc.Init(sampleRate);
  osc.SetWaveform(toDaisyWave(spec.wave));
  osc.SetPw(spec.pulseWidth);
  osc.SetAmp(1.f);
}

float filterOutput(daisysp::Svf& svf, FilterMode mode) {
  switch (mode) {
    case FilterMode::Band:
      return svf.Band();
    case FilterMode::High:
      return svf.High();
    case FilterMode::Notch:
      return svf.Notch();
    case FilterMode::Low:
    default:
      return svf.Low();
  }
}

// Post-voice chorus + delay (shared bus; params updated from the playing patch).
struct MasterEffects {
  static constexpr size_t kMaxDelaySamples = 19200;

  daisysp::Chorus chorus{};
  EffectsSpec spec{};
  float sampleRateHz_ = 48000.f;
  std::array<float, kMaxDelaySamples> delayLine_{};
  size_t writeIdx_ = 0;

  void init(float sampleRate) {
    sampleRateHz_ = sampleRate;
    chorus.Init(sampleRate);
    delayLine_.fill(0.f);
    writeIdx_ = 0;
    applySpec(spec);
  }

  void applySpec(const EffectsSpec& s) {
    spec = s;
    chorus.SetLfoDepth(std::clamp(spec.chorusDepth, 0.f, 1.f));
    chorus.SetLfoFreq(std::max(spec.chorusRateHz, 0.05f));
    chorus.SetDelayMs(12.f);
    chorus.SetFeedback(0.15f);
  }

  float processDelay(float in) {
    const int delaySamples = std::clamp(
        static_cast<int>(spec.delayMs * sampleRateHz_ / 1000.f), 1,
        static_cast<int>(kMaxDelaySamples) - 1);
    const size_t readIdx =
        (writeIdx_ + kMaxDelaySamples - static_cast<size_t>(delaySamples)) % kMaxDelaySamples;
    const float tap = delayLine_[readIdx];
    delayLine_[writeIdx_] = in + tap * std::clamp(spec.delayFeedback, 0.f, 0.92f);
    writeIdx_ = (writeIdx_ + 1) % kMaxDelaySamples;
    return tap;
  }

  void processSample(float& sample) {
    const float wet = std::clamp(spec.wet, 0.f, 1.f);
    if (wet < 0.001f) return;

    const float dry = sample;
    const float chor = chorus.Process(dry);
    const float del = processDelay(dry);
    const float mix = std::clamp(spec.delayMix, 0.f, 1.f);
    const float wetSig = chor * (1.f - mix) + del * mix;
    sample = dry * (1.f - wet) + wetSig * wet;
  }
};

struct ClickVoice {
  daisysp::Oscillator osc;
  daisysp::AdEnv env;
  daisysp::OnePole toneLp;
  bool active = false;
  float gain = 1.f;
  bool softenOffbeat = false;

  void init(float sampleRate) {
    osc.Init(sampleRate);
    osc.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SQUARE);
    osc.SetFreq(kClickFreqHz);
    env.Init(sampleRate);
    env.SetTime(daisysp::ADENV_SEG_ATTACK, 0.001f);
    env.SetTime(daisysp::ADENV_SEG_DECAY,
                static_cast<float>(Synthesizer::kDefaultClickMs) / 1000.0f - 0.001f);
    toneLp.Init();
    toneLp.SetFilterMode(daisysp::OnePole::FILTER_MODE_LOW_PASS);
    toneLp.SetFrequency(kOffbeatLpCutoff);
  }

  void beginClick(bool soften, float pairGain) {
    softenOffbeat = soften;
    gain = kClickGain * pairGain;
    osc.Reset();
    env.Trigger();
    active = true;
  }

  float processSample() {
    float sample = osc.Process() * env.Process() * gain;
    if (softenOffbeat) sample = toneLp.Process(sample);
    if (!env.IsRunning()) active = false;
    return sample;
  }
};

struct PatchVoice {
  daisysp::Oscillator osc1;
  daisysp::Oscillator osc2;
  daisysp::WhiteNoise noise;
  daisysp::Svf svf;
  daisysp::Adsr ampEnv;
  daisysp::AdEnv filterEnv;
  daisysp::Phasor lfo;

  SynthSoundSpec spec{};
  EffectsSpec effects{};
  bool active = false;
  bool gate = false;
  int midiNote = 60;
  float ampGain = 1.f;
  float osc1Mult = 1.f;
  float osc2Mult = 1.f;
  float sampleRate = 48000.f;

  float targetHz1_ = 440.f;
  float targetHz2_ = 440.f;
  float currentHz1_ = 440.f;
  float currentHz2_ = 440.f;
  float glideAlpha_ = 1.f;

  void init(float sr) {
    sampleRate = sr;
    osc1.Init(sr);
    osc2.Init(sr);
    noise.Init();
    svf.Init(sr);
    ampEnv.Init(sr);
    filterEnv.Init(sr);
    lfo.Init(sr);
    active = false;
    gate = false;
  }

  void updateGlideCoeff() {
    if (spec.glideSec < 0.001f) {
      glideAlpha_ = 1.f;
    } else {
      glideAlpha_ = 1.f - std::exp(-4.f / (spec.glideSec * sampleRate));
    }
  }

  void computeTargetHz(int note) {
    const float base = midiToHz(note);
    targetHz1_ = spec.osc1.enabled ? base * osc1Mult : base;
    targetHz2_ = spec.osc2.enabled
                     ? base * osc2Mult * std::pow(2.0f, spec.osc2.tuneSemis / 12.0f)
                     : base;
  }

  void applySpec(const SynthSoundSpec& s) {
    spec = s;
    effects = s.effects;
    configureOsc(osc1, sampleRate, s.osc1);
    configureOsc(osc2, sampleRate, s.osc2);
    osc1Mult = footageToMultiplier(s.osc1.range);
    osc2Mult = footageToMultiplier(s.osc2.range);
    updateGlideCoeff();

    svf.Init(sampleRate);
    svf.SetFreq(std::clamp(s.filter.cutoffHz, 20.f, sampleRate * 0.45f));
    svf.SetRes(std::clamp(s.filter.resonance, 0.f, 1.f));
    svf.SetDrive(std::clamp(s.filter.drive, 0.f, 1.f));

    ampEnv.Init(sampleRate);
    ampEnv.SetAttackTime(s.ampEnv.attackSec);
    ampEnv.SetDecayTime(s.ampEnv.decaySec);
    ampEnv.SetSustainLevel(s.ampEnv.sustain);
    ampEnv.SetReleaseTime(s.ampEnv.releaseSec);

    filterEnv.Init(sampleRate);
    filterEnv.SetTime(daisysp::ADENV_SEG_ATTACK, s.filterEnv.attackSec);
    filterEnv.SetTime(daisysp::ADENV_SEG_DECAY, s.filterEnv.decaySec);
    filterEnv.SetMin(0.f);
    filterEnv.SetMax(1.f);

    lfo.Init(sampleRate);
    lfo.SetFreq(std::max(s.lfo.rateHz, 0.02f));

    ampGain = s.masterVolume;
  }

  float lfoBipolar() {
    return std::sin(lfo.Process() * kPi2);
  }

  void applyFrequencies() {
    const float lfoPitch = lfoBipolar() * spec.lfo.pitchDepthSemis;
    const float pitchMul = std::pow(2.0f, lfoPitch / 12.0f);
    const float pwOff = lfoBipolar() * spec.lfo.pwmDepth;

    if (spec.osc1.enabled) {
      osc1.SetFreq(currentHz1_ * pitchMul);
      if (spec.osc1.wave == OscWave::Square || spec.osc1.wave == OscWave::PolySquare) {
        osc1.SetPw(std::clamp(spec.osc1.pulseWidth + pwOff, 0.05f, 0.95f));
      }
    }
    if (spec.osc2.enabled) {
      osc2.SetFreq(currentHz2_ * pitchMul);
      if (spec.osc2.wave == OscWave::Square || spec.osc2.wave == OscWave::PolySquare) {
        osc2.SetPw(std::clamp(spec.osc2.pulseWidth + pwOff, 0.05f, 0.95f));
      }
    }
  }

  void setPitch(int note, bool immediate) {
    midiNote = note;
    computeTargetHz(note);
    if (immediate || glideAlpha_ >= 1.f) {
      currentHz1_ = targetHz1_;
      currentHz2_ = targetHz2_;
    }
    applyFrequencies();
  }

  void trigger(int note, float velocity) {
    const bool glideFromPrev = spec.glideSec >= 0.001f && active;
    setPitch(note, !glideFromPrev);
    gate = true;
    active = true;
    ampGain = spec.masterVolume * std::clamp(velocity, 0.f, 1.f);
    ampEnv.Retrigger(false);
    filterEnv.Trigger();
  }

  void release() {
    gate = false;
  }

  float processSample() {
    if (glideAlpha_ < 1.f) {
      currentHz1_ += (targetHz1_ - currentHz1_) * glideAlpha_;
      currentHz2_ += (targetHz2_ - currentHz2_) * glideAlpha_;
      applyFrequencies();
    } else {
      applyFrequencies();
    }

    const float o1 = spec.osc1.enabled ? osc1.Process() : 0.f;
    const float o2 = spec.osc2.enabled ? osc2.Process() : 0.f;

    float mix = 0.f;
    mix += spec.mixer.osc1 * o1;
    mix += spec.mixer.osc2 * o2;
    if (spec.mixer.noise > 0.f) mix += spec.mixer.noise * noise.Process();
    if (spec.mixer.ringMod > 0.f && spec.osc1.enabled && spec.osc2.enabled) {
      mix += spec.mixer.ringMod * (o1 * o2);
    }

    const float keyOffset = spec.filter.keyTrack * static_cast<float>(midiNote - 60) * 8.f;
    const float envMod = filterEnv.Process() * spec.filter.envDepth * 4000.f;
    const float lfoFilter = lfoBipolar() * spec.lfo.filterDepth * 2500.f;
    svf.SetFreq(
        std::clamp(spec.filter.cutoffHz + keyOffset + envMod + lfoFilter, 20.f, 22000.f));
    svf.Process(mix);

    const float amp = ampEnv.Process(gate) * ampGain;
    if (!gate && !ampEnv.IsRunning()) active = false;
    return filterOutput(svf, spec.filter.mode) * amp;
  }
};

struct PluckedVoice {
  daisysp::StringVoice string;
  daisysp::Adsr ampEnv;

  SynthSoundSpec spec{};
  EffectsSpec effects{};
  bool active = false;
  bool gate = false;
  float ampGain = 1.f;
  float strikeAccent_ = 0.85f;
  float sampleRate = 48000.f;
  int midiNote = 60;
  int quietSamples_ = 0;

  void init(float sr) {
    sampleRate = sr;
    string.Init(sr);
    ampEnv.Init(sr);
    active = false;
    gate = false;
    quietSamples_ = 0;
  }

  void applySpec(const SynthSoundSpec& s) {
    spec = s;
    effects = s.effects;

    string.Init(sampleRate);
    string.SetSustain(s.pluck.sustain);
    string.SetStructure(std::clamp(s.pluck.structure, 0.f, 1.f));
    string.SetBrightness(std::clamp(s.pluck.brightness, 0.f, 1.f));
    string.SetDamping(std::clamp(s.pluck.damping, 0.f, 1.f));
    strikeAccent_ = std::clamp(s.pluck.accent, 0.f, 1.f);

    ampEnv.Init(sampleRate);
    ampEnv.SetAttackTime(s.ampEnv.attackSec);
    ampEnv.SetDecayTime(s.ampEnv.decaySec);
    ampEnv.SetSustainLevel(s.ampEnv.sustain);
    ampEnv.SetReleaseTime(s.ampEnv.releaseSec);

    ampGain = s.masterVolume;
  }

  void trigger(int note, float velocity) {
    midiNote = note;
    const float vel = std::clamp(velocity, 0.f, 1.f);
    string.SetFreq(midiToHz(note));
    string.SetAccent(strikeAccent_ * (0.35f + 0.65f * vel));
    string.Reset();
    string.Trig();
    gate = true;
    active = true;
    quietSamples_ = 0;
    ampGain = spec.masterVolume * vel;
    ampEnv.Retrigger(false);
  }

  void release() {
    gate = false;
  }

  float processSample() {
    float sample = string.Process(false) * spec.pluck.level;
    const float amp = ampEnv.Process(gate) * ampGain;
    sample *= amp;

    if (std::abs(sample) < 1e-4f) {
      quietSamples_++;
    } else {
      quietSamples_ = 0;
    }
    if (!gate && !ampEnv.IsRunning() && quietSamples_ > 800) active = false;

    return sample;
  }
};

struct PoolVoice {
  VoiceEngine engine = VoiceEngine::Subtractive;
  PatchVoice subtractive;
  PluckedVoice plucked;

  void init(float sr) {
    subtractive.init(sr);
    plucked.init(sr);
  }

  void applySpec(const SynthSoundSpec& s) {
    engine = s.engine;
    if (engine == VoiceEngine::Plucked) {
      plucked.applySpec(s);
    } else {
      subtractive.applySpec(s);
    }
  }

  const EffectsSpec& effects() const {
    return engine == VoiceEngine::Plucked ? plucked.effects : subtractive.effects;
  }

  void trigger(int note, float velocity) {
    if (engine == VoiceEngine::Plucked) {
      plucked.trigger(note, velocity);
    } else {
      subtractive.trigger(note, velocity);
    }
  }

  void release() {
    if (engine == VoiceEngine::Plucked) {
      plucked.release();
    } else {
      subtractive.release();
    }
  }

  bool isActive() const {
    return engine == VoiceEngine::Plucked ? plucked.active : subtractive.active;
  }

  bool ampRunning() const {
    if (engine == VoiceEngine::Plucked) {
      return plucked.ampEnv.IsRunning();
    }
    return subtractive.ampEnv.IsRunning();
  }

  float processSample() {
    return engine == VoiceEngine::Plucked ? plucked.processSample() : subtractive.processSample();
  }
};

}  // namespace

struct Synthesizer::VoiceStorage {
  ClickVoice click;
  std::array<PoolVoice, Synthesizer::kMaxVoices> pool{};
  MasterEffects effects;
};

Synthesizer::Synthesizer() : voices_(std::make_unique<VoiceStorage>()) {
  setSampleRate(48000);
}

Synthesizer::~Synthesizer() = default;

void Synthesizer::setSampleRate(int32_t sampleRate) {
  sampleRate_ = std::max(sampleRate, 1);
  const float sr = static_cast<float>(sampleRate_);

  voices_->click.init(sr);
  voices_->effects.init(sr);
  for (auto& v : voices_->pool) {
    v.init(sr);
  }

  clickDurationSamples_ = static_cast<int32_t>(
      static_cast<float>(sampleRate_) * static_cast<float>(kDefaultClickMs) / 1000.0f);
  voiceMask_.store(0, std::memory_order_relaxed);
}

int32_t Synthesizer::sampleRate() const {
  return sampleRate_;
}

int32_t Synthesizer::clickDurationSamples() const {
  return clickDurationSamples_;
}

void Synthesizer::renderClick(float* buf, int32_t frameOffset, int32_t clickSample, int32_t count,
                              bool softenOffbeat, float pairGainMul) {
  if (clickSample == 0) {
    voices_->click.beginClick(softenOffbeat, pairGainMul);
  }
  for (int32_t i = 0; i < count; ++i) {
    buf[frameOffset + i] += voices_->click.processSample();
  }
}

int Synthesizer::allocateVoice() {
  const uint32_t mask = voiceMask_.load(std::memory_order_relaxed);
  for (int i = kFirstAssignableVoice; i < kMaxVoices; ++i) {
    const uint32_t bit = 1u << static_cast<uint32_t>(i);
    if ((mask & bit) != 0) continue;
    voiceMask_.store(mask | bit, std::memory_order_relaxed);
    return i;
  }
  return -1;
}

void Synthesizer::releaseVoice(int voiceIndex) {
  if (voiceIndex < kFirstAssignableVoice || voiceIndex >= kMaxVoices) return;
  voices_->pool[static_cast<size_t>(voiceIndex)].release();
  voices_->pool[static_cast<size_t>(voiceIndex)].plucked.active = false;
  voices_->pool[static_cast<size_t>(voiceIndex)].subtractive.active = false;
  const uint32_t bit = 1u << static_cast<uint32_t>(voiceIndex);
  voiceMask_.fetch_and(~bit, std::memory_order_relaxed);
}

void Synthesizer::applySoundSpec(int voiceIndex, const SynthSoundSpec& spec) {
  if (voiceIndex < kFirstAssignableVoice || voiceIndex >= kMaxVoices) return;
  auto& voice = voices_->pool[static_cast<size_t>(voiceIndex)];
  voice.applySpec(spec);
  voices_->effects.applySpec(spec.effects);
}

void Synthesizer::triggerNote(int voiceIndex, int midiNote, float velocity) {
  if (voiceIndex < kFirstAssignableVoice || voiceIndex >= kMaxVoices) return;
  auto& voice = voices_->pool[static_cast<size_t>(voiceIndex)];
  voice.trigger(midiNote, velocity);
  voices_->effects.applySpec(voice.effects());
}

void Synthesizer::releaseNote(int voiceIndex) {
  if (voiceIndex < kFirstAssignableVoice || voiceIndex >= kMaxVoices) return;
  voices_->pool[static_cast<size_t>(voiceIndex)].release();
}

void Synthesizer::renderVoices(float* buf, int32_t frameOffset, int32_t count) {
  uint32_t mask = voiceMask_.load(std::memory_order_relaxed);
  if (mask == 0) return;

  for (int32_t s = 0; s < count; ++s) {
    float dry = 0.f;
    for (int i = kFirstAssignableVoice; i < kMaxVoices; ++i) {
      const uint32_t bit = 1u << static_cast<uint32_t>(i);
      if ((mask & bit) == 0) continue;

      auto& voice = voices_->pool[static_cast<size_t>(i)];
      if (!voice.isActive() && !voice.ampRunning()) {
        voiceMask_.fetch_and(~bit, std::memory_order_relaxed);
        mask &= ~bit;
        continue;
      }

      dry += voice.processSample();
    }

    voices_->effects.processSample(dry);
    buf[frameOffset + s] += dry;
  }
}
