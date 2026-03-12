# Host Test Matrix

## Platforms
- macOS Intel
- macOS Apple Silicon
- Windows 10/11

## Formats
- AU
- VST3
- Standalone (optional)

## DAWs
- Ableton Live
- FL Studio
- Reaper
- Logic Pro
- Cubase

## Smoke Tests
1. Plugin scans successfully
2. Opens editor with mascot splash
3. Loads Init Patch
4. MIDI note on/off works
5. Parameter changes persist
6. Preset save/load roundtrip works
7. No crash on close/reopen
8. Multiple instances open safely
9. Automation on master/osc/filter works
10. CPU remains reasonable at 8/16 voices
