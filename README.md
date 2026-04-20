# Mighty Music Core (MMC)

A C++ audio engine that powers the Mighty family of apps. MMC handles all audio scheduling, click track output, and (in future milestones) analysis and composition via swappable modules. Native UI layers only send commands and receive callbacks — they never touch audio internals.

## Architecture

```
iOS (SwiftUI)              Android (Kotlin)
      ↕  ObjC++ bridge           ↕  JNI bridge
            [Mighty Music Core (C++ / JUCE)]
            ┌─────────────────────────────┐
            │  Lifecycle & Scheduler      │
            │  Tempo Engine               │
            │  IAnalyzer (swappable)      │
            │  IComposer (swappable)      │
            │  MIDI Out                   │
            │  Audio I/O                  │
            └─────────────────────────────┘
```

## Public Interface

```cpp
MightyMusicCore core;

core.onTick = [](int beat) { /* fired from audio thread — do not block */ };

core.setBPM(120.0);
core.start();   // opens audio stream, begins scheduling
core.stop();    // closes audio stream
core.isPlaying();
core.getBPM();
```

## Building

MMC is consumed as a CMake static library. Add it to your project:

```cmake
add_subdirectory(path/to/mighty-core)
target_link_libraries(your_target mighty-core)
```

### Android
Requires NDK 26+ and CMake 3.22+. Oboe is fetched automatically via CMake `FetchContent` — no manual dependency setup needed.

### iOS
Not yet implemented (Milestone 5). Will use CoreAudio behind the same `MightyMusicCore` interface.

## Guiding Principles

- **Never block the audio thread.** No heap allocation, no locks, no I/O on the audio callback.
- **Lock-free communication.** Atomics and ring buffers pass data between threads.
- **MMC is a host, not a brain.** Intelligence lives in swappable `IAnalyzer` / `IComposer` modules.
- **Clean interface boundaries.** Platform layers only send commands and receive callbacks.

## Milestones

| # | Status | Description |
|---|--------|-------------|
| 2 | ✅ | MMC scaffolding — static lib, stub interface, Android CMake integration |
| 3 | ✅ | Tempo engine — audio-clock scheduler, Oboe click track, `onTick` callback |
| 5 | ⬜ | iOS CoreAudio backend |
| 5 | ⬜ | `IAnalyzer` / `IComposer` module system + `MusicalState` |
| 6 | ⬜ | Pitch detection (YIN) and bassline composer |
