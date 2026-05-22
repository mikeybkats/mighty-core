#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

class Sound;
class Synthesizer;

// -----------------------------------------------------------------------------
// Output — shared playback bus (device I/O) + metronome transport.
//
// Playback (open/close) and metronome (start/stop) are independent:
//   * Synth notes need openPlayback() only.
//   * Metronome ticks need openPlayback() + startMetronome().
//
// Sound = tick PCM + patch catalog; Synthesizer = DaisySP voice pool.
// -----------------------------------------------------------------------------

class Output {
 public:
  static constexpr int kTickSlotCount = 12;
  static constexpr int kTickSoundSynthesized = -1;

  Output();
  ~Output();

  /// Opens the playback device (idempotent).
  bool openPlayback();
  void closePlayback();
  [[nodiscard]] bool isPlaybackOpen() const;

  /// Beat scheduler + click triggers (requires playback open).
  void startMetronome(const std::function<void(int beatNumber)>& onTick);
  void stopMetronome();
  [[nodiscard]] bool isMetronomeRunning() const;

  /// Convenience: open playback and start metronome (legacy metronome-only apps).
  void start(const std::function<void(int beatNumber)>& onTick);
  /// Stops metronome only; leaves playback open for synth.
  void stop();
  /// True while metronome transport is running (not “device open”).
  [[nodiscard]] bool isPlaying() const;

  void setBPM(double bpm);
  [[nodiscard]] double getBPM() const;

  void setTwoBeatMeasure(bool enabled);
  void setSwingFraction(double fraction);

  /// Preload mono float PCM for metronome kit slot [0, kTickSlotCount).
  void setTickSoundPcm(int index, std::vector<float> samples, int32_t sourceSampleRate);
  /// kTickSoundSynthesized (-1) or PCM slot index (empty slot → synthesized click).
  void setTickSound(int soundIndex);

  [[nodiscard]] Sound& sound();
  [[nodiscard]] const Sound& sound() const;
  [[nodiscard]] Synthesizer& synthesizer();
  [[nodiscard]] const Synthesizer& synthesizer() const;

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};
