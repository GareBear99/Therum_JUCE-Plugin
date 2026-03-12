# v9 UI Component Scaffold

This package adds a fuller JUCE-side editor structure so THERUM is no longer just a processor-first scaffold.

## Added in v9
- Editor state layout plan
- panel shells for:
  - FilterPanel
  - ModPanel
  - FxPanel
  - BrowserPanel
- attachment host scaffold
- editor component ownership scaffold
- more explicit layout zones for top bar / hero row / lower row / diagnostics / mascot

## Honest status
This improves the editor architecture, but the project still requires:
- real compile verification
- component attachments wired for all controls
- final DSP depth
- final sprite animation runtime
- final preset browser UX
