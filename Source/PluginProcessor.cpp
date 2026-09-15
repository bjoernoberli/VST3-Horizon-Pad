#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <limits>

using namespace horizon;

namespace
{
    juce::String percentText (float value, int)
    {
        return juce::String (juce::roundToInt (value * 100.0f)) + " %";
    }

    float percentValue (const juce::String& text)
    {
        return juce::jlimit (0.0f, 1.0f, text.getFloatValue() * 0.01f);
    }

    /** ATTACK/FILTER macro mapping: 0.5 = the sound design's own validated
        value (no change), <0.5 halves it, >0.5 doubles it, on a log2 curve so
        the knob feels even in both directions. */
    float macroMultiplier (float macroValue) noexcept
    {
        return std::pow (2.0f, (juce::jlimit (0.0f, 1.0f, macroValue) - 0.5f) * 2.0f);
    }

    /** The eight (and only eight) host-automatable parameter IDs, in the
        fixed order used everywhere a "vols then macros" array is needed:
        Root, Clearing, Expanse, Bloom, Attack, Filter, Width, Reverb. */
    const std::array<const char*, (size_t) kNumLayers>& volumeIds()
    {
        static const std::array<const char*, (size_t) kNumLayers> ids {
            ParamID::rootVolume, ParamID::clearingVolume, ParamID::expanseVolume, ParamID::bloomVolume
        };
        return ids;
    }

    const std::array<const char*, (size_t) kNumGlobalParams>& macroIds()
    {
        static const std::array<const char*, (size_t) kNumGlobalParams> ids {
            ParamID::attackMacro, ParamID::filterMacro, ParamID::widthMacro, ParamID::reverbMacro
        };
        return ids;
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

    // The eight (and only eight) host-automatable parameters: four pad
    // volumes, then four macros - matching the GUI's knob row left to right.
    addPercent (ParamID::rootVolume,     "Root",     0.75f);
    addPercent (ParamID::clearingVolume, "Clearing", 0.30f);
    addPercent (ParamID::expanseVolume,  "Expanse",  0.15f);
    addPercent (ParamID::bloomVolume,    "Bloom",    0.25f);
    addPercent (ParamID::attackMacro,    "Attack",   0.40f);
    addPercent (ParamID::filterMacro,    "Filter",   0.30f);
    addPercent (ParamID::widthMacro,     "Width",    0.40f);
    addPercent (ParamID::reverbMacro,    "Reverb",   0.20f);

    return layout;
}

//==============================================================================
HorizonPadAudioProcessor::HorizonPadAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
                                .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    layers = { &warmFoundation, &analogEnsemble, &airyChoir, &motionPad };

    for (int i = 0; i < kNumLayers; ++i)
    {
        volumeParams[(size_t) i] = apvts.getRawParameterValue (volumeIds()[(size_t) i]);
        jassert (volumeParams[(size_t) i] != nullptr);
    }

    for (int i = 0; i < kNumGlobalParams; ++i)
    {
        macroParams[(size_t) i] = apvts.getRawParameterValue (macroIds()[(size_t) i]);
        jassert (macroParams[(size_t) i] != nullptr);
    }

    // The parameter defaults above are already Lagerfeuer's values, so there
    // is nothing else to seed at construction. Deliberately no
    // setValueNotifyingHost() here - hosts dislike parameter traffic from a
    // processor constructor.

    // Both A/B buffers start out identical, mirroring the live defaults - the
    // mockup's buffers also start as two copies of the same starting state.
    for (int i = 0; i < kNumLayers; ++i)
    {
        buffers[0].vols[(size_t) i].store (volumeParams[(size_t) i]->load());
        buffers[1].vols[(size_t) i].store (volumeParams[(size_t) i]->load());
    }

    for (int i = 0; i < kNumGlobalParams; ++i)
    {
        buffers[0].macros[(size_t) i].store (macroParams[(size_t) i]->load());
        buffers[1].macros[(size_t) i].store (macroParams[(size_t) i]->load());
    }

    // Keep the active buffer's snapshot in sync with every parameter change,
    // wherever it comes from (GUI, host automation, or a preset/program
    // recall) - see the class-level thread-safety note in the header.
    for (auto* id : volumeIds())
        apvts.addParameterListener (id, this);

    for (auto* id : macroIds())
        apvts.addParameterListener (id, this);

    userPresets = horizon::UserPresetStore::load();
}

HorizonPadAudioProcessor::~HorizonPadAudioProcessor()
{
    for (auto* id : volumeIds())
        apvts.removeParameterListener (id, this);

    for (auto* id : macroIds())
        apvts.removeParameterListener (id, this);
}

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

    activePresetKind.store ((int) PresetKind::factory, std::memory_order_relaxed);
    activePresetIndex.store (index, std::memory_order_relaxed);

    sendChangeMessage();
}

void HorizonPadAudioProcessor::applyPreset (const Preset& preset)
{
    auto setParam = [&] (const char* id, float value)
    {
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (value));
    };

    for (int i = 0; i < kNumLayers; ++i)
        setParam (volumeIds()[(size_t) i], preset.volumes[(size_t) i]);

    for (int i = 0; i < kNumGlobalParams; ++i)
        setParam (macroIds()[(size_t) i], preset.macros[(size_t) i]);

    // Wheels spring back to rest on a preset change, like a real keyboard.
    performanceState.setPitchBendSemitones (0.0f);
    performanceState.setModAmount (0.0f);
}

//==============================================================================
void HorizonPadAudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    // Keep the *active* buffer's snapshot in sync with whatever just changed -
    // this can be called from the audio thread (host automation) or the
    // message thread (a GUI edit), so only atomics may be touched here.
    const auto bufferIndex = (size_t) juce::jlimit (0, 1, activeBufferIndex.load (std::memory_order_relaxed));
    auto& buffer = buffers[bufferIndex];

    for (int i = 0; i < kNumLayers; ++i)
    {
        if (parameterID == volumeIds()[(size_t) i])
        {
            buffer.vols[(size_t) i].store (newValue, std::memory_order_relaxed);
            return;
        }
    }

    for (int i = 0; i < kNumGlobalParams; ++i)
    {
        if (parameterID == macroIds()[(size_t) i])
        {
            buffer.macros[(size_t) i].store (newValue, std::memory_order_relaxed);
            return;
        }
    }
}

void HorizonPadAudioProcessor::switchBuffer (int index)
{
    index = juce::jlimit (0, 1, index);

    if (index == activeBufferIndex.load (std::memory_order_relaxed))
        return;

    // Set the new active index *before* pushing parameter changes, so the
    // parameterChanged() callbacks those changes trigger update the buffer we
    // just switched to, not the one we're switching away from.
    activeBufferIndex.store (index, std::memory_order_relaxed);

    const auto& snapshot = buffers[(size_t) index];

    for (int i = 0; i < kNumLayers; ++i)
    {
        if (auto* p = apvts.getParameter (volumeIds()[(size_t) i]))
            p->setValueNotifyingHost (p->convertTo0to1 (snapshot.vols[(size_t) i].load (std::memory_order_relaxed)));
    }

    for (int i = 0; i < kNumGlobalParams; ++i)
    {
        if (auto* p = apvts.getParameter (macroIds()[(size_t) i]))
            p->setValueNotifyingHost (p->convertTo0to1 (snapshot.macros[(size_t) i].load (std::memory_order_relaxed)));
    }

    sendChangeMessage();
}

void HorizonPadAudioProcessor::copyActiveBufferToOtherBuffer()
{
    const auto activeIndex = (size_t) juce::jlimit (0, 1, activeBufferIndex.load (std::memory_order_relaxed));
    const auto otherIndex = 1 - activeIndex;

    auto& source = buffers[activeIndex];
    auto& dest = buffers[otherIndex];

    for (int i = 0; i < kNumLayers; ++i)
        dest.vols[(size_t) i].store (source.vols[(size_t) i].load (std::memory_order_relaxed), std::memory_order_relaxed);

    for (int i = 0; i < kNumGlobalParams; ++i)
        dest.macros[(size_t) i].store (source.macros[(size_t) i].load (std::memory_order_relaxed), std::memory_order_relaxed);

    sendChangeMessage();
}

//==============================================================================
void HorizonPadAudioProcessor::saveCurrentAsUserPreset (const juce::String& name)
{
    UserPreset preset;
    preset.name = name.isNotEmpty() ? name : "Untitled";

    for (int i = 0; i < kNumLayers; ++i)
        preset.vols[(size_t) i] = volumeParams[(size_t) i]->load (std::memory_order_relaxed);

    for (int i = 0; i < kNumGlobalParams; ++i)
        preset.macros[(size_t) i] = macroParams[(size_t) i]->load (std::memory_order_relaxed);

    userPresets.push_back (preset);
    UserPresetStore::save (userPresets);

    activePresetKind.store ((int) PresetKind::user, std::memory_order_relaxed);
    activePresetIndex.store ((int) userPresets.size() - 1, std::memory_order_relaxed);

    sendChangeMessage();
}

void HorizonPadAudioProcessor::deleteUserPreset (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) userPresets.size()))
        return;

    userPresets.erase (userPresets.begin() + index);
    UserPresetStore::save (userPresets);

    const auto activeKind = getActivePresetKind();
    const auto activeIndex = getActivePresetIndex();

    if (activeKind == PresetKind::user)
    {
        if (activeIndex == index)
            activePresetKind.store ((int) PresetKind::none, std::memory_order_relaxed);
        else if (activeIndex > index)
            activePresetIndex.store (activeIndex - 1, std::memory_order_relaxed);
    }

    sendChangeMessage();
}

void HorizonPadAudioProcessor::applyUserPreset (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) userPresets.size()))
        return;

    const auto& preset = userPresets[(size_t) index];

    auto setParam = [&] (const char* id, float value)
    {
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (value));
    };

    for (int i = 0; i < kNumLayers; ++i)
        setParam (volumeIds()[(size_t) i], preset.vols[(size_t) i]);

    for (int i = 0; i < kNumGlobalParams; ++i)
        setParam (macroIds()[(size_t) i], preset.macros[(size_t) i]);

    // Wheels spring back to rest on a preset change, like a real keyboard.
    performanceState.setPitchBendSemitones (0.0f);
    performanceState.setModAmount (0.0f);

    activePresetKind.store ((int) PresetKind::user, std::memory_order_relaxed);
    activePresetIndex.store (index, std::memory_order_relaxed);

    sendChangeMessage();
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
    else if (message.isPitchWheel())
    {
        // 14-bit, centred on 8192 -> -1..+1 -> +/- the wheel's semitone range.
        const auto normalised = ((float) message.getPitchWheelValue() - 8192.0f) / 8192.0f;
        performanceState.setPitchBendSemitones (juce::jlimit (-1.0f, 1.0f, normalised)
                                                * horizon::PerformanceState::kPitchBendRangeSemitones);
    }
    else if (message.isController() && message.getControllerNumber() == 1) // mod wheel (CC1)
    {
        performanceState.setModAmount ((float) message.getControllerValue() / 127.0f);
    }
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

    // --- Automatable parameters: set smoothing targets and per-block macros.
    for (int i = 0; i < kNumLayers; ++i)
        layerGain[(size_t) i].setTargetValue (volumeParams[(size_t) i]->load (std::memory_order_relaxed));

    const auto attackScale = macroMultiplier (macroParams[0]->load (std::memory_order_relaxed));
    const auto brightnessMul = macroMultiplier (macroParams[1]->load (std::memory_order_relaxed));
    const auto width = macroParams[2]->load (std::memory_order_relaxed);
    const auto reverbSend = macroParams[3]->load (std::memory_order_relaxed);

    const auto pitchBend = performanceState.getPitchBendSemitones();
    const auto modAmount = performanceState.getModAmount();

    for (auto* layer : layers)
    {
        layer->setMacros (attackScale, brightnessMul);
        layer->setPerformance (pitchBend, modAmount);
    }

    fxChain.setParameters (width, reverbSend);

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
    // Sternenzelt and Alpengluhen stack Expanse's shimmer/reverb on top of a
    // multi-note chord; the trim keeps that comfortably below the clipper, so
    // the clipper is only a safety net for pathological automation, not part
    // of the normal sound.
    constexpr float kOutputTrim = 0.75f;

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
    juce::ValueTree root ("HorizonPadState");
    root.setProperty ("program", currentProgram.load(), nullptr);
    root.setProperty ("activeBuffer", activeBufferIndex.load (std::memory_order_relaxed), nullptr);
    root.setProperty ("activePresetKind", activePresetKind.load (std::memory_order_relaxed), nullptr);
    root.setProperty ("activePresetIndex", activePresetIndex.load (std::memory_order_relaxed), nullptr);
    root.appendChild (apvts.copyState(), nullptr);

    // Both A/B snapshots, independent of whichever one is currently live in
    // the APVTS (which copyState() above already captures).
    const char* bufferNames[2] { "BufferA", "BufferB" };

    for (int b = 0; b < 2; ++b)
    {
        juce::ValueTree bufferTree (bufferNames[b]);

        for (int i = 0; i < kNumLayers; ++i)
            bufferTree.setProperty ("v" + juce::String (i), buffers[(size_t) b].vols[(size_t) i].load (std::memory_order_relaxed), nullptr);

        for (int i = 0; i < kNumGlobalParams; ++i)
            bufferTree.setProperty ("m" + juce::String (i), buffers[(size_t) b].macros[(size_t) i].load (std::memory_order_relaxed), nullptr);

        root.appendChild (bufferTree, nullptr);
    }

    if (auto xml = root.createXml())
        copyXmlToBinary (*xml, destData);
}

void HorizonPadAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);

    if (xml == nullptr)
        return;

    auto root = juce::ValueTree::fromXml (*xml);

    if (! root.hasType ("HorizonPadState"))
    {
        // Backwards/forwards tolerance: an APVTS-only state still loads.
        if (root.hasType (apvts.state.getType()))
            apvts.replaceState (root);

        return;
    }

    if (auto params = root.getChildWithName (apvts.state.getType()); params.isValid())
        apvts.replaceState (params);

    currentProgram.store (juce::jlimit (0, juce::jmax (0, getNumFactoryPresets() - 1),
                                        (int) root.getProperty ("program", 0)));

    activeBufferIndex.store (juce::jlimit (0, 1, (int) root.getProperty ("activeBuffer", 0)),
                             std::memory_order_relaxed);
    activePresetKind.store ((int) root.getProperty ("activePresetKind", (int) PresetKind::factory),
                            std::memory_order_relaxed);
    activePresetIndex.store ((int) root.getProperty ("activePresetIndex", 0), std::memory_order_relaxed);

    const char* bufferNames[2] { "BufferA", "BufferB" };
    bool haveSavedBuffers = false;

    for (int b = 0; b < 2; ++b)
    {
        auto bufferTree = root.getChildWithName (bufferNames[b]);

        if (! bufferTree.isValid())
            continue;

        haveSavedBuffers = true;

        for (int i = 0; i < kNumLayers; ++i)
            buffers[(size_t) b].vols[(size_t) i].store ((float) bufferTree.getProperty ("v" + juce::String (i), 0.0), std::memory_order_relaxed);

        for (int i = 0; i < kNumGlobalParams; ++i)
            buffers[(size_t) b].macros[(size_t) i].store ((float) bufferTree.getProperty ("m" + juce::String (i), 0.0), std::memory_order_relaxed);
    }

    if (! haveSavedBuffers)
    {
        // Older saved state with no A/B buffers: seed both from the
        // just-restored live parameter values, matching construction.
        for (int i = 0; i < kNumLayers; ++i)
        {
            buffers[0].vols[(size_t) i].store (volumeParams[(size_t) i]->load(), std::memory_order_relaxed);
            buffers[1].vols[(size_t) i].store (volumeParams[(size_t) i]->load(), std::memory_order_relaxed);
        }

        for (int i = 0; i < kNumGlobalParams; ++i)
        {
            buffers[0].macros[(size_t) i].store (macroParams[(size_t) i]->load(), std::memory_order_relaxed);
            buffers[1].macros[(size_t) i].store (macroParams[(size_t) i]->load(), std::memory_order_relaxed);
        }
    }

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
