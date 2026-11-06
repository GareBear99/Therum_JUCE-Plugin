# Therum DSP core tests — standalone build (no framework, no DAW).
#
# Usage:
#   make          build and run the tests
#   make check    verify the DSP has no framework includes, then test
#   make clean    remove the test binary
#
# The tests compile ONLY the DSP files. No JUCE, no iPlug2, no host is on
# the include path, so any framework include in the DSP fails the build.
# That is the point: a passing build proves the core is framework-free.

CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra

DSP_DIR = ../Source/DSP
DSP_SRCS = \
	$(DSP_DIR)/Oscillators/WavetableOscillator.cpp \
	$(DSP_DIR)/Oscillators/WavetableBank.cpp \
	$(DSP_DIR)/Oscillators/WavetableBankMips.cpp \
	$(DSP_DIR)/Utils/Fft.cpp \
	$(DSP_DIR)/Utils/WavImport.cpp \
	$(DSP_DIR)/Filters/SvfFilter.cpp \
	$(DSP_DIR)/Filters/MoogLadderFilter.cpp \
	$(DSP_DIR)/Filters/DualFilter.cpp \
	$(DSP_DIR)/Modulation/EnvelopeGenerator.cpp \
	$(DSP_DIR)/Voicing/TherumVoice.cpp \
	$(DSP_DIR)/Voicing/VoiceManager.cpp \
	$(DSP_DIR)/FX/FxRack.cpp

.PHONY: all check test clean

all: test

# The framework-freedom check: no framework symbol may appear in the DSP.
check:
	@if grep -rn "juce\|iplug\|IPlug\|JuceHeader" $(DSP_DIR) --include="*.h" --include="*.cpp"; then \
		echo "ERROR: framework include found in the DSP core (see above)"; exit 1; \
	else \
		echo "DSP core: no framework includes"; \
	fi

test_core: test_core.cpp $(DSP_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ test_core.cpp $(DSP_SRCS)

test_decramping: test_decramping.cpp $(DSP_DIR)/Filters/SvfFilter.cpp
	$(CXX) $(CXXFLAGS) -o $@ test_decramping.cpp $(DSP_DIR)/Filters/SvfFilter.cpp

test_moogladder: test_moogladder.cpp $(DSP_DIR)/Filters/MoogLadderFilter.cpp
	$(CXX) $(CXXFLAGS) -o $@ test_moogladder.cpp $(DSP_DIR)/Filters/MoogLadderFilter.cpp

test_voicefilter: test_voicefilter.cpp $(DSP_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ test_voicefilter.cpp $(DSP_SRCS)

render_wavetable: render_wavetable.cpp $(DSP_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ render_wavetable.cpp $(DSP_SRCS)

# Render a 997 Hz saw with mipmaps and the naive control, for the
# ARC Audio Bench alias test (paper section 7). The Bench is a
# separate repo; run it by hand afterwards:
#   python3 ~/workspace/ARC-AudioBench/scripts/alias_test.py \
#       --f0 997 wavetable_997hz.wav
render: render_wavetable
	./render_wavetable --f0 997 --out wavetable_997hz.wav
	./render_wavetable --f0 997 --out wavetable_997hz_naive.wav --no-mip

# The decramping test rig (paper section 4.5, Q2): reproduce the
# FreeEQ8 -3.383 dB baseline, verify the SVF bell mix against RBJ,
# and compare the Nyquist-pin (Orfanidis spec) and geometric-mean
# pin candidates on the same rig. The Bench's filter-response /
# cramping test (Bench priority 1) can adopt this rig.
decramp: test_decramping
	./test_decramping

# The Moog ladder test (paper section 4.5, Huovilainen 2004 at 2x):
# core response vs the analogue ladder prototype at fc = 100 / 1000 /
# 8000 / 16000 Hz (the low-fc rows are the "decramping at lower fc"
# measurement), DC gain 1/(1+k), rolloff, resonance peak,
# self-oscillation, stability sweep, 1x-vs-2x alias comparison,
# full-path response, and a CPU benchmark.
moog: test_moogladder
	./test_moogladder

test: check test_core test_voicefilter
	./test_core
	./test_voicefilter

# The voice filter test (paper section 4.5, ladder wired into the
# voice at 1x): ladder equivalence, SVF unchanged, the 1x cutoff
# clamp, click-free type switching, denormals, the 1x stability
# sweep, the voice render, the NMR renders for the Bench, and the
# UI selector path (FilterTypeChoice mapping + the processor's
# per-block drive of the crossfade).
voicefilter: test_voicefilter
	./test_voicefilter

clean:
	rm -f test_core render_wavetable test_decramping test_voicefilter
