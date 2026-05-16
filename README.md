# SONIX — generative music engine

A terminal-based algorithmic music engine written in C. Synthesizes sound
in real-time using additive waveforms, ADSR envelopes, a step sequencer,
Euclidean rhythm generation, and a Markov chain melody engine. Audio pipes
directly to ffplay.

```
  ╔══════════════════════════════════════════════════════════╗
  ║   S O N I X  —  generative music engine                 ║
  ╚══════════════════════════════════════════════════════════╝

  BPM: 120   SCALE: PENTATONIC   ROOT: C   TICK: 48   ▶ PLAYING

  TRACKS
  ──────────────────────────────────────────────────────────
  ▶ KICK   ■ · · · ■ · · ·  ■ · · · ■ · · ·  ████░░░░  NOISE
    BASS   · ■ · · · · ■ ·  · ■ · · · · · ■  ██░░░░░░  SAW   D3
    LEAD   · · ■ · · ■ · ·  ■ · · · · ■ · ·  ███░░░░░  SQUARE G4
    PAD    ■ · · · · · · ·  · · · · · · · ·  █░░░░░░░  SINE  C4
    ARPG   · ■ · ■ · · ■ ·  · ■ · ■ · · · ■  ██░░░░░░  TRIANGLE
    PERC   ■ · · ■ · ■ · ·  ■ · · ■ · ■ · ·  ███░░░░░  NOISE
```

## Requirements

- `gcc`
- `ffplay` (from ffmpeg — provides audio output)
- A terminal with ANSI color support

Install ffmpeg on Ubuntu/Debian:
```bash
sudo apt install ffmpeg
```

On macOS:
```bash
brew install ffmpeg
```

## Build

```bash
make
```

## Run

```bash
./sonix
```

Press `space` to start playing immediately.

## Controls

| Key       | Action                              |
|-----------|-------------------------------------|
| `space`   | Play / Stop                         |
| `g`       | Generate a new groove               |
| `e`       | Auto-evolve (mutate patterns live)  |
| `r`       | Randomize selected track            |
| `m`       | Mute / unmute selected track        |
| `w`       | Cycle waveform on selected track    |
| `s`       | Cycle musical scale                 |
| `o`       | Cycle octave on selected track      |
| `+` / `-` | Increase / decrease BPM             |
| `↑` / `↓` | Select track                        |
| `q`       | Quit                                |

## Project structure

```
sonix/
├── Makefile
├── README.md
├── include/
│   ├── sonix.h        base types, constants, shared structs
│   ├── synth.h        synthesizer voice definitions
│   ├── sequencer.h    step sequencer types and API
│   ├── generative.h   algorithmic pattern generator API
│   ├── audio.h        audio output (ffplay pipe)
│   ├── ui.h           terminal UI renderer
│   └── input.h        keyboard key constants
└── src/
    ├── main.c         event loop, keyboard input, timing
    ├── synth.c        waveform oscillators + ADSR envelopes
    ├── sequencer.c    beat clock, pattern scheduling
    ├── generative.c   Euclidean rhythms + Markov melodies
    ├── audio.c        PCM buffer pipe to ffplay
    └── ui.c           live terminal display with VU meters
```

## How it works

**Synthesis** (`synth.c`)
Each track has a voice rendered at 44100 Hz. Waveforms available: sine,
square, sawtooth, triangle, and noise. Each voice is shaped by a four-stage
ADSR envelope (attack, decay, sustain, release). All active voices are
mixed into a single 16-bit mono PCM buffer per tick.

**Sequencer** (`sequencer.c`)
A 16-step grid advances at BPM/4 resolution. Each step stores a MIDI note
number, velocity, and duration. Up to 8 tracks run in parallel, each with
8 swappable pattern slots.

**Generative engine** (`generative.c`)
Two algorithms drive pattern creation:

- *Euclidean rhythms* — distributes N pulses as evenly as possible across
  16 steps. This algorithm appears in traditional music from West Africa,
  the Middle East, and Latin America. Calling `[e]` will re-apply it.

- *Markov chain melodies* — a hand-crafted 12×12 transition matrix between
  the 12 pitch classes, biased toward tonal motion (stepwise and common
  harmonic intervals). Each new note is picked probabilistically from the
  current pitch class row.

**Auto-evolve** — pressing `[e]` triggers `gen_auto_evolve`, which picks a
random track and regenerates its pattern. Every 64 ticks the BPM also drifts
slightly, so a running session changes character over time without input.

**Audio output** (`audio.c`)
The synth writes raw signed 16-bit little-endian mono PCM at 44100 Hz into
a pipe opened with `popen("ffplay ...")`. No ALSA or PulseAudio headers
required — ffplay handles device negotiation transparently.

**Terminal UI** (`ui.c`)
Redraws at ~20 fps using ANSI escape codes. Each track row shows:
- the 16-step grid with the current playhead highlighted
- a VU meter (green → yellow → red) driven by RMS of the mix
- the active waveform type
- the current note name when a voice is sounding
