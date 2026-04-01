# Changelog

## 0.3 — 2026-04-01

- Add input transformer model: HPF at ~80 Hz + asymmetric soft saturation
- Add output transformer model: saturation varies with gain reduction (no buffer stage)
- Add sidechain HPF at ~18 Hz (2 uF AC-coupling cap in detector path)
- Increase compression ratio from 7:1 to 20:1 (closer to original brickwall behavior)
- Attack time now varies with limit setting (800 us at 0% to 300 us at 100%)
- Lower default limit from 50 to 25 for subtler out-of-box behavior
- Larger plugin name and knob labels
- Add package script for DMG distribution

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
