# Diagnostics and CPU Plan

## Required diagnostics
- voice count
- estimated voice cost
- estimated FX cost
- clipping indicators
- modulation route count
- quality mode

## Performance doctrine
- no UI work on audio thread
- avoid unbounded voice spawning
- cap grain counts in granular mode
- quality modes: Eco / Standard / Ultra
- expose route count and heavy features visibly

## Goal
THERUM should feel flagship without becoming a CPU black hole.
