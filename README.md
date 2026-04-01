# Loc-Box

![Loc-Box](docs/Loc-Box.png)

A digital emulation of the **Shure Level Loc (M62/M62V)** brickwall limiter, built as an AU/VST3 audio plugin with [JUCE](https://juce.com/).

The Level Loc is a discrete transistor limiter from the late 1960s, originally designed for PA and podium microphone use. It became a cult favorite in music production for its aggressive, pumping compression character.

## Controls

| Knob | Range | Description |
|------|-------|-------------|
| **INPUT** | 0 -- 100% | Signal level into the limiter. Audio taper, up to +24 dB. |
| **LIMIT** | 0 -- 100% | Compression amount. At 0% the threshold sits at 0 dBFS (no limiting); at 100% it drops to -24 dBFS (heavy brickwall compression). |
| **OUTPUT** | 0 -- 100% | Makeup gain. Audio taper, up to +24 dB. |

## What it models

- **Input transformer** -- cheap transformer LF roll-off (~80 Hz HPF) and asymmetric soft saturation. Per Dan Korneff: "the transformers are a huge part of this sound."
- **JFET gain element (2N5458)** -- voltage-controlled variable resistor with level-dependent even-harmonic distortion that increases with gain reduction.
- **Sidechain AC coupling** -- HPF at ~18 Hz from the 2 uF cap in the detector path, so the limiter is less sensitive to sub-bass.
- **Peak envelope detector** -- full-wave rectifier into an RC network with variable attack (800--300 us, faster at higher limit settings) and 700 ms release.
- **Brickwall compression** -- ~20:1 ratio. The original's output is "locked" above threshold.
- **Output transformer** -- saturation that varies with gain reduction. No buffer stage: the FET drives a step-up transformer directly, so impedance changes with compression.

## Building

Requires CMake 3.22+, Ninja, and JUCE (expected at `/Users/chris/src/github/JUCE`).

```bash
# Configure
cmake -B build -G Ninja \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=$(xcrun -f clang) \
    -DCMAKE_CXX_COMPILER=$(xcrun -f clang++)

# Build
cmake --build build --config Release
```

Artifacts land in `build/LocBox_artefacts/Release/`:
- `AU/Loc-Box.component`
- `VST3/Loc-Box.vst3`
- `Standalone/Loc-Box.app`

## Install

```bash
# AU
cp -R build/LocBox_artefacts/Release/AU/Loc-Box.component \
      ~/Library/Audio/Plug-Ins/Components/
codesign --force --sign "-" ~/Library/Audio/Plug-Ins/Components/Loc-Box.component

# VST3
cp -R build/LocBox_artefacts/Release/VST3/Loc-Box.vst3 \
      ~/Library/Audio/Plug-Ins/VST3/
codesign --force --sign "-" ~/Library/Audio/Plug-Ins/VST3/Loc-Box.vst3
```

Validate the AU with:

```bash
auval -v aufx LBOX CVDA
```

## License

GPL-3.0 -- see [LICENSE](LICENSE).
