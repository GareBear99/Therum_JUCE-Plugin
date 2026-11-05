# Therum DSP core (framework-free)

This directory is the DSP core: plain C++ with **no framework includes**. It
does not include JUCE, iPlug2, or any host code. It is the asset that survives
any future framework move — the core was extracted first (paper §6, rank 1)
so the synth can move to iPlug2, and later to a language of our own, without
rewriting the DSP.

## The rule

No file under `Source/DSP/` may include a framework header. Verify it:

```
grep -rn "juce\|iplug\|IPlug" Source/DSP/ --include="*.h" --include="*.cpp"
```

That must return nothing. The standalone test build (`Tests/`) proves it:
the tests compile only these files, with no framework on the include path,
so any framework include fails the build.

## What is here

| Directory | Contents |
|---|---|
| `Oscillators/` | Wavetable oscillator + bank (sine only today; the real multi-frame engine is paper §6 rank 2). Granular / spectral oscillators are stubs that return silence until ranks 10–11. Sample + noise oscillators exist but are orphaned (never instantiated) until rank 10. |
| `Filters/` | One-pole lowpass today. The SVF core + dual filters are paper §6 rank 3. |
| `Modulation/` | Linear ADSR envelope, sine LFO, mod matrix (storage only) + evaluator (macro-only today). Wiring the evaluator and the envelopes is paper §6 rank 5. |
| `Voicing/` | Voice pool + voice manager. Round-robin stealing, same-note retrigger, release tails, parameter smoothing, output limiter (all fixed in the step-1 rebuild). |
| `FX/` | FX rack. Fake today (distortion is a hard clip; delay/reverb only attenuate). The real rack is paper §6 rank 8. Do not mistake it for a real effect. |
| `Utils/` | `SmoothedParameter` (one-pole parameter smoother) and `Limiter` (peak limiter, instant attack, slow release). Both are used by the voice manager. |

## Step 1 fixes (paper §6 rank 1)

The scaffold bugs fixed when the core was extracted:

1. **Release tail.** `TherumVoice::stop()` used to deactivate the voice in the
   same call as `noteOff()`, so the release never rendered a sample. Now
   `stop()` only triggers the release; the voice keeps rendering until the
   envelope finishes, then deactivates itself in `render()`.
2. **Parameter smoothing.** Every parameter that reaches audio (cutoff, macro,
   FX mixes, master gain) is one-pole smoothed in the voice manager, once per
   sample, so knob and automation moves glide instead of zipping.
3. **Voice stealing.** Stealing always took voice 0. It is round-robin now:
   the cursor advances on every steal. A note-on for a note already sounding
   retriggers that voice instead of starting a duplicate.
4. **Phase reset.** Oscillator phases (and the filter state) are reset on
   `start()`, so every retrigger begins at the same point as a first trigger.
5. **Cutoff clamp.** The filter cutoff is clamped to 20 Hz – min(20 kHz,
   Nyquist) and non-finite values are ignored. The coefficient is only
   recomputed when the cutoff actually changes (no `exp()` per sample on a
   static cutoff).
6. **Limiter.** A peak limiter sits on the summed voice output (threshold
   0.99, instant attack, 100 ms release), so the mix can never clip the host
   even with all 16 voices at full velocity.
7. **Tail length.** `getTailLengthSeconds()` returns the envelope's release
   time (0.22 s) instead of zero, so the host does not cut the tail off.

## Tests

`Tests/` holds the standalone unit tests for this core. They compile only the
DSP files (no framework, no DAW) and cover the step-1 fixes: release tail,
round-robin stealing, same-note retrigger, phase reset, cutoff clamping,
smoothing, and the limiter. See `Tests/README.md` for how to run them.
