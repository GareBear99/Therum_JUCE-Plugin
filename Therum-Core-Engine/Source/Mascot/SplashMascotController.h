#pragma once

enum class SplashState
{
    SplashIdle,
    SplashPulse,
    SplashFadeOut,
    MainUIReady
};

class SplashMascotController
{
public:
    void tick();
    float getAlpha() const { return alpha; }
    SplashState getState() const { return state; }

private:
    SplashState state = SplashState::SplashIdle;
    int ticks = 0;
    float alpha = 1.0f;
};
