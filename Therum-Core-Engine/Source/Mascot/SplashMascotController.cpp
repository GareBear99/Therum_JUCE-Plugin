#include "SplashMascotController.h"

void SplashMascotController::tick()
{
    ++ticks;

    if (ticks > 20) state = SplashState::SplashPulse;
    if (ticks > 72) state = SplashState::SplashFadeOut;

    if (state == SplashState::SplashFadeOut)
    {
        alpha -= 0.06f;
        if (alpha <= 0.0f)
        {
            alpha = 0.0f;
            state = SplashState::MainUIReady;
        }
    }
}
