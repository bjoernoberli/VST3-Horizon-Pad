cmake_minimum_required(VERSION 3.22)

project(HorizonPad VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS deployment version")

    # Universal binary: one .vst3 bundle that runs natively on both Apple
    # Silicon and Intel Macs (CMake/lipo does the fat-binary linking). Only
    # set a default if the caller hasn't already picked architectures
    # explicitly (e.g. a single-arch local dev build for faster iteration).
    if (NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for macOS" FORCE)
    endif()
endif()

# ---------------------------------------------------------------------------
# JUCE
#
# Fetched with FetchContent so a fresh clone of this repository plus CMake is
# everything you need. To build against a JUCE checkout you already have on
# disk (much faster for repeat CI / local work), configure with:
#
#   cmake -B build -DFETCHCONTENT_SOURCE_DIR_JUCE=/path/to/JUCE
# ---------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# ---------------------------------------------------------------------------
# Plugin target
# ---------------------------------------------------------------------------
juce_add_plugin(HorizonPad
    COMPANY_NAME                "Quelle Music"
    BUNDLE_ID                   "com.quellemusic.horizonpad"
    PLUGIN_MANUFACTURER_CODE    Qumu
    PLUGIN_CODE                 Hzpd
    FORMATS                     VST3
    PRODUCT_NAME                "Horizon Pad"
    VST3_CATEGORIES             "Instrument" "Synth"
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE)

juce_generate_juce_header(HorizonPad)

target_sources(HorizonPad PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp

    Source/dsp/LayerBase.cpp
    Source/dsp/WarmFoundationLayer.cpp
    Source/dsp/AnalogEnsembleLayer.cpp
    Source/dsp/AiryChoirLayer.cpp
    Source/dsp/MotionPadLayer.cpp
    Source/dsp/FxChain.cpp

    Source/presets/Presets.cpp

    Source/gui/HorizonLookAndFeel.cpp
    Source/gui/TitleBanner.cpp
    Source/gui/PresetBar.cpp
    Source/gui/PadKnob.cpp
    Source/gui/MacrosPanel.cpp
    Source/gui/WheelSlider.cpp
    Source/gui/OutputMeter.cpp
    Source/gui/OnScreenKeyboard.cpp
    Source/gui/FooterBar.cpp)

target_include_directories(HorizonPad PRIVATE Source)

target_compile_definitions(HorizonPad PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1)

target_link_libraries(HorizonPad
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
