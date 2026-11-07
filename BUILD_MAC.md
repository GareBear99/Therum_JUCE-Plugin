# Therum — Mac build check (2026-11-07)

First time Therum has ever compiled as a plugin anywhere.

## Result: BUILDS — VST3 + AU, Release, EXIT 0

- Mac: Gary's Mac mini (2018 Intel, macOS 15.8.1), Xcode CLT
  (`/Library/Developer/CommandLineTools`), AppleClang 17.0.0
- JUCE 8.0.5 at `/Users/gary/JUCE` (not vendored; `JUCE_DIR` cache var)
- CMake 4.4.5 at `/Users/gary/cmake-4.4.5-macos-universal/CMake.app/Contents/bin/cmake`

## How to build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release   # -DJUCE_DIR=<path> to override
cmake --build build --config Release -j 6
```

Configure took ~94 s (mostly building JUCE's `juceaide`); full build ~5 min.

## Bundles (built 2026-11-07 ~00:34 PST)

- `/Users/gary/Therum_JUCE-Plugin/build/Therum_artefacts/Release/VST3/Therum.vst3`
  — binary `Contents/MacOS/Therum`, 8,977,440 bytes, x86_64, ad-hoc signed
- `/Users/gary/Therum_JUCE-Plugin/build/Therum_artefacts/Release/AU/Therum.component`
  — binary `Contents/MacOS/Therum`, 8,673,776 bytes, x86_64, ad-hoc signed
- `COPY_PLUGIN_AFTER_BUILD` also installed both to
  `~/Library/Audio/Plug-Ins/VST3/Therum.vst3` and
  `~/Library/Audio/Plug-Ins/Components/Therum.component` (same sizes),
  so Ableton Live 12 Lite can pick them up. Next step: DAW verification
  (load in Live, confirm the editor opens and sound comes out) — NOT done here.

## What was wired up (this commit)

- New top-level `CMakeLists.txt`: `juce_add_plugin(Therum ... FORMATS VST3 AU)`,
  all 42 `.cpp` files from both split trees, include roots
  `Therum-Core-Engine/Source` + `Therum-UI-Editor/Source`,
  `juce_generate_juce_header` (for the Projucer-style `<JuceHeader.h>`),
  links `juce_audio_utils`, `juce_audio_processors`, `juce_gui_basics`,
  `juce_dsp`. `JUCE_DIR` is a cache variable defaulting to `/Users/gary/JUCE`.
- Cross-tree include fixes (mechanical path changes only, no logic):
  `TherumPluginEditor.h` (`../UI/...` -> `UI/...`),
  `TherumAttachments.cpp` (`../Core/...` -> `Core/...`),
  `FilterPanel.cpp` / `OscPanel.cpp` (`../../DSP/...` -> `DSP/...`),
  `MascotPanel.h` (`../../Mascot/...` -> `Mascot/...`).
- Added the standard JUCE entry point `createPluginFilter()` at the end of
  `TherumPluginProcessor.cpp` (it did not exist anywhere in the repo).
- No source files deleted, no DSP/UI code written, no files excluded from
  the target. The old scaffold `Therum-Core-Engine/CMakeLists.txt` is
  untouched (no build target there by design; this root file is the entry).

## Checks

- `make check` in `Therum-Core-Engine/Tests/`: "DSP core: no framework
  includes" — DSP still framework-free after these changes.
- Compile errors fixed during the build: none. First configure and first
  build both passed clean (EXIT 0). Only warnings: `juce::Font(float)`
  deprecated-constructor warnings in several panels (pre-existing style,
  harmless), plus `ranlib: ... has no symbols` notes for the stub objects
  (`FxRack`, `DualFilter`, `ModMatrix`, `GranularOscillator`,
  `SpectralOscillator`, `PunchReactorState` — empty/stub .cpps, expected).
- Full logs on the Mac: `/Users/gary/therum-configure.log`,
  `/Users/gary/therum-build.log`.
