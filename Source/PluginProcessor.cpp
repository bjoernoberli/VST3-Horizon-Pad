#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <limits>

using namespace horizon;

namespace
{
    const juce::Identifier kStateType { "HorizonPadState" };
    const juce::Identifier kToneType  { "Tone" };
    const juce::Identifier kLayerType { "Layer" };
    const juce::Identifier kProgramProperty { "program" };

    juce::String percentText (float value, int)
    {
        return juce::String (juce::roundToInt (value * 100.0f)) + " %";
    }

    float percentValue (const juce::String& text)
    {
        return juce::jlimit (0.0f, 1.0f, text.getFloatValue() * 0.01f);
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout HorizonPadAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    const juce::NormalisableRange<float> unitRange { 0.0f, 1.0f };

    auto addPercent = [&] (const char* id, const char* name, float defaultValue)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 },
            name,
            unitRange,
            defaultValue,
            juce::AudioParameterFloatAttributes()
                .withStringFromValueFunction (percentText)
                .withValueFromStringFunction (percentValue)
                .withLabel ("%")));
    };

    // The eight (and only eight) host-automatable parameters, in the order the
    // product spec lists them.
    addPercent (ParamID::warmPadVolume,       "Warm Pad Volume",        0.82f);
    addPercent (ParamID::analogStringsVolume, "Analog Strings Volume",  0.66f);
    addPercent (ParamID::granularVolume,      "Granular Texture Volume", 0.28f);
    addPercent (ParamID::subPadVolume,        "Sub Pad Volume",         0.45f);
    addPercent (ParamID::reverb,              "Reverb",                 0.45f);
    addPercent (ParamID::delay,               "Delay",                  0.22f);
    addPercent (ParamID::filter,              "Filter",                 0.68f);
    addPercent (ParamID::fxAmount,            "FX Amount",              0.60f);

    return layout;
}

//==============================================================================
HorizonPadAudioProcessor::HorizonPadAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
                                .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    layers = { &warmPad, &analogStrings, &granular, &subPad };

    const char* volumeIds[] { ParamID::warmPadVolume, ParamID::analogStringsVolume,
                              ParamID::granularVolume, ParamID::subPadVolume };
    const char* globalIds[] { ParamID::reverb, ParamID::delay, ParamID::filter, ParamID::fxAmount };

    for (int i = 0; i < kNumLayers; ++i)
    {
        volumeParams[(size_t) i] = apvts.getRawParameterValue (volumeIds[i]);
        jassert (volumeParams[(size_t) i] != nullptr);
    }

    for (int i = 0; i < kNumGlobalParams; ++i)
    {
        globalParams[(size_t) i] = apvts.getRawParameterValue (globalIds[i]);
        jassert (globalParams[(size_t) i] != nullptr);
    }

    // The parameter defaults above are already Golden Horizon's values, so the
    // constructor only has to seed the non-automated tone block. Deliberately
    // no setValueNotifyingHost() here - hosts dislike parameter traffic from a
    // processor constructor.
    toneState.store (getFactoryPresets().front().tone);
}

HorizonPadAudioProcessor::~HorizonPadAudioProcessor() = default;

//==============================================================================
void HorizonPadAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto maxBlock = juce::jmax (1, samplesPerBlock);

    layerBuffer.setSize (2, maxBlock, false, true, false);
    layerBuffer.clear();

    for (auto* layer : layers)
        layer->prepare (sampleRate, maxBlock);

    fxChain.prepare (sampleRate, maxBlock);

    for (auto& g : layerGain)
    {
        g.reset (sampleRate, 0.03);
        g.setCurrentAndTargetValue (0.0f);
    }

    for (int i = 0; i < kNumLayers; ++i)
        layerGain[(size_t) i].setCurrentAndTargetValue (volumeParams[(size_t) i]->load());

    // Force a tone refresh on the first block.
    lastToneGeneration = 0;
    const auto tones = toneState.load();
    for (int i = 0; i < kNumLayers; ++i)
        layers[(size_t) i]->setTone (tones[(size_t) i]);

    allNotesOff (true);
}

void HorizonPadAudioProcessor::releaseResources()
{
    for (auto* layer : layers)
        layer->reset();

    fxChain.reset();
}

bool HorizonPadAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Stereo out only; no inputs (it is an instrument).
    if (layouts.getMainInputChannels() != 0)
        return false;

    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

//==============================================================================
int HorizonPadAudioProcessor::getNumPrograms()   { return getNumFactoryPresets(); }
int HorizonPadAudioProcessor::getCurrentProgram() { return currentProgram.load(); }

const juce::String HorizonPadAudioProcessor::getProgramName (int index)
{
    const auto& presets = getFactoryPresets();

    if (! juce::isPositiveAndBelow (index, (int) presets.size()))
        return {};

    return presets[(size_t) index].name;
}

void HorizonPadAudioProcessor::changeProgramName (int, const juce::String&)
{
    // Factory programs are read-only; user variations live in the plugin state,
    // which the host saves separately.
}

void HorizonPadAudioProcessor::setCurrentProgram (int index)
{
    const auto& presets = getFactoryPresets();

    if (! juce::isPositiveAndBelow (index, (int) presets.size()))
        return;

    currentProgram.store (index);
    applyPreset (presets[(size_t) index]);
    sendChangeMessage();
}

void HorizonPadAudioProcessor::applyPreset (const Preset& preset)
{
    const char* volumeIds[] { ParamID::warmPadVolume, ParamID::analogStringsVolume,
                              ParamID::granularVolume, ParamID::subPadVolume };
    const char* globalIds[] { ParamID::reverb, ParamID::delay, ParamID::filter, ParamID::fxAmount };

    auto setParam = [&] (const char* id, float value)
    {
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (value));
    };

    for (int i = 0; i < kNumLayers; ++i)
        setParam (volumeIds[i], preset.volumes[(size_t) i]);

    for (int i = 0; i < kNumGlobalParams; ++i)
        setParam (globalIds[i], preset.globals[(size_t) i]);

    // One store() bumps the generation counter once, so the audio thread sees
    // the whole tone block change atomically enough for its purposes.
    toneState.store (preset.tone);
}

void HorizonPadAudioProcessor::setToneValue (int layer, int field, float value)
{
    if (! juce::isPositiveAndBelow (layer, kNumLayers)
        || ! juce::isPositiveAndBelow (field, AtomicToneState::kNumToneFields))
        return;

    toneState.storeValue (layer, field, juce::jlimit (0.0f, 1.0f, value));
}

//==============================================================================
int HorizonPadAudioProcessor::findFreeVoiceSlot()
{
    // 1) A slot that is neither held nor still sounding.
    for (int i = 0; i < kMaxVoices; ++i)
    {
        if (voiceSlots[(size_t) i].midiNote >= 0)
            continue;

        bool sounding = false;

        for (auto* layer : layers)
            sounding = sounding || layer->isVoiceActive (i);

        if (! sounding)
            return i;
    }

    // 2) The oldest slot that has been released but is still ringing out.
    int best = -1;
    juce::uint32 bestOrder = std::numeric_limits<juce::uint32>::max();

    for (int i = 0; i < kMaxVoices; ++i)
    {
        if (voiceSlots[(size_t) i].midiNote >= 0)
            continue;

        if (voiceSlots[(size_t) i].order < bestOrder)
        {
            bestOrder = voiceSlots[(size_t) i].order;
            best = i;
        }
    }

    if (best >= 0)
        return best;

    // 3) Everything is held: steal the oldest.
    bestOrder = std::numeric_limits<juce::uint32>::max();
    best = 0;

    for (int i = 0; i < kMaxVoices; ++i)
    {
        if (voiceSlots[(size_t) i].order < bestOrder)
        {
            bestOrder = voiceSlots[(size_t) i].order;
            best = i;
        }
    }

    return best;
}

void HorizonPadAudioProcessor::noteOn (int midiNote, float velocity)
{
    const auto slot = findFreeVoiceSlot();

    for (auto* layer : layers)
        layer->killVoice (slot);

    voiceSlots[(size_t) slot].midiNote = midiNote;
    voiceSlots[(size_t) slot].order = ++voiceOrderCounter;

    // Ambient pads want a gentle velocity curve, not a linear one.
    const auto shaped = 0.35f + 0.65f * std::sqrt (juce::jlimit (0.0f, 1.0f, velocity));

    for (auto* layer : layers)
        layer->noteOn (slot, midiNote, shaped);
}

void HorizonPadAudioProcessor::noteOff (int midiNote)
{
    for (int i = 0; i < kMaxVoices; ++i)
    {
        if (voiceSlots[(size_t) i].midiNote != midiNote)
            continue;

        voiceSlots[(size_t) i].midiNote = -1;

        for (auto* layer : layers)
            layer->noteOff (i);
    }
}

void HorizonPadAudioProcessor::allNotesOff (bool immediately)
{
    for (int i = 0; i < kMaxVoices; ++i)
    {
        voiceSlots[(size_t) i].midiNote = -1;

        for (auto* layer : layers)
        {
            if (immediately)
                layer->killVoice (i);
            else
                layer->noteOff (i);
        }
    }
}

void HorizonPadAudioProcessor::handleMidiMessage (const juce::MidiMessage& message)
{
    if (message.isNoteOn())
        noteOn (message.getNoteNumber(), message.getFloatVelocity());
    else if (message.isNoteOff())
        noteOff (message.getNoteNumber());
    else if (message.isAllNotesOff())
        allNotesOff (false);
    else if (message.isAllSoundOff())
        allNotesOff (true);
}

//==============================================================================
void HorizonPadAudioProcessor::renderSegment (juce::AudioBuffer<float>& output, int startSample, int numSamples)
{
    const auto numChannels = juce::jmin (2, output.getNumChannels());
    numSamples = juce::jmin (numSamples, layerBuffer.getNumSamples());

    if (numSamples <= 0 || numChannels <= 0)
        return;

    auto* outL = output.getWritePointer (0, startSample);
    auto* outR = numChannels > 1 ? output.getWritePointer (1, startSample) : nullptr;

    for (int i = 0; i < kNumLayers; ++i)
    {
        layerBuffer.clear (0, numSamples);
        layers[(size_t) i]->render (layerBuffer, numSamples);

        const auto* srcL = layerBuffer.getReadPointer (0);
        const auto* srcR = layerBuffer.getReadPointer (1);
        auto& gain = layerGain[(size_t) i];

        for (int n = 0; n < numSamples; ++n)
        {
            const auto g = gain.getNextValue();
            outL[n] += srcL[n] * g;

            if (outR != nullptr)
                outR[n] += srcR[n] * g;
        }
    }
}

void HorizonPadAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const auto numSamples = buffer.getNumSamples();
    buffer.clear();

    if (numSamples <= 0)
        return;

    // --- Non-automated tone block: one relaxed load per block in the common case.
    if (toneState.pollGeneration (lastToneGeneration))
    {
        const auto tones = toneState.load();

        for (int i = 0; i < kNumLayers; ++i)
            layers[(size_t) i]->setTone (tones[(size_t) i]);
    }

    // --- Automatable parameters: set smoothing targets once per block.
    for (int i = 0; i < kNumLayers; ++i)
        layerGain[(size_t) i].setTargetValue (volumeParams[(size_t) i]->load (std::memory_order_relaxed));

    fxChain.setParameters (globalParams[0]->load (std::memory_order_relaxed),
                           globalParams[1]->load (std::memory_order_relaxed),
                           globalParams[2]->load (std::memory_order_relaxed),
                           globalParams[3]->load (std::memory_order_relaxed));

    // --- Render, splitting the block at MIDI event boundaries.
    int position = 0;

    for (const auto metadata : midiMessages)
    {
        const auto eventTime = juce::jlimit (0, numSamples, metadata.samplePosition);

        if (eventTime > position)
        {
            renderSegment (buffer, position, eventTime - position);
            position = eventTime;
        }

        handleMidiMessage (metadata.getMessage());
    }

    renderSegment (buffer, position, numSamples - position);

    fxChain.process (buffer, numSamples);

    // --- Output stage: a fixed headroom trim, then a tanh soft clip.
    //
    // The trim exists because the wettest programs (Dreamscape, Ocean Mist)
    // stack four layers plus a long reverb tail and would otherwise sit right on
    // the soft clipper with a three-note chord, let alone a six-note one. -4.4 dB
    // keeps them comfortably below it; the clipper is then only a safety net for
    // pathological automation, not part of the normal sound.
    constexpr float kOutputTrim = 0.6f;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer (ch);

        for (int n = 0; n < numSamples; ++n)
            data[n] = std::tanh (data[n] * kOutputTrim);
    }

    if (buffer.getNumChannels() > 0)
        outputLevel.store (buffer.getRMSLevel (0, 0, numSamples), std::memory_order_relaxed);
}

//==============================================================================
void HorizonPadAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ValueTree root (kStateType);
    root.setProperty (kProgramProperty, currentProgram.load(), nullptr);

    // The eight automatable parameters.
    root.appendChild (apvts.copyState(), nullptr);

    // The non-automated tone block.
    juce::ValueTree toneTree (kToneType);

    for (int layer = 0; layer < kNumLayers; ++layer)
    {
        juce::ValueTree layerTree (kLayerType);
        layerTree.setProperty ("index", layer, nullptr);

        for (int field = 0; field < AtomicToneState::kNumToneFields; ++field)
            layerTree.setProperty (juce::Identifier (AtomicToneState::fieldName (field)),
                                   toneState.loadValue (layer, field),
                                   nullptr);

        toneTree.appendChild (layerTree, nullptr);
    }

    root.appendChild (toneTree, nullptr);

    if (auto xml = root.createXml())
        copyXmlToBinary (*xml, destData);
}

void HorizonPadAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);

    if (xml == nullptr)
        return;

    auto root = juce::ValueTree::fromXml (*xml);

    if (! root.hasType (kStateType))
    {
        // Backwards/forwards tolerance: an APVTS-only state still loads.
        if (root.hasType (apvts.state.getType()))
            apvts.replaceState (root);

        return;
    }

    if (auto params = root.getChildWithName (apvts.state.getType()); params.isValid())
        apvts.replaceState (params);

    if (auto toneTree = root.getChildWithName (kToneType); toneTree.isValid())
    {
        auto tones = toneState.load();

        for (int i = 0; i < toneTree.getNumChildren(); ++i)
        {
            auto layerTree = toneTree.getChild (i);
            const int layer = layerTree.getProperty ("index", i);

            if (! juce::isPositiveAndBelow (layer, kNumLayers))
                continue;

            for (int field = 0; field < AtomicToneState::kNumToneFields; ++field)
            {
                const juce::Identifier id (AtomicToneState::fieldName (field));

                if (layerTree.hasProperty (id))
                    AtomicToneState::setField (tones[(size_t) layer], field, (float) layerTree.getProperty (id));
            }
        }

        toneState.store (tones);
    }

    currentProgram.store (juce::jlimit (0, juce::jmax (0, getNumFactoryPresets() - 1),
                                        (int) root.getProperty (kProgramProperty, 0)));

    sendChangeMessage();
}

//==============================================================================
juce::AudioProcessorEditor* HorizonPadAudioProcessor::createEditor()
{
    return new HorizonPadAudioProcessorEditor (*this);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HorizonPadAudioProcessor();
}
