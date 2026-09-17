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
    FORMATS                     VST3 Standalone
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
    Source/presets/UserPresetStore.cpp

    Source/gui/HorizonLookAndFeel.cpp
    Source/gui/TitleBanner.cpp
    Source/gui/PresetBar.cpp
    Source/gui/PadKnob.cpp
    Source/gui/MacrosPanel.cpp
    Source/gui/WheelSlider.cpp
    Source/gui/OutputMeter.cpp
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

# ---------------------------------------------------------------------------
# HorizonPadSoundTool
#
# An offline render+analysis CLI for the sound-design iteration loop (see
# tools/sound_tool/Main.cpp and .claude/agents/sound-designer.md). It links
# directly against the "HorizonPad" target - the same static "SharedCode"
# library (its build output is literally named libHorizon Pad_SharedCode.a,
# via juce_add_plugin()'s internal _juce_set_output_name call) that the VST3
# and Standalone targets above link against - rather than recompiling any
# DSP or plugin-processor code. (juce_add_plugin(HorizonPad ...) makes
# "HorizonPad" itself the shared-code static library target; HorizonPad_VST3
# / HorizonPad_Standalone are the separate per-format targets that link
# against it - that's why target_sources/target_include_directories/
# target_link_libraries above are called directly on "HorizonPad".)
#
# HorizonPad (SharedCode) already contains a full compiled copy of every
# JUCE module it needs (juce_core, juce_dsp, juce_audio_processors, etc. -
# see its object list in the generated build files), so this target must
# NOT also link those juce:: module targets directly: that would recompile
# each module's implementation a second time into this target and produce
# duplicate-symbol linker errors. It only needs:
#   - the same include paths / preprocessor defines PluginProcessor.cpp was
#     compiled with (so the headers it includes resolve identically), and
#   - the same platform frameworks/system libraries the plugin formats above
#     link against (SharedCode's compiled JUCE modules call into these, but
#     a static library itself doesn't pull them in - the final linked
#     executable has to supply them).
# ---------------------------------------------------------------------------
add_executable(HorizonPadSoundTool tools/sound_tool/Main.cpp)

target_include_directories(HorizonPadSoundTool PRIVATE
    Source
    "${CMAKE_CURRENT_BINARY_DIR}/HorizonPad_artefacts/JuceLibraryCode"
    "${juce_SOURCE_DIR}/modules")

target_compile_definitions(HorizonPadSoundTool PRIVATE
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_MODULE_AVAILABLE_juce_audio_basics=1
    JUCE_MODULE_AVAILABLE_juce_audio_devices=1
    JUCE_MODULE_AVAILABLE_juce_audio_formats=1
    JUCE_MODULE_AVAILABLE_juce_audio_processors=1
    JUCE_MODULE_AVAILABLE_juce_audio_utils=1
    JUCE_MODULE_AVAILABLE_juce_core=1
    JUCE_MODULE_AVAILABLE_juce_data_structures=1
    JUCE_MODULE_AVAILABLE_juce_dsp=1
    JUCE_MODULE_AVAILABLE_juce_events=1
    JUCE_MODULE_AVAILABLE_juce_graphics=1
    JUCE_MODULE_AVAILABLE_juce_gui_basics=1
    JUCE_MODULE_AVAILABLE_juce_gui_extra=1
    # PluginProcessor.h's getName() override returns JucePlugin_Name inline;
    # a handful of other JucePlugin_* macros are referenced the same way
    # across Source/. These mirror the values juce_add_plugin() generates
    # for the real plugin targets above (PRODUCT_NAME / COMPANY_NAME /
    # PLUGIN_CODE / PLUGIN_MANUFACTURER_CODE / IS_SYNTH), so this tool's
    # translation units see identical values to PluginProcessor.cpp's.
    "JucePlugin_Name=\"Horizon Pad\""
    "JucePlugin_Desc=\"Horizon Pad\""
    "JucePlugin_Manufacturer=\"Quelle Music\""
    JucePlugin_ManufacturerCode=0x51756d75
    JucePlugin_PluginCode=0x487a7064
    JucePlugin_VersionCode=0x10000
    "JucePlugin_VersionString=\"1.0.0\""
    JucePlugin_Version=1.0.0
    JucePlugin_IsSynth=1
    JucePlugin_WantsMidiInput=1
    JucePlugin_ProducesMidiOutput=0
    JucePlugin_IsMidiEffect=0
    JucePlugin_EditorRequiresKeyboardFocus=0
    JucePlugin_Build_VST3=1
    JucePlugin_Build_Standalone=1
    JucePlugin_Build_VST=0
    JucePlugin_Build_AU=0
    JucePlugin_Build_AUv3=0
    JucePlugin_Build_AAX=0
    JucePlugin_Build_LV2=0
    JucePlugin_Build_Unity=0
    JucePlugin_VSTUniqueID=JucePlugin_PluginCode
    JucePlugin_VSTCategory=kPlugCategSynth
    "JucePlugin_Vst3Category=\"Instrument|Synth\""
    "JucePlugin_AUMainType='aumu'"
    JucePlugin_AUSubType=JucePlugin_PluginCode
    JucePlugin_AUManufacturerCode=JucePlugin_ManufacturerCode
    JucePlugin_AUExportPrefix=Horizon_PadAU
    "JucePlugin_AUExportPrefixQuoted=\"Horizon_PadAU\""
    JucePlugin_CFBundleIdentifier=com.quellemusic.horizonpad
    JucePlugin_AAXIdentifier=com.quellemusic.horizonpad
    JucePlugin_AAXManufacturerCode=JucePlugin_ManufacturerCode
    JucePlugin_AAXProductId=JucePlugin_PluginCode
    JucePlugin_AAXCategory=2048
    JucePlugin_AAXDisableBypass=0
    JucePlugin_AAXDisableMultiMono=0
    JucePlugin_VSTNumMidiInputs=16
    JucePlugin_VSTNumMidiOutputs=16
    JucePlugin_Enable_ARA=0
    JucePlugin_ARAContentTypes=0
    JucePlugin_ARATransformationFlags=0
    "JucePlugin_ARAFactoryID=\"com.quellemusic.horizonpad.arafactory.1.0.0\""
    "JucePlugin_ARADocumentArchiveID=\"com.quellemusic.horizonpad.aradocumentarchive.1\""
    "JucePlugin_ARACompatibleArchiveIDs=\"\""
    "JucePlugin_ManufacturerWebsite=\"\""
    "JucePlugin_ManufacturerEmail=\"\"")

target_compile_features(HorizonPadSoundTool PRIVATE cxx_std_17)

target_link_libraries(HorizonPadSoundTool PRIVATE HorizonPad)

# HorizonPad (SharedCode)'s own object compiles depend (order-only) on the
# generated JuceHeader.h; make sure it exists before we compile Main.cpp too
# (target_link_libraries alone only orders the *link* step, not our own
# source compilation, against HorizonPad).
add_dependencies(HorizonPadSoundTool HorizonPad)


if (APPLE)
    target_link_libraries(HorizonPadSoundTool PRIVATE
        "-framework Cocoa"
        "-framework Foundation"
        "-framework IOKit"
        "-framework Security"
        "-framework QuartzCore"
        "-framework Accelerate"
        "-framework AudioToolbox"
        "-framework CoreAudio"
        "-framework CoreMIDI"
        "-framework CoreAudioKit"
        "-framework DiscRecording"
        "-weak_framework Metal"
        "-weak_framework MetalKit"
        "-framework WebKit")
endif()
