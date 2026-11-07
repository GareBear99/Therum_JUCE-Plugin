# Therum: A Serum-2-Quality Hybrid Synthesizer in Framework-Free C++

**Gary Doman** (GareBear99 / TizWildin)
Therum / FreeEQ8 Open-Source DSP Project
https://github.com/GareBear99/Therum_JUCE-Plugin

---

## Abstract

This paper specifies the digital signal processing (DSP) architecture required to build Therum — a hybrid synthesizer — to Serum 2 quality or better, on chosen axes, from a framework-free C++ core. It is written as a companion to the FreeEQ8 paper [1], and it carries that paper's method forward: every architectural choice is tied to a cited source, every measured number comes from a cited measurement or from a measurement Therum will make, and every claim is labelled as proven or planned. Nothing here is asserted on authority alone.

The bar is Serum 2 (Xfer Records, released 18 March 2025, $249, free for Serum 1 owners): three oscillators, each running one of five engines (wavetable, sample, multisample, granular, spectral), 256-frame wavetables with smooth interpolation, dual warp, two filters with drawable shapes, ten LFOs, four envelopes, eight macros, a modular dual-bus FX rack, and 626 factory presets [2, 3, 4]. Matching Serum 2 feature-for-feature is a multi-year team project. Therum's winnable game is chosen superiority: a wavetable engine that is measurably cleaner (band-limited by construction, verified by a noise-to-mask alias test rather than a bare dB number), a filter set whose decramping is actually implemented and measured (the FreeEQ8 paper proved the SVF does not decramp — this paper specifies the fix as a tested hypothesis, not a claim), a modulation system that is faster to program, and a price of free.

The paper is organised as follows. §1 states the target and the axes on which Therum beats Serum 2. §2 documents the bar with evidence. §3 states honestly what Therum is today — a plugin that builds, loads and sounds, with a broken editor. §§4–5 specify the DSP architecture area by area, with the papers behind each choice and the key formulas. §6 separates what is proven from what is planned. §7 gives the ranked build order with dependencies. §8 gives the measurement plan — the tests and benchmarks that will prove Serum 2 quality or better, not merely assert it. §9 lists the limitations and the claims this paper could not verify. §10 states the long-term direction.

---

## 1. Introduction

Therum began as a JUCE wavetable synthesizer scaffold. It is not one any more. As of 2026-11-07 the repository contains 9,578 lines of C++ (excluding build artefacts), with a framework-free DSP core and a 97,380-check test suite (0 failures), and it has compiled as a plugin for the first time: VST3 + AU, Release, exit 0 (commit `a7059b8`, pushed to `origin/main`). It loads in Ableton Live 12 Lite and it sounds — proven with a recorded test (16.86 s, peak −18 dBFS, C4/D4/E4/F4/G4, no stuck notes). The editor is broken (the blocker): it opens at 52×20 px instead of the 1460×900 the code specifies, and closing it crashes the host. §3 gives the full honest inventory. The documentation in the repository says this consistently and honestly; the HUB catalogue calling it "production" is wrong and should be fixed [5].

[Correction 2026-11-07: the v1.0 text said Therum "has never compiled as a plugin: no JUCE is wired, no build target exists, the include paths across its split folders do not resolve, and there is no plugin entry point." That was true when written; it is superseded. JUCE is wired (top-level `CMakeLists.txt`, `juce_add_plugin`, 42 .cpp files), the include paths resolve, `createPluginFilter()` exists, and the plugin builds. The v1.0 inventory figure (1,748 lines of C++, 568 DSP) is likewise superseded — see §3.]

The framework model is Gary's (2026-11-07), and it refines the migration plan [5]: Therum JUCE is the FREE version — workshopped, kept as the free tier. Therum iPlug2 is the FULL / PRO version — built in parallel, mirroring the free version's progress plus its pro features. This mirrors the FreeVox8 (JUCE, free) / Vox8 (iPlug2, paid) split. The framework-free plain C++ DSP core is what makes the parallel build possible: the core is shared, only the shell differs. iPlug2 carries a zlib-like licence (free for closed-source commercial use, no tier or cap), ships VST3, AUv2, AUv3, CLAP, AAX, Standalone and Web targets, and ships almost no DSP helpers — no FFT, no convolution, no filter library, no SIMD helpers [6]. That last fact is the point of this paper: Therum must bring its own DSP, and this paper specifies what that DSP is, from the literature, before a line of it is written.

[Correction 2026-11-07: the v1.0 text said Therum "leaves JUCE entirely and is built on iPlug2." That is superseded — JUCE is the free version and there is new JUCE work (workshopping the free tier). The free/full split is: FREE = 2 wavetable oscs, crossfade morph, WT import, SVF LP/HP/BP/Notch, 1 ADSR, macro, 16 voices. FULL/PRO = Spectral morph (ZeroFundamental/ZeroAll), Moog ladder filter, Unison (1–16 voices/osc), sub osc, warp modes, Full mod matrix, MSEG, LFOs beyond 1–2, MPE, Sample / granular / spectral engines, Real FX (FDN/Dattorro reverb, delay, chorus, distortion w/ ADAA), Wavetable editor / drawing / harmonic edit, Full preset library (100+), browser, preset packs.]

The FreeEQ8 paper [1] is the model for how this paper treats evidence. That paper measured its claims (SVF vs RBJ magnitude response to 0.0000 dB, CPU in ns/sample, 0 data tears across 239 million samples on a SPSC triple-buffer) and, when review on the JUCE forum and r/DSP showed its decramping claims were wrong, it withdrew them in print: the Simper SVF does not decramp, both topologies measure identical, and a −3.383 dB cramping error remains at the geometric-mean frequency [1, §2.4]. This paper does the same. Where a choice is proven by a paper, it says so and cites it. Where a choice is a plan, it says "planned". Where the literature could not be verified, §9 says so. A claim of Serum 2 quality that is not backed by a measurement is a marketing line, not an engineering result, and this paper does not contain one.

### 1.1 What "Serum 2 quality or better" means here

"Better" is not "more features". Serum 2 ships 626 presets, 288 wavetables, five engines, an arpeggiator and a clip sequencer [2, 3]. Therum will not match that list on day one, and this paper does not pretend it will. "Better" means, on axes chosen deliberately:

1. **Cleaner wavetable oscillator.** Serum's wavetable engine is respected, but its warps modify the phase used to read the table, which destroys the mipmap's band-limiting — reported behaviour is that Serum aliases under warp [7]. Therum's wavetable oscillator is band-limited by construction (mipmapped tables, §4.1) and its warps are band-limited too (§4.4), verified by a noise-to-mask alias test (§8), not by listening alone.
2. **Filters that actually decramp.** The FreeEQ8 paper proved the SVF does not decramp and quantified the remaining error (−3.383 dB at the geometric mean of fc and Nyquist, 16 kHz at 44.1 kHz) [1, §2.4]. Therum implements a decramping method — the geometric-mean 5th constraint, Orfanidis, or Särkkä–Huovilainen — and measures the result against the analogue prototype (§4.5). No filter in Therum is described as "decramped" until that measurement exists.
3. **Faster to program.** Serum's workflow crown is drag-to-modulate [3]. Therum matches it (drag any source onto any target, animated depth rings) and adds a modulation system whose routing is visible in one place, not spread across tabs (§4.7).
4. **Free.** Serum 2 is $249 [2, 3]. Therum is free. That is itself a feature, and it is the only axis on which Therum beats Serum 2 from the first release.
5. **Ecosystem.** Preset and pack sharing through the TizWildin hub, and integration with the MixMaid / BassMaid / GlueMaid / SpaceMaid family [5]. This is a differentiator, not a DSP claim, and it is listed here so the build order (§7) does not mistake it for one.

Axes Therum does **not** claim on day one: preset count, multisample library depth, arpeggiator/sequencer, or spectral-engine parity. Those are later phases (§7), and the paper labels them as such.

---

## 2. The bar: Serum 2 (evidence)

Serum 2 shipped 18 March 2025 (Dubspot [3]; Xfer's product page confirms it as the current version, free upgrade for Serum 1 owners with lifetime free updates [2]). Price $249 for new buyers ($189 introductory; $9.99/month rent-to-own on Splice) [3, 4]. Formats VST3, AU, AAX 64-bit, Windows and macOS [2].

| Feature | Serum 2 | Source |
|---|---|---|
| Oscillators | 3 slots × 5 engines: Wavetable, Sample, Multisample, Granular, Spectral | [2, 3, 4] |
| Wavetable | 256-frame tables (2048 samples/frame), smooth interpolation mode (near-infinite frame positions, no morph needed), warp modes incl. dual warp, FM, PD, ring mod, distortion | [2, 3] |
| Unison | Up to 16 voices per oscillator, detune + stereo spread + phase | [3, 4] |
| Filters | 2 independent, series/parallel, PZSVF draw-your-own filter shapes, 11+ new types, mixer page with routing | [3, 4] |
| Modulation | 10 LFOs (incl. 2D, chaos, path), 4 envelopes, 8 macros, drag-to-modulate, MPE | [2, 3, 4] |
| FX | Modular rack, multiple instances of the same effect, dual buses, convolution reverb, Bode shifter, M/S + Low/High splitters | [3, 4] |
| Sequencing | Arpeggiator + clip sequencer (12 slots, per-pattern automation) | [3] |
| Presets / tables | 626+ presets, 288 wavetables shipped | [2] |
| UI | Resizable, tabbed pages, visual waveform editing | [3] |

Two facts from Xfer's own page are worth recording because they shape Therum's design. First, the wavetable oscillator's "Smooth Interpolation mode allows for near-infinite frame positions in the wavetable (no morph needed)" [2] — Serum 2's morphing is a product feature with **no published algorithm**; Therum targets the behaviour (spectral morph, §4.1) and does not claim to reproduce Serum's method [7]. Second, the granular oscillator and the spectral oscillator (real-time harmonic resynthesis with transient detection "similar to that found in advanced timestretching algorithms" [2]) are the long tail — they are the last engines in Therum's build order, not the first (§7).

Serum 2's price and release date are stated here from secondary sources (Dubspot [3], Plugin Boutique / equipboard summaries [4]) — Xfer's own page does not list a price. That is recorded as a sourcing limitation, not hidden.

---

## 3. Therum today (honest inventory)

The full inventory is in the migration plan [5]; the DSP-relevant facts are:

| Subsystem | State | Evidence |
|---|---|---|
| Wavetable osc | Real | 256 frames × 2048 samples, Serum-format import, mipmaps at load (1 per 3 semitones, crossfaded), Hermite interpolation, crossfade + spectral morph. Morph + ADSR wired end-to-end (1b22878). Warp modes and unison not built. |
| Sample osc | Real but orphaned, minimal | Nearest-sample playback, no interpolation, no pitch mapping, no data ever loaded. |
| Granular osc | Stub — returns silence | `process() { return 0.0f; }`. No FFT anywhere in the repo. |
| Spectral osc | Stub — returns silence | Same. No FFT anywhere in the repo. |
| Noise osc | Real, minimal, orphaned | LCG white noise; never instantiated; `noise_level` param never read. |
| Filters | Simper SVF + Huovilainen ladder | SVF (LP/HP/BP/Notch) + Huovilainen 2004 Moog ladder, 5 types selectable in UI, 128-sample type-switch crossfade, ladder at 1× in production. `DualFilter` is SVF-only and never instantiated in the voice path. |
| FX rack | Fake | Distortion = hard clip; delay/reverb mixes only *attenuate* the signal. No delay line, no reverb, no chorus. Per-voice, not a bus. |
| Envelopes | Real ADSR, **release fixed** | `TherumVoice::stop()` calls `ampEnv.noteOff()` only; the voice deactivates itself in `render()` once the envelope finishes, so the release tail sounds. APVTS env params (`env1_attack/decay/sustain/release`) are read in `processBlock` and passed via `setADSR`. Defaults still 10/120/0.75/220 ms. |
| LFOs | Real sine LFO, orphaned | Unipolar sine only, no sync/retrigger. Never instantiated. |
| Mod matrix | Stub | Storage only. `ModMatrixEvaluator` handles Macro 0/1 only; LFO/Env/Random/Velocity/Keytrack/Aftertouch fall through to 0. **Never called.** The only modulation reaching audio is a hardcoded hack: `modulatedCutoff = filterCutoff × (0.5 + macro×0.5)` — Macro 1 can only *halve* the cutoff. |
| Voices | Real | 16-voice pool; round-robin stealing (fixed — consecutive steals take consecutive voices, not always voice 0); osc phase + filter state reset on `start()` (fixed). Remaining gaps: osc B detune hardcoded ×1.005; mix hardcoded 0.62/0.38; output `× 0.2f`, no limiter; duplicate same-note voices possible. |
| Parameters | 27 defined, **18 reach audio** | Read in `processBlock`: `masterGain, filter1_cutoff, filter1_reso, filter1_type, macro1, fx_delayMix, fx_reverbMix, fx_distMix, oscA/B_wtPos, oscA/B_morphMode, oscA/B_morphPhase, env1_attack/decay/sustain/release`. Unwired (9): `qualityMode, oscA/B_level, oscA/B_detune, sub_level, noise_level, filter1_drive, macro2`. No parameter smoothing — knob/automation moves zipper. Mono duplicated to stereo. MIDI: note on/off only — no pitch bend, sustain, aftertouch, MPE. |
| Tests | **97,380 checks, 0 failures** | `test_core` 97,054; `test_voicefilter` 117; `test_morph` 57; `test_envelope` 10; `test_moogladder` 110; `test_decramping` 32. Re-verified 2026-11-07. No CI yet. |
| Plugin build | **VST3 + AU, first compiled 2026-11-07** | Release, exit 0 (commit `a7059b8`, pushed to `origin/main`). Installed to `~/Library/Audio/Plug-Ins/VST3/` + `Components/` by `COPY_PLUGIN_AFTER_BUILD`. Loads in Ableton Live 12 Lite (AU picked; VST3 editor not yet tested). Sounds — proven (see below). |
| Editor | **BROKEN — the blocker** | Opens at 52×20 px (titled "Window") instead of the 1460×900 the code specifies — `setSize(1460, 900)` IS called in the constructor, so the 52×20 is not a missing `setSize`; root cause under investigation. Activating the 52×20 window closes it; Live thinks the editor is open but nothing is visible. Destroying the editor segfaults in `TherumAttachments` / `SliderParameterAttachment` (EXC_BAD_ACCESS, null + 0x48) — closing the editor crashes Live (crash `Live-2026-11-07-014655.ips`, incident `EECF1F25-4355-4D52-B7D3-C8E44A69CFC3`). Destructor fix committed locally (`c0d91ea`, "Editor: release attachments before panels on close" — attachments declared last, released explicitly while sliders are alive, splash timer stopped first), not yet verified in Live, not pushed. |

Real audio-correctness bugs already in the scaffold, and their status (2026-11-07): **fixed** — release never sounds (now renders its tail); voice stealing always takes voice 0 (now round-robin); osc phase never reset on `start()` (now reset, with filter state). **Fixed by the build** — no band-limiting/mipmaps/oversampling (the wavetable engine now has mipmaps at load, 1 per 3 semitones, crossfaded; oversampling still only on the ladder at 1× in production). **Still open** — no parameter smoothing; unclamped cutoff; hard-clip FX with no oversampling; mono-duplicated stereo at fixed `×0.2f`, no limiter; UI timer reads a plain `bool active` cross-thread (no atomic); per-voice FX instead of a bus; `getTailLengthSeconds() == 0` [5].

**Sound proof (2026-11-07, Mac build check).** Therum loads in Ableton Live 12 Lite (AU picked; VST3 editor not yet tested) and sounds — proven objectively with Gary's own saved test (`/Users/gary/Desktop/Slave's Night Song Project/Therum-test.aif`, 16.86 s, stereo 16-bit 44.1 kHz, peak −18.0 dBFS, RMS −37.6 dBFS, 31% of samples non-zero). Five note events (C4, D4, E4, F4, G4 — the A/S/D/F/G computer-keyboard notes in C major) stop cleanly after release: no stuck notes, no drone, no idle noise. Live CPU meter 1% → 2–3% with the plugin loaded.

**What this means for this paper.** Nothing in §§4–5 is a description of Therum as it stands. Everything there is a specification of what must be built, tied to the literature that proves each choice. §6 labels the split explicitly.

---

## 4. DSP architecture

### 4.1 Wavetable oscillator

**The paper.** Bristow-Johnson, "Wavetable Synthesis 101, A Fundamental Perspective," Proc. AES 101st Convention, Nov. 1996, **Preprint 4400** [8]. (Correction: the FreeEQ8 paper [1] cites this as Preprint 4486. That is wrong — 4400 is verified against the AES E-Library, the CCRMA bibliography, and the full-text PDF. The FreeEQ8 paper should be corrected to 4400.) Bristow-Johnson proved wavetable synthesis is equivalent to additive synthesis for quasi-periodic tones — as general, as compact, and far less real-time computation — and proposed a perceptual error metric: each synthesised harmonic within a dB bound of the target (Eq. 14). Harmonic phase is likely inaudible; instantaneous frequency is salient. The paper prescribes **no** table size, mipmap scheme, or interpolation method — those are engineering choices, and this section makes them from the later literature.

**The oscillator core.** Stilson & Smith [9] give the bound that governs the whole design. With table length `N`, fundamental `f₀`, sample rate `fₛ`, the phase increment is `Inc = N·f₀/fₛ`, and exact band-limiting requires `max(Inc) = N/n_h`, where `n_h` is the highest harmonic below Nyquist. Sinc interpolation is ideal for periodic tables; linear is the most common, Lagrange/Hermite better, windowed sinc best. Therum's within-frame interpolation is cubic/Hermite as the default (linear is kept as the Eco quality mode) — the literature does not give a primary-source dB figure for linear vs cubic first-sideband levels, so Therum will measure its own interpolation error rather than quote one [7].

**Mipmaps — the standard, proven solution.** Take the FFT of a frame, zero every harmonic above Nyquist for the target pitch, IFFT back, and store one table per octave (or finer). Select the table by `n_h·f₀ < fₛ/2`, and crossfade between levels — without the crossfade, harmonics drop out in audible steps. Memory cost is 2× the base table. Store only the base frame on disk and generate the mips in RAM at load (Serum does this) [7]. **1/octave alone is proven insufficient** (a sweep aliases by design above `fₛ/3`); 1 per 3 semitones is the common rule of thumb, and 1/octave + 2× oversampling is the other accepted trade-off [7]. Therum v1 uses precomputed mipmaps at 1 per 3 semitones with crossfading. The v2 path is on-the-fly band-limiting (Vital-style: store FFT partials, zero above Nyquist, IFFT per note/block — no mip steps, harmonics to Nyquist), documented in Tytel's ADC21 talk and Vital's GPLv3 source [7, 10]; that is product practice and open source, not a paper, and Therum reads the Vital source before implementing it.

**Morphing.** Crossfade is per-sample `(1−α)A + αB` — it sounds phas-y when frames are out of phase. Spectral morph interpolates per-harmonic magnitude and phase (FFT bins); Serum has three spectral modes, two of which zero the phase of the fundamental or of all harmonics, and that is what kills the phasing [7]. Therum implements both: crossfade as the cheap mode, spectral morph (with zero-phase options) as the quality mode. Serum 2's "Smooth Interpolation / infinite-position" mode has no published algorithm — it is a target behaviour, not a citable method [7].

**Table format.** 256 frames × 2048 samples, matching Serum 2's format [2, 3], so Serum-format tables can be imported. Import is WAV (single-cycle and multi-frame) and Serum-format; drawing and harmonic editing are the editor's job (§7, Phase 4 of the migration plan [5]).

### 4.2 Anti-aliased oscillators — and where they do and do not apply

**The cost of not band-limiting.** A naive saw/square's harmonics fall 6 dB/octave to infinity; everything above Nyquist mirrors back as inharmonic beating and heterodyning (Välimäki & Huovilainen [11]). Any modulation re-introduces aliasing even with perfect mipmaps — a fact the build order (§7) accounts for by putting oversampling and ADAA on every modulated nonlinear path.

**The BLEP lineage.** BLIT (Stilson & Smith [9], 1996): a windowed-sinc impulse train, integrated to give saw/square/triangle. BLEP/minBLEP (Brandt [12], 2001, "Hard Sync Without Aliasing"): replace discontinuities with a band-limited step; min-phase via cepstrum, integrate, scale to 1.0; no look-ahead or latency; hard sync is a BLEP at reset; known DC-bias limitation. PolyBLEP (Välimäki & Huovilainen [11], 2007, IEEE Signal Process. Mag. 24(2):116–125): the ideal BLEP is the sine integral `Si(x)`; the practical form is a 2-sample polynomial correction. With `t` the normalised phase and `dt = f₀/fₛ`, `x = t/dt` (or `(t−1)/dt`): `2x−x²−1` if `t<dt`; `x²+2x+1` if `t>1−dt`; else 0. **Sign-convention caveat:** sources differ on add vs subtract (BLEP vs BLEP residual) — Therum verifies against a spectrum test before shipping, it does not trust the sign from the page. PolyBLAMP (Esqueda, Bilbao & Välimäki [13], DAFx-16) is the same idea for slope discontinuities (triangle, clipping). DPW (Välimäki, Nam, Smith & Abel [14], 2010, IEEE TASLP 18(4):786): `y(n) = c·(x²(n)−x²(n−1))`, `c = fₛ/(4f₀)`, `H(z) = c(1−z⁻¹)`; 4th-order DPW is "perceptually alias-free over the whole grand-piano register."

**Measured alias-suppression figures (primary sources only).** 4-point BLEP on a 1 kHz square: spurious components near DC and Nyquist **>100 dB down**, "virtually eliminated" with enough corrected samples (Esqueda, Bilbao & Välimäki [15], ISMR 2016). PolyBLAMP: aliasing −50 dB, SNR +~20 dB, beating 2×/4× oversampling (Esqueda et al. [13], DAFx-16). Integrated 3rd-order B-spline PolyBLEP: perceptually alias-free saw to `f₀ = 7.8 kHz` at 44.1 kHz (Välimäki, Pekonen & Nam [16], JASA 2012). The Välimäki school's criterion is **noise-to-mask ratio (NMR)** — aliases below the auditory masking curve — not an absolute dB floor. Therum's acceptance test is NMR-style (§8), not a bare dB number.

**Critical scope point.** BLEP, PolyBLEP and DPW are for **algorithmic oscillators** (naive saw/square, sync). They do **not** apply to Therum's wavetable path — a wavetable read has no step discontinuity to correct. Therum needs them only for a VA sub/sync oscillator (§4.3, §4.4). Any design document that proposes "PolyBLEP for the wavetable oscillator" has misunderstood the method, and this paper states that plainly so the mistake is not made during the build.

### 4.3 Unison, supersaw, sub

**Supersaw.** Roland's JP-8000 (1996) defined it: "a free-run oscillator whose shape resembles **7 sawtooth oscillators detuned** against each other" [17]. The exact detune curve is unpublished — Therum does not claim to reproduce it, and the "outer voices spread further" description is secondary lore, not a specification [7].

**Unison.** Serum runs up to 16 voices per oscillator, with even counts split left/right [3, 4]. CPU is **~linear in voices** (5× unison ≈ 4× CPU per the G-Stomper manual); 16 voices × 8-note polyphony = 128 concurrent table reads, and Therum budgets for it (§8). Detune, pan, phase and blend are per-voice, and the mipmap level must be chosen **per voice** (each voice has a different `f₀`) [7]. Therum's unison is 1–16 voices per oscillator with detune, stereo spread, phase control and blend, and the voice-count cap is enforced by the CPU budget, not by hope.

**Sub.** Serum's sub is a sine (clean, cannot alias) plus triangle/saw/square/pulse, −1/−2 octaves (range −2…+2), with a Direct Out bypassing filter and FX, phase-locked at note-on (reverse-engineered measurement, not an Xfer specification) [7]. Therum's sub is a sine plus the same waveshapes, −1/−2 octaves, Direct Out, phase-locked at note-on.

### 4.4 Warp modes — FM/PM and phase distortion

**FM/PM.** Chowning [18], 1973, JAES 21(7):525–534: `y = sin(2πf_c·t + I·sin 2πf_m·t) = Σ J_n(I)·sin(2π(f_c + n·f_m)t)`. Carson's rule gives the bandwidth `BW ≈ 2f_m(I+1)` (~99% power — a radio rule of thumb, warned as insufficient for digital aliasing). Implement **PM, not FM**: in FM, a DC offset in the modulator integrates to pitch drift; PM has no such failure. Anti-alias with an index ceiling `I_max ≈ (fₛ/2 − f_c)/f_m` (safer `fₛ/4`) plus 2× oversampling; 4× for non-sine operators [7].

**Phase distortion.** Casio's CZ series (mid-1980s): read a sine table at varying speed, with a DCW envelope controlling depth. Naive PD has sharp phase knees and residual aliasing. Band-limited PD is Lazzarini & Timoney [19], 2010, Computer Music J. 34(1):28–40: the modulator is a raised, scaled, band-limited sawtooth, with an FM/Bessel expansion giving band-limited PD. The generalisation is phaseshaping (Kleimola, Lazzarini, Timoney & Välimäki [20], SMC 2010): `y(n) = cos[2πφ(n)]`, `φ(n) = [φ(n−1) + f₀/fₛ] mod 1`, with PolyBLEP correcting phaseshaping discontinuities.

**Why warps alias, and the fix.** Serum-style warps (Bend, Asym, PWM, Flip, Mirror, Sync, Remap, Quantize) modify the **phase used to read the table**, not the waveform — and that destroys the mipmap's band-limiting. Serum is reported to skip mipmaps when warps are on ("aliases like hell" — KVR report, no Xfer confirmation; treat as reported behaviour, not a specification) [7]. The correct fixes are: warp → FFT → band-limit → IFFT per block, or 2× oversample the warped read. Sync specifically is a PolyBLEP at reset (§4.2). Waveshaping warps use ADAA (§4.6). Therum's warp list for parity is: Sync, Bend ±/+/−, PWM, Asym ±/+/−, Flip, Mirror, Remap 1–4, Quantize, FM/AM/RM from another oscillator, FM from noise/sub; Serum 2 adds true FM, PD, filter/distortion warps, and dual warp (32 warp/filter/distortion options reported) [7]. Therum implements dual warp — two warps per oscillator, in series — because it is the newest bar, and because the literature (Lazzarini & Timoney [19], Kleimola et al. [20]) supports both warps being band-limited independently.

### 4.5 Filters

**The carry-over from the FreeEQ8 paper [1] — not re-proved here.** Simper SVF and RBJ biquad produce identical steady-state magnitude responses. Both use BLT prewarping (`g = tan(π·fc/fₛ)`) and both cramp near Nyquist. The SVF's advantage is modulation stability, not decramping. The remaining error is −3.383 dB at the geometric mean of fc and Nyquist (16 kHz at 44.1 kHz). The FreeEQ8 paper's decramping claims were withdrawn; this paper does not repeat them.

**Simper SVF + Zavalishin TPT.** Simper [21], 2013 (original PDF read): `g = tan(π·fc/fₛ)`, `k = 1/Q`, `a1 = 1/(1+g(g+k))`, `a2 = g·a1`, `a3 = g·a2`; `v3 = v0−ic2eq`, `v1 = a1·ic1eq + a2·v3` (bandpass), `v2 = ic2eq + a2·ic1eq + a3·v3` (lowpass), `ic = 2v − ic`, `out = m0·v0 + m1·v1 + m2·v2`. LP (0,0,1), HP (1,−k,−1), BP (0,1,0), Notch (1,−k,0) are verified from the core equations. Simper p.32 **proves** that morphing between responses by interpolating the mix coefficients stays stable and smooth under audio-rate modulation — that is the mechanism behind Serum 2's PZSVF draw-your-own filter shapes. Wishnick [22], 2014, DAFx-14, proves time-varying BIBO stability of the trapezoidal SVF; DF-II/TDF-II/lattice are *not* necessarily stable per-sample. (Bencina 2013 attempted the proof and did not finish it — cite Wishnick, not Bencina.) Zavalishin [23], *The Art of VA Filter Design* rev. 1.1.1, generalises it: TPT replaces integrators with trapezoidal ones and preserves the topology; ZDF *solves* the delay-free loop, never breaks it with a `z⁻¹`. The 1-pole case is `y = (g·x+s)/(1+g)`. Note RBJ's terminological objection (a *resolved* ZDF loop is not ZDF) — this paper uses "TPT/trapezoidal" as the primary term.

**⚠️ What could not be verified — Bell/shelf/allpass mix coefficients.** Simper's PDF text extraction is garbled for those sections. The FreeEQ8 paper's table (Bell `1, kA·(A²−1), 0`; LowShelf `1, k·(A−1), A²−1`; HighShelf `A², k·(1−A)·A, 1−A²`) is reproduced here **flagged as re-derive-before-coding** — and the known SVF shelf defect (+50.96 dB at +24 dB/Q=24, from the FreeEQ8 deep dive) remains unexplained by the literature found. It may be the mix, the √A cutoff shift, or the denominator modification. Therum does not ship bell/shelf filters from this table until the coefficients are re-derived from Simper's `H(s)` numerators or Wishnick's matching formulas and the shelf is measured (§8).

**RBJ, the 5 degrees of freedom, Orfanidis, Särkkä–Huovilainen.** A normalised biquad has exactly 5 coefficients, so at most 5 constraints — the 5-DOF analysis is arithmetic, not opinion. BLT spends one constraint pinning the Nyquist gain to the analogue gain at ∞, and *that forced pin is cramping*. Any fix reallocates a degree of freedom; it cannot add one. Orfanidis [24], 1997, JAES 45(6):444–455, pins Nyquist to the analogue value at `fₛ/2` (`G1`), with a modified analogue `H(s) = (G1·s²+B·s+G0·W²)/(s²+A·s+W²)`, matching the analogue exactly at 0, f0 and `fₛ/2` plus the same bandwidth. Its limits are proven: magnitude at the prescribed points only — **phase is not matched** (Särkkä–Huovilainen [25], §1). The FreeEQ8 geometric-mean proposal pins a *different* 5th point (the geometric mean of fc and Nyquist). The paper must state this distinction: Orfanidis knew the DOF count; his paper is built on it. Särkkä & Huovilainen [25], 2011, IEEE TASLP 19(8):2486–2493 (DOI 10.1109/TASL.2011.2144970), use Shannon-interpolation discretisation matching magnitude *and* phase — the honest citation if Therum claims genuine decramping rather than reallocation. Therum's decramping is a **tested hypothesis**: implement the geometric-mean constraint, measure against the analogue prototype, and compare against Orfanidis and Särkkä–Huovilainen on the same test. The method that measures closest wins; none is declared the winner in this paper.

**Moog ladder — the practical choice.** Stilson & Smith [26], 1996, gave the first linear model, but a 1-sample delay in the feedback makes it a 5-pole filter with cutoff and resonance coupled — do not use it; cite it as history. Huovilainen [27], 2004, DAFx-04, is the v1 choice: nonlinear ODEs with `tanh` per differential pair, discrete form using **5 tanh/sample**, `k = 4·reso` self-oscillating at finite amplitude (tanh saturates), **2× oversampling** published (DaisySP uses 4×). Precedent: Csound `moogladder`, DaisySP. D'Angelo & Välimäki [28, 29], 2013 (ICASSP) / 2014 (IEEE TASLP 22(12):1825–1832), are the v2 / accurate mode: circuit-derived, SPICE-matched distortion and realistic self-oscillation, **only 12 more basic ops/sample than Huovilainen, same tanh count** — the upgrade is nearly free in CPU. DAFx26 (Oyama 2026) is still benchmarking Huovilainen vs. D'Angelo against SPICE. A Zavalishin ZDF ladder is topology-exact and unified with the SVF codebase, but costs a Newton/analytic solve per sample — deferred unless Therum commits to TPT everywhere. Therum's ladder is Huovilainen at 2× oversampling first, D'Angelo–Välimäki as the accurate mode second.

**Formant and comb.** Klatt [30], 1980, JASA 67(3):971–995: the resonator `y[n] = a·x[n]+b·y[n−1]+c·y[n−2]`, `c = −exp(−2πBT)`, `b = 2exp(−πBT)cos(2πFT)`, `a = 1−b−c`. The **parallel bank** (independent amplitudes, differencing filter `y[n]=x[n]−x[n−1]` on F2–F6) is the synth choice; cascade is the speech choice. Vowel data are Peterson & Barney [31], 1952 (male means, e.g. /i/ 270/2290/3010, /ɑ/ 730/1090/2440, /u/ 300/870/2240 Hz) with bandwidths B1 50–80, B2 70–120, B3 100–180 Hz. The alternative — parallel SVF bandpasses with `Q = F/B` — is the same transfer function up to BLT pinning; the Klatt form is recommended. Comb: feedforward `H(z) = 1+g·z^(−D)` (always stable); feedback `H(z) = 1/(1−g·z^(−D))`, requires `|g|<1`; teeth spaced `fₛ/D`; a flanger is a feedforward comb with D < 10 ms, modulated. Karplus–Strong [32], 1983: `N = fₛ/f0`, loop filter `(1+z⁻¹)/2`; the integer-N tuning problem is fixed by Jaffe & Smith [33], 1983, with a first-order allpass fractional delay. Schroeder [34], 1962: 4+ parallel combs (unequal lengths) + 2 series allpasses, ~1,000 echoes/s target. The synth-distinctive use is a **key-tracked comb** (`D = fₛ/f0(note)`).

**Filter set for Therum v1.** SVF core (LP/HP/BP/Notch) → dual filters in series/parallel → resonance + drive → Huovilainen ladder at 2× → parallel Klatt formant bank → comb (both forms, key-tracked) → bell/shelves *after re-derivation* → PZSVF morphing (stability proven by Simper p.32; the curve→mix mapping is Therum's own work).

### 4.6 Oversampling and ADAA

**The filter question.** Kahles, Esqueda & Välimäki [35], 2019, JAES 67(6):440–449 (full paper read): hard clipping, M = 8, passband 0–16 kHz ±1 dB, **stopband ≥40 dB from 28.1 kHz**. Results: linear interpolation + HF shelf is sufficient for upsampling; **only CIC decimation achieves perceptually sufficient suppression across the full range** with that interpolator. **40 dB is a study floor, not a product target** — product references are 96 dB (Pirkle 4× FIR, order ~300) to >114 dB (commercial 2× polyphase FIR + ADAA, 127-tap half-band). Therum reports **measured alias level in dB** (A-weighted noise-to-mask, Kahles method) per quality setting — the oversampling factor itself is not a quality figure.

**ADAA.** Parker, Zavalishin & Le Bivic [36], 2016, DAFx-16: `y[n] = (F(x[n])−F(x[n−1]))/(x[n]−x[n−1])`, where F is the antiderivative (for tanh, `F = ln(cosh x)`). Formalised in Bilbao, Esqueda, Parker & Välimäki [37], 2017, IEEE Signal Process. Lett. 24:1049–1053. ADAA is proven, but it introduces a half-sample delay and an in-band lowpass — counter with **2× oversampling** (Esqueda et al. [38], DAFx-19, extending ADAA to stateful systems). "ADAA ≈ 4–8× oversampling" is a secondary-source rule of thumb only, not a measured figure in Parker 2016 — Therum does not quote it as a result. **Rule of thumb for the build** (judgement, to be measured): gentle tanh 2× or 1×+ADAA; drive/distortion 4× IIR real-time / 4× FIR render; hard clip/wavefold 8× or 2×+ADAA; ladder 2–4×; linear stages **1× (never oversample)**. Oversample only the nonlinear block. JUCE `dsp::Oversampling` (2/4/8/16×, polyphase IIR vs FIR equiripple) is precedent, not a dependency — Therum is iPlug2 and brings its own oversampler (iPlug2 ships a 2–16× HIIR oversampler [6], which is the starting point, not the finished filter).

### 4.7 Modulation

**LFOs.** Serum 2 ships up to 10 LFOs, including 2D and chaos modes [2, 3]. Therum matches the counts that matter: 4+ LFOs at v1 (10 at parity), with shapes (sine, triangle, saw, square, sample-and-hold, noise), rate (free, tempo-synced, retrigger, one-shot), and phase. The 2D LFO (an X/Y path, Serum 2's Path LFO) is a product feature, not literature — Therum implements it as a path through a 2D grid, and does not cite a paper for it.

**Chaos.** Lorenz [39], 1963, J. Atmospheric Sciences 20:130–141: `dx/dt = σ(y−x)`, `dy/dt = x(ρ−z)−y`, `dz/dt = xy−βz`, with σ=10, ρ=28, β=8/3. Deterministic, aperiodic; x, y and z are three correlated outputs of **one** trajectory, not three independent LFOs — any design that treats them as independent has misunderstood the system. Xfer's own page shows a Lorenz attractor visualisation for Serum 2 [2], confirming chaos modulation is a shipped Serum 2 feature. Therum's chaos LFO is a Lorenz system with rate (integration step) and the three outputs available as separate routings.

**Envelopes.** The ADSR shape should be exponential, and the release must start from the current level — Therum's linear ADSR with a dead release (§3) is the first fix, not a design choice. MSEG (multi-stage envelope generator) is product practice (UVI up to 128 breakpoints, Korg Kronos per-stage curves) — **no canonical MSEG paper was found**; Therum implements MSEG as a breakpoint envelope with per-stage curve (linear/exponential/logarithmic) and does not cite literature for it [40]. Therum's envelopes: 4 at v1 (Serum 2 parity), ADSR + MSEG per envelope, velocity-scaled, with per-voice and global instances.

**Velocity, aftertouch, MPE.** MPE is specified by the MIDI Association [41]: per-note channel, pitch bend ±48 semitones (master ±2, RPN 0, range 0–±96), aftertouch as Channel Pressure, CC74 as timbre/slide, maximum 15 notes per zone. The voice struct must carry per-voice bend, pressure, CC74 and velocity — if the voice struct does not have these fields, MPE is not implemented, whatever the UI says. Therum implements MPE (iPlug2 ships an MPE class [6]) and CLAP note-expression parity is checked against the CLAP spec before claiming it [40].

**Modulation matrix.** Per-voice sources are evaluated inside the voice loop; global sources once per block. That is the whole rule — a matrix that evaluates a per-voice LFO once per block has already broken the voice. Drag-to-modulate is UX, not DSP literature: drag any source onto any target, with animated depth rings and one-click routing. Therum's matrix is a single visible routing table (source → target → depth → bipolar/unipolar → curve), not spread across tabs, because the plan's "faster to program" axis (§1.1) is a workflow claim and the matrix is where it is won or lost [5].

**Parameter smoothing / dezippering.** The one-pole leaky integrator `y[n] = y[n−1] + a·(x[n]−y[n−1])`, `a = 1−exp(−1/(τ·fₛ))` (Smith/CCRMA; KVR/JUCE practitioner sources) [40]. Practitioner time constants: τ 5–15 ms for knobs, 50 ms suggested for delay time. **No verified zipper-noise audibility threshold was found** — this paper does not cite one, and neither should any Therum document. Two hard rules, both verified: delay time must **not** be naively one-pole smoothed (the read pointer can run backwards — verified KVR discussion); use a crossfade or a slew-limit instead. Filter coefficients must be interpolated **per-sample**, not stepped every N samples (JUCE forum evidence: 100-sample steps still zipper) [40]. Therum smooths every audio parameter with the one-pole smoother at the appropriate τ, interpolates filter coefficients per-sample, and crossfades delay-time changes. The FreeEQ8 variable-cadence engine [1, §4] (0.1 dB delta threshold, 4-sample batching, 80% reduction in coefficient updates, per-sample accuracy restored on transients) is adapted for Therum's dynamic modulation — the threshold and batch interval are re-measured for a synth's faster modulation, not copied blindly.

### 4.8 Effects

**Reverb — the verified chain.** Schroeder [34], 1962 (4 parallel combs → 2 series allpasses, ~1,000 echoes/s) → Moorer [42], 1979 (FIR early reflections + LPF-in-comb + 6 combs) → Stautner & Puckette [43], 1982, CMJ 6(1):52–65 (feedback delay network, `Y = D(X + G·Y)`) → Jot & Chaigne [44], 1991, AES 90th Conv. Preprint 3030 (`H(z) = cᵀ[D(z⁻¹)−G]⁻¹b`; stability iff `G = g·M`, M unitary, `|g|<1`; Householder `H = I−(2/N)·11ᵀ`) → Jot [45], 1997 (8–16 delays, 1–2 s total, suffice for density; `gᵢ = γ^(mᵢ)`) → Dattorro [46], 1997, JAES 45(9):660–684 (plate: input diffuser + figure-8 tank + taps, full coefficients published). **Build the FDN first, Dattorro second; do not ship raw Schroeder** — Schroeder's comb/allpass structure is the textbook starting point, and its metallic ringing is why the field moved on. Therum's reverb is an FDN (Jot & Chaigne, Householder feedback matrix, per-line absorptive lowpass filters) as the algorithmic reverb, Dattorro's plate as the second mode, and a convolution reverb as the later tier.

**Convolution reverb.** Gardner [47], 1995, JAES 43(3):127–136: non-uniform partitioned convolution, with the time-domain first block giving **zero latency**; uniform partition latency is 2L. Therum's convolution reverb is a later tier (after the algorithmic reverbs), using non-uniform partitions so the first block is zero-latency. Impulse-response loading is the product work; the DSP is Gardner's.

**Delay, chorus, flanger, phaser.** Feedback delay `y[n] = x[n−D] + g·y[n−D]`; chorus 15–40 ms, flanger 0.5–15 ms + feedback, phaser a series of allpasses (no delay line). Fractional delay interpolation is required — an integer-only delay line is a tuning error, not a design choice [40]. Tempo sync is a host-tempo division, not DSP.

**Distortion / saturation.** `tanh(k·x)` plus ADAA (§4.6; Parker, Zavalishin & Le Bivic [36]; Bilbao et al. [37]). ADAA suppresses aliasing; it does not eliminate it. Hard clip is not distortion — it is the absence of a waveshaper, and Therum's scaffold calling a hard clip "distortion" is one of the bugs §3 lists. Therum's distortion is tanh waveshaping with ADAA and oversampling (§4.6), with drive, tone and mix.

**Compressor.** Giannoulis, Massberg & Reiss [48], 2012, JAES 60(6):399–408: feedforward (can perfectly limit, `c_dB = (1/R−1)(x_G−T)`) vs feedback; branching vs decoupled peak detector — the decoupled detector avoids the attack→release discontinuity and holds across zero-crossings. Therum's compressor is feedforward with a decoupled peak detector, attack/release/ threshold/ratio/knee/makeup, per the tutorial.

**Rack routing.** No paper proves an effect order — convention is tone-shaping first, time-based last; per-voice inserts (distortion, compressor) vs global sends (delay, reverb); band split via a Linkwitz–Riley crossover, M/S via `(L±R)/2` [40]. Therum's rack is modular and reorderable with dual buses (Serum 2 parity [3, 4]), multiple instances of the same effect, and M/S + Low/High splitters. The order is the user's choice; the default order is the convention, and the paper does not claim a paper proves it.

**Bode frequency shifter.** Bode [49], 1965, AES Preprint 395: a Hilbert 90° network plus a complex sinusoid, giving a fixed ±Hz shift (not a ratio) — inharmonic by construction. Serum 2 ships it [3, 4]; Therum implements it as a documented effect, not as a pitch shifter (it is not one).

### 4.9 Granular and spectral engines

**Granular.** Gabor [50], 1947, Nature 159:591–594: the time–frequency uncertainty relation `Δt·Δf ≥ 1/(4π)`, with the Gaussian window attaining the bound. Roads [51], 1978/2001, *Microsound* (MIT Press): grains under 50 ms, more than 20 per second fuse into a continuous texture; grainlets, pulsars, clouds. Truax [52], 1986: the first real-time granular system (2,375 grains/s peak). Therum v1 granular: Hann window, asynchronous scheduler, position/pitch/size/density/spread, a 256-grain fixed pool (Serum 2's "up to 256 simultaneous grains" [3]), no audio-thread allocation. The grain-size perceptual bands (<10 ms no pitch, 10–25 ms partial, >50 ms clear) are plausible and widely repeated but were **not traced to a single experiment** — Therum does not cite them as thresholds [40].

**Spectral.** Flanagan & Golden [53], 1966, Bell Syst. Tech. J. 45(9):1493–1509: the phase vocoder (STFT → modify → ISTFT, horizontal phase coherence). Laroche & Dolson [54], 1999, IEEE Trans. Speech & Audio 7(3):323–332: identity phase-locking, `∠Y[m,k] = ∠Y[m−1,k] + H_s·ω_inst`. Serra & Smith [55], 1990, CMJ 14(4):12–24: spectral modelling synthesis, `s(n) = Σ A_p(n)cos θ_p(n) + e(n)` — partials plus a filtered-noise residual. Griffin & Lim [56], 1984: iterative magnitude-only reconstruction (slow; only if phase is missing). Serum 2's spectral engine is harmonic-level additive resynthesis plus warps (spread, gate, twist, mask, vocode); its cost is partials-per-voice [3, 4]. Therum's spectral engine is Serra & Smith's model (harmonic partials + noise residual) with spectral warps, built last (§7) because its cost is the highest and its benefit is the narrowest.

### 4.10 Real-time safety, sample rate, and CPU budget

**The carry-over from the FreeEQ8 paper [1].** A lock-free SPSC triple-buffer (three slots, `{writeSlot, midSlot, readSlot}`, audio thread release-swaps `writeSlot ↔ midSlot`, UI thread acquire-swaps `midSlot ↔ readSlot`, no mutex, no lock, no blocking) with **0 data tears across 239 million samples** on a 2012 Ivy Bridge i7-3720QM. All oversamplers pre-constructed in `prepareToPlay()` into a fixed pool; mid-playback order changes call `reset()` (non-allocating) with a 128-sample crossfade. All analysis (FFT, peak detection, scoring) runs on the UI timer thread at 30 Hz, never on the audio thread; the audio thread's only job is a `memcpy` + atomic swap. These are FreeEQ8's measurements, not Therum's — Therum adopts the architecture and re-measures it (§8).

**The literature behind it.** Bencina [57] ("Some notes on lock-free and wait-free algorithms") is the canonical reference for the SPSC pattern: a bounded single-producer single-consumer ring buffer is wait-free on both sides with no compare-and-swap, using acquire/release on two indices, with `alignas(64)` keeping the indices on separate cache lines to prevent false sharing. Rowland [58] (ADC talk, "Lock-free queues in the multiverse of madness") surveys the implementations (moodycamel, `boost::lockfree::spsc_queue`, rigtorp::SPSCQueue, choc::SingleReaderSingleWriterFIFO) and the rule: pick the right queue for the job, fixed capacity (no allocations, real-time safe), unbounded is not real-time safe. Therum's audio thread allocates **nothing** — no heap, no locks, no blocking calls — and the rule is enforced by a test (§8), not by convention.

**Sample rate and oversampling strategy.** Therum runs at the host sample rate with oversampling applied only to nonlinear blocks (§4.6). Linear stages run at 1× — never oversampled. The oversampling factor is a quality setting (Eco / Standard / Ultra), and each setting reports its measured alias level (§8); the factor itself is not a quality figure. Latency is reported per mode in samples and milliseconds, and the host's automatic delay compensation is honoured — a synth that hides its latency is a synth that phases against the rest of the mix.

**CPU budget.** No CPU figures exist in the primary literature for unison, mipmap, PolyBLEP or ladder costs at a given voice count — all CPU claims in the literature are asymptotic (linear in voices) or product-manual prose [7]. Therum therefore benchmarks its own build and reports cycles/sample (and ns/sample, the FreeEQ8 paper's unit [1]) per voice, per filter, per oversampling factor, on the target hardware. The budget that governs the design: 16 voices × 8-note polyphony = 128 concurrent table reads at maximum unison (§4.3), and the voice-count cap is set by that measurement, not by a number in this paper. The FreeEQ8 paper's instance-scaling result (per-instance cost rising only 5% from 1→128 instances, 85.8% of one core at 128 SVF instances [1, §6.2]) is the method Therum copies — the numbers will be Therum's own.

---

## 5. What is proven vs what is planned

This section is the honesty ledger. Anything labelled **proven** has a cited paper or a cited measurement behind it. Anything labelled **planned** is a design decision this paper makes, to be verified by the measurement plan (§8). Nothing is labelled proven on the strength of this paper alone.

| # | Claim | Status | Evidence |
|---|---|---|---|
| P1 | Wavetable ≡ additive synthesis for quasi-periodic tones | Proven | Bristow-Johnson [8], Preprint 4400 |
| P2 | Mipmapped wavetables band-limit the oscillator; 1/octave alone is insufficient | Proven | Stilson & Smith [9]; mipmap analysis [7] |
| P3 | PolyBLEP / DPW suppress aliasing in algorithmic oscillators (measured figures in §4.2) | Proven | Välimäki & Huovilainen [11]; Välimäki et al. [14]; Esqueda et al. [13, 15]; Välimäki, Pekonen & Nam [16] |
| P4 | BLEP/PolyBLEP do **not** apply to wavetable reads | Proven (scope) | §4.2 — a wavetable read has no step discontinuity to correct |
| P5 | FM = Bessel expansion; implement PM, not FM | Proven | Chowning [18] |
| P6 | Band-limited phase distortion via raised/scaled/band-limited sawtooth + FM/Bessel expansion | Proven | Lazzarini & Timoney [19] |
| P7 | Simper SVF ≡ RBJ biquad in steady-state magnitude; both cramp; SVF's edge is modulation stability | Proven | FreeEQ8 paper [1, §2.3–2.4]; Simper [21]; Wishnick [22] |
| P8 | Trapezoidal SVF is time-varying BIBO stable; DF-II/TDF-II/lattice are not necessarily | Proven | Wishnick [22] |
| P9 | Orfanidis pins Nyquist gain; magnitude-only, phase not matched | Proven | Orfanidis [24]; Särkkä & Huovilainen [25, §1] |
| P10 | Särkkä–Huovilainen discretisation matches magnitude **and** phase | Proven | Särkkä & Huovilainen [25] |
| P11 | Huovilainen ladder: 5 tanh/sample, 2× OS; D'Angelo–Välimäki: +12 ops, same tanh, SPICE-matched | Proven | Huovilainen [27]; D'Angelo & Välimäki [28, 29] |
| P12 | ADAA suppresses (does not eliminate) aliasing in memoryless nonlinearities; half-sample delay + in-band lowpass, countered by 2× OS | Proven | Parker et al. [36]; Bilbao et al. [37]; Esqueda et al. [38] |
| P13 | Oversampling filter selection (Kahles method; only CIC decimation perceptually sufficient with linear-interp + HF shelf upsampling) | Proven | Kahles, Esqueda & Välimäki [35] |
| P14 | Klatt parallel formant bank; Peterson–Barney vowel data | Proven | Klatt [30]; Peterson & Barney [31] |
| P15 | Karplus–Strong plucked string; Jaffe–Smith fractional delay | Proven | Karplus & Strong [32]; Jaffe & Smith [33] |
| P16 | FDN reverb stability (unitary feedback matrix); Dattorro plate coefficients | Proven | Stautner & Puckette [43]; Jot & Chaigne [44]; Jot [45]; Dattorro [46] |
| P17 | Non-uniform partitioned convolution, zero-latency first block | Proven | Gardner [47] |
| P18 | Feedforward vs feedback compressor; decoupled peak detector | Proven | Giannoulis, Massberg & Reiss [48] |
| P19 | Lorenz chaos equations (σ=10, ρ=28, β=8/3); x/y/z are one trajectory | Proven | Lorenz [39] |
| P20 | MPE specification (per-note channel, bend ±48 st, CC74 timbre, 15 notes/zone) | Proven | MIDI Association [41] |
| P21 | One-pole smoothing formula; delay time must not be naively smoothed; filter coefficients per-sample | Proven (practice) | Smith/CCRMA; KVR/JUCE practitioner sources [40] |
| P22 | SPSC triple-buffer, 0 tears / 239M samples; variable-cadence 80% reduction; SVF 72.7 ns/sample, 0.63% CPU, 161× headroom | Proven (FreeEQ8) | FreeEQ8 paper [1] — **FreeEQ8's measurements, not Therum's** |
| P23 | Gabor uncertainty `Δt·Δf ≥ 1/(4π)`; grains <50 ms, >20/s fuse | Proven | Gabor [50]; Roads [51] |
| P24 | Phase vocoder; identity phase-locking; SMS partials + noise residual | Proven | Flanagan & Golden [53]; Laroche & Dolson [54]; Serra & Smith [55] |
| | | | |
| Q1 | Therum's wavetable oscillator is cleaner than Serum 2's under warp | **Planned** | Requires the NMR alias test (§8). Serum's warp aliasing is reported behaviour, not a specification [7]. |
| Q2 | Geometric-mean 5th-constraint decramping closes the −3.383 dB gap | **Planned (hypothesis)** | Test against Orfanidis [24] and Särkkä–Huovilainen [25] on the same measurement (§8). |
| Q3 | Simper bell/shelf/allpass mix coefficients as tabulated in the FreeEQ8 paper | **Unverified** | PDF text extraction garbled; re-derive before coding. The +50.96 dB shelf defect is unexplained [1; 59]. |
| Q4 | Therum's CPU budget (cycles/sample per voice, instance scaling) | **Planned** | No literature CPU figures exist; Therum benchmarks its own build (§8). |
| Q5 | "ADAA ≈ 4–8× oversampling" | **Unverified (rule of thumb)** | Secondary sources only; not a measured figure in Parker 2016 [36]. Therum measures its own alias-rejection dB. |
| Q6 | Zipper-noise audibility threshold | **Unverified** | No measured threshold found in the literature [40]. Therum does not cite one. |
| Q7 | Serum 2 internals (grain window, spectral partial count, LFO max rate, FX order defaults) | **Unverified** | Product claims from secondary reviews/manuals, not Xfer DSP documentation [40]. |
| Q8 | Therum's preset library, browser, arpeggiator, sequencer | **Planned** | Later phases (§7); content and sequencing are not DSP claims. |
| Q9 | On-the-fly band-limiting (Vital-style per-block IFFT) as Therum's v2 oscillator | **Planned** | Product practice + open source (Tytel [10]), not a paper. Read Vital's GPLv3 source first. |
| Q10 | PZSVF draw-your-own filter curve→mix mapping | **Planned (Therum's own work)** | Stability of mix interpolation is proven (Simper p.32 [21]); the mapping from a drawn curve to mix coefficients is not in the literature. |

---

## 6. Ranked build order (with dependencies)

Ranked by (impact on beating Serum 2) ÷ (cost), with the dependency that each needs the phase before it. This follows the migration plan's order [5, §4] — framework-free DSP core first, prove it in iPlug2's `IPlugInstrument` example, parameters + state, UI last, Serum-beating features only after the platform is proven — and fills in the DSP that plan left as "the real work."

| Rank | Build | Depends on | Why this order |
|---|---|---|---|
| 1 | **Fix the scaffold bugs + extract the framework-free DSP core.** Release tail, one-pole smoothing on every parameter, voice stealing (round-robin, not voice 0), phase reset on `start()`, clamp the cutoff, limiter on the output. Core in plain C++ with no `juce::` includes, own unit tests. | — | **Mostly done (2026-11-07):** core extracted (framework-free, `make check` passes), release fixed, voice stealing fixed (round-robin), phase reset fixed. Remaining: one-pole smoothing, clamp the cutoff, limiter. |
| 2 | **Real wavetable engine.** 256-frame tables, mipmaps at load (1 per 3 semitones, crossfaded), cubic/Hermite interpolation, crossfade + spectral morph, import (WAV, Serum-format), 2048-sample frames. | 1 | **DONE (2026-11-07).** 256 frames × 2048 samples, Serum-format import, mipmaps at load, Hermite interpolation, crossfade + spectral morph. Morph + ADSR wired end-to-end (1b22878). |
| 3 | **SVF core + dual filters.** LP/HP/BP/Notch, series/parallel, resonance + drive, per-sample coefficient interpolation. | 1 | **SVF + ladder DONE (2026-11-07):** SVF (LP/HP/BP/Notch) + Huovilainen 2004 Moog ladder, 5 types selectable in UI, 128-sample type-switch crossfade, ladder at 1× in production. DualFilter still SVF-only and never instantiated — remains. |
| 4 | **Unison + sub.** 1–16 voices/osc, detune/pan/phase/blend, per-voice mipmap selection; sub (sine + waveshapes, −1/−2 oct, Direct Out). | 2 | Serum's signature width. CPU is linear in voices — cap by the measured budget (§8). |
| 5 | **Modulation foundation.** Wire the evaluator (it is never called today), fix the ADSR release, one-pole smoothing everywhere, LFOs (shapes, sync, retrigger), envelopes (ADSR + MSEG), mod matrix (per-voice in the voice loop, global once per block), MPE voice fields. | 1, 3 | **PARTIAL (2026-11-07):** ADSR wired end-to-end (1b22878), env params read, release fixed. Mod matrix still never called (the big remaining item). LFOs still orphaned. MPE voice fields still missing. |
| 6 | **Warp modes.** Phase-read warps (Bend/PWM/Asym/Sync/Flip/Mirror/Remap/Quantize) with a documented aliasing caveat and a 2× oversampling quality switch; sync via PolyBLEP at reset; FM/PM as PM with Carson index ceiling; PD per Lazzarini & Timoney; dual warp. | 2, 4 | Serum 2's headline wavetable feature. Band-limiting the warps is what makes Therum cleaner (§4.4). |
| 7 | **Moog ladder + formant + comb.** Huovilainen ladder at 2× oversampling, then D'Angelo–Välimäki accurate mode; parallel Klatt formant bank; comb (both forms, key-tracked). | 3 | The filter set that makes the synth sound like an instrument, not an EQ. |
| 8 | **FX inserts, then sends.** Distortion (tanh + ADAA), compressor (feedforward, decoupled detector) as per-voice inserts; then delay, chorus/flanger/phaser, Bode shifter, FDN reverb → Dattorro plate as global sends; dual buses, reorderable, M/S + band splitters. Convolution reverb last. | 1, 5, 6 | Inserts first (they shape the voice), sends second (they shape the mix). Convolution is a later tier (Gardner [47]). |
| 9 | **Bell/shelves after re-derivation + decramping as a tested hypothesis.** Re-derive the Simper bell/shelf mixes, measure the shelf, then implement and measure the geometric-mean constraint vs Orfanidis vs Särkkä–Huovilainen. | 3 | The FreeEQ8 paper proved the SVF does not decramp [1]. Therum does not claim a decramped filter until this step measures one. |
| 10 | **Sample engine, then multisample.** Sample playback (interpolated, looped, snap loop detection) reuses the wavetable infrastructure; multisample (SFZ) after it. | 2 | The easiest of Serum 2's five engines, and the one that reuses the most existing code. |
| 11 | **Granular, then spectral.** Granular (256-grain fixed pool, Hann window, async scheduler); spectral (Serra & Smith partials + noise residual, spectral warps). | 2, 8 | The long tail. Highest cost, narrowest benefit. Built last, not first. |
| 12 | **On-the-fly band-limiting (v2 oscillator).** Vital-style per-block IFFT, replacing precomputed mipmaps, fixing warped reads properly. | 2, 6 | The correct fix for warp aliasing (§4.4). V2 because it needs the wavetable engine and the warps to exist first. |
| 13 | **PZSVF morphing + preset library + browser.** Draw-your-own filter shapes (mix interpolation proven [21]; curve→mix mapping is Therum's own work); 100+ real factory presets, tagged browser with search, preset sharing via the hub. | 3, 9 | Content is the moat — budget sound-design time, not just code. |
| 14 | **Arpeggiator + clip sequencer.** | 5 | Serum 2 added these; each is a whole product. Deferred — they are not the wavetable fight. |

**Immediate work (2026-11-07, the blocker).** Fix the editor lifecycle before any further DSP work: (a) the editor window opens at 52×20 px instead of 1460×900 (root cause under investigation — `setSize(1460, 900)` IS called in the constructor); (b) destroying the editor segfaults in `TherumAttachments` / `SliderParameterAttachment` (destructor fix committed locally as `c0d91ea`, not yet verified in Live). Until those are fixed, no editor-based verification (Morph/ADSR, filters, top-right tables, drop test) is possible in Live. After the fix: re-test the editor open/render, the top-right tables, dropping a sound onto them, Morph/ADSR and filter switching — and test the VST3 editor too (only the AU was exercised so far).

**Free / full split (Gary's model, 2026-11-07).** Therum JUCE (this repo) is the FREE version: 2 wavetable oscs, crossfade morph, WT import, SVF LP/HP/BP/Notch, 1 ADSR, macro, 16 voices. Therum iPlug2 is the FULL / PRO version, built in parallel, mirroring the free version's progress plus: Spectral morph (ZeroFundamental/ZeroAll), Moog ladder filter, Unison (1–16 voices/osc), sub osc, warp modes, Full mod matrix, MSEG, LFOs beyond 1–2, MPE, Sample / granular / spectral engines, Real FX (FDN/Dattorro reverb, delay, chorus, distortion w/ ADAA), Wavetable editor / drawing / harmonic edit, Full preset library (100+), browser, preset packs. The ranks above are the free version's build order; the full version follows the same order and adds the pro features as it goes.

---

## 7. Measurement plan

A claim in this paper is not a result until the test in this section produces a number. The tests are written before the DSP they verify, and they run in CI (the FreeEQ8 paper's method [1, §6.7–6.8]: standalone C++ tests, no DAW, no mock, reproducible build command, median of 16 trials with warmup discarded).

**Alias test (the headline test).** Noise-to-mask ratio (NMR), the Välimäki school's criterion [11, 16] — aliases below the auditory masking curve, not a bare dB number. Method: sine sweep through each oscillator (wavetable, unison, warped, FM/PM, PD) at representative fundamentals; measure alias products; report the NMR and the worst alias level in dB. Target: aliases below the masking curve at every tested fundamental, at 44.1 kHz and 48 kHz, in Standard quality. Report the measured alias level in dB per quality setting (Eco / Standard / Ultra), per Kahles et al. [35]. A wavetable oscillator that fails this test does not ship, whatever it sounds like in isolation.

**Filter response test.** The FreeEQ8 method [1, §2.3]: sweep a sine (20 Hz–20 kHz, log-spaced points) through each filter type and measure the RMS magnitude ratio against the analogue prototype. Report the error at the geometric-mean pinning frequency (the −3.383 dB figure [1, §2.4] is the baseline to beat). The decramping hypothesis (§4.5, Q2) is tested on this rig: geometric-mean constraint vs Orfanidis vs Särkkä–Huovilainen, same sweep, same prototype, and the method that measures closest wins. Bell/shelf filters are not shipped until the re-derived mixes pass this test (Q3).

**CPU benchmarks.** ns/sample and cycles/sample per voice, per filter, per oversampling factor, measured on the target hardware (the FreeEQ8 paper's unit [1, §6]). Instance scaling 1→128 (the FreeEQ8 method [1, §6.2]) — report the per-instance cost rise and the total CPU at 128 instances. Voice-count cap is set by this measurement (§4.3). No CPU figure from the literature is quoted as a Therum result (§5, Q4).

**Latency.** Report in samples and milliseconds per mode: base (no oversampling), each oversampling factor, convolution partition size, linear-phase mode if added. The host's automatic delay compensation is honoured in every mode.

**Real-time safety.** Zero allocations on the audio thread (verified by a test that counts allocations during `processBlock`, not by inspection). SPSC stress test: concurrent producer/consumer, 400 ms trials, report samples produced, buffers consumed, data tears — the FreeEQ8 result to match is 0 tears across 239M samples [1, §10.3]. Denormal handling: `ScopedNoDenormals`-equivalent on the audio thread, verified by a denormal-input test (the FreeEQ8 paper's NaN failure, §10 of [1], is the regression case).

**Perceptual tests.** ABX listening tests using the FreeEQ8 ABX infrastructure [1, §7.1] (40 randomised trials per participant, binomial test, d-prime, Wilson 95% CI) for every perceptual claim: variable-cadence vs per-sample modulation, alias audibility (NMR vs ABX — if listeners cannot distinguish, the NMR target is met in practice), reverb quality (FDN vs Dattorro vs convolution), and filter decramping audibility. The FreeEQ8 pilot (N=1, 55% hit rate, p=0.635, d-prime 0.25 — not significantly different from chance) is the standard for reporting a null result honestly [1, §7.1]. A formal study (N≥20, power >0.85 for d-prime ≥0.5) is planned before any perceptual claim is published.

**Plugin validation.** pluginval at strictness level 10 on all formats (VST3, AU, CLAP), verifying buffer-size changes (32–8192 samples), sample-rate switches (22050–192000 Hz), rapid parameter automation, and thread-safety compliance — the FreeEQ8 CI standard [1, §6.8]. AU validation via auval; CLAP via the CLAP validator. Failures block release artifact upload.

**Status 2026-11-07:** VST3 + AU built and loaded in Ableton Live 12 Lite (AU picked; VST3 editor not yet tested). pluginval strictness 10 NOT yet run; auval NOT yet run; no DAW smoke test beyond the Live load + sound proof. The editor is broken (52×20 window, destructor segfault — see §3), which blocks editor-based validation until fixed.

**Preset/state round-trip.** Save → load → bit-identical parameters, including wavetables and mod-matrix routings (the migration plan's Phase 3 test [5]). Parameter indices are frozen before the first release; reordering them after release breaks every saved session.

---

## 8. Limitations and open questions

1. **Simper bell/shelf/allpass mixes are unverified** (Q3). The FreeEQ8 table is reproduced in this paper flagged as re-derive-before-coding, and the +50.96 dB shelf defect is unexplained. Therum does not ship bell/shelf filters from an unverified table.
2. **No verified zipper-noise audibility threshold** (Q6). Practitioner time constants exist; a measured threshold does not. Therum smooths and measures, it does not cite a threshold.
3. **"ADAA ≈ 4–8× oversampling" is a rule of thumb** (Q5), not a measured figure. Therum measures its own alias-rejection dB.
4. **Serum 2 internals are product claims** (Q7) — grain window, spectral partial count, LFO maximum rate, FX order defaults, and whether warps skip mipmaps. Therum treats them as reported behaviour and tests empirically where they matter.
5. **No CPU figures exist in the primary literature** (Q4) for the DSP in this paper. Every CPU number Therum publishes will be its own benchmark.
6. **Serum 2's price and release date** are stated from secondary sources (§2). Xfer's own page confirms the features and the free upgrade, not the price.
7. **MSEG has no canonical paper**; it is product practice. Therum implements it and does not cite literature for it.
8. **The ISMR 2016 PDF fetch failed** during the research for this paper (upstream error); the >100 dB 4-point BLEP figure comes from search-indexed text of that paper, not a direct PDF read. Re-fetch before final citation.
9. **McAulay & Quatieri (1986) full citation** (volume/pages) was not verified in this task — verify before citing.
10. **The FreeEQ8 paper's Bristow-Johnson citation (Preprint 4486) is wrong** — it is 4400 [8]. This paper cites 4400, and the FreeEQ8 paper should be corrected.

---

## 9. Future work

**The immediate work is the Therum build (§6), not the items in this section.** These are the long-term direction, recorded here so the build does not close them off.

- **ARC Audio Bench.** By the time the work reaches Vox8, Therum's measurement plan (§7) should run on ARC Audio Bench — the benchmark harness in the ARC ecosystem. The bench is the vehicle for the alias test, the CPU benchmarks and the instance-scaling test, run the same way on every machine. The ARC repos (ARC Core) are the listing for the bench.
- **A compiling language of our own.** Eventually, the ARC ecosystem should include a compiling language of our own for DSP — so the DSP in this paper is written once, in a language that understands sample rates, block sizes and real-time constraints, and compiled to the targets Therum ships. That is a multi-year direction, not a task for this paper or this build.
- **JUCE = free, iPlug2 = full (Gary's model, 2026-11-07).** Therum JUCE is the FREE version — workshopped, kept as the free tier. Therum iPlug2 is the FULL / PRO version, built in parallel, mirroring the free version. The framework-free DSP core (§6, rank 1) is what makes that direction possible: the core survives any future framework move, including a move to a language of our own. [Correction 2026-11-07: the v1.0 text said "No JUCE again… FreeEQ8 is the only JUCE plugin… there is no new JUCE work." That is superseded — Therum JUCE is the free version and there is new JUCE work.]
- **DSP items deferred from this paper:** spectral dynamics mode (per-bin FFT threshold clamping, Soothe2 territory); zero-lag auto-switch between linear-phase and minimum-phase on transient detection; embedded/fixed-point port (Direct Form I for fixed-point, noise-shaped quantisation) — all from the FreeEQ8 paper's future work [1, §9], carried forward, none started.

---

## References

[1] G. Doman, "Lock-Free Dynamic EQ Architecture: A Production-Grade SVF Implementation in JUCE/C++," FreeEQ8 / ProEQ8 Open-Source DSP Project, 2026. https://github.com/GareBear99/FreeEQ8 (PAPER.md; DAFx26 submission, LaTeX source `paper/DAFx26_FreeEQ8.tex`).

[2] Xfer Records, "Serum 2: Advanced Hybrid Synthesizer," product page. https://www.xferrecords.com/products/serum-2 (features, 626 presets, 288 wavetables, free upgrade for Serum 1 owners, VST3/AU/AAX).

[3] Dubspot, "Serum 2 Review: Xfer's Hybrid Synth, One Year On," blog.dubspot.com, 2025. https://blog.dubspot.com/xfer-records-releases-serum-2 (release 18 March 2025, $249, 3 oscillators × 5 engines, 10 LFOs / 4 envs / 8 macros, dual FX buses, arpeggiator + clip sequencer).

[4] O-Audio / VST-Development, "Xfer Records Serum 2 — Comprehensive Feature Breakdown," GitHub, 2026. https://github.com/taylorbrook/o-audio-vst-development/blob/HEAD/research/serum-2-feature-breakdown-competitive-analysis.md ($189 introductory / $249 full price / $9.99/mo rent-to-own; 256-frame wavetables; 16-voice unison; PZSVF; spectral warps).

[5] "Therum — JUCE → iPlug2 migration plan," 2026-11-04. `~/workspace/therum-iplug2-plan.md` (scaffold inventory: 1,748 lines C++, 568 lines DSP, never compiled, sine-only wavetable, fake FX, no modulation, no licence; Serum 2 as the bar; iPlug2 evaluation; phased migration; Serum-beating roadmap; decisions D1–D10). Note 2026-11-07: the plan's inventory is superseded by §3 of this paper (the plugin now builds, loads and sounds); the plan remains the historical record of the migration design, and Gary's 2026-11-07 model (JUCE = free, iPlug2 = full, built in parallel) refines its "leave JUCE entirely" decision.

[6] "iPlug2 as a JUCE replacement for Therum — research report," 2026-11-05. `~/workspace/iplug2-research.md` (zlib-like licence; VST3/AU/CLAP/AAX/Standalone/Web; no `juce::dsp` equivalent; IGraphics NanoVG/Skia; flat `IParam`, binary positional state; single-maintainer risk).

[7] "Therum — Wavetable Synthesis & Anti-Aliased Oscillators: DSP Literature Research," 2026-11-05. `~/workspace/therum-wavetable-anti-aliasing-research.md` (17 references; Bristow-Johnson 4400 correction; Stilson & Smith bound; mipmaps; BLEP/PolyBLEP/DPW; supersaw/unison/sub; FM/PM and PD warps; measured alias-suppression figures; unverified list).

[8] R. Bristow-Johnson, "Wavetable Synthesis 101, A Fundamental Perspective," Proc. AES 101st Convention, Los Angeles, Nov. 1996, Preprint **4400**. (Cited as 4486 in [1] — that is an error; 4400 is verified against the AES E-Library, the CCRMA bibliography, and the full-text PDF.)

[9] T. Stilson and J. O. Smith, "Alias-Free Digital Synthesis of Classic Analog Waveforms," Proc. ICMC 1996, Hong Kong, pp. 332–335. (BLIT; bound `max(Inc) = N/n_h`, `Inc = N·f₀/fₛ`.)

[10] M. Tytel, "Practical Guide to Optimized High-Quality Wavetable Oscillators," ADC21 conference talk, 2021, + Vital source (GPLv3, github.com/mtytel/vital). (On-the-fly band-limiting: FFT → zero bins above Nyquist → IFFT per note/block.)

[11] V. Välimäki and A. Huovilainen, "Antialiasing Oscillators in Subtractive Synthesis," IEEE Signal Process. Mag. 24(2):116–125, 2007. (Taxonomy; PolyBLEP introduced; NMR auditory error measure.)

[12] E. Brandt, "Hard Sync Without Aliasing," Proc. ICMC 2001, pp. 365–368. (BLEP / minBLEP.)

[13] F. Esqueda, S. Bilbao and V. Välimäki, "Rounding Corners with BLAMP," Proc. DAFx-16, Brno, 2016. (PolyBLAMP: aliasing −50 dB, SNR +~20 dB.)

[14] V. Välimäki, J. Nam, J. O. Smith and J. S. Abel, "Alias-Suppressed Oscillators Based on Differentiated Polynomial Waveforms," IEEE Trans. Audio Speech Lang. Process. 18(4):786–, 2010. (DPW: `y(n)=c·(x²(n)−x²(n−1))`, `c=fₛ/(4f₀)`; 4th-order DPW perceptually alias-free over the grand-piano register.)

[15] F. Esqueda, S. Bilbao and V. Välimäki, "Eliminating Aliasing Caused by Discontinuities Using Integrals of the Sinc Function," Proc. ISMR 2016, Paper ISMR2016-48. (4-point BLEP: spurious near DC/Nyquist >100 dB down. Note §8.8: the PDF fetch failed during research; the figure comes from search-indexed text — re-fetch before final citation.)

[16] V. Välimäki, J. Pekonen and J. Nam, "Perceptually Informed Synthesis of Bandlimited Classical Waveforms Using Integrated Polynomial Interpolation," J. Acoust. Soc. Am., 2012. (Ideal BLEP ≡ sine integral `Si(x)`; integrated 3rd-order B-spline perceptually alias-free to f₀=7.8 kHz at 44.1 kHz.)

[17] Roland, "JP-8000 / JP-8080 — Super Saw," product documentation, 1996. ("Free-run oscillator whose shape resembles 7 sawtooth oscillators detuned against each other.")

[18] J. M. Chowning, "The Synthesis of Complex Audio Spectra by Means of Frequency Modulation," J. Audio Eng. Soc. 21(7):525–534, 1973. (`y=sin(2πf_c·t+I·sin 2πf_m·t)=Σ J_n(I)·sin(2π(f_c+n f_m)t)`; Carson `BW≈2f_m(I+1)`.)

[19] V. Lazzarini and J. Timoney, "New Perspectives on Distortion Synthesis for Virtual Analog Oscillators," Computer Music J. 34(1):28–40, 2010. (Band-limited phase distortion.)

[20] J. Kleimola, V. Lazzarini, J. Timoney and V. Välimäki, "Phaseshaping Oscillator Algorithms for Musical Sound Synthesis," Proc. SMC 2010. (`y(n)=cos[2πφ(n)]`; PolyBLEP corrects phaseshaping discontinuities.)

[21] A. Simper, "Solving the continuous SVF equations using trapezoidal integration and equivalent currents," Cytomic, 2013. https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf (`g = tan(π·fc/fₛ)`, `k = 1/Q`, `a1 = 1/(1+g(g+k))`; mix-coefficient morphing proven stable, p.32.)

[22] A. Wishnick, "Time-Varying Filters for Musical Applications," Proc. DAFx-14, Erlangen, 2014. (Time-varying BIBO stability proof for the trapezoidal SVF.)

[23] V. Zavalishin, *The Art of VA Filter Design*, rev. 1.1.1, Native Instruments, 2012 (v2.1.2 current). (TPT; ZDF; 1-pole `y = (g·x+s)/(1+g)`; ladder, ch. 4.)

[24] S. P. Orfanidis, "Digital Parametric Equalizer Design with Prescribed Nyquist-Frequency Gain," J. Audio Eng. Soc. 45(6):444–455, 1997. (`H(s) = (G1·s²+B·s+G0·W²)/(s²+A·s+W²)`; magnitude at prescribed points only.)

[25] S. Särkkä and A. Huovilainen, "Accurate Discretization of Analog Audio Filters with Application to Parametric Equalizer Design," IEEE Trans. Audio, Speech, Lang. Process. 19(8):2486–2493, 2011. DOI 10.1109/TASL.2011.2144970. (Shannon-interpolation discretisation, magnitude *and* phase.)

[26] T. Stilson and J. O. Smith, "Analyzing the Moog VCF with Considerations for Digital Implementation," CCRMA, Stanford, 1996. (First linear digital Moog model; 1-sample-delay feedback ⇒ 5-pole, coupled fc/res — do not use.)

[27] A. Huovilainen, "Non-linear Digital Implementation of the Moog Ladder Filter," Proc. DAFx-04, Naples, 2004. (Nonlinear ladder ODEs; 5 tanh/sample; 2× oversampling; `k = 4·reso`.)

[28] S. D'Angelo and V. Välimäki, "An Improved Virtual Analog Model of the Moog Ladder Filter," Proc. ICASSP 2013. (Circuit-based model; SPICE-matched; +12 ops vs Huovilainen, same tanh count.)

[29] S. D'Angelo and V. Välimäki, "Generalized Moog Ladder Filter: Part I — Linear Analysis and Parameterization," IEEE Trans. Audio, Speech, Lang. Process. 22(12):1825–1832, 2014.

[30] D. H. Klatt, "Software for a Cascade/Parallel Formant Synthesizer," J. Acoust. Soc. Am. 67(3):971–995, 1980. (Resonator `y[n] = a·x[n]+b·y[n−1]+c·y[n−2]`, `c = −exp(−2πBT)`, `b = 2exp(−πBT)cos(2πFT)`, `a = 1−b−c`.)

[31] G. E. Peterson and H. L. Barney, "Control Methods Used in a Study of the Vowels," J. Acoust. Soc. Am. 24(2):175–184, 1952. (Vowel formant table; male means.)

[32] K. Karplus and A. Strong, "Digital Synthesis of Plucked String and Drum Timbres," Computer Music Journal 7(2):43–55, 1983. (`N = fₛ/f0`; loop filter `(1+z⁻¹)/2`.)

[33] D. A. Jaffe and J. O. Smith, "Extensions of the Karplus-Strong Plucked-String Algorithm," Computer Music Journal 7(2), 1983. (Fractional delay via first-order allpass.)

[34] M. R. Schroeder, "Natural Sounding Artificial Reverberation," J. Audio Eng. Soc. 10(3):219–223, 1962. (4 parallel combs → 2 series allpasses; ~1,000 echoes/s.)

[35] J. Kahles, F. Esqueda and V. Välimäki, "Oversampling for Nonlinear Waveshaping: Choosing the Right Filters," J. Audio Eng. Soc. 67(6):440–449, 2019. DOI 10.17743/jaes.2019.0012. (M=8; passband 0–16 kHz ±1 dB; stopband ≥40 dB from 28.1 kHz; only CIC decimation perceptually sufficient.)

[36] J. D. Parker, V. Zavalishin and E. Le Bivic, "Reducing the Aliasing of Nonlinear Waveshaping Using Continuous-Time Convolution," Proc. DAFx-16, Brno, pp. 137–144, 2016. (ADAA: `y[n]=(F(x[n])−F(x[n−1]))/(x[n]−x[n−1])`; tanh `F=ln(cosh x)`.)

[37] S. Bilbao, F. Esqueda, J. D. Parker and V. Välimäki, "Antiderivative Antialiasing for Memoryless Nonlinearities," IEEE Signal Process. Lett. 24:1049–1053, 2017.

[38] F. Esqueda et al., "Antiderivative Antialiasing for Stateful Systems," Proc. DAFx-19, 2019. (ADAA extended to stateful systems; 2× oversampling counters ADAA's in-band lowpass + half-sample delay.)

[39] E. N. Lorenz, "Deterministic Nonperiodic Flow," J. Atmospheric Sciences 20:130–141, 1963. (Lorenz ODEs; σ=10, ρ=28, β=8/3.)

[40] "Therum — Modulation, Effects, Granular & Spectral Synthesis: DSP Literature Research," 2026-11-05. `~/workspace/therum-modulation-effects-granular-spectral-research.md` (24 references; smoothing formula and τ values; Lorenz; MPE; FDN/Dattorro chain; Gardner convolution; Giannoulis compressor; Bode shifter; Gabor/Roads/Truax granular; Flanagan & Golden / Laroche & Dolson / Serra & Smith spectral; unverified list).

[41] MIDI Association, "MIDI Polyphonic Expression (MPE)," midi.org (current specification). (Per-note channel; bend ±48 st, master ±2; aftertouch = Channel Pressure; CC74 = timbre/slide; max 15 notes/zone.)

[42] J. A. Moorer, "About This Reverberation Business," Computer Music Journal 3(2):13–28, 1979. (FIR early reflections + LPF-in-comb + 6 combs.)

[43] J. Stautner and M. Puckette, "Designing Multi-Channel Reverberators," Computer Music Journal 6(1):52–65, 1982. (FDN: `Y=D(X+G·Y)`.)

[44] J.-M. Jot and A. Chaigne, "Digital Delay Networks for Designing Artificial Reverberators," AES 90th Convention, Preprint 3030, 1991. (FDN `H(z)=cᵀ[D(z⁻¹)−G]⁻¹b`; stability iff `G=g·M`, M unitary, `|g|<1`; Householder.)

[45] J.-M. Jot, "Efficient Models for Reverberation and Distance Rendering," Proc. ICMC, Thessaloniki, 1997. (8–16 delays, 1–2 s total, suffice for density.)

[46] J. Dattorro, "Effect Design, Part 1: Reverberator and Other Filters," J. Audio Eng. Soc. 45(9):660–684, 1997. (Plate: input diffuser + figure-8 tank; full coefficients published.)

[47] W. G. Gardner, "Efficient Convolution Without Input–Output Delay," J. Audio Eng. Soc. 43(3):127–136, 1995. (Non-uniform partitioned convolution; time-domain first block = zero latency.)

[48] D. Giannoulis, M. Massberg and J. D. Reiss, "Digital Dynamic Range Compressor Design — A Tutorial and Analysis," J. Audio Eng. Soc. 60(6):399–408, 2012. (Feedforward vs feedback; decoupled peak detector; `c_dB=(1/R−1)(x_G−T)`.)

[49] H. Bode, "Solid State Audio Frequency Spectrum Shifter," AES Preprint 395, 1965. (Hilbert 90° network + complex sinusoid; fixed ±Hz shift.)

[50] D. Gabor, "Acoustical Quanta and the Theory of Hearing," Nature 159(4044):591–594, 1947. (`Δt·Δf ≥ 1/(4π)`; Gaussian window attains the bound.)

[51] C. Roads, *Microsound*, MIT Press, 2001 (see also 1978). (Grains <50 ms, >20/s fuse; grainlets/pulsars/clouds.)

[52] B. Truax, "Real-Time Granular Synthesis with a Digital Signal Processor," Computer Music Journal, 1986+ (first real-time granular system; 2,375 grains/s peak).

[53] J. L. Flanagan and R. M. Golden, "Phase Vocoder," Bell Syst. Tech. J. 45(9):1493–1509, 1966. (STFT → modify → ISTFT.)

[54] J. Laroche and M. Dolson, "Improved Phase Vocoder Time-Scale Modification of Audio," IEEE Trans. Speech & Audio 7(3):323–332, 1999. (Identity phase-locking: `∠Y[m,k]=∠Y[m−1,k]+H_s·ω_inst`.)

[55] X. Serra and J. O. Smith, "Spectral Modeling Synthesis (Deterministic + Stochastic)," Computer Music Journal 14(4):12–24, 1990. (`s(n)=Σ A_p(n)cos θ_p(n)+e(n)`; partials + filtered-noise residual.)

[56] D. W. Griffin and J. S. Lim, "Signal Estimation from Modified STFT," IEEE Trans. ASSP 32(2):236–243, 1984. (Iterative ISTFT/STFT magnitude replacement.)

[57] R. Bencina, "Some notes on lock-free and wait-free algorithms," rossbencina.com/code/lockfree. (SPSC ring buffer: wait-free, no CAS, acquire/release on two indices, `alignas(64)` against false sharing.)

[58] D. Rowland, "Lock-free queues in the multiverse of madness," ADC talk, 2025. https://drowaudio.github.io/presentations/ADC%202025%20-%20Lock-free%20queues%20in%20the%20multiverse%20of%20madness/Lock-free%20queues%20in%20the%20multiverse%20of%20madness.pdf (FIFO survey; fixed capacity = real-time safe; unbounded is not.)

[59] "Therum — Digital Filters & Oversampling: Literature Research Report," 2026-11-05. `~/workspace/therum-filters-oversampling-research.md` (30 references; Simper SVF + Zavalishin ZDF; Wishnick stability; RBJ/Orfanidis/Särkkä–Huovilainen; Moog ladder models; Kahles oversampling; ADAA; Klatt formant; Karplus–Strong; unverified list — including the Simper bell/shelf mix coefficients, which could not be independently verified.)

---

*Therum paper, v1.1 — 2026-11-07. Updated: Therum now builds as a plugin (VST3 + AU, first compiled 2026-11-07), loads in Ableton Live 12 Lite, and sounds (proven). The editor is broken (the blocker). The JUCE version is the FREE tier; the iPlug2 version is the FULL/PRO tier, built in parallel. The DSP in §§4–5 remains a specification to be built and measured (§§6–7) where not yet built; §3 states what is built and measured. The numbers that are Therum's are Therum's measurements; every other number in this paper belongs to the paper cited next to it.*
