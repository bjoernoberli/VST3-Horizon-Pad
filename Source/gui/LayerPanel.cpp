#include "LayerPanel.h"
#include "../PluginProcessor.h"

namespace horizon::ui
{

namespace
{
    const char* const kVolumeParamIds[] {
        ParamID::warmPadVolume, ParamID::analogStringsVolume,
        ParamID::granularVolume, ParamID::subPadVolume
    };

    // Tone-block field indices used by this panel (see AtomicToneState::fieldName).
    constexpr int kToneField    = 2;
    constexpr int kAttackField  = 3;
    constexpr int kReleaseField = 4;
}

LayerPanel::LayerPanel (HorizonPadAudioProcessor& processorToUse, int layerIndex, juce::String displayName)
    : processor (processorToUse),
      layer (layerIndex),
      name (std::move (displayName)),
      accent (Palette::layerAccents[juce::jlimit (0, kNumLayers - 1, layerIndex)])
{
    styleKnob (volumeKnob,  "VOLUME");
    styleKnob (toneKnob,    "TONE");
    styleKnob (attackKnob,  "ATTACK");
    styleKnob (releaseKnob, "RELEASE");

    volumeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.getAPVTS(), kVolumeParamIds[juce::jlimit (0, kNumLayers - 1, layerIndex)], volumeKnob.slider);

    auto bindTone = [this] (Knob& knob, int field)
    {
        knob.slider.setRange (0.0, 1.0, 0.001);
        knob.slider.onValueChange = [this, &knob, field]
        {
            if (! updating)
                processor.setToneValue (layer, field, (float) knob.slider.getValue());

            repaint();   // the percentage readout under the knob is painted, not a Label
        };
    };

    volumeKnob.slider.onValueChange = [this] { repaint(); };

    bindTone (toneKnob,    kToneField);
    bindTone (attackKnob,  kAttackField);
    bindTone (releaseKnob, kReleaseField);

    // The solo button is presentational for now: solo is not a host parameter
    // and there is no per-layer mute in the DSP yet, so toggling it only marks
    // the panel. Wiring it up is a deliberate follow-up, not an oversight.
    soloButton.setClickingTogglesState (true);
    soloButton.setColour (juce::TextButton::buttonColourId, Palette::panelRaised);
    soloButton.setColour (juce::TextButton::buttonOnColourId, accent);
    soloButton.setColour (juce::TextButton::textColourOffId, Palette::textFaint);
    soloButton.setColour (juce::TextButton::textColourOnId, Palette::background);
    addAndMakeVisible (soloButton);

    refreshFromProcessor();
}

LayerPanel::~LayerPanel() = default;

void LayerPanel::styleKnob (Knob& knob, const juce::String& caption)
{
    knob.caption = caption;
    knob.slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    knob.slider.setColour (juce::Slider::rotarySliderOutlineColourId, Palette::knobTrack);
    knob.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.22f,
                                     juce::MathConstants<float>::pi * 2.78f,
                                     true);
    knob.slider.onValueChange = nullptr;
    addAndMakeVisible (knob.slider);
}

void LayerPanel::refreshFromProcessor()
{
    const juce::ScopedValueSetter<bool> guard (updating, true);

    toneKnob.slider.setValue    (processor.getToneValue (layer, kToneField),    juce::dontSendNotification);
    attackKnob.slider.setValue  (processor.getToneValue (layer, kAttackField),  juce::dontSendNotification);
    releaseKnob.slider.setValue (processor.getToneValue (layer, kReleaseField), juce::dontSendNotification);

    repaint();
}

void LayerPanel::resized()
{
    auto r = getLocalBounds().reduced (12);

    auto header = r.removeFromTop (22);
    soloButton.setBounds (header.removeFromRight (22).withSizeKeepingCentre (20, 20));

    r.removeFromTop (8);
    r.removeFromTop (26);   // thumbnail swatch strip, painted
    r.removeFromTop (12);

    // Two rows of two knobs.
    const auto rowHeight = r.getHeight() / 2;

    auto placeRow = [this] (juce::Rectangle<int> row, Knob& a, Knob& b)
    {
        const auto half = row.getWidth() / 2;
        auto left = row.removeFromLeft (half);
        auto right = row;

        // Leave 22px under each knob for the caption + percentage readout.
        a.slider.setBounds (left.withTrimmedBottom (24).reduced (6));
        b.slider.setBounds (right.withTrimmedBottom (24).reduced (6));
    };

    placeRow (r.removeFromTop (rowHeight), volumeKnob, toneKnob);
    placeRow (r, attackKnob, releaseKnob);
}

void LayerPanel::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    drawPanel (g, bounds.reduced (0.5f));

    auto r = getLocalBounds().reduced (12);

    // ------------------------------------------------------- accent + title
    {
        auto header = r.removeFromTop (22);
        header.removeFromRight (26);   // solo button

        auto bar = header.removeFromLeft (4).toFloat().reduced (0.0f, 2.0f);
        g.setColour (accent);
        g.fillRoundedRectangle (bar, 2.0f);

        header.removeFromLeft (9);

        g.setColour (Palette::text);
        g.setFont (labelFont (12.0f, true));
        g.drawText (juce::String (layer + 1) + ". " + name, header, juce::Justification::centredLeft);
    }

    r.removeFromTop (8);

    // --------------------------------------------- thumbnail strip (gradient)
    {
        auto strip = r.removeFromTop (26).toFloat();

        // Three swatches, the mockup's little preview thumbnails, rendered as
        // accent-tinted gradients rather than photography.
        const auto gap = 5.0f;
        const auto swatchWidth = (strip.getWidth() - gap * 2.0f) / 3.0f;

        for (int i = 0; i < 3; ++i)
        {
            auto swatch = strip.withX (strip.getX() + (float) i * (swatchWidth + gap))
                               .withWidth (swatchWidth);

            const auto tint = accent.withRotatedHue ((float) i * 0.04f);

            juce::ColourGradient grad (tint.withMultipliedBrightness (0.5f + 0.22f * (float) i),
                                       swatch.getX(), swatch.getBottom(),
                                       tint.withMultipliedBrightness (1.15f).withMultipliedSaturation (0.75f),
                                       swatch.getRight(), swatch.getY(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (swatch, 4.0f);

            g.setColour (Palette::background.withAlpha (0.35f));
            g.drawRoundedRectangle (swatch.reduced (0.5f), 4.0f, 1.0f);
        }
    }

    r.removeFromTop (12);

    // ------------------------------------------- knob captions and readouts
    auto drawKnobText = [&g] (const Knob& knob)
    {
        const auto sliderBounds = knob.slider.getBounds();

        if (sliderBounds.isEmpty())
            return;

        auto textArea = juce::Rectangle<int> (sliderBounds.getX() - 6, sliderBounds.getBottom(),
                                              sliderBounds.getWidth() + 12, 24);

        auto caption = textArea.removeFromTop (12);
        g.setColour (Palette::textFaint);
        g.setFont (labelFont (9.0f, true));
        g.drawText (knob.caption, caption, juce::Justification::centred);

        g.setColour (Palette::text);
        g.setFont (labelFont (11.0f));
        g.drawText (juce::String (juce::roundToInt (knob.slider.getValue() * 100.0)) + "%",
                    textArea, juce::Justification::centred);
    };

    for (const auto* knob : { &volumeKnob, &toneKnob, &attackKnob, &releaseKnob })
        drawKnobText (*knob);
}

} // namespace horizon::ui
