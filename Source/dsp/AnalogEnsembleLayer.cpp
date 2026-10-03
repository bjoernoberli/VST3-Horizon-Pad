#include "AnalogEnsembleLayer.h"

namespace horizon
{

void AnalogEnsembleLayer::prepareLayer (const juce::dsp::ProcessSpec&)
{
    const juce::dsp::ProcessSpec stereoSpec { sampleRate, (juce::uint32) maxBlockSize, 2 };

    for (auto& vs : voiceState)
    {
        vs.filter.prepare (stereoSpec);
        vs.filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        vs.filter.setResonance (0.5f);
    }

    // Longest read: centre + every depth, plus the Hermite neighbours.
    const auto maxDelaySeconds = kEnsembleCentreSeconds + kEnsembleSlowDepthSeconds
                               + kEnsembleFastDepthSeconds + kEnsembleDriftDepthSeconds;
    const auto size = (size_t) std::ceil (maxDelaySeconds * sampleRate) + 8;

    for (auto& buffer : ensembleBuffer)
        buffer.assign (size, 0.0f);

    ensembleBassSplit.prepare ({ sampleRate, (juce::uint32) maxBlockSize, 2 });
    ensembleBassSplit.setType (juce::dsp::FirstOrderTPTFilterType::lowpass);
    ensembleBassSplit.setCutoffFrequency (kEnsembleBassHz);

    resetLayer();
}

void AnalogEnsembleLayer::resetLayer()
{
    for (auto& vs : voiceState)
    {
        vs.phase.fill (0.0f);
        vs.drift = {};
        vs.filter.reset();
    }

    for (auto& buffer : ensembleBuffer)
        std::fill (buffer.begin(), buffer.end(), 0.0f);

    ensembleWritePos = 0;
    ensembleBassSplit.reset();
    reseedLayerState();
}

void AnalogEnsembleLayer::reseedLayerState()
{
    // One instance's ensemble should not run in phase with another's.
    slowPhase = rng.nextFloat();
    fastPhase = rng.nextFloat();
    driftPhase = rng.nextFloat();
}

void AnalogEnsembleLayer::startVoice (int voiceIndex, float, float)
{
    auto& vs = voiceState[(size_t) voiceIndex];

    for (int i = 0; i < kNumOscs; ++i)
    {
        vs.phase[(size_t) i] = rng.nextFloat();
        vs.drift[(size_t) i].start (rng, kOscDriftRateHz[i], sampleRate);
    }

    vs.filter.reset();
}

void AnalogEnsembleLayer::renderVoice (int voiceIndex, juce::AudioBuffer<float>& target, int numSamples)
{
    auto& v = voices[(size_t) voiceIndex];
    auto& vs = voiceState[(size_t) voiceIndex];

    auto* left  = target.getWritePointer (0);
    auto* right = target.getWritePointer (1);

    const auto invSr = 1.0f / (float) sampleRate;
    const auto baseFreq = bentFrequency (v.frequency);
    const auto unison = unisonFor (baseFreq, kNumOscs);
    auto detune = makeDetuneRamp (baseFreq);
    const auto level = kLayerLevel * v.velocity;
    const auto tracking = keyTrack (baseFreq, kCutoffTrackingBelowC4, kCutoffTrackingAboveC4);

    std::array<PanRamp, kNumOscs> pan;
    for (int i = 0; i < kNumOscs; ++i)
        pan[(size_t) i] = makePanRamp (kOscSpread[i], voiceIndex);

    for (int n = 0; n < numSamples; ++n)
    {
        const auto envGain = v.env.getNextSample();
        // Voice-steal declick ramp; 1.0 unless this slot is being taken over.
        const auto stealGain = nextStealGain (v);
        const auto brightness = effectiveBrightness (voiceIndex, n);
        const auto depth = driftDepth (detune.value) * unison.driftScale;

        float stackL = 0.0f, stackR = 0.0f;

        for (int i = 0; i < kNumOscs; ++i)
        {
            const auto drift = vs.drift[(size_t) i].next (rng, kOscDriftRateHz[i], sampleRate) * depth;
            const auto freqHz = baseFreq * (1.0f + kOscDetuneFraction[i] * detune.value + drift);
            const auto inc = juce::jlimit (0.0f, 0.49f, freqHz * invSr);
            vs.phase[(size_t) i] = wrapPhase (vs.phase[(size_t) i] + inc);

            const auto saw = polyBlepSaw (vs.phase[(size_t) i], inc) * (i == 0 ? 1.0f : unison.partnerGain);
            auto& p = pan[(size_t) i];
            stackL += saw * p.left;
            stackR += saw * p.right;
            p.advance();
        }

        detune.advance();

        const auto cutoff = juce::jlimit (40.0f, (float) (sampleRate * 0.45),
                                          (700.0f + envGain * 2200.0f) * brightness * tracking);
        vs.filter.setCutoffFrequency (cutoff);

        const auto gain = envGain * level * stealGain;
        left[n]  += vs.filter.processSample (0, stackL * 0.3f * unison.normalise) * gain;
        right[n] += vs.filter.processSample (1, stackR * 0.3f * unison.normalise) * gain;
    }
}

void AnalogEnsembleLayer::renderLayerTail (juce::AudioBuffer<float>& target, int numSamples)
{
    auto* channels = target.getArrayOfWritePointers();
    const auto size = (int) ensembleBuffer[0].size();
    const auto sr = (float) sampleRate;
    const auto twoPi = juce::MathConstants<float>::twoPi;
    const auto tapGain = 1.0f / (float) kEnsembleTaps;

    for (int n = 0; n < numSamples; ++n)
    {
        slowPhase  = wrapPhase (slowPhase  + kEnsembleSlowHz  / sr);
        fastPhase  = wrapPhase (fastPhase  + kEnsembleFastHz  / sr);
        driftPhase = wrapPhase (driftPhase + kEnsembleDriftHz / sr);

        const auto common = kEnsembleCentreSeconds + kEnsembleDriftDepthSeconds * std::sin (twoPi * driftPhase);

        for (int ch = 0; ch < 2; ++ch)
        {
            auto& buffer = ensembleBuffer[(size_t) ch];

            // Only the part above kEnsembleBassHz is chorused (see the header).
            const auto input = channels[ch][n];
            const auto bass = ensembleBassSplit.processSample (ch, input);
            const auto dry = input - bass;
            buffer[(size_t) ensembleWritePos] = dry;

            // Right channel taps sit 60 degrees from the left's.
            const auto channelOffset = ch == 0 ? 0.0f : 1.0f / 6.0f;
            float wet = 0.0f;

            for (int tap = 0; tap < kEnsembleTaps; ++tap)
            {
                const auto offset = channelOffset + (float) tap / (float) kEnsembleTaps;
                const auto delaySeconds = common
                    + kEnsembleSlowDepthSeconds * std::sin (twoPi * (slowPhase + offset))
                    + kEnsembleFastDepthSeconds * std::sin (twoPi * (fastPhase + offset));

                const auto readPos = (float) ensembleWritePos - delaySeconds * sr;
                const auto base = (int) std::floor (readPos);
                const auto frac = readPos - (float) base;

                auto at = [&] (int i) noexcept { return buffer[(size_t) (((i % size) + size) % size)]; };
                wet += hermite (at (base - 1), at (base), at (base + 1), at (base + 2), frac);
            }

            channels[ch][n] = bass + dry * kEnsembleDry + wet * tapGain * kEnsembleWet;
        }

        ensembleWritePos = (ensembleWritePos + 1) % size;
    }
}

} // namespace horizon
