#pragma once

#include "HorizonLookAndFeel.h"

class HorizonPadAudioProcessor;

namespace horizon::ui
{

/**
    The on-screen A-S-D-F-G-H-J-K keyboard from the mockup: eight keys over one
    diatonic octave (matching the classic "typing keyboard" layout used by
    trackers/DAWs), clickable-and-holdable with the mouse, and mirrored by the
    computer keyboard via periodic polling (see pollComputerKeyboard(), called
    from the editor's timer - JUCE's key-state API tells you what's down now,
    not exactly when it changed, so polling is the robust cross-platform way
    to turn that into note on/off edges).

    Notes are injected through juce::MidiKeyboardState, the standard JUCE
    mechanism for a GUI to trigger notes without touching the audio thread's
    voice-allocation state directly - see HorizonPadAudioProcessor::getKeyboardState().
*/
class OnScreenKeyboard final : public juce::Component
{
public:
    explicit OnScreenKeyboard (HorizonPadAudioProcessor&);
    ~OnScreenKeyboard() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    /** Called ~15-30 Hz from the editor's timer. */
    void pollComputerKeyboard();

private:
    static constexpr int kNumKeys = 8;
    static constexpr char kKeyChars[kNumKeys] { 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K' };
    static constexpr int kMidiNotes[kNumKeys] { 60, 62, 64, 65, 67, 69, 71, 72 }; // C4 major scale

    int keyIndexAt (juce::Point<int> position) const;
    void setKeyDown (int index, bool down, bool fromMouse);

    HorizonPadAudioProcessor& processor;

    std::array<juce::Rectangle<int>, kNumKeys> keyBounds;
    std::array<bool, kNumKeys> mouseKeyDown {};
    std::array<bool, kNumKeys> computerKeyDown {};
    int mouseHeldKey = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OnScreenKeyboard)
};

} // namespace horizon::ui
