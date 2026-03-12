# v10 Control Wiring Status

This package adds the first real control-layer bridge between editor and processor state.

## Added in v10
- actual control widgets inside key panels
- APVTS attachment ownership scaffold
- controls for:
  - master gain
  - osc A wavetable position
  - filter cutoff
  - macro 1
  - FX delay mix
- panel classes now own JUCE sliders/labels
- Browser panel gets placeholder preset entries
- Diagnostics panel text still driven from editor timer

## Honest state
This is a stronger prototype/editor scaffold, not a production-finished flagship release.

Still missing:
- full control coverage
- complete attachment map
- final DSP parity
- compile-verified release project
- full preset browser workflow
- final mascot sprite animation runtime
