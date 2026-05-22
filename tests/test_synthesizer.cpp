#include <gtest/gtest.h>

#include <vector>

#include "engine/Sound.h"
#include "engine/Synthesizer.h"

TEST(Synthesizer, ClickRendersNonSilentSamples) {
  Synthesizer synth;
  synth.setSampleRate(48000);

  std::vector<float> buf(256, 0.f);
  synth.renderClick(buf.data(), 0, 0, static_cast<int32_t>(buf.size()), false, 1.f);

  float peak = 0.f;
  for (float s : buf) peak = std::max(peak, std::abs(s));
  EXPECT_GT(peak, 0.01f);
}

TEST(Sound, CreateSoundAndPlayVoice) {
  Synthesizer synth;
  Sound sound(synth);
  synth.setSampleRate(48000);

  const SoundId custom = sound.createSound({
      .osc1 = {.range = OscFootage::Foot8, .wave = OscWave::PolySaw, .level = 0.8f},
      .filter = {.cutoffHz = 600.f, .resonance = 0.3f},
      .ampEnv = {.attackSec = 0.01f, .decaySec = 0.2f, .sustain = 0.5f, .releaseSec = 0.2f},
  });
  ASSERT_TRUE(sound.isValidSound(custom));

  const int voice = sound.allocateVoice(custom);
  ASSERT_GE(voice, Synthesizer::kFirstAssignableVoice);

  sound.triggerNote(voice, 48, 0.9f);
  std::vector<float> buf(256, 0.f);
  synth.renderVoices(buf.data(), 0, static_cast<int32_t>(buf.size()));

  float peak = 0.f;
  for (float s : buf) peak = std::max(peak, std::abs(s));
  EXPECT_GT(peak, 0.001f);

  sound.releaseVoice(voice);
}

TEST(Sound, BuiltInGuitarPreset) {
  Synthesizer synth;
  Sound sound(synth);
  synth.setSampleRate(48000);

  ASSERT_TRUE(sound.isValidSound(sound.guitarSound()));
  const int v = sound.allocateVoice(sound.guitarSound());
  ASSERT_GE(v, Synthesizer::kFirstAssignableVoice);
  sound.triggerNote(v, 64, 0.8f);

  std::vector<float> buf(512, 0.f);
  synth.renderVoices(buf.data(), 0, static_cast<int32_t>(buf.size()));

  float peak = 0.f;
  for (float s : buf) peak = std::max(peak, std::abs(s));
  EXPECT_GT(peak, 0.001f);
}

TEST(Sound, BuiltInPianoPreset) {
  Synthesizer synth;
  Sound sound(synth);
  synth.setSampleRate(48000);

  ASSERT_TRUE(sound.isValidSound(sound.pianoSound()));
  const int v = sound.allocateVoice(sound.pianoSound());
  sound.triggerNote(v, 72, 0.85f);

  std::vector<float> buf(4096, 0.f);
  synth.renderVoices(buf.data(), 0, static_cast<int32_t>(buf.size()));

  float peak = 0.f;
  for (float s : buf) peak = std::max(peak, std::abs(s));
  EXPECT_GT(peak, 0.01f);
}

TEST(Sound, RingModVoiceProducesHarmonics) {
  Synthesizer synth;
  Sound sound(synth);
  synth.setSampleRate(48000);

  const SoundId ring = sound.createSound({
      .osc1 = {.wave = OscWave::Sine, .level = 0.7f},
      .osc2 = {.wave = OscWave::Sine, .tuneSemis = 7.f, .level = 0.7f},
      .mixer = {.osc1 = 0.f, .osc2 = 0.f, .ringMod = 0.9f},
      .filter = {.cutoffHz = 8000.f, .resonance = 0.1f, .envDepth = 0.f},
      .ampEnv = {.attackSec = 0.001f, .decaySec = 0.1f, .sustain = 0.8f, .releaseSec = 0.1f},
      .filterEnv = {.attackSec = 0.001f, .decaySec = 0.05f},
  });
  const int v = sound.allocateVoice(ring);
  sound.triggerNote(v, 60, 1.f);

  std::vector<float> buf(512, 0.f);
  synth.renderVoices(buf.data(), 0, static_cast<int32_t>(buf.size()));

  float peak = 0.f;
  for (float s : buf) peak = std::max(peak, std::abs(s));
  EXPECT_GT(peak, 0.05f);
}
