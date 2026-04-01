# Changelog

## 0.2 — 2026-03-31

- Fix input/output gain structure: unity gain at 12 o'clock (max +24 dB)
- Add plugin name label to UI
- Add 7 tick marks around knobs
- Title-case knob labels (Input, Limit, Output)

## 0.1 — 2026-03-31

- Initial release
- Shure Level Loc (M62/M62V) brickwall limiter emulation
- JFET (2N5458) variable resistor model with even-harmonic distortion
- Peak envelope detector: 500 us attack, 700 ms release
- Transistor (2N5088) soft saturation at high levels
- Three controls: Input, Limit, Output
- AU, VST3, and Standalone formats
- Universal binary (arm64 + x86_64), macOS 11.0+
