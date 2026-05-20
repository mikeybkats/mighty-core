_Revision History_

| Version | Date        | Author          | Changes        |
| ------- | ----------- | --------------- | -------------- |
| 1.0     | Feb 5, 2026 | Michael Barakat | Initial draft  |
| 1.0     | Feb 6, 2026 | Michael Barakat | First Revision |

# Bandmate Product Specification

**Version:** 1.0  
**Author:** Michael Barakat  
**Status:** Draft

---

## Executive Summary

Bandmate is an adaptive practice partner for musicians, it:

- provides tempo like a drummer or metronome
- listens to the live performance of a solo musician
- analyzes in real-time what's being played (tempo, frequency, rythmn, pattern)
- plays back accompanying melodic, rythmic and percussive parts to support the target musician
- learns the preferences and tendencies of the musician to create a collaborative experience just like playing with a real band or real bandmate.

### Core Value Proposition

- **Always Available**: Practice with accompaniment anytime, without coordinating with other musicians
- **Adaptive & Intelligent**: Responds to the musician's playing style, mistakes, and improvisations
- **Learning System**: Remembers patterns, preferences, and frequently played material
- **Responsive**: Can stop, loop, and resume based on the musician's playing, not a fixed timeline
- **Better than backing tracks**: backing tracks are cumbersome because they are not responsive and must be manually controlled. Bandmate can replace backing tracks through intelligent responsive playback.

### Target Users

- **Primary**: Beginning to intermediate musicians who don't have regular access to other players
- **Secondary**: Experienced musicians who want a flexible practice tool
- **Use Cases**: Solo practice, learning new songs, developing improvisational skills, rehearsing without a full band

---

## Product Vision

### Short-term (MVP)

A desktop/mobile application that listens to a musician play and generates basic accompaniment (bass, drums, or chordal instruments) based on detected key (circle of fifths, standard interval chord progressions), tempo, and chord progressions. Responds in real-time to the actions of the musician to dynamically adjust the accompaniment.

### Medium-term

An adaptive system that learns individual playing styles, responds to voice commands ("that sounds good do more of that", "take it back a few measures", ect), and can work with imported backing midi data tracks in an intelligent (or convert mp3 to midi).

### Long-term

A dedicated hardware unit with professional-grade audio I/O that musicians can bring to practice spaces and potentially use for live performance, with cloud-synced learning profiles.

---

## Non-Goals (Out of Scope)

- **Perfect Transcription**: Not trying to create sheet music or note-perfect transcription
- **Music Production**: Not a DAW or recording studio tool
- **Live Performance (MVP)**: Initial focus is practice/rehearsal, not stage performance
- **Teaching Tool**: Not providing lessons or tutorials (though it supports practice)
- **Social Network**: Not building a community platform (though sharing features may come later)

---

## Core Features

### MVP Features (Phase 1)

#### 1. Real-time Audio Analysis

- **Pitch Detection**: Identify notes being played (monophonic priority)
- **Root Note Detection**: Extract bass/root notes from chordal instruments
- **Tempo Detection**: Automatically detect and lock to the musician's tempo
- **Key Detection**: Identify the key being played in
- **Beat Tracking**: Maintain accurate rhythm analysis

**Success Criteria:**

- Accurately detect key within 2-4 bars of playing
- Tempo lock within ±2 BPM of actual playing
- Root note detection with 80%+ accuracy

#### 2. Basic Accompaniment Generation

- **Instrument Selection**: User chooses what they're playing; Bandmate fills in missing instruments
- **Supported Instruments**:
  - Input: Bass, guitar, piano/organ, drums (metronome only in MVP)
  - Output: Bass lines, drum patterns, chord progressions
- **Pattern-Based Generation**: Use common chord progressions (I-IV-V, ii-V-I, etc.) and standard rhythmic patterns
- **Musical Intelligence**: Apply circle of fifths and basic music theory to generate appropriate accompaniment

**Success Criteria:**

- Generate accompaniment that stays in key 95%+ of the time
- Rhythmic patterns that feel locked to user's tempo
- Smooth transitions between detected chords

#### 3. Simple User Interface

- **Instrument Selection**: Choose what user is playing
- **Visual Feedback**:
  - Current detected key
  - Current tempo (BPM)
  - Current chord/root note
  - Beat indicator
- **Basic Controls**:
  - Start/Stop listening
  - Volume control for accompaniment
  - Mute individual instrument tracks

**Success Criteria:**

- Latency from playing to visual feedback < 200ms
- Clear indication of what Bandmate is "hearing"
- One-click start to practice session

#### 4. Audio Pipeline

- **Input**: Microphone or line-in audio
- **Processing**: Real-time analysis with acceptable latency (100-200ms buffer)
- **Output**: Mixed accompaniment sent to speakers/headphones
- **Latency Target**: Total round-trip latency < 200ms (acceptable for practice)

**Success Criteria:**

- No audio dropouts or glitches
- Clean mixing of input monitoring and generated accompaniment
- Adjustable latency/buffer size for different hardware

---

### Phase 2 Features (Post-MVP)

#### 1. Adaptive Learning

- **Pattern Recognition**: Remember frequently played progressions and song structures
- **Style Learning**: Adapt to individual playing characteristics (rhythm feel, note choices, dynamics)
- **Session History**: Track what was practiced and when
- **Favorite Patterns**: Mark and recall successful jam sessions

**Implementation Notes:**

- Store user playing data locally
- Build pattern library over time
- ML model fine-tuning based on user's style

#### 2. Voice Control & Feedback

- **Voice Commands**:
  - "That sounds good" → Save current pattern
  - "Simpler" / "More complex" → Adjust density
  - "Again" → Loop current section
  - "Next section" → Advance to next part
  - "From the top" → Return to beginning
- **Natural Language Processing**: Understand musical terminology and casual commands

**Implementation Notes:**

- Integrate speech recognition API (system-level or cloud-based)
- Define command vocabulary
- Hands-free operation critical for real practice workflow

#### 3. Responsive Backing Track Mode

- **Import & Analyze**: Load MP3/WAV files and extract musical structure
- **Structure Detection**: Automatically identify intro, verse, chorus, bridge, outro
- **Adaptive Playback**:
  - Pause when musician stops playing
  - Loop current section on command
  - Resume from where musician re-enters
  - Allow non-linear navigation (repeat sections, skip ahead)
- **Analysis Output**: Generate chord charts and tempo maps from any song

**Technical Requirements:**

- Audio source separation (isolate instruments)
- Structure segmentation algorithms
- Chord detection on mixed audio
- Tempo map extraction

#### 4. Enhanced Accompaniment

- **More Varied Patterns**: Expand beyond basic I-IV-V progressions
- **Style Selection**: Choose genre-appropriate patterns (rock, jazz, blues, funk, etc.)
- **Dynamic Variation**: Vary intensity and complexity during a session
- **Multi-track Generation**: Generate bass, drums, and chordal instruments simultaneously

#### 5. Social/Sharing Features

- **Share Sessions**: Export recordings of practice sessions
- **Pattern Library**: Community-contributed accompaniment patterns

#### 6. DAW Integration (Future)

- **Plugin/VST Support**: Bandmate as a plugin within DAWs (Pro Tools, Logic, Ableton, etc.)
- **MIDI Input**: Accept MIDI data from DAW tracks as input instead of audio
- **MIDI Output**: Generate accompaniment as MIDI tracks that can be routed to virtual instruments
- **Use Case**: Music production workflow - generate intelligent accompaniment during songwriting/arranging
- **Benefits**:
  - Seamless integration with existing production workflows
  - Leverage DAW's virtual instruments and effects
  - Non-destructive editing and arrangement capabilities

**Note**: Primary use case remains standalone practice tool; DAW integration is a secondary feature for production-oriented users.

---

### Phase 3 Features (Hardware Unit)

#### 1. Dedicated Hardware

- **Professional Audio I/O**: XLR/TRS inputs, balanced outputs
- **Lower Latency**: Dedicated DSP for <50ms total latency
- **Physical Controls**: Knobs, buttons for quick adjustments without screen interaction
- **Robust Build**: Gigging-ready durability
- **Battery Option**: Portable power for rehearsal spaces

#### 2. Cloud Sync

- **Profile Backup**: Sync learned patterns across devices
- **Software Updates**: Over-the-air firmware updates
- **Pattern Library**: Access community patterns from hardware unit

---

## Technical Considerations

### Core Technologies (TBD - to be determined in next phase)

Areas requiring technical decisions:

- **Audio Processing Libraries**: Consider aubio, librosa, or JUCE
- **ML Framework**: TensorFlow, PyTorch for learning components
- **Real-time Audio**: Considerations for low-latency audio processing
- **Platform**: Desktop (Electron?), Mobile (React Native? Native?), or Web Audio API
- **Database**: Local storage for pattern learning

### Key Technical Challenges

1. **Real-time Processing**: Balancing analysis accuracy with latency requirements
2. **Polyphonic Analysis**: Detecting chords from guitar/piano with reasonable accuracy
3. **Beat Tracking**: Maintaining tempo lock even with human timing variations
4. **Pattern Generation**: Creating musical accompaniment that doesn't sound robotic
5. **Audio Quality**: Clean mixing and output without artifacts

---

## Success Metrics

### MVP Success Criteria

- **Technical Performance**:
  - Key detection accuracy: >90%
  - Tempo lock accuracy: ±2 BPM
  - Total latency: <200ms
  - Crash-free operation: >95% of sessions
- **User Experience**:
  - Time to first accompaniment: <30 seconds from app launch
  - User can complete 15-minute practice session without manual intervention
  - Accompaniment stays in key throughout session

- **Product Validation**:
  - 10 beta users complete 5+ practice sessions each
  - User feedback indicates they would use this regularly
  - Users report it's better than static backing tracks

### Phase 2 Success Criteria

- Users return for 3+ sessions per week
- Voice commands used in 50%+ of sessions
- Backing track analysis completes in <30 seconds per song
- Users report they "feel like playing with a band"

---

## Open Questions

1. **Monetization**: Freemium model? One-time purchase? Subscription for cloud features?
2. **Platform Priority**: Desktop first for development ease, or mobile first for portability?
3. **Audio Quality**: 44.1kHz/16-bit (CD quality) - sufficient for real-time pitch/tempo analysis and playback, with lower computational overhead than professional formats. Professional quality (48kHz/24-bit) unnecessary for practice use case.
4. **Offline Operation**: Must work fully offline, or can rely on cloud processing for some features?
5. **Pattern Library**: MIDI format - industry standard, widely supported, and familiar to musicians. Enables compatibility with existing MIDI libraries and easy import/export.

---

## Next Steps

1. **API Design**: Define audio analysis API and accompaniment generation interfaces
2. **Technology Stack**: Select frameworks, libraries, and platforms
3. **MVP Scope Refinement**: Prioritize specific features for initial release
4. **Prototype Development**: Build proof-of-concept for core audio analysis loop
5. **User Research**: Interview target musicians to validate assumptions

---

## Appendix

### Competitive Landscape

**Existing Solutions:**

- **Backing Track Apps** (e.g., iReal Pro, Band-in-a-Box): Static playback, require manual chord entry
- **Jam Apps** (e.g., JamKazam, JamStudio): Focus on remote playing with others, not solo practice
- **Looper Pedals** (e.g., Boss Loop Station): Record/playback only, no intelligent generation
- **Smart Speakers/Assistants**: Can play backing tracks but no real-time listening/adaptation

**Key Differentiators:**

- Real-time listening and adaptation
- Learning individual playing style over time
- Responsive (stop/loop/resume) rather than linear playback
- Voice control while playing

### Musical Theory Foundation

**Common Chord Progressions** (Short list - Reference for pattern generation):

- I-IV-V (most common in rock/pop)
- I-V-vi-IV ("Four Chord Song")
- ii-V-I (jazz standard)
- I-vi-IV-V (50s progression)
- I-bVII-IV (mixolydian rock progression)

**Circle of Fifths**: Used for intelligent key relationships and modulation

**Rhythm Patterns**: Standard rock/pop patterns (4/4 time with kick on 1&3, snare on 2&4)

### User Personas

**Persona 1: The Bedroom Bassist**

- Age: 22, plays bass for 2 years
- Practice environment: Bedroom/apartment
- Challenge: No bandmates, backing tracks get boring
- Goal: Improve timing and learn to play with others
- Bandmate Use: Daily 30-60 minute practice sessions

**Persona 2: The Singer-Songwriter**

- Age: 28, plays guitar and sings
- Practice environment: Home studio
- Challenge: Wants to hear how songs sound with full band
- Goal: Write and arrange songs
- Bandmate Use: Songwriting sessions, trying different arrangements

**Persona 3: The Weekend Warrior**

- Age: 35 or 45, played guitar in college, getting back into it
- Practice environment: Garage
- Challenge: Rusty, needs motivation to practice
- Goal: Get good enough to jam with friends again
- Bandmate Use: 2-3 times per week, learning covers

---
