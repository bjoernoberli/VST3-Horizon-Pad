#include "AiryChoirLayer.h"

namespace horizon
{

void AiryChoirLayer::prepareLayer (const juce::dsp::ProcessSpec&)
{
    const juce::dsp::ProcessSpec monoSpec { sampleRate, (juce::uint32) maxBlockSize, 1 };
    const juce::dsp::ProcessSpec stereoSpec { sampleRate, (juce::uint32) maxBlockSize, 2 };

    for (auto& vs : voiceState)
    {
        vs.bandpass.prepare (stereoSpec);
        vs.bandpass.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
        vs.bandpass.setResonance (1.6f);

        vs.safetyLowpass.prepare (stereoSpec);
        vs.safetyLowpass.setType (juce::dsp::FirstOrderTPTFilterType::lowpass);
        vs.safetyLowpass.setCutoffFrequency (1800.0f);
    }

    shimmerBus.setSize (2, maxBlockSize, false, true, false);

    shimmerHighpass.prepare (monoSpec);
    shimmerHighpass.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    shimmerHighpass.setResonance (0.6f);
    shimmerHighpass.setCutoffFrequency (1200.0f);

    shimmerAntiAlias.prepare (monoSpec);
    shimmerAntiAlias.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    shimmerAntiAlias.setResonance (0.7071f);
    shimmerAntiAlias.setCutoffFrequency ((float) (0.2 * sampleRate));

    shimmerPitch.prepare (sampleRate);

    shimmerReverb.prepare (stereoSpec);
    juce::Reverb::Parameters rp;
    rp.roomSize = 0.45f;
    rp.damping = 0.6f;   // darkened per later taming passes on this pad's tail
    rp.width = 0.0f;     // follows the layer's WIDTH, see renderLayerTail()
    rp.wetLevel = 1.0f;  // fully wet here; the blend happens in renderLayerTail
    rp.dryLevel = 0.0f;
    rp.freezeMode = 0.0f;
    shimmerReverb.setParameters (rp);
    shimmerReverbWidth = 0.0f;

    resetLayer();
}

void AiryChoirLayer::resetLayer()
{
    for (auto& vs : voiceState)
    {
        vs.phase.fill (0.0f);
        vs.upperPhase.fill (0.0f);
        vs.drift = {};
        vs.sweepPhase = 0.0f;
        vs.bandpass.reset();
        vs.safetyLowpass.reset();
    }

    shimmerBus.clear();
    shimmerHighpass.reset();
    shimmerAntiAlias.reset();
    shimmerPitch.reset();
    shimmerReverb.reset();
}

void AiryChoirLayer::startVoice (int voiceIndex, float, float)
{
    auto& vs = voiceState[(size_t) voiceIndex];

    for (int i = 0; i < kNumOscs; ++i)
    {
        vs.phase[(size_t) i] = rng.nextFloat();
        vs.upperPhase[(size_t) i] = rng.nextFloat();
        vs.drift[(size_t) i].start (rng, kOscDriftRateHz[i], sampleRate);
    }

    vs.sweepPhase = rng.nextFloat();
    vs.bandpass.reset();
    vs.safetyLowpass.reset();
}

void AiryChoirLayer::beginBlock (int numSamples)
{
    shimmerBus.clear (0, 0, juce::jmin (numSamples, shimmerBus.getNumSamples()));
}

void AiryChoirLayer::renderVoice (int voiceIndex, juce::AudioBuffer<float>& target, int numSamples)
{
    auto& v = voices[(size_t) voiceIndex];
    auto& vs = voiceState[(size_t) voiceIndex];

    auto* left  = target.getWritePointer (0);
    auto* right = target.getWritePointer (1);
    auto* shimmerIn = shimmerBus.getWritePointer (0);
    const auto shimmerLen = juce::jmin (numSamples, shimmerBus.getNumSamples());

    const auto invSr = 1.0f / (float) sampleRate;
    const auto level = 0.185f * v.velocity; // 0.22 in v1: -1.5 dB matches v1 at C4, the voicing anchor, after the one-pole LP restored the prototype's top end
    const auto noteFreq = bentFrequency (v.frequency);

    // Register pinning (see the header): below C4 the stack sounds in the
    // octave(s) nearest the one it was voiced in, crossfaded equal-power
    // between the octave below and above that point. Worked out from the
    // unbent note, so a pitch bend cannot swap octaves mid-note.
    const auto shift = 1.0f + juce::jmax (0.0f, std::log2 (kPinHz / v.frequency));
    const auto octave = std::floor (shift);
    const auto upper = shift - octave;
    const auto lowerGain = std::sqrt (1.0f - upper);
    const auto upperGain = std::sqrt (upper);
    const auto baseFreq = noteFreq * std::pow (2.0f, octave);

    // DETUNE eases out in the bass, judged by where the stack sounds - which,
    // pinned, is never the bass.
    auto detune = makeDetuneRamp (baseFreq);

    // Tracking follows where the stack actually sounds, so below C4 the
    // filters stay where they were voiced.
    const auto tracking = keyTrack (noteFreq * std::pow (2.0f, shift - 1.0f), kSweepTrackingBelowC4, kSweepTrackingAboveC4);
    const auto maxCutoff = (float) (sampleRate * 0.45);
    vs.safetyLowpass.setCutoffFrequency (juce::jlimit (40.0f, maxCutoff, 1800.0f * tracking));

    std::array<PanRamp, kNumOscs> pan;
    for (int i = 0; i < kNumOscs; ++i)
        pan[(size_t) i] = makePanRamp (kOscSpread[i], voiceIndex);

    for (int n = 0; n < numSamples; ++n)
    {
        const auto envGain = v.env.getNextSample();
        // Voice-steal declick ramp; 1.0 unless this slot is being taken over.
        const auto stealGain = nextStealGain (v);
        const auto brightness = effectiveBrightness (voiceIndex, n);
        const auto depth = driftDepth (detune.value);

        float stackL = 0.0f, stackR = 0.0f;

        for (int i = 0; i < kNumOscs; ++i)
        {
            const auto drift = vs.drift[(size_t) i].next (rng, kOscDriftRateHz[i], sampleRate) * depth;
            const auto freqHz = baseFreq * (1.0f + kOscDetuneFraction[i] * detune.value + drift);
            const auto inc = juce::jlimit (0.0f, 0.49f, freqHz * invSr);
            vs.phase[(size_t) i] = wrapPhase (vs.phase[(size_t) i] + inc);

            auto tri = polyBlampTriangle (vs.phase[(size_t) i], inc) * lowerGain;

            if (upper > 0.0f)
            {
                const auto upperInc = juce::jlimit (0.0f, 0.49f, 2.0f * freqHz * invSr);
                vs.upperPhase[(size_t) i] = wrapPhase (vs.upperPhase[(size_t) i] + upperInc);
                tri += polyBlampTriangle (vs.upperPhase[(size_t) i], upperInc) * upperGain;
            }

            auto& p = pan[(size_t) i];
            stackL += tri * p.left;
            stackR += tri * p.right;
            p.advance();
        }

        detune.advance();

        vs.sweepPhase = wrapPhase (vs.sweepPhase + 0.09f * invSr);
        const auto sweepHz = 450.0f + (std::sin (vs.sweepPhase * juce::MathConstants<float>::twoPi) * 0.5f + 0.5f) * 900.0f;
        vs.bandpass.setCutoffFrequency (juce::jlimit (40.0f, maxCutoff, sweepHz * brightness * tracking));

        const auto sweptL = vs.safetyLowpass.processSample (0, vs.bandpass.processSample (0, stackL * 0.3f));
        const auto sweptR = vs.safetyLowpass.processSample (1, vs.bandpass.processSample (1, stackR * 0.3f));

        const auto gain = envGain * level * stealGain;
        left[n]  += sweptL * gain * 0.55f;
        right[n] += sweptR * gain * 0.55f;

        if (n < shimmerLen)
            shimmerIn[n] += 0.5f * (sweptL + sweptR) * gain;
    }
}

void AiryChoirLayer::renderLayerTail (juce::AudioBuffer<float>& target, int numSamples)
{
    const auto n = juce::jmin (numSamples, shimmerBus.getNumSamples());
    auto* busL = shimmerBus.getWritePointer (0);
    auto* busR = shimmerBus.getWritePointer (1);
    auto* left  = target.getWritePointer (0);
    auto* right = target.getWritePointer (1);

    for (int i = 0; i < n; ++i)
    {
        const auto banded = shimmerAntiAlias.processSample (0, shimmerHighpass.processSample (0, busL[i]));
        busL[i] = shimmerPitch.processSample (banded);
        busR[i] = busL[i];
    }

    // The reverb's own stereo spread is what widens the shimmer; WIDTH 0
    // keeps it mono like the rest of the layer. juce::Reverb smooths its
    // gains internally, so updating the width per block does not zipper.
    const auto width = currentWidth();

    if (std::abs (width - shimmerReverbWidth) > 1.0e-3f)
    {
        auto rp = shimmerReverb.getParameters();
        rp.width = width;
        shimmerReverb.setParameters (rp);
        shimmerReverbWidth = width;
    }

    {
        juce::dsp::AudioBlock<float> block (shimmerBus.getArrayOfWritePointers(), 2, 0, (size_t) n);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        shimmerReverb.process (ctx);
    }

    for (int i = 0; i < n; ++i)
    {
        left[i]  += busL[i] * kShimmerBlend;
        right[i] += busR[i] * kShimmerBlend;
    }
}

} // namespace horizon
