> 🎛️ Part of the [TizWildin Plugin Ecosystem](https://garebear99.github.io/TizWildinEntertainmentHUB/) — 19 free audio plugins with a live update dashboard.
>
> [FreeEQ8](https://github.com/GareBear99/FreeEQ8) · [XyloCore](https://github.com/GareBear99/XyloCore) · [Instrudio](https://github.com/GareBear99/Instrudio) · [Therum](https://github.com/GareBear99/Therum_JUCE-Plugin) · [BassMaid](https://github.com/GareBear99/BassMaid) · [SpaceMaid](https://github.com/GareBear99/SpaceMaid) · [GlueMaid](https://github.com/GareBear99/GlueMaid) · [MixMaid](https://github.com/GareBear99/MixMaid) · [MultiMaid](https://github.com/GareBear99/MultiMaid) · [MeterMaid](https://github.com/GareBear99/MeterMaid) · [ChainMaid](https://github.com/GareBear99/ChainMaid) · [PaintMask](https://github.com/GareBear99/PaintMask_Free-JUCE-Plugin) · [WURP](https://github.com/GareBear99/WURP_Toxic-Motion-Engine_JUCE) · [AETHER](https://github.com/GareBear99/AETHER_Choir-Atmosphere-Designer) · [WhisperGate](https://github.com/GareBear99/WhisperGate_Free-JUCE-Plugin) · [RiftWave](https://github.com/GareBear99/RiftWaveSuite_RiftSynth_WaveForm_Lite) · [FreeSampler](https://github.com/GareBear99/FreeSampler_v0.3) · [VF-PlexLab](https://github.com/GareBear99/VF-PlexLab) · [PAP-Forge-Audio](https://github.com/GareBear99/PAP-Forge-Audio)
>
> 🎁 [Free Packs & Samples](#tizwildin-free-sample-packs) — jump to free packs & samples
>
> 🎵 [Awesome Audio](https://github.com/GareBear99/awesome-audio-plugins-dev) — (FREE) Awesome Audio Dev List

# THERUM — Flagship Synthesizer


Therum is a next‑generation wavetable synthesizer designed to push beyond the workflow and usability of modern flagship synths.  
It is built on the architectural foundations developed from the FreeEQ8 ecosystem and expanded into a full synthesis platform.

Therum is designed to be:

• Faster to program than typical wavetable synths  
• Visually intuitive with a dark neon interface  
• Modular but not overwhelming  
• Deep enough for professional sound design  
• Integrated into the wider plugin ecosystem (MixMaid, BassMaid, GlueMaid, SpaceMaid, WaveForm, RiftSynth)

---

# Core Concept

Therum aims to deliver **Serum‑class synthesis with a cleaner architecture and better workflow**.

Where many synths become cluttered or difficult to navigate, Therum focuses on:

• visual clarity  
• macro‑driven modulation  
• simplified wavetable editing  
• integrated spectral tools  
• strong preset architecture

The result is a flagship synth that is powerful without becoming chaotic.

---

# Major Systems

## Oscillators

Therum includes multiple oscillator engines:

• Wavetable oscillator  
• Sample oscillator  
• Granular oscillator  
• Spectral oscillator

Each oscillator can be layered or routed independently.

Features include:

• frame morphing  
• spectral shaping  
• warp modes  
• phase control  
• detune stacks
• sub oscillator support

---

## Modulation System

Therum uses a flexible modulation architecture:

• envelopes  
• LFOs  
• macros  
• performance modulation

Routing is designed to be **visual and fast**, avoiding complicated routing menus.

---

## Filters

Dual filter architecture supporting:

• analog‑style filters  
• digital precision filters  
• multi‑stage routing

Filter types planned:

• ladder
• state variable
• comb
• spectral

---

## Effects Rack

Integrated FX chain including:

• distortion
• delay
• reverb
• chorus
• stereo imaging

The effects rack is designed to work seamlessly with the modulation system.

---

# Mascot System

Therum includes a visual mascot system used during startup and UI interaction.

The mascot:

• appears during splash startup
• reacts to internal states
• can later be extended into animated UI feedback

---

# Preset System

Therum uses a structured preset architecture with metadata tagging.

Preset metadata includes:

• name
• category
• tags
• author
• description

Planned categories:

• bass
• lead
• pad
• pluck
• keys
• FX
• atmosphere

---

# Wavetable Editor (Planned)

Future versions will include an integrated wavetable editor supporting:

• waveform editing
• spectral editing
• frame morphing
• harmonic drawing
• audio import

---

# Plugin Formats

Therum builds as:

• **VST3** — built (first compiled 2026-11-07)  
• **AU** — built (first compiled 2026-11-07), loads in Ableton Live 12 Lite  
• Standalone (optional — not built)

Target hosts:

• Ableton Live (verified: Live 12 Lite, loads + sounds)  
• FL Studio  
• Logic Pro  
• Cubase  
• Reaper (cross-check only after Ableton confirms)

---

# Project Status

**Therum JUCE is the FREE version** (Gary's model, 2026-11-07). **Therum iPlug2 is the FULL / PRO version**, built in parallel, mirroring the free version's progress plus its pro features.

Current state (2026-11-07, local HEAD `41779a9` / `origin/main` `a7059b8`, this doc update on top, unpushed):

• **Builds — first time ever as a plugin:** VST3 + AU, Release, exit 0 (2026-11-07, commit `a7059b8`).
• **Loads in Ableton Live 12 Lite** (AU + VST3 both load; VST3 exposes all 27 parameters). CPU 1% → 2–3%.
• **Sounds — proven:** Gary's saved test (16.86 s, peak −18 dBFS, C4/D4/E4/F4/G4, no stuck notes).
• **Validation PASSED 2026-11-07:** auval SUCCEEDED (exit 0; cold open 74.843 ms; Cocoa view available; all 27 parameters PASS; render PASS 22050–192000 Hz; MIDI PASS; one non-failure warning: multi-channel output without a channel layout). pluginval v1.0.4 strictness 10 SUCCEEDED (exit 0; all tests passed, including editor open/close while processing).
• **Editor window still missing — the blocker:** no editor window materializes in Live at all (AU or VST3) even with the editor toggle checked — Live reports it open but System Events lists only Live's "Untitled" window. CORRECTION 2026-11-07: the earlier "52×20 window titled 'Window'" was a computer-control infrastructure artifact, not the Therum editor. The JUCE 8.0.5 AU wrapper sizing path was checked against source and eliminated as a lead (it sizes the Cocoa view from the editor's 1460×900 bounds). Root cause still open and format-independent.
• **Destructor segfault FIXED as far as Live allows:** fix committed locally (`c0d91ea`), rebuilt + installed on the Mac 2026-11-07 (configure exit 0, build exit 0; new VST3 8,981,648 B + AU 8,673,856 B) — not pushed. Closing the editor in Live now survives for the AU (track 1 toggle OFF, Live alive, no new crash report) and the VST3 (toggle OFF then ON, Live alive, no new crash report); pluginval's in-process editor open/close also passes. Caveat: since no editor window ever appears, it is not 100% certain the editor object is created/destroyed in Live — pluginval is the stronger evidence. Editor-based verification (Morph/ADSR, filters, top-right tables, drop test) still blocked until a window appears.
• **Test suite: 97,380 checks, 0 failures** (re-verified 2026-11-07).

Included in the current release:

• wavetable engine (256 frames × 2048 samples, Serum-format import, mipmaps, Hermite interpolation, crossfade + spectral morph)
• SVF + Moog ladder filters (5 types, selectable in UI)
• Morph + ADSR wired end-to-end
• UI architecture (1460×900 editor — currently no window appears in the host, see above)
• preset system scaffolds
• mascot system scaffolds
• documentation and planning materials (PAPER.md is the DSP specification — kept up to date with the code)

# Free / Full

Therum ships as two versions from one framework-free DSP core (Gary's model, 2026-11-07):

**FREE — Therum JUCE (this repo):**
• 2 wavetable oscs, crossfade morph, WT import
• SVF LP/HP/BP/Notch
• 1 ADSR, macro, 16 voices

**FULL / PRO — Therum iPlug2 (built in parallel, mirroring the free version):**
• Spectral morph (ZeroFundamental/ZeroAll), Moog ladder filter
• Unison (1–16 voices/osc), sub osc, warp modes
• Full mod matrix, MSEG, LFOs beyond 1–2, MPE
• Sample / granular / spectral engines
• Real FX (FDN/Dattorro reverb, delay, chorus, distortion w/ ADAA)
• Wavetable editor / drawing / harmonic edit
• Full preset library (100+), browser, preset packs

---

# Repository Structure

Typical structure:

Source/
- Core
- DSP
- UI
- Preset
- Mascot

Resources/
- Presets
- Sprites

Docs/
- Architecture
- Roadmaps
- Host test matrix
- Wavetable editor plans

---

# Relationship to Other Plugins

Therum is part of a broader plugin ecosystem:

• FreeEQ8  
• MixMaid  
• BassMaid  
• GlueMaid  
• SpaceMaid  
• WaveForm  
• RiftSynth

Each plugin has a specific role while Therum acts as the **flagship synthesis engine**.

---

# License

Open source scaffold with optional commercial distribution.

**No LICENSE file in the repo yet — the licence is undecided.** (Noted 2026-11-07.)

Future builds may introduce:

• official releases
• preset packs
• optional expansion systems

---

# Development

This project is currently in **workshop stage — the JUCE free version builds, loads and sounds; the editor is the blocker**.

Current work (2026-11-07):

1. **Fix the editor lifecycle (the blocker)** — 52×20 window + destructor segfault; destructor fix built + installed on the Mac 2026-11-07, Live restart + re-test pending
2. Re-test in Live: editor open/render, top-right tables, dropping a sound onto them, Morph/ADSR and filter switching, VST3 editor
3. pluginval strictness 10 + auval

Next development milestones:

1. Implement full modulation matrix (the evaluator is never called today)
2. Unison (1–16 voices/osc) + sub osc
3. Warp modes
4. Real FX rack (FDN/Dattorro reverb, delay, chorus, distortion w/ ADAA)
5. Implement granular and spectral engines
6. Build wavetable editor
7. Complete preset browser UX + full preset library (100+)
8. Host testing and optimization

---

# Vision

Therum is intended to become a **modern flagship synthesizer platform** that combines:

• powerful sound design
• intuitive workflow
• modular architecture
• strong visual identity

## TizWildin FREE sample packs

| Pack | Description |
|------|-------------|
| [**TizWildin-Aurora**](https://github.com/GareBear99/TizWildin-Aurora) | 3-segment original synth melody pack with loops, stems, demo renders, and neon/cinematic phrasing |
| [**TizWildin-Obsidian**](https://github.com/GareBear99/TizWildin-Obsidian) | Dark cinematic sample pack with choir textures, menu loops, transitions, bass, atmosphere, drums, and electric-banjo extensions |
| [**TizWildin-Skyline**](https://github.com/GareBear99/TizWildin-Skyline) | 30 BPM-tagged synthwave and darkwave loops with generator snapshot and dark neon additions |
| [**TizWildin-Chroma**](https://github.com/GareBear99/TizWildin-Chroma) | Multi-segment game synthwave loop sample pack from TizWildin Entertainment |
| [**TizWildin-Chime**](https://github.com/GareBear99/TizWildin-Chime) | Multi-part 88 BPM chime collection spanning glass, void, halo, reed, and neon synthwave lanes |
| [**Free Violin Synth Sample Kit**](https://github.com/GareBear99/Free-Violin-Synth-Sample-Kit) | Physical-model violin sample kit rendered from the Instrudio violin instrument |
| [**Free Dark Piano Sound Kit**](https://github.com/GareBear99/Free-Dark-Piano-Sound-Kit) | 88 piano notes + dark/cinematic loops and MIDI |
| [**Free 808 Producer Kit**](https://github.com/GareBear99/Free-808-Producer-Kit) | 94 hand-crafted 808 bass samples tuned to every chromatic key |
| [**Free Riser Producer Kit**](https://github.com/GareBear99/Free-Riser-Producer-Kit) | 115+ risers and 63 downlifters - noise, synth, drum, FX, cinematic |
| [**Phonk Producer Toolkit**](https://github.com/GareBear99/Phonk_Producer_Toolkit) | Drift phonk starter kit - 808s, cowbells, drums, MIDI, templates |
| [**Free Future Bass Producer Kit**](https://github.com/GareBear99/Free-Future-Bass-Producer-Kit) | Loops, fills, drums, bass, synths, pads, and FX |

### Related audio projects
- [**VF-PlexLab**](https://github.com/GareBear99/VF-PlexLab) - VocalForge PersonaPlex Lab starter repo for a JUCE plugin + local backend + HTML tester around NVIDIA PersonaPlex.
- [**PAP-Forge-Audio**](https://github.com/GareBear99/PAP-Forge-Audio) - Procedural Autonomous Plugins runtime for generating, branching, validating, and restoring plugin projects from natural-language sound intent.

