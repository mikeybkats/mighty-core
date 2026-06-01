# Bandmate Product Specification

**Version:** 1.0  
**Date:** February 5, 2026  
**Author:** Michael Barakat  
**Status:** Draft

---

## Executive Summary

Bandmate is an adaptive practice partner and creative songwriting tool for musicians.

#### P0 - player / composer

- provides tempo like a drummer or metronome
- listens to the live performance of a solo musician
- recognizes common chord patterns and creates an index of potential **fits** for accompaniment using circle of 5ths. playback choices can be **weighted** to be more or less **random** or go toward a certain kind of musical **feeling**
- likewise with percussion, bandmate will determine appropriate drum patterns according to the chord changes and intervals the musician has selected. Musician can similarly to chord patterns, **weight** the output to be more or less **spontaneous** or **deterministic**.
- generates an accompaniment **track** that matches the musician's input.
- musician uses bandmate to play back accompaniment while they practice along with their instrument
- player can issue commands to bandmate like "stop" or "start"
- four instruments are provided: drums, piano, bass, and guitar
- provides midi export to export projects to ableton, logic, fl studio, or reaper

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

## Revision History

| Version | Date           | Author  | Changes                                       |
| ------- | -------------- | ------- | --------------------------------------------- |
| 1.0     | Feb 5th, 2026  | Michael | Initial Draft                                 |
| 1.1     | May 25th, 2026 | Michael | Deletions and Refinement of Executive Summary |
| 1.2     | June 1st, 2026 | Michael | Adds product line                             |
