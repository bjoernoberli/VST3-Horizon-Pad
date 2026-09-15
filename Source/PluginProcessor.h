#pragma once

#include <JuceHeader.h>

#include "dsp/HorizonTypes.h"
#include "dsp/PerformanceState.h"
#include "dsp/LayerBase.h"
#include "dsp/WarmFoundationLayer.h"
#include "dsp/AnalogEnsembleLayer.h"
#include "dsp/AiryChoirLayer.h"
#include "dsp/MotionPadLayer.h"
#include "dsp/FxChain.h"
#include "presets/Presets.h"

/**
    Horizon Pad - a four-layer ambient pad synthesiser.

    Thread-safety model
    -------------------
      * The eight host-automatable parameters (four pad volumes, four macros)
        live in an AudioProcessorValueTreeState; the audio thread reads them
        through cached std::atomic<float>* pointers and applies them per block.

      * PITCH and MOD are performance controls, not host parameters (matching
        a real keyboard's wheels): horizon::PerformanceState holds them as two
        independent atomics, written by either incoming MIDI or the on-screen
        wheel being dragged, and read once per block by the audio thread. No
        lock anywhere on the audio path.

      * Programs are applied on whatever thread the host calls setCurrentProgram
        on; that only touches parameters (thread-safe), then posts a
        ChangeBroadcaster message for the editor.
*/
class HorizonPadAudioProcessor final : public juce::AudioProcessor,
                                       public juce::ChangeBroadcaster
{
public:
    HorizonPadAudioProcessor();
    ~HorizonPadAudioProcessor() override;

    // --- AudioProcessor -----------------------------------------------------
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- Horizon Pad --------------------------------------------------------
    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

    /** For the on-screen PITCH/MOD wheels (mouse-dragged, not host-automated). */
    horizon::PerformanceState& getPerformanceState() noexcept { return performanceState; }

    /** For the on-screen A-K keyboard: the standard JUCE mechanism for a GUI
        to trigger notes without touching audio-thread voice state directly. */
    juce::MidiKeyboardState& getKeyboardState() noexcept { return keyboardState; }

    /** Convenience for the editor's preset browser. */
    const std::vector<horizon::Preset>& getPresets() const { return horizon::getFactoryPresets(); }

    /** RMS of the last processed block, for the editor's live meter. 0..1. */
    float getOutputLevel() const noexcept { return outputLevel.load (std::memory_order_relaxed); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void applyPreset (const horizon::Preset& preset);
    void renderSegment (juce::AudioBuffer<float>& output, int startSample, int numSamples);
    void handleMidiMessage (const juce::MidiMessage& message);
    void noteOn (int midiNote, float velocity);
    void noteOff (int midiNote);
    void allNotesOff (bool immediately);
    int findFreeVoiceSlot();

    juce::AudioProcessorValueTreeState apvts;

    // Cached raw parameter pointers: no string lookups on the audio thread.
    std::array<std::atomic<float>*, (size_t) horizon::kNumLayers> volumeParams {};
    std::array<std::atomic<float>*, (size_t) horizon::kNumGlobalParams> macroParams {};

    horizon::PerformanceState performanceState;
    juce::MidiKeyboardState keyboardState;

    horizon::WarmFoundationLayer warmFoundation;
    horizon::AnalogEnsembleLayer analogEnsemble;
    horizon::AiryChoirLayer airyChoir;
    horizon::MotionPadLayer motionPad;
    std::array<horizon::LayerBase*, (size_t) horizon::kNumLayers> layers {};

    horizon::FxChain fxChain;

    std::array<juce::SmoothedValue<float>, (size_t) horizon::kNumLayers> layerGain;

    juce::AudioBuffer<float> layerBuffer;

    struct VoiceSlot
    {
        int midiNote = -1;          ///< -1 when not held
        juce::uint32 order = 0;     ///< monotonic allocation counter, for voice stealing
    };

    std::array<VoiceSlot, (size_t) horizon::kMaxVoices> voiceSlots;
    juce::uint32 voiceOrderCounter = 0;

    std::atomic<int> currentProgram { 0 };
    std::atomic<float> outputLevel { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HorizonPadAudioProcessor)
};
