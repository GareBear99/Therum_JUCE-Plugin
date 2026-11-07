# Therum — top-level JUCE build (VST3 + AU)
#
# This is the build entry point. The repo is split across two trees
# (see SPLIT_MANIFEST.json):
#   Therum-Core-Engine/Source/  — Core, DSP, Preset, Mascot
#   Therum-UI-Editor/Source/    — UI (panels, attachments, visualizers)
# The old scaffold Therum-Core-Engine/CMakeLists.txt has no build target
# and predates most of the code; it is kept for reference only.
#
# JUCE is NOT vendored here. It lives on the build machine and is pointed
# at with -DJUCE_DIR=<path to JUCE SDK> (default: /Users/gary/JUCE).
#   cmake -S . -B build -DJUCE_DIR=/Users/gary/JUCE
#   cmake --build build --config Release
#
# The DSP core stays framework-free (no JUCE includes under Source/DSP);
# verify with `make check` in Therum-Core-Engine/Tests/.

cmake_minimum_required(VERSION 3.22)
project(Therum VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(JUCE_DIR "/Users/gary/JUCE" CACHE PATH "Path to the JUCE SDK (must contain CMakeLists.txt)")
if(NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE not found at JUCE_DIR=${JUCE_DIR}. Pass -DJUCE_DIR=<path to JUCE SDK>.")
endif()
add_subdirectory(${JUCE_DIR} JUCE)

juce_add_plugin(Therum
    PRODUCT_NAME "Therum"
    COMPANY_NAME "GareBear99"
    BUNDLE_ID "com.garebear99.therum"
    PLUGIN_MANUFACTURER_CODE "GB99"
    PLUGIN_CODE "Thrm"
    FORMATS VST3 AU
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

# All 42 .cpp files in the repo (Core + DSP + Mascot + Preset + UI).
# Paths are relative to the repo root and span both split trees.
target_sources(Therum PRIVATE
    Therum-Core-Engine/Source/Core/TherumPluginProcessor.cpp
    Therum-Core-Engine/Source/Core/TherumPluginEditor.cpp

    Therum-Core-Engine/Source/DSP/FX/FxRack.cpp
    Therum-Core-Engine/Source/DSP/Filters/DualFilter.cpp
    Therum-Core-Engine/Source/DSP/Filters/MoogLadderFilter.cpp
    Therum-Core-Engine/Source/DSP/Filters/SvfFilter.cpp
    Therum-Core-Engine/Source/DSP/Modulation/EnvelopeGenerator.cpp
    Therum-Core-Engine/Source/DSP/Modulation/LfoGenerator.cpp
    Therum-Core-Engine/Source/DSP/Modulation/ModMatrix.cpp
    Therum-Core-Engine/Source/DSP/Modulation/ModMatrixEvaluator.cpp
    Therum-Core-Engine/Source/DSP/Oscillators/GranularOscillator.cpp
    Therum-Core-Engine/Source/DSP/Oscillators/NoiseOscillator.cpp
    Therum-Core-Engine/Source/DSP/Oscillators/SampleOscillator.cpp
    Therum-Core-Engine/Source/DSP/Oscillators/SpectralOscillator.cpp
    Therum-Core-Engine/Source/DSP/Oscillators/WavetableBank.cpp
    Therum-Core-Engine/Source/DSP/Oscillators/WavetableBankMips.cpp
    Therum-Core-Engine/Source/DSP/Oscillators/WavetableOscillator.cpp
    Therum-Core-Engine/Source/DSP/Utils/Fft.cpp
    Therum-Core-Engine/Source/DSP/Utils/WavImport.cpp
    Therum-Core-Engine/Source/DSP/Voicing/TherumVoice.cpp
    Therum-Core-Engine/Source/DSP/Voicing/VoiceManager.cpp

    Therum-Core-Engine/Source/Mascot/MascotAnimator.cpp
    Therum-Core-Engine/Source/Mascot/MascotRuntime.cpp
    Therum-Core-Engine/Source/Mascot/PunchReactorState.cpp
    Therum-Core-Engine/Source/Mascot/SplashMascotController.cpp

    Therum-Core-Engine/Source/Preset/PresetBrowserFilter.cpp
    Therum-Core-Engine/Source/Preset/PresetBrowserModel.cpp
    Therum-Core-Engine/Source/Preset/PresetIO.cpp
    Therum-Core-Engine/Source/Preset/PresetState.cpp
    Therum-Core-Engine/Source/Preset/PresetTagModel.cpp

    Therum-UI-Editor/Source/UI/Panels/BrowserPanel.cpp
    Therum-UI-Editor/Source/UI/Panels/DiagnosticsPanel.cpp
    Therum-UI-Editor/Source/UI/Panels/FilterPanel.cpp
    Therum-UI-Editor/Source/UI/Panels/FxPanel.cpp
    Therum-UI-Editor/Source/UI/Panels/MascotPanel.cpp
    Therum-UI-Editor/Source/UI/Panels/MatrixPanel.cpp
    Therum-UI-Editor/Source/UI/Panels/ModPanel.cpp
    Therum-UI-Editor/Source/UI/Panels/OscPanel.cpp
    Therum-UI-Editor/Source/UI/Panels/SplashPanel.cpp
    Therum-UI-Editor/Source/UI/Panels/TopBarPanel.cpp
    Therum-UI-Editor/Source/UI/TherumAttachments.cpp
    Therum-UI-Editor/Source/UI/Visualizers/BootBarView.cpp
)

# Cross-tree includes resolve through these two roots, e.g. the editor
# includes "UI/Panels/OscPanel.h" and UI code includes "DSP/...",
# "Core/...", "Mascot/...". (Same-directory and within-tree relative
# includes keep working as before.)
target_include_directories(Therum PRIVATE
    Therum-Core-Engine/Source
    Therum-UI-Editor/Source
)

# Generates JuceHeader.h in the build dir for the Projucer-style
# `#include <JuceHeader.h>` used across Core/UI/Preset/Mascot.
juce_generate_juce_header(Therum)

target_compile_definitions(Therum PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Therum PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_basics
    juce::juce_dsp
)
