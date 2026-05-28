#pragma once

#ifdef __ANDROID__
#include <oboe/Oboe.h>
#endif

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include "engine/RealtimeCommandQueue.h"
#include "engine/Sound.h"
#include "engine/Synthesizer.h"
#include "scheduler/Scheduler.h"

// -----------------------------------------------------------------------------
// TempoEngine — shared playback device + mix bus.
//
// Two independent layers share one output stream:
//   * Playback (open/close) — device I/O; always mixes Synthesizer pool voices.
//   * Metronome (startMetronome/stopMetronome) — Scheduler + Sound clicks only when enabled.
//
// Each processAudio() pass:
//   1. Zero buffer; finish in-flight click tail (if any).
//   2. Synthesizer::renderVoices — pool notes (independent of metronome).
//   3. When metronome running: Scheduler::advance, Sound::triggerAt, onTick.
//
// __ANDROID__: Oboe output stream; this class is the DataCallback.
// Desktop: miniaudio playback device (see TempoEngine.cpp).
// -----------------------------------------------------------------------------
class TempoEngine
#ifdef __ANDROID__
    : public oboe::AudioStreamDataCallback
#endif
{
 public:
  static constexpr int kTickSlotCount = Sound::kSlotCount;

  TempoEngine();
#ifdef __ANDROID__
  ~TempoEngine() override;
#else
  ~TempoEngine();
#endif

  std::function<void(int beatNumber)> onTick;

  /// Opens the OS playback device (idempotent). Required for synth or metronome audio.
  bool open();
  void close();
  [[nodiscard]] bool isRunning() const;

  /// Enables beat scheduling and metronome clicks; does not open the device.
  void startMetronome();
  void stopMetronome();
  [[nodiscard]] bool isMetronomeRunning() const;

  void setBPM(double bpm);
  double getBPM() const;

  void setTwoBeatMeasure(bool enabled);
  void setSwingFraction(double fraction);  // 0 .. 0.5

  Synthesizer& synthesizer();
  [[nodiscard]] const Synthesizer& synthesizer() const;
  Sound& sound();
  [[nodiscard]] const Sound& sound() const;

  void setTickSoundPcm(int index, std::vector<float> samples, int32_t sourceSampleRate);
  void setTickSound(int index);
  bool queueCommand(const RealtimeCommand& command);

#ifdef __ANDROID__
  oboe::DataCallbackResult onAudioReady(oboe::AudioStream*, void*, int32_t) override;
#endif

  /// Called once per audio buffer from Oboe/miniaudio (audio thread).
  void processAudio(float* output, int32_t numFrames);

 private:
  void prepareOutputAudio(int32_t deviceSampleRate);

  Synthesizer synthesizer_;
  Sound sound_;  // constructed with synthesizer_ reference; handles tick PCM/synth mix.
  Scheduler scheduler_;
  bool metronomeRunning_{false};
  /// State for a click that may span multiple callbacks when the buffer is small.
  Sound::ClickPlaybackState click_{};
  RealtimeCommandQueue commandQueue_{};

#ifdef __ANDROID__
  std::shared_ptr<oboe::AudioStream> stream_;
#else
  struct DesktopImpl;
  std::unique_ptr<DesktopImpl> desktop_;
#endif
};
