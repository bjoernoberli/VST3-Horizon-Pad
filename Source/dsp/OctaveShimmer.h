#pragma once

#include <JuceHeader.h>
#include <vector>

namespace horizon
{

/**
    Fixed +1-octave pitch shifter using the classic two-grain variable-speed-
    read technique (no FFT, no external dependency): two overlapping read
    "grains" scan back through a short history buffer at twice normal speed
    (so they cover ground twice as fast = an octave up), each windowed with a
    Hann envelope offset by half the grain length from the other so their
    windows sum to a constant 1.0 and the join between them is inaudible.

    This is the C++ equivalent of the Faust source's `ef.transpose(2048, 512, 12)`
    used for Airy Choir's shimmer send.
*/
class OctaveShimmer
{
public:
    void prepare (double sampleRateToUse)
    {
        sampleRate = sampleRateToUse;
        grainLength = juce::jmax (64, (int) (0.046 * sampleRate)); // ~2048 samples @ 44.1 kHz
        bufferSize = grainLength * 4 + 16;
        buffer.assign ((size_t) bufferSize, 0.0f);
        reset();
    }

    void reset() noexcept
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
        age1 = 0;
        age2 = grainLength / 2;
    }

    float processSample (float input) noexcept
    {
        buffer[(size_t) writePos] = input;

        constexpr float ratio = 2.0f; // fixed +1 octave

        auto readGrain = [this] (int age) noexcept
        {
            const auto readPosF = (float) writePos - (float) age * ratio;
            auto i0 = (int) std::floor (readPosF);
            const auto frac = readPosF - (float) i0;
            i0 = ((i0 % bufferSize) + bufferSize) % bufferSize;
            const auto i1 = (i0 + 1) % bufferSize;
            return buffer[(size_t) i0] * (1.0f - frac) + buffer[(size_t) i1] * frac;
        };

        auto windowOf = [this] (int age) noexcept
        {
            const auto t = (float) age / (float) grainLength;
            return 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * t);
        };

        const auto out = readGrain (age1) * windowOf (age1) + readGrain (age2) * windowOf (age2);

        writePos = (writePos + 1) % bufferSize;
        age1 = (age1 + 1) % grainLength;
        age2 = (age2 + 1) % grainLength;

        return out;
    }

private:
    double sampleRate = 44100.0;
    int grainLength = 2048;
    int bufferSize = 8192;
    std::vector<float> buffer;
    int writePos = 0;
    int age1 = 0;
    int age2 = 0;
};

} // namespace horizon
