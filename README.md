# Mighty Music Core (MMC)

A C++ audio engine that powers the Mighty family of apps. MMC handles all audio scheduling, click track output, and (in future milestones) analysis and composition via swappable modules. Native UI layers only send commands and receive callbacks — they never touch audio internals.

## Architecture

```
Android (Kotlin)
      ↕  ObjC++ bridge           ↕  JNI bridge
            [Mighty Music Core (C++)]
            ┌─────────────────────────────┐
            │  Lifecycle & Scheduler      │
            │  Tempo Engine               │
            │  Interpret (swappable)      │
            │  Compose (swappable)      │
            │  MIDI Out                   │
            │  Audio I/O                  │
            └─────────────────────────────┘
```

## Public Interface

### `MightyMusicCore` (primitives)

```cpp
MightyMusicCore core;

core.onTick = [](int beat) { /* fired from audio thread — do not block */ };

core.setBPM(120.0);
core.start();   // opens audio stream, begins scheduling
core.stop();    // closes audio stream
core.isPlaying();
core.getBPM();
```

### `Metronome` (app/domain policy)

```cpp
Metronome metronome;

metronome.onTick = [](int beat) { /* fired from audio thread — do not block */ };
metronome.setBPM(120.0);
metronome.setSwingFraction(0.2);
metronome.setTwoBeatMeasure(true);
metronome.start();
```

## Building

MMC is consumed as a CMake static library. Add it to your project:

```cmake
add_subdirectory(path/to/mighty-core)
target_link_libraries(your_target mighty-core)
```

### Desktop (macOS / Linux debug hosts)

Configure and build **out of tree** (outputs go under `build/` only):

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build --parallel
./build/mighty-core-ui          # metronome debug UI
./build/mighty-core-synth-host  # synth patch UI
./build/mighty-core-host-check  # headless click smoke test
./build/tests/mmc-tests         # unit tests
```

VS Code: use the default **Build** task (`mighty-core: build (Debug)`).

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

| #   | Status | Description                                                               |
| --- | ------ | ------------------------------------------------------------------------- |
| 2   | ✅     | MMC scaffolding — static lib, stub interface, Android CMake integration   |
| 3   | ✅     | Tempo engine — audio-clock scheduler, Oboe click track, `onTick` callback |
| 4   | ⬜     | `Interpret` / `Compose` module system + `MusicalState`                    |
| 5   | ⬜     | Pitch detection (YIN) and bassline composer                               |
| 6   | ⬜     | iOS CoreAudio backend                                                     |

## TODO:

1. Analyzer / Composer
   1. listening / tracking / handling state
      1. modes: `input`, `jam`, `song`
         1. `input` - for v2 would rely on midi composition
         2. `jam` - user just starts playing
2. Synthesizer
   1. a simple multitimbral polyphonic synthesizer.
      1. V1 Not adjustable by user. Comes with pre-set sounds: `guitar`, `bass`, `piano`
