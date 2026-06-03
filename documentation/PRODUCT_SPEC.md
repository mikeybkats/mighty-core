---
Version: 1.0
Date: February 5, 2026
Author: Michael Barakat
Status: Draft
---

# Bandmate Product Specification

## Executive Summary

Bandmate is an adaptive practice partner and creative songwriting tool for musicians.

#### P0 - player / composer

modules: metronome, synthesizer, recorder, analyzer, composer, player

- [ ] design bandmate ui
- [ ] provides tempo with a metronome
- [ ] listens to and **records** the full live performance of a solo musician
- [ ] recognizes common chord patterns and creates an index of potential **fits** for accompaniment using circle of 5ths. playback choices can be **weighted** to be more or less **random** or go toward a certain kind of musical **feeling**
- [ ] four possible instruments are provided: drums, piano, bass, and guitar
- [ ] generates a single instrument accompaniment **track** that matches the musician's input.
- [ ] musician uses bandmate to play back accompaniment while they practice along with their instrument
- [ ] provides midi export option to export projects for ableton, logic, fl studio, or reaper

#### P0.5 - input mode

- [ ] adds percussion engine for more complex tempo
- [ ] with percussion, bandmate will determine appropriate drum patterns according to the chord changes and intervals the musician has selected. Musician can similarly to chord patterns, **weight** the output to be more or less **spontaneous** or **deterministic**.
- [ ] player can issue commands to bandmate like "stop" or "start" or "go back a few measures" ect
- [ ] input mode: user gives bandmate a few chords and bandmate creates the index and **fits** from there

#### P1 - collaborator / jam

- analyzes in real-time what's being played (tempo, frequency, rythmn, pattern) similar to how a live musician would
- plays back accompaniment melodic, rythmic and percussive parts to support the target musician in real time
- learns the preferences and tendencies of the musician to create a collaborative experience just like playing with a real band or real bandmate.
- musician can issue commands like in a real jam session, "take it back to the verse" "go back two measures" "pick up the tempo" ect

#### P3 - translator / sound player+

- in sound player mode bandmate is more like a video/audio player
- it takes audio tracks in and converts them to playable midi practice tracks. will probably use off the shelf tools to do this if they are available.
- practice tracks can then be more like playing with a real band. removes the need for interacting with a screen to control the playback. The musician can just say "stop, take it back a few measures" ect.

#### P4 - community and marketplace

- the website component offers a place for teachers to offer courses and midi practice tracks
- the website offers students a place to get practice materials

## Product line: Mighty Core Ecosystem:

- Mighty Metronome - loads patches from `Synthesizer`.
- Mighty Syntheszier - 2 osc analog style mono synth. Has poly synth mode for strings and plucked instruments.
- Mighty Drum - a drum looper that uses probabilities, retrigger and pattern definitiion to create loops.
- Mighty Composer - a songwriting generator that given an input creates backing track with guitar, bass, keys and drums
- Mighty Bandmate - a musical collaborator and playback engine. Creates backing tracks, acts as musical librarian and dynamic audio player.
- Mighty Music - a marketplace and collaboration hub where students, teachers and songwriters can find and share music they've created for the Mighty Music Platform

## User Personas

### Jordan

Guitarist & Hobbyist Songwriter

Jordan works a 9-to-5 in UX design and plays guitar in the evenings and on weekends. He's been playing for 12 years — solid enough to write his own chord progressions and riffs, but he practices alone because his schedule and obligations to his family makes it impossible to coordinate with other musicians regularly.

#### Jordan's musical goals

- Stay sharp and improve his timing without needing a practice partner
- Explore chord progressions and hear how they sound with a full backing arrangement
- Leverage technology to write music, but still feel like he owns the creative process
- Eventually export his ideas into Ableton to develop them into full tracks
- Wants to have a strong practice routine that's as close to practicing with a real band as possible. So he can be ready for band rehearsals when the day comes.

#### Frustrations

- Practicing with a metronome has limitations
- DAWs like Ableton are very open ended and take too long just to mock up a simple backing idea
- Backing track apps on YouTube are static — he can't adapt them to his own chord changes or easily change tempo for practicing.
- Stopping, starting, and reqinding backing tracks is annoying

#### How he uses Bandmate

Jordan sets a tempo, plays through a chord progression, and lets Bandmate generate a bass and drum accompaniment. He loops it and practices soloing or refining his rhythm playing on top. In songwriter mode he feeds Bandmate a few chords, Jordan likes the output, but it's a bit too high energy. He adjusts it slightly in the ui to change the bassline from `energetic` to `laid back` during the chorus. He also decides to have the bass become more `energetic` as the song builds to the chorus.

#### Quote

"I need something that makes assembling my song ideas easier. I could do this myself, but bandmate helps me hear the musical choices much faster."

#### Device & context

Primarily mobile or tablet, in a home office. Sessions are 30–60 minutes, usually early in the morning.

## Revision History

| Version | Date           | Author  | Changes                                       |
| ------- | -------------- | ------- | --------------------------------------------- |
| 1.0     | Feb 5th, 2026  | Michael | Initial Draft                                 |
| 1.1     | May 25th, 2026 | Michael | Deletions and Refinement of Executive Summary |
| 1.2     | June 1st, 2026 | Michael | Adds product line                             |
| 1.3     | June 3rd, 2026 | Michael | Refines P0, adds user personas                |
