#pragma once

#include <memory>

// Input owns microphone capture, monophonic note detection, and session logging.
// MightyMusicCore composes this class and exposes high-level orchestration APIs.
class Input {
 public:
  Input();
  ~Input();

  // Starts listening to audio input and recording detected note events.
  void startListening();
  // Stops listening, clears session buffers, and writes session logs to disk.
  void stopListening();
  [[nodiscard]] bool isListening() const;
  // True once pitch detection has seen at least one valid signal since startListening.
  [[nodiscard]] bool hasDetectedSignal() const;
  // Returns last detected MIDI note for current session, or -1 when none detected.
  [[nodiscard]] int lastDetectedMidiNote() const;

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};
