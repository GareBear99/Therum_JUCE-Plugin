# Therum DSP core tests

Standalone unit tests for the framework-free DSP core (`../Source/DSP/`).
No JUCE, no iPlug2, no DAW, no test framework — plain C++ and a `main()`.

## Run

```
make
```

That first checks the DSP has no framework includes (`grep` for `juce` /
`iplug` / `IPlug` / `JuceHeader` — it must find nothing), then builds and
runs the tests. Exit code 0 means every check passed.

A plain build without the check is `make test_core && ./test_core`.

## What is covered

| Test | What it proves (paper §6 rank 1) |
|---|---|
| `testReleaseTail` | The release actually sounds: the voice stays active after note-off, renders most of its ~9700-sample release audibly, then frees itself and renders silence. Tail length reports 0.22 s. |
| `testVoiceStealingRoundRobin` | Stealing is round-robin: consecutive steals take consecutive voices (notes 60, 61, 62), not voice 0 every time. |
| `testDuplicateNoteRetrigger` | A note-on for a note already sounding retriggers that voice; the active count stays 1. |
| `testPhaseReset` | A retriggered voice produces the same first sample as a fresh voice: phase and filter state reset on start. |
| `testCutoffClamp` | Cutoffs of 0, -100, 30000, 1e9 and NaN produce only finite samples and do not break the filter. |
| `testSmoothing` | The one-pole smoother glides instead of jumping, and arrives at its target. Through the voice manager, a cutoff move from 18000 to 100 Hz leaves the output loud at first and cuts it once settled. |
| `testLimiter` | All 16 voices at full velocity (and at +12 dB master) never exceed 0.99. Quiet signals pass through untouched. |
