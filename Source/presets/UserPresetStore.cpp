#include "UserPresetStore.h"

namespace horizon
{

juce::File UserPresetStore::getPresetFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("Quelle Music")
        .getChildFile ("Horizon Pad")
        .getChildFile ("UserPresets.xml");
}

std::vector<UserPreset> UserPresetStore::load()
{
    std::vector<UserPreset> result;

    const auto file = getPresetFile();

    if (! file.existsAsFile())
        return result;

    auto xml = juce::XmlDocument::parse (file);

    if (xml == nullptr || ! xml->hasTagName ("HorizonPadUserPresets"))
        return result;

    for (auto* child = xml->getFirstChildElement(); child != nullptr; child = child->getNextElement())
    {
        if (! child->hasTagName ("Preset"))
            continue;

        UserPreset preset;
        preset.name = child->getStringAttribute ("name");

        preset.vols[0] = (float) child->getDoubleAttribute ("v0");
        preset.vols[1] = (float) child->getDoubleAttribute ("v1");
        preset.vols[2] = (float) child->getDoubleAttribute ("v2");
        preset.vols[3] = (float) child->getDoubleAttribute ("v3");

        for (int i = 0; i < kNumGlobalParams; ++i)
            preset.macros[(size_t) i] = (float) child->getDoubleAttribute ("m" + juce::String (i));

        result.push_back (std::move (preset));
    }

    return result;
}

void UserPresetStore::save (const std::vector<UserPreset>& presets)
{
    const auto file = getPresetFile();
    file.getParentDirectory().createDirectory();

    juce::XmlElement root ("HorizonPadUserPresets");

    for (const auto& preset : presets)
    {
        auto* child = root.createNewChildElement ("Preset");
        child->setAttribute ("name", preset.name);

        child->setAttribute ("v0", (double) preset.vols[0]);
        child->setAttribute ("v1", (double) preset.vols[1]);
        child->setAttribute ("v2", (double) preset.vols[2]);
        child->setAttribute ("v3", (double) preset.vols[3]);

        for (int i = 0; i < kNumGlobalParams; ++i)
            child->setAttribute ("m" + juce::String (i), (double) preset.macros[(size_t) i]);
    }

    root.writeTo (file);
}

} // namespace horizon
