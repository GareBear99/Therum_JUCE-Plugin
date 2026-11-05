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
	$(DSP_DIR)/Filters/SimpleLPF.cpp \
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

test: check test_core
	./test_core

clean:
	rm -f test_core render_wavetable
