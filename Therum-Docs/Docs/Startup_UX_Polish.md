# Startup UX Polish

## Canonical Therum startup
1. Host opens plugin instance
2. Mascot splash pre-boot appears immediately
3. Glow pulse begins
4. Boot bar advances
5. User may click to skip at any time
6. Splash fades out
7. Main editor becomes fully interactive

## Rules
- Never stall host init waiting on cosmetic logic
- Splash is cosmetic, not a blocking loader
- Skip input must be instant and safe
- If diagnostics detect an initialization problem, surface an error card instead of silently fading

## Visual polish targets
- boxer sprite sheet instead of placeholder panel
- subtle scanline / CRT shimmer kept lightweight
- cyan/violet ambient glow, amber reserved for mascot energy
- short pulse on successful fade completion
