// DualFilter — two Simper SVF cores with series / parallel routing.
//
// Paper section 4.5, rank 3: the dual filter is the second half of
// the voice (Serum 2 parity: two independent filters, series or
// parallel). Each half is a full SvfFilter (LP/HP/BP/Notch, resonance
// via Q, drive), so the two halves can be different types at
// different cutoffs.
//
// Routing:
//   Serial   — b(a(x)): the second filter shapes the first's output.
//   Parallel — (a(x) + b(x)) / 2: the two filters shape the same
//              input and are mixed equally.
//   Split    — not yet defined. It currently returns a(x) (the first
//              filter only). A real split is a crossover (LP + HP at
//              the same cutoff, summed) and needs a spec before it
//              is built; until then it is a documented placeholder,
//              not a silent behaviour change.
#pragma once
#include "SvfFilter.h"

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
    void setTypeA(SvfFilter::Type t) { a.setType(t); }
    void setTypeB(SvfFilter::Type t) { b.setType(t); }
    void setCutoffA(float hz) { a.setCutoff(hz); }
    void setCutoffB(float hz) { b.setCutoff(hz); }
    void setQA(float q) { a.setQ(q); }
    void setQB(float q) { b.setQ(q); }
    void setDriveA(float d) { a.setDrive(d); }
    void setDriveB(float d) { b.setDrive(d); }
    void reset() { a.reset(); b.reset(); }

    float process(float x)
    {
        switch (routing)
        {
            case Routing::Serial:   return b.process(a.process(x));
            case Routing::Parallel: return 0.5f * (a.process(x) + b.process(x));
            case Routing::Split:    return a.process(x); // placeholder, see header
        }
        return x;
    }

private:
    Routing routing = Routing::Serial;
    SvfFilter a;
    SvfFilter b;
};
