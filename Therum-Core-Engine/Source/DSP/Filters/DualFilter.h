#pragma once
#include "SimpleLPF.h"

class DualFilter
{
public:
    enum class Routing { Serial, Parallel, Split };

    void prepare(double sampleRate)
    {
        a.prepare(sampleRate);
        b.prepare(sampleRate);
    }

    void setRouting(Routing r) { routing = r; }
    void setCutoffA(float hz) { a.setCutoff(hz); }
    void setCutoffB(float hz) { b.setCutoff(hz); }

    float process(float x)
    {
        switch (routing)
        {
            case Routing::Serial:   return b.process(a.process(x));
            case Routing::Parallel: return 0.5f * (a.process(x) + b.process(x));
            case Routing::Split:    return a.process(x); // placeholder split behavior
        }
        return x;
    }

private:
    Routing routing = Routing::Serial;
    SimpleLPF a;
    SimpleLPF b;
};
