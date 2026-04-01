# Requirements

## Overview

Loc-Box is a digital emulation of the Shure Level Loc (M62/M62V), a discrete transistor brickwall limiter from the late 1960s. The plugin targets AU and VST3 formats on macOS.

## Reference hardware

- **Shure M62 / M62V Level Loc**
- Original use case: automatic gain control for PA/podium microphones
- Production use: aggressive, pumping brickwall compression (popularized by Tchad Blake and others)

### Original circuit summary

| Stage | Components | Role |
|-------|-----------|------|
| Input | Transformer T1, impedance switch | Balanced mic-level input |
| Gain control | 2N5458 N-channel JFET (Q9) | Voltage-controlled variable resistor in audio path |
| Amplifier | 2N5088 NPN transistors (Q1-Q5, Q8) | Discrete gain stages with Darlington buffer |
| Sidechain | TIS93 PNP transistors (Q6, Q7), diodes | Phase splitter + full-wave rectifier driving JFET gate |
| Timing | 39 ohm charge resistors, 1M ohm discharge, 2 uF capacitor | 500 us attack, 700 ms release |
| Threshold | Distance selector switch (S4) | 3-position voltage divider: 0 / -6 / -12 dB |
| Output | Transformer T2 | Balanced mic-level output |

### Key specifications (from Shure documentation)

- Compression ratio: ~infinite (40 dB input change -> 6 dB output change)
- Attack time: 500 us (for 20 dB step, within 2 dB of final value)
- Release time: 700 ms (for 20 dB step decrease)
- THD: 3% maximum at any level of regulation
- Frequency response: 20 Hz -- 20 kHz +/- 2 dB

## Functional requirements

### F1: Signal processing

- **F1.1** Input gain stage with audio taper (squared law), 0 to +24 dB.
- **F1.2** JFET limiter with peak envelope detection.
  - Attack time constant: 500 us.
  - Release time constant: 700 ms.
  - Compression ratio: approximately 7:1 (brickwall behavior).
- **F1.3** JFET nonlinearity modeling: even-harmonic distortion proportional to gain reduction, simulating the 2N5458 variable resistor behavior.
- **F1.4** Transistor soft saturation at high signal levels, modeling the 2N5088 amplifier stages.
- **F1.5** Output gain stage with audio taper (squared law), 0 to +24 dB.
- **F1.6** All gain and threshold parameters smoothed per-sample to avoid zipper noise.

### F2: Parameters

| ID | Name | Range | Default | Mapping |
|----|------|-------|---------|---------|
| `input` | Input | 0 -- 100 | 50 | `(v/100)^2 * 16.0` (audio taper, max +24 dB) |
| `limit` | Limit | 0 -- 100 | 50 | Threshold = `-48 * (v/100)` dBFS |
| `output` | Output | 0 -- 100 | 50 | `(v/100)^2 * 16.0` (audio taper, max +24 dB) |

- Input and output knobs use skewed ranges (centres at 25 and 35 respectively) for finer control at lower gain values.
- Limit is linear: 0% = no limiting (0 dBFS threshold), 100% = heavy limiting (-48 dBFS threshold).

### F3: Plugin formats and compatibility

- **F3.1** AU (Audio Unit) format, type `aufx`.
- **F3.2** VST3 format.
- **F3.3** Standalone application.
- **F3.4** macOS 11.0+ (Big Sur and later).
- **F3.5** Universal binary (arm64 + x86_64).
- **F3.6** Mono and stereo bus layouts supported; input/output channel counts must match.

### F4: User interface

- **F4.1** Fixed window size: 320 x 200 pixels.
- **F4.2** Three rotary knobs (INPUT, LIMIT, OUTPUT) with labels.
- **F4.3** Matte black knob style with white indicator line, drop shadow, and subtle top highlight.
- **F4.4** Silver-grey panel background (`#d8d8d8`) with subtle top-to-bottom gradient overlay.
- **F4.5** Bold uppercase labels, dark color (`#111111`), 10 pt.

### F5: State management

- **F5.1** Full preset recall via APVTS XML serialization.
- **F5.2** DAW session save/restore for all parameters.

## Non-functional requirements

- **NF1** Zero latency (no lookahead or FFT processing).
- **NF2** Tail length declared as 0.7 s (envelope release time) so hosts do not cut audio prematurely after input stops.
- **NF3** Denormal protection via `ScopedNoDenormals` in the process loop.
- **NF4** Must pass `auval` validation (`auval -v aufx LBOX CVDA`).

## Plugin identity

| Field | Value |
|-------|-------|
| Product name | Loc-Box |
| CMake target | LocBox |
| Manufacturer code | CVDA |
| Plugin code | LBOX |
| AU export prefix | LocBoxAU |
| Bundle ID | com.CorvidAudio.LocBox |
| Company | CorvidAudio |
