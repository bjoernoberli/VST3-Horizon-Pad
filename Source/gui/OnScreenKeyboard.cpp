#include "OnScreenKeyboard.h"
#include "../PluginProcessor.h"

namespace horizon::ui
{

OnScreenKeyboard::OnScreenKeyboard (HorizonPadAudioProcessor& processorToUse)
    : processor (processorToUse)
{
    setWantsKeyboardFocus (false); // polled instead - see pollComputerKeyboard()
}

OnScreenKeyboard::~OnScreenKeyboard()
{
    for (int i = 0; i < kNumKeys; ++i)
        setKeyDown (i, false, true);
}

int OnScreenKeyboard::keyIndexAt (juce::Point<int> position) const
{
    for (int i = 0; i < kNumKeys; ++i)
        if (keyBounds[(size_t) i].contains (position))
            return i;

    return -1;
}

void OnScreenKeyboard::setKeyDown (int index, bool down, bool fromMouse)
{
    if (! juce::isPositiveAndBelow (index, kNumKeys))
        return;

    auto& state = fromMouse ? mouseKeyDown[(size_t) index] : computerKeyDown[(size_t) index];

    if (state == down)
        return;

    const auto wasAnyDown = mouseKeyDown[(size_t) index] || computerKeyDown[(size_t) index];
    state = down;
    const auto isAnyDown = mouseKeyDown[(size_t) index] || computerKeyDown[(size_t) index];

    if (wasAnyDown == isAnyDown)
        return;

    if (isAnyDown)
        processor.getKeyboardState().noteOn (1, kMidiNotes[index], 0.85f);
    else
        processor.getKeyboardState().noteOff (1, kMidiNotes[index], 0.0f);

    repaint (keyBounds[(size_t) index]);
}

void OnScreenKeyboard::pollComputerKeyboard()
{
    for (int i = 0; i < kNumKeys; ++i)
    {
        const auto down = juce::KeyPress::isKeyCurrentlyDown (kKeyChars[i]);
        setKeyDown (i, down, false);
    }
}

void OnScreenKeyboard::mouseDown (const juce::MouseEvent& e)
{
    mouseHeldKey = keyIndexAt (e.getPosition());
    setKeyDown (mouseHeldKey, true, true);
}

void OnScreenKeyboard::mouseDrag (const juce::MouseEvent& e)
{
    const auto index = keyIndexAt (e.getPosition());

    if (index == mouseHeldKey)
        return;

    setKeyDown (mouseHeldKey, false, true);
    mouseHeldKey = index;
    setKeyDown (mouseHeldKey, true, true);
}

void OnScreenKeyboard::mouseUp (const juce::MouseEvent&)
{
    setKeyDown (mouseHeldKey, false, true);
    mouseHeldKey = -1;
}

void OnScreenKeyboard::mouseExit (const juce::MouseEvent&)
{
    if (mouseHeldKey >= 0)
    {
        setKeyDown (mouseHeldKey, false, true);
        mouseHeldKey = -1;
    }
}

void OnScreenKeyboard::resized()
{
    auto r = getLocalBounds();
    r.removeFromBottom (34); // caption line, painted

    const auto gap = 6;
    const auto keyWidth = (r.getWidth() - gap * (kNumKeys - 1)) / kNumKeys;

    for (int i = 0; i < kNumKeys; ++i)
    {
        keyBounds[(size_t) i] = juce::Rectangle<int> (r.getX() + i * (keyWidth + gap), r.getY(), keyWidth, r.getHeight());
    }
}

void OnScreenKeyboard::paint (juce::Graphics& g)
{
    for (int i = 0; i < kNumKeys; ++i)
    {
        auto bounds = keyBounds[(size_t) i].toFloat();
        const auto down = mouseKeyDown[(size_t) i] || computerKeyDown[(size_t) i];

        g.setColour (down ? Palette::wheelAccent : Palette::panelRaised);
        g.fillRoundedRectangle (bounds, 8.0f);

        g.setColour (Palette::panelBorder);
        g.drawRoundedRectangle (bounds.reduced (0.5f), 8.0f, 1.0f);

        g.setColour (down ? Palette::background : Palette::textDim);
        g.setFont (labelFont (14.0f, true));
        g.drawText (juce::String::charToString (kKeyChars[i]), bounds, juce::Justification::centred);
    }

    auto caption = getLocalBounds().removeFromBottom (34).reduced (4, 0);
    g.setColour (Palette::textFaint);
    g.setFont (labelFont (10.5f));
    g.drawFittedText (
        "Click and hold, or play A-K on your keyboard. Eight knobs mirror a Launchkey 25 - "
        "four blend the pads, four shape the tone. Pitch and mod ride the wheels.",
        caption, juce::Justification::centred, 2);
}

} // namespace horizon::ui
