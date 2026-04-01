# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

**Configure** (once, or after CMakeLists.txt changes):
```bash
/opt/homebrew/bin/cmake -B build -G Ninja \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=$(xcrun -f clang) \
    -DCMAKE_CXX_COMPILER=$(xcrun -f clang++)
```

**Build:**
```bash
/opt/homebrew/bin/cmake --build build --config Release
```

**Install + sign:**
```bash
cp -R "build/LocBox_artefacts/Release/AU/LocBox.component" \
      "$HOME/Library/Audio/Plug-Ins/Components/"
codesign --force --sign "-" "$HOME/Library/Audio/Plug-Ins/Components/LocBox.component"

cp -R "build/LocBox_artefacts/Release/VST3/LocBox.vst3" \
      "$HOME/Library/Audio/Plug-Ins/VST3/"
codesign --force --sign "-" "$HOME/Library/Audio/Plug-Ins/VST3/LocBox.vst3"
```

**Validate AU:**
```bash
auval -v aufx LBOX CVDA
```

There are no automated tests. `auval` is the primary correctness check.

## Architecture

Standard JUCE 4-file plugin split:

- `src/PluginProcessor.h/.cpp` — `LocBoxAudioProcessor`: all DSP, APVTS, state serialisation.
- `src/PluginEditor.h/.cpp` — `LocBoxAudioProcessorEditor` (320x200 px) + `BlackKnobLookAndFeel`.

**Signal chain:**
```
Input -> [Input Gain (audio taper)] -> [JFET Limiter (envelope -> gain reduction)] -> [Transistor soft clip] -> [Output Gain] -> Output
```

The JFET limiter models the Shure Level Loc M62/M62V circuit:
- Peak envelope detector with 500 us attack, 700 ms release
- Brickwall compression ratio ~7:1 (40 dB input change -> ~6 dB output change)
- JFET (2N5458) nonlinearity: even-harmonic distortion proportional to gain reduction
- Transistor stage (2N5088) soft saturation at high levels (3% THD spec)

**Parameters:**
- `input`  [0-100, skew centre 25, default 50] -> gain `(v/100)^2 * 16.0` (max +24 dB)
- `limit`  [0-100, linear, default 50]         -> threshold `-48 * (v/100)` dBFS
- `output` [0-100, skew centre 35, default 50] -> gain `(v/100)^2 * 16.0` (max +24 dB)

**SmoothedValue** — one per channel per parameter (3x2 = 6 instances), targets set per block, advanced per sample.

## Plugin Identity

| Field | Value |
|---|---|
| Product name | Loc-Box |
| CMake target | LocBox |
| Manufacturer code | CVDA |
| Plugin code | LBOX |
| AU type | aufx (kAudioUnitType_Effect) |
| AU export prefix | LocBoxAU |
| Bundle ID | com.CorvidAudio.LocBox |
| JUCE path | `/Users/chris/src/github/JUCE` |
| Formats | AU, VST3, Standalone |
