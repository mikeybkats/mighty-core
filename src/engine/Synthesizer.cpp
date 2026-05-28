#include "engine/Synthesizer.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "Control/adenv.h"
#include "Control/adsr.h"
#include "Control/phasor.h"
#include "Effects/chorus.h"
#include "Filters/ladder.h"
#include "Filters/onepole.h"
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

OscFootage footageFromFeet(float feet) {
  if (feet >= 28.f) return OscFootage::Foot32;
  if (feet >= 12.f) return OscFootage::Foot16;
  if (feet >= 6.f) return OscFootage::Foot8;
  if (feet >= 3.f) return OscFootage::Foot4;
  if (feet >= 1.5f) return OscFootage::Foot2;
  return OscFootage::Low;
}

OscWave waveFromSelector(float selector) {
  const int idx = std::clamp(static_cast<int>(selector + 0.5f), 0, 2);
  switch (idx) {
    case 0:
      return OscWave::Square;
    case 1:
      return OscWave::Tri;
    case 2:
    default:
      return OscWave::Saw;
  }
}

float footageToMultiplier(OscFootage foot) {
  if (foot == OscFootage::Low) return 0.25f;
  const int f = static_cast<int>(foot);
  if (f <= 0) return 1.f;
  return static_cast<float>(f) / 8.0f;
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

constexpr float kLadderMaxRes = 1.8f;
constexpr float kLadderMaxInputDrive = 4.f;
/// Fixed ladder input drive (UI drive removed; always run filter hot like Boomstar mixer into VCF).
constexpr float kFixedLadderDrive = 1.f;
/// Filter envelope depth at 1.0 ≈ this many octaves (Minimoog-style exponential sweep).
constexpr float kFilterEnvOctavesMax = 3.5f;

float smoothstep(float edge0, float edge1, float x) {
  const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.f, 1.f);
  return t * t * (3.f - 2.f * t);
}

float resonanceEmphasis(float res) {
  return smoothstep(0.55f, kLadderMaxRes, res);
}

float passbandGainForResonance(float res) {
  const float t = resonanceEmphasis(res);
  return 0.45f + (0.02f - 0.45f) * t;
}

float inputDriveForResonance(float res) {
  return kFixedLadderDrive * kLadderMaxInputDrive
         * (1.f - 0.8f * resonanceEmphasis(res));
}

float filterExciteGain(float res) {
  return 1.f - 0.3f * smoothstep(1.0f, kLadderMaxRes, res);
}

float lfoBipolar(daisysp::Phasor& phasor) {
  return std::sin(phasor.Process() * kPi2);
}

void applyLadderVoicing(daisysp::LadderFilter& ladder, float res) {
  ladder.SetRes(std::clamp(res, 0.f, kLadderMaxRes));
  ladder.SetPassbandGain(passbandGainForResonance(res));
  ladder.SetInputDrive(inputDriveForResonance(res));
}

float computeFilterCutoffHz(float sampleRate,
                            float cutoffHz,
                            float keyTrack,
                            int midiNote,
                            float filterEnvVal,
                            float envDepth,
                            float primaryOscHz,
                            float res) {
  const float maxCutoff = sampleRate * 0.49f;
  const float semisFromKey = keyTrack * static_cast<float>(midiNote - 60);
  float hz = cutoffHz * std::pow(2.f, semisFromKey / 12.f);
  hz *= std::pow(2.f, filterEnvVal * std::clamp(envDepth, 0.f, 1.f) * kFilterEnvOctavesMax);

  const float emphasize = resonanceEmphasis(res);
  if (emphasize > 0.f && primaryOscHz > 20.f) {
    const float pull =
        emphasize * smoothstep(1.0f, kLadderMaxRes, res) * 0.88f;
    hz = std::exp(std::log(hz) * (1.f - pull) + std::log(primaryOscHz) * pull);
  }

  return std::clamp(hz, 5.f, maxCutoff);
}

daisysp::LadderFilter::FilterMode toLadderMode(FilterMode mode) {
  switch (mode) {
    case FilterMode::Band:
      return daisysp::LadderFilter::FilterMode::BP24;
    case FilterMode::High:
      return daisysp::LadderFilter::FilterMode::HP24;
    case FilterMode::Notch:
      return daisysp::LadderFilter::FilterMode::BP12;
    case FilterMode::Low:
    default:
      return daisysp::LadderFilter::FilterMode::LP24;
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
  daisysp::Oscillator subOsc;
  daisysp::WhiteNoise noise;
  daisysp::LadderFilter ladder;
  daisysp::Adsr ampEnv;
  daisysp::Adsr filterEnv;
  daisysp::Phasor lfo1;
  daisysp::Phasor lfo2;

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
    subOsc.Init(sr);
    subOsc.SetWaveform(daisysp::Oscillator::WAVE_SQUARE);
    subOsc.SetAmp(1.f);
    noise.Init();
    ladder.Init(sr);
    ampEnv.Init(sr);
    filterEnv.Init(sr);
    lfo1.Init(sr);
    lfo2.Init(sr);
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
    subOsc.Init(sampleRate);
    subOsc.SetWaveform(daisysp::Oscillator::WAVE_SQUARE);
    subOsc.SetAmp(1.f);
    osc1Mult = footageToMultiplier(s.osc1.range);
    osc2Mult = footageToMultiplier(s.osc2.range);
    updateGlideCoeff();

    ladder.Init(sampleRate);
    syncLadderFromSpec();

    ampEnv.Init(sampleRate);
    ampEnv.SetAttackTime(s.ampEnv.attackSec);
    ampEnv.SetDecayTime(s.ampEnv.decaySec);
    ampEnv.SetSustainLevel(s.ampEnv.sustain);
    ampEnv.SetReleaseTime(s.ampEnv.releaseSec);

    filterEnv.Init(sampleRate);
    filterEnv.SetAttackTime(s.filterEnv.attackSec);
    filterEnv.SetDecayTime(s.filterEnv.decaySec);
    filterEnv.SetSustainLevel(s.filterEnv.sustain);
    filterEnv.SetReleaseTime(s.filterEnv.releaseSec);

    lfo1.Init(sampleRate);
    lfo1.SetFreq(std::max(s.osc1Lfo.rateHz, 0.02f));
    lfo2.Init(sampleRate);
    lfo2.SetFreq(std::max(s.osc2Lfo.rateHz, 0.02f));

    ampGain = s.masterVolume;
  }

  void syncAmpEnvFromSpec() {
    ampEnv.SetAttackTime(spec.ampEnv.attackSec);
    ampEnv.SetDecayTime(spec.ampEnv.decaySec);
    ampEnv.SetSustainLevel(spec.ampEnv.sustain);
    ampEnv.SetReleaseTime(spec.ampEnv.releaseSec);
  }

  void syncFilterEnvFromSpec() {
    filterEnv.SetAttackTime(spec.filterEnv.attackSec);
    filterEnv.SetDecayTime(spec.filterEnv.decaySec);
    filterEnv.SetSustainLevel(spec.filterEnv.sustain);
    filterEnv.SetReleaseTime(spec.filterEnv.releaseSec);
  }

  void syncLadderFromSpec() {
    ladder.SetFilterMode(toLadderMode(spec.filter.mode));
    applyLadderVoicing(ladder, spec.filter.resonance);
  }

  float primaryOscHz() const {
    if (spec.osc1.enabled) return currentHz1_;
    if (spec.osc2.enabled) return currentHz2_;
    return midiToHz(midiNote);
  }

  static constexpr float kLfoPitchSemisMax = 12.f;
  static constexpr float kLfoPwmSwingMax = 0.45f;

  static float pitchMulFromLfo(float lfoValue, float depth) {
    const float semis = lfoValue * depth * kLfoPitchSemisMax;
    return std::pow(2.0f, semis / 12.0f);
  }

  static float pwmFromLfo(float basePw, float lfoValue, float depth) {
    return std::clamp(basePw + lfoValue * depth * kLfoPwmSwingMax, 0.05f, 0.95f);
  }

  void applyFrequencies(float lfo1v, float lfo2v) {

    float pitchMul1 = 1.f;
    float pitchMul2 = 1.f;
    float pw1 = spec.osc1.pulseWidth;
    float pw2 = spec.osc2.pulseWidth;

    if (spec.osc1Lfo.depth > 0.f) {
      if (spec.osc1Lfo.target == LfoTarget::Pitch) {
        pitchMul1 = pitchMulFromLfo(lfo1v, spec.osc1Lfo.depth);
      } else if (spec.osc1.wave == OscWave::Square || spec.osc1.wave == OscWave::PolySquare) {
        pw1 = pwmFromLfo(spec.osc1.pulseWidth, lfo1v, spec.osc1Lfo.depth);
      }
    }
    if (spec.osc2Lfo.depth > 0.f) {
      if (spec.osc2Lfo.target == LfoTarget::Pitch) {
        pitchMul2 = pitchMulFromLfo(lfo2v, spec.osc2Lfo.depth);
      } else if (spec.osc2.wave == OscWave::Square || spec.osc2.wave == OscWave::PolySquare) {
        pw2 = pwmFromLfo(spec.osc2.pulseWidth, lfo2v, spec.osc2Lfo.depth);
      }
    }

    if (spec.osc1.enabled) {
      osc1.SetFreq(currentHz1_ * pitchMul1);
      if (spec.osc1.wave == OscWave::Square || spec.osc1.wave == OscWave::PolySquare) {
        osc1.SetPw(pw1);
      }
      if (spec.osc1.subOsc) {
        subOsc.SetFreq((currentHz1_ * pitchMul1) * 0.5f);
      }
    }
    if (spec.osc2.enabled) {
      if (spec.osc1.sync) {
        osc2.SetFreq(currentHz1_ * pitchMul1);
      } else {
        osc2.SetFreq(currentHz2_ * pitchMul2);
      }
      if (spec.osc2.wave == OscWave::Square || spec.osc2.wave == OscWave::PolySquare) {
        osc2.SetPw(pw2);
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
    applyFrequencies(0.f, 0.f);
  }

  void trigger(int note, float velocity) {
    const bool glideFromPrev = spec.glideSec >= 0.001f && active;
    setPitch(note, !glideFromPrev);
    gate = true;
    active = true;
    ampGain = spec.masterVolume * std::clamp(velocity, 0.f, 1.f);
    ampEnv.Retrigger(false);
    filterEnv.Retrigger(false);
  }

  void release() {
    gate = false;
  }

  void setPitchNoRetrigger(int note) {
    setPitch(note, false);
  }

  void refreshPitchTargets() {
    computeTargetHz(midiNote);
    currentHz1_ = targetHz1_;
    currentHz2_ = targetHz2_;
    applyFrequencies(0.f, 0.f);
  }

  float processSample() {
    const float lfo1v = lfoBipolar(lfo1);
    const float lfo2v = lfoBipolar(lfo2);

    bool effectiveGate = gate;
    if (spec.osc1Lfo.gateTrigger) {
      effectiveGate = gate && (lfo1v > 0.f);
    }

    const float filterEnvVal = filterEnv.Process(effectiveGate);
    const float ampEnvVal = ampEnv.Process(effectiveGate);

    if (glideAlpha_ < 1.f) {
      currentHz1_ += (targetHz1_ - currentHz1_) * glideAlpha_;
      currentHz2_ += (targetHz2_ - currentHz2_) * glideAlpha_;
      applyFrequencies(lfo1v, lfo2v);
    } else {
      applyFrequencies(lfo1v, lfo2v);
    }

    if (spec.osc2EnvMod && spec.osc2.enabled) {
      const float envPitchSemis = ampEnvVal * spec.osc2EnvAmountSemis;
      const float envMul = std::pow(2.0f, envPitchSemis / 12.0f);
      const float baseHz = spec.osc1.sync ? currentHz1_ : currentHz2_;
      osc2.SetFreq(baseHz * envMul);
    }

    const float o1 = spec.osc1.enabled ? osc1.Process() : 0.f;
    const float o2 = spec.osc2.enabled ? osc2.Process() : 0.f;
    const float oSub = (spec.osc1.enabled && spec.osc1.subOsc) ? subOsc.Process() : 0.f;

    const float o1Lev = spec.mixer.osc1 * o1;
    const float o2Lev = spec.mixer.osc2 * o2;

    float mix = o1Lev + o2Lev;
    if (spec.osc1.subOsc) mix += 0.35f * spec.mixer.osc1 * oSub;
    if (spec.mixer.noise > 0.f) mix += spec.mixer.noise * noise.Process();
    if (spec.mixer.ringMod > 0.f && spec.osc1.enabled && spec.osc2.enabled) {
      mix += spec.mixer.ringMod * o1Lev * o2Lev;
    }

    applyLadderVoicing(ladder, spec.filter.resonance);
    ladder.SetFreq(computeFilterCutoffHz(sampleRate,
                                         spec.filter.cutoffHz,
                                         spec.filter.keyTrack,
                                         midiNote,
                                         filterEnvVal,
                                         spec.filter.envDepth,
                                         primaryOscHz(),
                                         spec.filter.resonance));
    const float filtered =
        ladder.Process(mix * filterExciteGain(spec.filter.resonance));

    const float amp = ampEnvVal * ampGain;
    if (!gate && !ampEnv.IsRunning() && !filterEnv.IsRunning()) {
      active = false;
    }
    return filtered * amp;
  }
};

struct PluckedVoice {
  daisysp::StringVoice string;
  daisysp::Adsr ampEnv;
  daisysp::Phasor lfo1;

  SynthSoundSpec spec{};
  EffectsSpec effects{};
  bool active = false;
  bool gate = false;
  bool lfo1WasPositive_ = false;
  bool pendingStrike_ = false;
  float ampGain = 1.f;
  float strikeAccent_ = 0.85f;
  float sampleRate = 48000.f;
  int midiNote = 60;
  int quietSamples_ = 0;

  void init(float sr) {
    sampleRate = sr;
    string.Init(sr);
    ampEnv.Init(sr);
    lfo1.Init(sr);
    active = false;
    gate = false;
    lfo1WasPositive_ = false;
    pendingStrike_ = false;
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

    lfo1.Init(sampleRate);
    lfo1.SetFreq(std::max(s.osc1Lfo.rateHz, 0.02f));

    ampGain = s.masterVolume;
  }

  void syncAmpEnvFromSpec() {
    ampEnv.SetAttackTime(spec.ampEnv.attackSec);
    ampEnv.SetDecayTime(spec.ampEnv.decaySec);
    ampEnv.SetSustainLevel(spec.ampEnv.sustain);
    ampEnv.SetReleaseTime(spec.ampEnv.releaseSec);
  }

  void trigger(int note, float velocity) {
    midiNote = note;
    const float vel = std::clamp(velocity, 0.f, 1.f);
    string.SetFreq(midiToHz(note));
    string.SetAccent(strikeAccent_ * (0.35f + 0.65f * vel));
    string.Reset();
    lfo1WasPositive_ = false;
    if (!spec.osc1Lfo.gateTrigger) {
      pendingStrike_ = true;
    } else {
      const float lfo1v = lfoBipolar(lfo1);
      pendingStrike_ = lfo1v > 0.f;
      lfo1WasPositive_ = lfo1v > 0.f;
    }
    gate = true;
    active = true;
    quietSamples_ = 0;
    ampGain = spec.masterVolume * vel;
    ampEnv.Retrigger(false);
  }

  void release() {
    gate = false;
    lfo1WasPositive_ = false;
    pendingStrike_ = false;
  }

  void setPitchNoRetrigger(int note) {
    midiNote = note;
    string.SetFreq(midiToHz(note));
  }

  float processSample() {
    const float lfo1v = lfoBipolar(lfo1);
    bool strike = pendingStrike_;
    pendingStrike_ = false;
    if (spec.osc1Lfo.gateTrigger && gate) {
      const bool lfoPositive = lfo1v > 0.f;
      if (lfoPositive && !lfo1WasPositive_) {
        strike = true;
      }
      lfo1WasPositive_ = lfoPositive;
    }

    bool effectiveGate = gate;
    if (spec.osc1Lfo.gateTrigger) {
      effectiveGate = gate && (lfo1v > 0.f);
    }

    float sample = string.Process(strike) * spec.pluck.level;
    const float amp = ampEnv.Process(effectiveGate) * ampGain;
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

  void setEngine(VoiceEngine newEngine) {
    if (engine == newEngine) return;
    const SynthSoundSpec& primary =
        (engine == VoiceEngine::Plucked) ? plucked.spec : subtractive.spec;
    const OscLfoSpec& osc1Lfo =
        (newEngine == VoiceEngine::Plucked) ? plucked.spec.osc1Lfo : subtractive.spec.osc1Lfo;
    SynthSoundSpec merged = primary;
    merged.engine = newEngine;
    merged.osc1Lfo = osc1Lfo;
    applySpec(merged);
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

  void setPitchNoRetrigger(int note) {
    if (engine == VoiceEngine::Plucked) {
      plucked.setPitchNoRetrigger(note);
    } else {
      subtractive.setPitchNoRetrigger(note);
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

  void applyRealtimeParam(SynthRealtimeParamId paramId, float value) {
    auto clamp01 = [](float v) { return std::clamp(v, 0.f, 1.f); };
    switch (paramId) {
      case SynthRealtimeParamId::MasterVolume:
        subtractive.spec.masterVolume = std::max(value, 0.f);
        subtractive.ampGain = subtractive.spec.masterVolume;
        plucked.spec.masterVolume = std::max(value, 0.f);
        plucked.ampGain = plucked.spec.masterVolume;
        break;
      case SynthRealtimeParamId::FilterCutoffHz:
        subtractive.spec.filter.cutoffHz = std::clamp(value, 20.f, 22000.f);
        break;
      case SynthRealtimeParamId::FilterResonance:
        subtractive.spec.filter.resonance = std::clamp(value, 0.f, kLadderMaxRes);
        subtractive.syncLadderFromSpec();
        break;
      case SynthRealtimeParamId::FilterDrive:
        break;  // Fixed at max input drive; param kept for API compatibility.
      case SynthRealtimeParamId::FilterEnvAttackSec:
        subtractive.spec.filterEnv.attackSec = std::max(value, 0.f);
        subtractive.syncFilterEnvFromSpec();
        break;
      case SynthRealtimeParamId::FilterEnvDecaySec:
        subtractive.spec.filterEnv.decaySec = std::max(value, 0.f);
        subtractive.syncFilterEnvFromSpec();
        break;
      case SynthRealtimeParamId::FilterEnvSustain:
        subtractive.spec.filterEnv.sustain = clamp01(value);
        subtractive.syncFilterEnvFromSpec();
        break;
      case SynthRealtimeParamId::FilterEnvReleaseSec:
        subtractive.spec.filterEnv.releaseSec = std::max(value, 0.f);
        subtractive.syncFilterEnvFromSpec();
        break;
      case SynthRealtimeParamId::FilterEnvDepth:
        subtractive.spec.filter.envDepth = clamp01(value);
        break;
      case SynthRealtimeParamId::FilterKeyTrack:
        subtractive.spec.filter.keyTrack = clamp01(value);
        break;
      case SynthRealtimeParamId::AmpAttackSec:
        subtractive.spec.ampEnv.attackSec = std::max(value, 0.f);
        subtractive.syncAmpEnvFromSpec();
        plucked.spec.ampEnv.attackSec = std::max(value, 0.f);
        plucked.syncAmpEnvFromSpec();
        break;
      case SynthRealtimeParamId::AmpDecaySec:
        subtractive.spec.ampEnv.decaySec = std::max(value, 0.f);
        subtractive.syncAmpEnvFromSpec();
        plucked.spec.ampEnv.decaySec = std::max(value, 0.f);
        plucked.syncAmpEnvFromSpec();
        break;
      case SynthRealtimeParamId::AmpSustain:
        subtractive.spec.ampEnv.sustain = clamp01(value);
        subtractive.syncAmpEnvFromSpec();
        plucked.spec.ampEnv.sustain = clamp01(value);
        plucked.syncAmpEnvFromSpec();
        break;
      case SynthRealtimeParamId::AmpReleaseSec:
        subtractive.spec.ampEnv.releaseSec = std::max(value, 0.f);
        subtractive.syncAmpEnvFromSpec();
        plucked.spec.ampEnv.releaseSec = std::max(value, 0.f);
        plucked.syncAmpEnvFromSpec();
        break;
      case SynthRealtimeParamId::LfoRateHz:
        subtractive.spec.osc1Lfo.rateHz = std::max(value, 0.02f);
        subtractive.lfo1.SetFreq(subtractive.spec.osc1Lfo.rateHz);
        break;
      case SynthRealtimeParamId::EffectsWet:
        subtractive.effects.wet = clamp01(value);
        plucked.effects.wet = clamp01(value);
        break;
      case SynthRealtimeParamId::EffectsDelayMs:
        subtractive.effects.delayMs = std::max(value, 1.f);
        plucked.effects.delayMs = std::max(value, 1.f);
        break;
      case SynthRealtimeParamId::EffectsDelayFeedback:
        subtractive.effects.delayFeedback = std::clamp(value, 0.f, 0.92f);
        plucked.effects.delayFeedback = std::clamp(value, 0.f, 0.92f);
        break;
      case SynthRealtimeParamId::EffectsDelayMix:
        subtractive.effects.delayMix = clamp01(value);
        plucked.effects.delayMix = clamp01(value);
        break;
      case SynthRealtimeParamId::PluckBrightness:
        plucked.spec.pluck.brightness = clamp01(value);
        plucked.string.SetBrightness(plucked.spec.pluck.brightness);
        break;
      case SynthRealtimeParamId::PluckDamping:
        plucked.spec.pluck.damping = clamp01(value);
        plucked.string.SetDamping(plucked.spec.pluck.damping);
        break;
      case SynthRealtimeParamId::PluckStructure:
        plucked.spec.pluck.structure = clamp01(value);
        plucked.string.SetStructure(plucked.spec.pluck.structure);
        break;
      case SynthRealtimeParamId::PluckAccent:
        plucked.spec.pluck.accent = clamp01(value);
        break;
      case SynthRealtimeParamId::GlideSec:
        subtractive.spec.glideSec = std::max(value, 0.f);
        subtractive.updateGlideCoeff();
        break;
      case SynthRealtimeParamId::Osc1RangeFeet:
        subtractive.spec.osc1.range = footageFromFeet(value);
        subtractive.osc1Mult = footageToMultiplier(subtractive.spec.osc1.range);
        subtractive.refreshPitchTargets();
        break;
      case SynthRealtimeParamId::Osc1Sync:
        subtractive.spec.osc1.sync = value >= 0.5f;
        break;
      case SynthRealtimeParamId::Osc1SubOsc:
        subtractive.spec.osc1.subOsc = value >= 0.5f;
        break;
      case SynthRealtimeParamId::Osc1Waveform:
        subtractive.spec.osc1.wave = waveFromSelector(value);
        subtractive.osc1.SetWaveform(toDaisyWave(subtractive.spec.osc1.wave));
        break;
      case SynthRealtimeParamId::Osc1PulseWidth:
        subtractive.spec.osc1.pulseWidth = std::clamp(value, 0.05f, 0.95f);
        subtractive.osc1.SetPw(subtractive.spec.osc1.pulseWidth);
        break;
      case SynthRealtimeParamId::Osc2RangeFeet:
        subtractive.spec.osc2.range = footageFromFeet(value);
        subtractive.osc2Mult = footageToMultiplier(subtractive.spec.osc2.range);
        subtractive.refreshPitchTargets();
        break;
      case SynthRealtimeParamId::Osc2Waveform:
        subtractive.spec.osc2.wave = waveFromSelector(value);
        subtractive.osc2.SetWaveform(toDaisyWave(subtractive.spec.osc2.wave));
        break;
      case SynthRealtimeParamId::Osc2EnvMod:
        subtractive.spec.osc2EnvMod = value >= 0.5f;
        break;
      case SynthRealtimeParamId::Osc2EnvAmountSemis:
        subtractive.spec.osc2EnvAmountSemis = std::clamp(value, -48.f, 48.f);
        break;
      case SynthRealtimeParamId::PluckMode:
        setEngine((value >= 0.5f) ? VoiceEngine::Plucked : VoiceEngine::Subtractive);
        break;
      case SynthRealtimeParamId::Osc1Level:
        subtractive.spec.osc1.level = clamp01(value);
        subtractive.spec.mixer.osc1 = clamp01(value);
        break;
      case SynthRealtimeParamId::Osc2Level:
        subtractive.spec.osc2.level = clamp01(value);
        subtractive.spec.mixer.osc2 = clamp01(value);
        break;
      case SynthRealtimeParamId::MixerNoise:
        subtractive.spec.mixer.noise = clamp01(value);
        break;
      case SynthRealtimeParamId::MixerRingMod:
        subtractive.spec.mixer.ringMod = clamp01(value);
        break;
      case SynthRealtimeParamId::LfoDepth: {
        const float d = clamp01(value);
        subtractive.spec.osc1Lfo.depth = d;
        plucked.spec.osc1Lfo.depth = d;
        break;
      }
      case SynthRealtimeParamId::Osc1LfoRate: {
        const float hz = std::max(value, 0.02f);
        subtractive.spec.osc1Lfo.rateHz = hz;
        subtractive.lfo1.SetFreq(hz);
        plucked.spec.osc1Lfo.rateHz = hz;
        plucked.lfo1.SetFreq(hz);
        break;
      }
      case SynthRealtimeParamId::Osc1LfoDepth: {
        const float d = clamp01(value);
        subtractive.spec.osc1Lfo.depth = d;
        plucked.spec.osc1Lfo.depth = d;
        break;
      }
      case SynthRealtimeParamId::Osc1LfoTarget: {
        const LfoTarget t =
            (value >= 0.5f) ? LfoTarget::PulseWidth : LfoTarget::Pitch;
        subtractive.spec.osc1Lfo.target = t;
        plucked.spec.osc1Lfo.target = t;
        break;
      }
      case SynthRealtimeParamId::Osc1LfoGate: {
        const bool gt = value >= 0.5f;
        subtractive.spec.osc1Lfo.gateTrigger = gt;
        plucked.spec.osc1Lfo.gateTrigger = gt;
        break;
      }
      case SynthRealtimeParamId::Osc2LfoRate:
        subtractive.spec.osc2Lfo.rateHz = std::max(value, 0.02f);
        subtractive.lfo2.SetFreq(subtractive.spec.osc2Lfo.rateHz);
        break;
      case SynthRealtimeParamId::Osc2LfoDepth:
        subtractive.spec.osc2Lfo.depth = clamp01(value);
        break;
      case SynthRealtimeParamId::Osc2LfoTarget:
        subtractive.spec.osc2Lfo.target =
            (value >= 0.5f) ? LfoTarget::PulseWidth : LfoTarget::Pitch;
        break;
      default:
        break;
    }
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
  SynthSoundSpec merged = spec;
  merged.osc1Lfo = panelOsc1Lfo_;
  if (panelPluckMode_) {
    merged.engine = VoiceEngine::Plucked;
  }
  auto& voice = voices_->pool[static_cast<size_t>(voiceIndex)];
  voice.applySpec(merged);
  voices_->effects.applySpec(merged.effects);
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

void Synthesizer::setNotePitch(int voiceIndex, int midiNote) {
  if (voiceIndex < kFirstAssignableVoice || voiceIndex >= kMaxVoices) return;
  voices_->pool[static_cast<size_t>(voiceIndex)].setPitchNoRetrigger(midiNote);
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

void Synthesizer::updatePanelState(SynthRealtimeParamId paramId, float value) {
  switch (paramId) {
    case SynthRealtimeParamId::PluckMode:
      panelPluckMode_ = value >= 0.5f;
      break;
    case SynthRealtimeParamId::Osc1LfoRate:
      panelOsc1Lfo_.rateHz = std::max(value, 0.02f);
      break;
    case SynthRealtimeParamId::Osc1LfoDepth:
    case SynthRealtimeParamId::LfoDepth:
      panelOsc1Lfo_.depth = std::clamp(value, 0.f, 1.f);
      break;
    case SynthRealtimeParamId::Osc1LfoTarget:
      panelOsc1Lfo_.target =
          (value >= 0.5f) ? LfoTarget::PulseWidth : LfoTarget::Pitch;
      break;
    case SynthRealtimeParamId::Osc1LfoGate:
      panelOsc1Lfo_.gateTrigger = value >= 0.5f;
      break;
    default:
      break;
  }
}

void Synthesizer::applyRealtimeParam(int voiceIndex, SynthRealtimeParamId paramId, float value) {
  updatePanelState(paramId, value);
  if (voiceIndex == -1) {
    for (int i = kFirstAssignableVoice; i < kMaxVoices; ++i) {
      voices_->pool[static_cast<size_t>(i)].applyRealtimeParam(paramId, value);
    }
  } else {
    if (voiceIndex < kFirstAssignableVoice || voiceIndex >= kMaxVoices) return;
    voices_->pool[static_cast<size_t>(voiceIndex)].applyRealtimeParam(paramId, value);
  }

  // Keep shared FX bus aligned with one currently playing voice.
  for (int i = kFirstAssignableVoice; i < kMaxVoices; ++i) {
    auto& voice = voices_->pool[static_cast<size_t>(i)];
    if (!voice.isActive() && !voice.ampRunning()) continue;
    voices_->effects.applySpec(voice.effects());
    break;
  }
}
