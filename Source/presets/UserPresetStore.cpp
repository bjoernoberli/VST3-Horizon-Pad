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

        // Older saved presets have no per-layer width yet - default to 0.5
        // (the struct's own default) rather than 0, so pre-existing presets
        // don't suddenly go mono the first time they're loaded post-update.
        preset.widths[0] = (float) child->getDoubleAttribute ("w0", 0.5);
        preset.widths[1] = (float) child->getDoubleAttribute ("w1", 0.5);
        preset.widths[2] = (float) child->getDoubleAttribute ("w2", 0.5);
        preset.widths[3] = (float) child->getDoubleAttribute ("w3", 0.5);

        preset.macros[0] = (float) child->getDoubleAttribute ("m0");
        preset.macros[1] = (float) child->getDoubleAttribute ("m1");
        preset.macros[2] = (float) child->getDoubleAttribute ("m2");
        preset.macros[3] = (float) child->getDoubleAttribute ("m3");

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

        child->setAttribute ("w0", (double) preset.widths[0]);
        child->setAttribute ("w1", (double) preset.widths[1]);
        child->setAttribute ("w2", (double) preset.widths[2]);
        child->setAttribute ("w3", (double) preset.widths[3]);

        child->setAttribute ("m0", (double) preset.macros[0]);
        child->setAttribute ("m1", (double) preset.macros[1]);
        child->setAttribute ("m2", (double) preset.macros[2]);
        child->setAttribute ("m3", (double) preset.macros[3]);
    }

    root.writeTo (file);
}

} // namespace horizon
