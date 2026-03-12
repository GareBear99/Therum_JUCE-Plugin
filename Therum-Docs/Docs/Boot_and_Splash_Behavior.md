# THERUM Boot and Splash Behavior

## Canonical startup sequence

1. Plugin instance opens
2. **Mascot Splash Pre-Boot** appears immediately
3. THERUM title + subtitle fade in
4. Short boot indicator animates
5. Main editor shell resolves underneath
6. Splash dissolves into the normal UI

## Mascot splash purpose
- brand identity
- premium first impression
- hides cold-open emptiness
- makes startup feel intentional

## Rules
- splash duration should be brief (roughly 1.0–1.8 seconds target)
- never block the host unnecessarily
- allow instant skip on click/key/mouse
- never cover fatal initialization errors
- if boot fails, splash transitions to a visible diagnostic/error surface

## Visual direction
- dark background
- neon cyan / violet accent glow
- mascot front and center
- THERUM wordmark beneath
- subtle scanline / glow / pulse
- lightweight animation only

## States
- SplashIdle
- SplashPulse
- SplashFadeOut
- MainUIReady
