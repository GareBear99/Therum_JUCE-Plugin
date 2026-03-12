cmake_minimum_required(VERSION 3.22)
project(Therum VERSION 0.1.0)

# This is a scaffold CMake file intended for later JUCE integration.
# Add JUCE as a submodule or package and replace placeholder sections accordingly.

set(THERUM_SOURCES
    Source/Core/TherumPluginProcessor.cpp
    Source/Core/TherumPluginEditor.cpp
    Source/DSP/Oscillators/WavetableOscillator.cpp
    Source/DSP/Modulation/EnvelopeGenerator.cpp
    Source/DSP/Modulation/LfoGenerator.cpp
    Source/DSP/Modulation/ModMatrix.cpp
    Source/DSP/Voicing/TherumVoice.cpp
    Source/DSP/Voicing/VoiceManager.cpp
    Source/Preset/PresetState.cpp
    Source/UI/Panels/OscPanel.cpp
    Source/UI/Panels/MatrixPanel.cpp
    Source/Mascot/PunchReactorState.cpp
)


# Suggested additions
#    Source/UI/Panels/SplashPanel.cpp
#    Source/Mascot/SplashMascotController.cpp


# Suggested additions
#    Source/Core/ParameterIDs.h
#    Source/UI/Panels/TopBarPanel.cpp
#    Source/UI/Panels/DiagnosticsPanel.cpp
#    Source/UI/Visualizers/BootBarView.cpp


# Suggested additions
#    Source/Core/TherumDiagnostics.h
#    Source/DSP/FX/FxRack.cpp
#    Source/DSP/Filters/DualFilter.cpp


# Suggested additions
#    Source/DSP/Oscillators/WavetableBank.cpp
#    Source/DSP/Filters/SimpleLPF.cpp
#    Source/Preset/PresetIO.cpp


# Suggested additions
#    Source/DSP/Modulation/ModMatrixEvaluator.cpp


# Suggested additions
#    Source/UI/Panels/FilterPanel.cpp
#    Source/UI/Panels/ModPanel.cpp
#    Source/UI/Panels/FxPanel.cpp
#    Source/UI/Panels/BrowserPanel.cpp


# Suggested additions
#    Source/UI/TherumAttachments.cpp


# Suggested additions
#    Source/DSP/Oscillators/SampleOscillator.cpp
#    Source/DSP/Oscillators/GranularOscillator.cpp
#    Source/DSP/Oscillators/SpectralOscillator.cpp
#    Source/DSP/Filters/DualFilter.cpp
#    Source/Mascot/MascotRuntime.cpp
#    Source/Preset/PresetTagModel.cpp


# Suggested additions
#    Source/Preset/PresetBrowserFilter.cpp
#    Source/UI/Panels/MascotPanel.cpp


# Suggested additions
#    Source/Mascot/MascotAnimator.cpp
