# Stringtime Bellworks (JUCE)

A JUCE-based synthesizer plugin/standalone app focused on struck-object physical modeling.

## Features

- On-screen piano keyboard (plus host MIDI input)
- Physical-modeling inspired mallet excitation with selectable body material:
  - Wood Block
  - Gong
  - Cymbal
  - Glass
  - Aerogel Cathedral (improbable)
  - Neutron Porcelain (improbable)
  - Plasma Ice (improbable)
  - Time Crystal (improbable)
- Built-in preset bank with curated playable patches (including impossible materials)
- ADSR envelope
- Band-pass filter with cutoff + resonance
- Sympathetic resonator string bank (coupled to struck model) for sustained pad-like tails
- E-bow style string exciter for continuous sympathetic sustain
- LFO with patch matrix routing (3 slots) for animated modulation
- FX section:
  - Delay (time, feedback, mix)
  - Reverb (mix, room size, damping)
  - Sympathetic controls: mix, decay, brightness, coupling
  - E-bow controls: drive, focus
  - Modulation controls: LFO rate/depth/waveform + matrix destination/amount per slot

## Build

Requirements:

- CMake 3.22+
- A C++20 compiler
- Xcode command line tools on macOS

Configure and build:

```bash
cmake -S . -B build
cmake --build build --config Release
```

Create a redistributable macOS package zip (AU + VST3 + Standalone):

```bash
cmake -S . -B build
cmake --build build --config Release --target package_macos
```

The generated zip will be placed in `build/dist/` and contains:

- `AU/Stringtime Bellworks.component`
- `VST3/Stringtime Bellworks.vst3`
- `Standalone/Stringtime Bellworks.app`

This project uses CMake `FetchContent` to download JUCE automatically at configure time.

## Notes

This is a pragmatic first version of a physical modeling synth voice based on modal resonances plus mallet-noise excitation, designed to be easy to extend.

## Presets

- Use the Preset dropdown at the top of the plugin UI.
- Selecting a preset updates all synthesis, filter, and FX parameters at once.
- Presets are intentionally varied for quick sound design starting points.
