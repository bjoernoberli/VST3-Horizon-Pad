#pragma once

#include <JuceHeader.h>

#include "dsp/ToneState.h"
#include "dsp/LayerBase.h"
#include "dsp/WarmPadLayer.h"
#include "dsp/AnalogStringsLayer.h"
#include "dsp/GranularTextureLayer.h"
#include "dsp/SubPadLayer.h"
#include "dsp/FxChain.h"
#include "presets/Presets.h"

/**
    Horizon Pad - a four-layer ambient pad synthesiser.

    Thread-safety model
    -------------------
      * The eight host-automatable parameters live in an
        AudioProcessorValueTreeState; the audio thread reads them through cached
        std::atomic<float>* pointers and smooths them per block.

      * The non-automated tone block lives in horizon::AtomicToneState, a
        lock-free array of std::atomic<float> guarded by a generation counter.
        The UI and preset loading write it; the audio thread polls the counter
        once per block. There is no shared mutable struct and no lock anywhere
        on the audio path.

      * Programs are applied on whatever thread the host calls setCurrentProgram
        on; that only touches parameters (thread-safe) and the atomic tone block,
        then posts a ChangeBroadcaster message for the editor.
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
    double getTailLengthSeconds() const override { return 14.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- Horizon Pad --------------------------------------------------------
    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

    /** Reads a single tone-block value (message thread; safe from anywhere). */
    float getToneValue (int layer, int field) const noexcept
    {
        return toneState.loadValue (layer, field);
    }

    /** Writes a single tone-block value from the UI. Lock-free. */
    void setToneValue (int layer, int field, float value);

    /** Convenience for the editor's preset browser. */
    const std::vector<horizon::Preset>& getPresets() const { return horizon::getFactoryPresets(); }

    /** RMS of the last processed block, for the editor's live curves. 0..1. */
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
    std::array<std::atomic<float>*, (size_t) horizon::kNumGlobalParams> globalParams {};

    horizon::AtomicToneState toneState;
    juce::uint32 lastToneGeneration = 0;

    horizon::WarmPadLayer warmPad;
    horizon::AnalogStringsLayer analogStrings;
    horizon::GranularTextureLayer granular;
    horizon::SubPadLayer subPad;
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
