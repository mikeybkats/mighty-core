---
Project: Mighty Metronome
Doc: Build Timeline — Phases 0-2 (Pre-Departure)
Status: Draft — validated against Daisy Seed docs, tap tempo removed (not in scope)
Last updated: June 22, 2026
---

# Mighty Metronome — Pre-Departure Build Timeline

Scope: Daisy Seed firmware foundation through rough enclosure prototype. This covers everything intended to be complete before leaving Microsoft. Phases 3-6 (design lock, production, crowdfunding, fulfillment) are post-departure and not included here.

Guiding constraint: **reliable click before features.**

---

## Phase 0 — Firmware Foundation

_Goal: reliable click, nothing else._

| #   | Step                                | Estimate    | Notes                                                                                                                                                            |
| --- | ----------------------------------- | ----------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1   | Dev environment + Blink             | 0.5–1 day   | Toolchain install, first flash via DFU. Add 0.5 day buffer for Windows USB/driver issues (Zadig) if applicable.                                                  |
| 2   | Basic audio output                  | 1–2 days    | Default project template is passthrough out of the box; swap in a DaisySP oscillator to confirm a clean signal path to the output jack/speaker.                  |
| 3   | Click sound design                  | 3–5 days    | Synthesize or sample a clean tick; accent vs. regular beat distinction; layering options. Pulls from existing Mighty Synth work.                                 |
| 4   | Timing engine                       | 1–1.5 weeks | Sample-accurate clock loop, BPM-to-interval math, accent-on-beat-1 logic, drift prevention over long sessions. This is the real engineering core of the product. |
| 5   | BPM persistence across power cycles | 3–4 days    | Store pot-derived BPM/state in flash so it survives power-off. Can be tested with a placeholder value before the pot is physically wired.                        |

**Phase 0 total: ~2.5 weeks**

---

## Phase 1 — Input Hardware Integration

_Goal: physical controls actually drive the firmware._

| #   | Step                               | Estimate        | Notes                                                                                                                                                               |
| --- | ---------------------------------- | --------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 6   | Wire detented BPM pot              | 3–4 days        | Alps RK09K or Bourns PTV09. ADC input, map voltage range to BPM range.                                                                                              |
| 7   | Volume + swing encoders            | 3–4 days        | Wire and map both.                                                                                                                                                  |
| 8   | Toggle switches for sound layering | 2–3 days        | GPIO-driven, mapped to alternate click samples/layers.                                                                                                              |
| 9   | Breadboard integration test        | 1 week (buffer) | All controls + audio output together. Check for ADC/encoder electrical noise bleeding into the audio signal — most likely real snag in this phase, not a formality. |

**Phase 1 total: ~2.5–3 weeks**

---

## Phase 2 — Rough Enclosure Prototype

_Goal: validate dimensions and ergonomics, not final finish._

| #   | Step                         | Estimate                 | Notes                                                                                                   |
| --- | ---------------------------- | ------------------------ | ------------------------------------------------------------------------------------------------------- |
| 10  | Rough CAD pass in Fusion 360 | 1 week                   | Block out 4–6" × 7.5" × 2–3" enclosure, panel cutouts for pot/encoders/switches/jacks.                  |
| 11  | First CNC test cut           | 1 week (incl. iteration) | Carvera Air, cheap material (MDF/scrap pine) — not final wood. Validate tolerances and mounting points. |
| 12  | Fit-check                    | 2–3 days                 | Confirm breadboard electronics fit, control shafts align with panel holes, room for speaker/jack.       |

**Phase 2 total: ~2.5–3 weeks**

---

## Summary

**Phases 0–2 combined: ~7.5–8.5 weeks**

This is the working-breadboard-plus-validated-rough-enclosure milestone — the target state before leaving Microsoft. Original estimate for this same scope was ~16 weeks; the difference came from correcting inflated toolchain/audio-setup estimates (Daisy's official on-ramp is genuinely fast) and removing tap tempo, which was not part of the original spec and got added in error during planning.

**Where the real risk sits:** Phase 1, step 9 (breadboard noise debugging) and Phase 0, step 4 (timing engine) are the two steps most likely to run long for a first-time build of this kind. Worth treating the time saved elsewhere as schedule buffer rather than pulling the departure date forward.

**Explicitly out of scope for this phase:** tap tempo, custom PCB, final wood finish, anything beyond a single rough prototype unit.
