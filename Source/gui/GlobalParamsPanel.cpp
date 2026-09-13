#include "GlobalParamsPanel.h"
#include "../PluginProcessor.h"

namespace horizon::ui
{

GlobalParamsPanel::GlobalParamsPanel (HorizonPadAudioProcessor& processorToUse)
    : processor (processorToUse)
{
    struct Spec { const char* id; const char* caption; juce::Colour accent; Icon icon; };

    const Spec specs[] {
        { ParamID::reverb,   "REVERB",    Palette::layerAccents[1], Icon::reverb },
        { ParamID::delay,    "DELAY",     Palette::layerAccents[2], Icon::delay  },
        { ParamID::filter,   "FILTER",    Palette::layerAccents[0], Icon::filter },
        { ParamID::fxAmount, "FX AMOUNT", Palette::layerAccents[3], Icon::fx     }
    };

    for (size_t i = 0; i < entries.size(); ++i)
    {
        auto& e = entries[i];
        const auto& spec = specs[i];

        e.caption = spec.caption;
        e.accent = spec.accent;
        e.icon = spec.icon;

        e.slider.setColour (juce::Slider::rotarySliderFillColourId, spec.accent);
        e.slider.setColour (juce::Slider::rotarySliderOutlineColourId, Palette::knobTrack);
        e.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.22f,
                                      juce::MathConstants<float>::pi * 2.78f,
                                      true);
        e.slider.onValueChange = [this] { repaint(); };
        addAndMakeVisible (e.slider);

        e.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            processor.getAPVTS(), spec.id, e.slider);
    }
}

GlobalParamsPanel::~GlobalParamsPanel() = default;

void GlobalParamsPanel::resized()
{
    auto r = getLocalBounds().reduced (16, 14);
    r.removeFromTop (20);   // panel title
    r.removeFromTop (10);

    const auto cellWidth = r.getWidth() / (int) entries.size();

    for (size_t i = 0; i < entries.size(); ++i)
    {
        auto cell = r.removeFromLeft (i + 1 == entries.size() ? r.getWidth() : cellWidth);
        cell.removeFromTop (16);            // icon strip, painted
        cell.removeFromBottom (26);         // caption + readout, painted
        entries[i].slider.setBounds (cell.reduced (10, 2));
    }
}

void GlobalParamsPanel::drawIcon (juce::Graphics& g, Icon icon, juce::Rectangle<float> area, juce::Colour colour)
{
    g.setColour (colour.withAlpha (0.85f));

    const auto cx = area.getCentreX();
    const auto cy = area.getCentreY();
    const auto w = area.getWidth();
    const auto h = area.getHeight();

    switch (icon)
    {
        case Icon::reverb:
            // Three expanding arcs: a space opening up.
            for (int i = 1; i <= 3; ++i)
            {
                const auto rad = w * 0.16f * (float) i;
                juce::Path arc;
                arc.addCentredArc (cx, cy, rad, rad, 0.0f,
                                   -juce::MathConstants<float>::halfPi * 1.1f,
                                    juce::MathConstants<float>::halfPi * 1.1f, true);
                g.strokePath (arc, juce::PathStrokeType (1.3f));
            }
            break;

        case Icon::delay:
            // Three decaying vertical taps.
            for (int i = 0; i < 3; ++i)
            {
                const auto x = cx - w * 0.26f + (float) i * w * 0.26f;
                const auto tapHeight = h * (0.78f - 0.22f * (float) i);
                g.setColour (colour.withAlpha (0.9f - 0.25f * (float) i));
                g.fillRoundedRectangle (x, cy - tapHeight * 0.5f, 1.8f, tapHeight, 0.9f);
            }
            break;

        case Icon::filter:
        {
            // A lowpass response curve.
            juce::Path p;
            p.startNewSubPath (cx - w * 0.34f, cy - h * 0.18f);
            p.lineTo (cx + w * 0.02f, cy - h * 0.18f);
            p.quadraticTo (cx + w * 0.16f, cy - h * 0.18f, cx + w * 0.34f, cy + h * 0.34f);
            g.strokePath (p, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved));
            break;
        }

        case Icon::fx:
        {
            // A four-pointed sparkle.
            juce::Path p;
            const auto rOuter = w * 0.30f;
            const auto rInner = w * 0.10f;

            for (int i = 0; i < 8; ++i)
            {
                const auto angle = juce::MathConstants<float>::pi * 0.25f * (float) i
                                 - juce::MathConstants<float>::halfPi;
                const auto rad = (i % 2 == 0) ? rOuter : rInner;
                const auto x = cx + std::cos (angle) * rad;
                const auto y = cy + std::sin (angle) * rad;

                if (i == 0) p.startNewSubPath (x, y);
                else        p.lineTo (x, y);
            }

            p.closeSubPath();
            g.fillPath (p);
            break;
        }
    }
}

void GlobalParamsPanel::paint (juce::Graphics& g)
{
    drawPanel (g, getLocalBounds().toFloat().reduced (0.5f));

    auto r = getLocalBounds().reduced (16, 14);

    g.setColour (Palette::textDim);
    g.setFont (labelFont (11.0f, true));
    g.drawText ("GLOBAL PARAMETERS", r.removeFromTop (20), juce::Justification::centredLeft);

    for (const auto& e : entries)
    {
        const auto sliderBounds = e.slider.getBounds();

        if (sliderBounds.isEmpty())
            continue;

        // Icon above the knob.
        drawIcon (g, e.icon,
                  juce::Rectangle<float> ((float) sliderBounds.getX(), (float) sliderBounds.getY() - 18.0f,
                                          (float) sliderBounds.getWidth(), 16.0f)
                      .withSizeKeepingCentre (16.0f, 14.0f)
                      .withX ((float) sliderBounds.getCentreX() - 8.0f),
                  e.accent);

        auto textArea = juce::Rectangle<int> (sliderBounds.getX() - 10, sliderBounds.getBottom() + 2,
                                              sliderBounds.getWidth() + 20, 24);

        g.setColour (Palette::textFaint);
        g.setFont (labelFont (9.0f, true));
        g.drawText (e.caption, textArea.removeFromTop (12), juce::Justification::centred);

        g.setColour (Palette::text);
        g.setFont (labelFont (11.0f));
        g.drawText (juce::String (juce::roundToInt (e.slider.getValue() * 100.0)) + "%",
                    textArea, juce::Justification::centred);
    }
}

} // namespace horizon::ui
