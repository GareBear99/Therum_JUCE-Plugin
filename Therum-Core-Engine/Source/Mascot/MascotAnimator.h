#pragma once
#include <JuceHeader.h>

enum class MascotAnimState
{
    Idle,
    Splash,
    Jab,
    Uppercut,
    Guard,
    Power
};

class MascotAnimator
{
public:
    void setState(MascotAnimState s) { state = s; frame = 0; }
    void tick() { ++frame; }
    MascotAnimState getState() const { return state; }
    int getFrame() const { return frame; }

private:
    MascotAnimState state = MascotAnimState::Idle;
    int frame = 0;
};
