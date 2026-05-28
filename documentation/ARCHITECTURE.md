# Mighty Music Core

## Architectural Pipeline

## Sound output

```
[ App / Output API ]
        │
        ▼
  TempoEngine::open()  →  Oboe / miniaudio playback stream
        │
        ▼  (every N ms, audio thread)
  onAudioReady / maDataCallback
        │
        ▼
  processAudio(output, numFrames)
    1. memset(output, 0)
    2. finish click tail → Sound::renderPartial
    3. notes → Synthesizer::renderVoices  (DaisySP per sample)
    4. if metronome on → Scheduler → Sound::triggerAt (PCM or synth click)
        │
        ▼
  output[]  →  OS audio HAL  →  speaker
```

## Libraries

[DaisySp](https://github.com/electro-smith/daisysp) - sound synthesis engine
[Miniaudio](https://miniaud.io) - sound playback / output
[GLFW](https://github.com/glfw/glfw) - for OpenGL, OpenGL ES. Used for Debug UI Harness
[Dear ImGui](https://github.com/ocornut/imgui) - for creating UI for debug harness
[Oboe](https://github.com/google/oboe) - audio backend for android
