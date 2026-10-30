#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PresetData.h"

using namespace juce;

// ============================================================================
//  Parámetros
// ============================================================================
namespace
{
String fmtHz (float v, int)  { return v >= 1000.0f ? String (v / 1000.0f, 2) + " kHz" : String ((int) std::round (v)) + " Hz"; }
String fmtPct (float v, int) { return String ((int) std::round (v * 100.0f)) + " %"; }
String fmtBip (float v, int) { return (v > 0 ? "+" : "") + String ((int) std::round (v * 100.0f)) + " %"; }
String fmtSec (float v, int) { return v < 1.0f ? String ((int) std::round (v * 1000.0f)) + " ms" : String (v, 2) + " s"; }
String fmtDb  (float v, int) { return String (v, 1) + " dB"; }
String fmtSemi (float v, int) { return (v > 0 ? "+" : "") + String ((int) v) + " st"; }
String fmtCent (float v, int) { return String (v, 1) + " ct"; }
String fmtLfoHz (float v, int) { return String (v, 2) + " Hz"; }
String fmtSemiF (float v, int) { return (v > 0 ? "+" : "") + String (v, 1) + " st"; }

NormalisableRange<float> timeRange (float lo, float hi)
{
    NormalisableRange<float> r (lo, hi);
    r.setSkewForCentre (std::sqrt (lo * hi) * 1.5f);
    return r;
}

std::unique_ptr<AudioParameterFloat> fparam (const char* id, const char* name, NormalisableRange<float> range,
                                             float def, String (*fmt) (float, int))
{
    return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, range, def,
                                                  AudioParameterFloatAttributes().withStringFromValueFunction (fmt));
}

std::unique_ptr<AudioParameterChoice> cparam (const char* id, const char* name, StringArray choices, int def)
{
    return std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, choices, def);
}

const StringArray waveNames { "Saw", "Square", "Pulse", "Triangle" };
} // namespace

StringArray RawTekProcessor::divisionNames()
{
    return { "4/1", "2/1", "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/4T", "1/8T", "1/16T", "1/8D", "1/16D" };
}

StringArray RawTekProcessor::modSourceNames()
{
    return { "Off", "LFO 1", "LFO 2", "Mod Env", "Amp Env", "Velocity", "Note", "Random", "Mod Wheel", "Aftertouch" };
}

StringArray RawTekProcessor::modDestNames()
{
    return { "Off", "Cutoff", "Resonance", "Pitch", "Osc B Pitch", "Pulse Width", "Osc Mix", "FM", "Ring",
             "Unison Det", "Noise", "Sub", "Drive", "Res Tune", "Res Mix", "Amp", "Pan" };
}

static double divisionBeats (int idx)
{
    static const double beats[] = { 16.0, 8.0, 4.0, 2.0, 1.0, 0.5, 0.25, 0.125, 2.0 / 3.0, 1.0 / 3.0, 1.0 / 6.0, 0.75, 0.375 };
    return beats[jlimit (0, 12, idx)];
}

AudioProcessorValueTreeState::ParameterLayout RawTekProcessor::createLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    // --- Osciladores
    p.push_back (cparam ("osc1Wave", "Osc1 Wave", waveNames, 0));
    p.push_back (cparam ("osc2Wave", "Osc2 Wave", waveNames, 0));
    p.push_back (fparam ("pw", "Pulse Width", { 0.05f, 0.5f }, 0.25f, fmtPct));
    p.push_back (fparam ("osc2Semi", "Osc2 Semi", { -24.0f, 24.0f, 1.0f }, 0.0f, fmtSemi));
    p.push_back (fparam ("osc2Detune", "Osc2 Detune", { 0.0f, 50.0f }, 7.0f, fmtCent));
    p.push_back (fparam ("oscMix", "Osc Mix", { 0.0f, 1.0f }, 0.5f, fmtPct));
    p.push_back (fparam ("sub", "Sub", { 0.0f, 1.0f }, 0.0f, fmtPct));
    p.push_back (fparam ("noise", "Noise", { 0.0f, 1.0f }, 0.0f, fmtPct));

    // --- Unison (supersaw)
    p.push_back (std::make_unique<AudioParameterInt> (ParameterID { "uniA", 1 }, "Osc A Unison", 1, rawtek::kMaxUnison, 1));
    p.push_back (fparam ("detA", "Osc A Detune", { 0.0f, 1.0f }, 0.25f, fmtPct));
    p.push_back (std::make_unique<AudioParameterInt> (ParameterID { "uniB", 1 }, "Osc B Unison", 1, rawtek::kMaxUnison, 1));
    p.push_back (fparam ("detB", "Osc B Detune", { 0.0f, 1.0f }, 0.25f, fmtPct));
    p.push_back (fparam ("spread", "Stereo Spread", { 0.0f, 1.0f }, 0.6f, fmtPct));

    // --- Metal: FM / ring / sync / pitch env
    p.push_back (fparam ("fm", "FM Amount", { 0.0f, 1.0f }, 0.0f, fmtPct));
    p.push_back (fparam ("fmEnv", "FM Env", { -1.0f, 1.0f }, 0.0f, fmtBip));
    p.push_back (fparam ("ring", "Ring Mod", { 0.0f, 1.0f }, 0.0f, fmtPct));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "sync", 1 }, "Hard Sync", false));
    p.push_back (fparam ("pitchEnv", "Pitch Env", { -24.0f, 48.0f }, 0.0f, fmtSemiF));
    p.push_back (fparam ("pitchDecay", "Pitch Decay", timeRange (0.002f, 2.0f), 0.08f, fmtSec));

    // --- Resonador
    p.push_back (fparam ("combMix", "Resonator Mix", { 0.0f, 1.0f }, 0.0f, fmtPct));
    p.push_back (fparam ("combTune", "Resonator Tune", { -24.0f, 24.0f }, 0.0f, fmtSemiF));
    p.push_back (fparam ("combFb", "Resonator Feedback", { 0.0f, 0.98f }, 0.85f, fmtPct));
    p.push_back (fparam ("combDamp", "Resonator Damp", { 0.0f, 1.0f }, 0.3f, fmtPct));

    // --- Filtro
    NormalisableRange<float> cutRange (20.0f, 18000.0f);
    cutRange.setSkewForCentre (800.0f);
    p.push_back (cparam ("filterMode", "Filter Mode", { "LP 24", "LP 12", "Band Pass", "High Pass" }, 0));
    p.push_back (fparam ("cutoff", "Cutoff", cutRange, 1200.0f, fmtHz));
    p.push_back (fparam ("reso", "Resonance", { 0.0f, 1.0f }, 0.3f, fmtPct));
    p.push_back (fparam ("drive", "Drive", { 0.0f, 1.0f }, 0.25f, fmtPct));
    p.push_back (fparam ("envAmt", "Env Amount", { -1.0f, 1.0f }, 0.45f, fmtBip));
    p.push_back (fparam ("keyTrack", "Key Track", { 0.0f, 1.0f }, 0.3f, fmtPct));

    // --- Envolventes
    p.push_back (fparam ("fA", "Filter Attack",  timeRange (0.0005f, 5.0f), 0.001f, fmtSec));
    p.push_back (fparam ("fD", "Filter Decay",   timeRange (0.005f, 5.0f),  0.25f,  fmtSec));
    p.push_back (fparam ("fS", "Filter Sustain", { 0.0f, 1.0f },             0.0f,   fmtPct));
    p.push_back (fparam ("fR", "Filter Release", timeRange (0.005f, 5.0f),  0.2f,   fmtSec));
    p.push_back (fparam ("aA", "Amp Attack",     timeRange (0.0005f, 5.0f), 0.001f, fmtSec));
    p.push_back (fparam ("aD", "Amp Decay",      timeRange (0.005f, 5.0f),  0.35f,  fmtSec));
    p.push_back (fparam ("aS", "Amp Sustain",    { 0.0f, 1.0f },             0.7f,   fmtPct));
    p.push_back (fparam ("aR", "Amp Release",    timeRange (0.005f, 5.0f),  0.12f,  fmtSec));

    // --- LFO
    const StringArray lfoWaves { "Sine", "Triangle", "Square", "S&H", "Saw Down", "Saw Up" };
    p.push_back (cparam ("lfoWave", "LFO Wave", lfoWaves, 0));
    NormalisableRange<float> lfoRange (0.05f, 30.0f);
    lfoRange.setSkewForCentre (3.0f);
    p.push_back (fparam ("lfoRate", "LFO Rate", lfoRange, 2.0f, fmtLfoHz));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "lfoSync", 1 }, "LFO Sync", false));
    p.push_back (cparam ("lfoDiv", "LFO Division", divisionNames(), 6));
    p.push_back (fparam ("lfoDepth", "LFO > Cutoff", { 0.0f, 1.0f }, 0.0f, fmtPct));
    p.push_back (fparam ("lfoPitch", "LFO > Pitch", { 0.0f, 1.0f }, 0.0f, fmtPct));

    // --- LFO 2
    p.push_back (cparam ("lfo2Wave", "LFO2 Wave", lfoWaves, 1));
    p.push_back (fparam ("lfo2Rate", "LFO2 Rate", lfoRange, 0.5f, fmtLfoHz));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "lfo2Sync", 1 }, "LFO2 Sync", false));
    p.push_back (cparam ("lfo2Div", "LFO2 Division", divisionNames(), 3));

    // --- Matriz de modulación
    for (int i = 1; i <= rawtek::kModSlots; ++i)
    {
        const String n (i);
        p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "m" + n + "Src", 1 }, "Mod " + n + " Source", modSourceNames(), 0));
        p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "m" + n + "Dst", 1 }, "Mod " + n + " Dest", modDestNames(), 0));
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "m" + n + "Amt", 1 }, "Mod " + n + " Amount",
                                                            NormalisableRange<float> (-1.0f, 1.0f), 0.0f,
                                                            AudioParameterFloatAttributes().withStringFromValueFunction (fmtBip)));
    }

    // --- Distorsión
    p.push_back (cparam ("distType", "Dist Type", { "Soft", "Hard", "Fold", "Crush" }, 0));
    p.push_back (fparam ("distAmt", "Dist Amount", { 0.0f, 1.0f }, 0.3f, fmtPct));
    p.push_back (fparam ("distMix", "Dist Mix", { 0.0f, 1.0f }, 1.0f, fmtPct));

    // --- Chorus
    p.push_back (fparam ("chMix", "Chorus Mix", { 0.0f, 1.0f }, 0.0f, fmtPct));
    NormalisableRange<float> chRate (0.05f, 5.0f); chRate.setSkewForCentre (0.6f);
    p.push_back (fparam ("chRate", "Chorus Rate", chRate, 0.4f, fmtLfoHz));
    p.push_back (fparam ("chDepth", "Chorus Depth", { 0.0f, 1.0f }, 0.5f, fmtPct));

    // --- Delay
    p.push_back (cparam ("dlyDiv", "Delay Time", divisionNames(), 11));
    p.push_back (fparam ("dlyFb", "Delay Feedback", { 0.0f, 0.95f }, 0.45f, fmtPct));
    p.push_back (fparam ("dlyTone", "Delay Tone", { 0.0f, 1.0f }, 0.45f, fmtPct));
    p.push_back (fparam ("dlyMix", "Delay Mix", { 0.0f, 1.0f }, 0.0f, fmtPct));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "dlyPing", 1 }, "Delay Ping-Pong", true));

    // --- Reverb
    p.push_back (fparam ("revSize", "Reverb Size", { 0.0f, 1.0f }, 0.55f, fmtPct));
    p.push_back (fparam ("revDamp", "Reverb Damp", { 0.0f, 1.0f }, 0.45f, fmtPct));
    NormalisableRange<float> preRange (0.0f, 0.25f); preRange.setSkewForCentre (0.04f);
    p.push_back (fparam ("revPre", "Reverb Predelay", preRange, 0.01f, fmtSec));
    NormalisableRange<float> lcRange (20.0f, 1500.0f); lcRange.setSkewForCentre (200.0f);
    p.push_back (fparam ("revLowCut", "Reverb Low Cut", lcRange, 150.0f, fmtHz));
    p.push_back (fparam ("revMix", "Reverb Mix", { 0.0f, 1.0f }, 0.0f, fmtPct));

    // --- Voz / master
    p.push_back (cparam ("mode", "Voice Mode", { "Mono", "Poly" }, 0));
    NormalisableRange<float> glideRange (0.0f, 1.0f);
    glideRange.setSkewForCentre (0.1f);
    p.push_back (fparam ("glide", "Glide", glideRange, 0.0f, fmtSec));
    p.push_back (fparam ("velSens", "Velocity", { 0.0f, 1.0f }, 0.5f, fmtPct));
    p.push_back (fparam ("volume", "Volume", { -36.0f, 12.0f }, -6.0f, fmtDb));

    return { p.begin(), p.end() };
}

// ============================================================================
//  Presets de fábrica
// ============================================================================
namespace
{
struct Preset { const char* name; std::vector<std::pair<const char*, float>> values; };

const std::vector<Preset>& factoryPresets()
{
    static const std::vector<Preset> presets {
        { "Init", {} },
        { "Rumble Sub", { { "osc1Wave", 0 }, { "oscMix", 0.0f }, { "sub", 0.8f }, { "cutoff", 160.0f }, { "reso", 0.2f },
                          { "drive", 0.55f }, { "envAmt", 0.3f }, { "fD", 0.4f }, { "keyTrack", 0.5f },
                          { "aD", 0.6f }, { "aS", 0.5f }, { "aR", 0.2f },
                          { "distType", 0 }, { "distAmt", 0.35f }, { "mode", 0 } } },
        { "Acid Line",  { { "osc1Wave", 0 }, { "oscMix", 0.0f }, { "cutoff", 280.0f }, { "reso", 0.84f }, { "drive", 0.4f },
                          { "envAmt", 0.65f }, { "fD", 0.18f }, { "aD", 0.4f }, { "aS", 0.6f }, { "aR", 0.05f },
                          { "glide", 0.07f }, { "velSens", 0.85f }, { "distType", 0 }, { "distAmt", 0.45f }, { "mode", 0 } } },
        { "Industrial Stab", { { "osc1Wave", 0 }, { "osc2Wave", 2 }, { "osc2Semi", 7.0f }, { "osc2Detune", 12.0f },
                          { "cutoff", 900.0f }, { "reso", 0.45f }, { "drive", 0.6f }, { "envAmt", 0.55f }, { "fD", 0.22f },
                          { "aD", 0.25f }, { "aS", 0.0f }, { "aR", 0.15f },
                          { "distType", 1 }, { "distAmt", 0.5f }, { "distMix", 0.7f }, { "mode", 1 } } },
        { "Warehouse Lead", { { "osc1Wave", 1 }, { "osc2Wave", 0 }, { "osc2Semi", 12.0f }, { "osc2Detune", 8.0f },
                          { "cutoff", 1500.0f }, { "reso", 0.35f }, { "envAmt", 0.3f },
                          { "lfoWave", 3 }, { "lfoSync", 1 }, { "lfoDiv", 6 }, { "lfoDepth", 0.35f },
                          { "distType", 2 }, { "distAmt", 0.35f }, { "mode", 0 }, { "glide", 0.03f } } },
        { "Crushed Perc", { { "osc1Wave", 3 }, { "oscMix", 0.0f }, { "noise", 0.5f }, { "cutoff", 2500.0f }, { "reso", 0.6f },
                          { "envAmt", 0.4f }, { "fD", 0.08f }, { "aD", 0.12f }, { "aS", 0.0f }, { "aR", 0.08f },
                          { "distType", 3 }, { "distAmt", 0.55f }, { "mode", 1 }, { "volume", 3.0f } } },
        { "Hoover Rave", { { "osc1Wave", 2 }, { "pw", 0.3f }, { "osc2Wave", 0 }, { "osc2Semi", -12.0f }, { "osc2Detune", 25.0f },
                          { "cutoff", 3000.0f }, { "reso", 0.2f }, { "envAmt", 0.2f }, { "glide", 0.12f },
                          { "lfoWave", 0 }, { "lfoRate", 5.0f }, { "lfoPitch", 0.02f },
                          { "aA", 0.01f }, { "aS", 0.9f }, { "aR", 0.3f },
                          { "distType", 0 }, { "distAmt", 0.5f }, { "mode", 0 } } },
        { "Detroit Chord", { { "osc1Wave", 0 }, { "osc2Wave", 0 }, { "osc2Detune", 15.0f }, { "cutoff", 1200.0f }, { "reso", 0.25f },
                          { "envAmt", 0.35f }, { "fA", 0.01f }, { "fD", 0.5f }, { "fS", 0.2f },
                          { "aA", 0.005f }, { "aD", 0.8f }, { "aS", 0.4f }, { "aR", 0.4f },
                          { "distType", 0 }, { "distAmt", 0.15f }, { "mode", 1 }, { "volume", -4.0f } } },
        { "Distorted Kick Bass", { { "osc1Wave", 3 }, { "oscMix", 0.0f }, { "sub", 1.0f }, { "cutoff", 120.0f }, { "reso", 0.1f },
                          { "envAmt", 0.8f }, { "fD", 0.06f }, { "aD", 0.35f }, { "aS", 0.0f }, { "aR", 0.1f },
                          { "drive", 0.8f }, { "distType", 1 }, { "distAmt", 0.65f }, { "mode", 0 }, { "volume", -10.0f } } },
    };
    return presets;
}
} // namespace

// ============================================================================
//  Pack de presets embebido en el binario (Presets/*/*.rtkpreset -> BinaryData)
// ============================================================================
namespace
{
struct EmbeddedPreset { String name, category, xml; };

int categoryRank (const String& c)
{
    static const StringArray order { "Hard Groove", "Groove Perc", "Old School", "Groove", "Rave", "Metal", "Pluck", "Bass", "Stab", "Perc", "Lead", "Drone", "FX" };
    const int i = order.indexOf (c);
    return i >= 0 ? i : 500;
}

const std::vector<EmbeddedPreset>& embeddedPresets()
{
    static const std::vector<EmbeddedPreset> list = [] {
        std::vector<EmbeddedPreset> v;
        for (int i = 0; i < PresetData::namedResourceListSize; ++i)
        {
            int size = 0;
            const char* data = PresetData::getNamedResource (PresetData::namedResourceList[i], size);
            if (data == nullptr || size <= 0) continue;
            String text (CharPointer_UTF8 (data), (size_t) size);
            auto xml = parseXML (text);
            if (xml == nullptr || ! xml->hasTagName ("RawTekPreset")) continue;
            v.push_back ({ xml->getStringAttribute ("name"), xml->getStringAttribute ("category", "Misc"), text });
        }
        std::sort (v.begin(), v.end(), [] (const EmbeddedPreset& a, const EmbeddedPreset& b) {
            if (categoryRank (a.category) != categoryRank (b.category)) return categoryRank (a.category) < categoryRank (b.category);
            if (a.category != b.category) return a.category < b.category;
            return a.name.compareNatural (b.name) < 0;
        });
        return v;
    }();
    return list;
}
} // namespace

int RawTekProcessor::getNumFactoryPrograms() { return (int) factoryPresets().size(); }

int RawTekProcessor::getNumPrograms() { return (int) (factoryPresets().size() + embeddedPresets().size()); }

String RawTekProcessor::getProgramShortName (int index)
{
    const auto& fp = factoryPresets();
    if (isPositiveAndBelow (index, (int) fp.size())) return fp[(size_t) index].name;
    const int e = index - (int) fp.size();
    const auto& ep = embeddedPresets();
    return isPositiveAndBelow (e, (int) ep.size()) ? ep[(size_t) e].name : String();
}

String RawTekProcessor::getProgramCategory (int index)
{
    if (isPositiveAndBelow (index, (int) factoryPresets().size())) return "Factory";
    const int e = index - (int) factoryPresets().size();
    const auto& ep = embeddedPresets();
    return isPositiveAndBelow (e, (int) ep.size()) ? ep[(size_t) e].category : String();
}

const String RawTekProcessor::getProgramName (int index)
{
    const auto cat = getProgramCategory (index);
    if (cat.isEmpty()) return {};
    if (cat == "Factory") return getProgramShortName (index);
    return cat.toUpperCase() + " - " + getProgramShortName (index);
}

void RawTekProcessor::setCurrentProgram (int index)
{
    const auto& ps = factoryPresets();
    if (! isPositiveAndBelow (index, getNumPrograms())) return;
    currentProgram = index;

    if (index >= (int) ps.size())
    {
        const auto& ep = embeddedPresets()[(size_t) (index - (int) ps.size())];
        if (auto xml = parseXML (ep.xml)) loadPresetXml (*xml, ep.name);
        return;
    }

    resetParametersToDefault();

    for (const auto& [id, value] : ps[(size_t) index].values)
        if (auto* rp = apvts.getParameter (id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (value));

    setCurrentPresetName (ps[(size_t) index].name);
}

// ============================================================================
//  Presets en archivo (.rtkpreset)
// ============================================================================
File RawTekProcessor::getPresetFolder()
{
   #if JUCE_MAC
    return File::getSpecialLocation (File::userHomeDirectory)
               .getChildFile ("Library/Audio/Presets/Panic Audio/Panic Raw Tek");
   #else
    return File::getSpecialLocation (File::userApplicationDataDirectory).getChildFile ("Panic Raw Tek/Presets");
   #endif
}

void RawTekProcessor::resetParametersToDefault()
{
    for (auto* param : getParameters())
        if (auto* rp = dynamic_cast<RangedAudioParameter*> (param))
            rp->setValueNotifyingHost (rp->getDefaultValue());
}

void RawTekProcessor::setCurrentPresetName (const String& name)
{
    apvts.state.setProperty ("presetName", name, nullptr);
}

String RawTekProcessor::getCurrentPresetName() const
{
    return apvts.state.getProperty ("presetName", "Init").toString();
}

bool RawTekProcessor::loadPresetFile (const File& file)
{
    auto xml = parseXML (file);
    if (xml == nullptr) return false;
    return loadPresetXml (*xml, file.getFileNameWithoutExtension());
}

bool RawTekProcessor::loadPresetXml (const XmlElement& xml, const String& fallbackName)
{
    if (! xml.hasTagName ("RawTekPreset"))
        return false;

    resetParametersToDefault();
    for (auto* e : xml.getChildWithTagNameIterator ("PARAM"))
        if (auto* rp = apvts.getParameter (e->getStringAttribute ("id")))
            rp->setValueNotifyingHost (rp->convertTo0to1 ((float) e->getDoubleAttribute ("value")));

    setCurrentPresetName (xml.getStringAttribute ("name", fallbackName));
    return true;
}

bool RawTekProcessor::savePresetFile (const File& file, const String& name, const String& category)
{
    XmlElement xml ("RawTekPreset");
    xml.setAttribute ("name", name);
    xml.setAttribute ("category", category);
    xml.setAttribute ("version", 3);

    for (auto* param : getParameters())
        if (auto* rp = dynamic_cast<RangedAudioParameter*> (param))
        {
            auto* e = xml.createNewChildElement ("PARAM");
            e->setAttribute ("id", rp->getParameterID());
            e->setAttribute ("value", (double) rp->convertFrom0to1 (rp->getValue()));
        }

    file.getParentDirectory().createDirectory();
    if (! xml.writeTo (file)) return false;
    setCurrentPresetName (name);
    return true;
}

// ============================================================================
RawTekProcessor::RawTekProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PanicRawTekState", createLayout())
{
}

bool RawTekProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == AudioChannelSet::mono() || out == AudioChannelSet::stereo();
}

void RawTekProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sampleRateHz = sampleRate;
    maxBlock = jmax (32, samplesPerBlock);
    engine.prepare (sampleRate);
    distL.reset(); distR.reset();
    chorus.prepare (sampleRate);
    delay.prepare (sampleRate);
    reverb.prepare (sampleRate);

    oversampler = std::make_unique<dsp::Oversampling<float>> (2, 2, dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
    oversampler->initProcessing ((size_t) maxBlock);
    setLatencySamples ((int) std::round (oversampler->getLatencyInSamples()));

    work.setSize (2, maxBlock);
    dcX1[0] = dcX1[1] = dcY1[0] = dcY1[1] = 0.0f;
    lastGain = Decibels::decibelsToGain (getP ("volume"));
    updateEngineParams();
}

void RawTekProcessor::updateEngineParams()
{
    rawtek::EngineParams e;
    e.osc1Wave   = (rawtek::Wave) (int) getP ("osc1Wave");
    e.osc2Wave   = (rawtek::Wave) (int) getP ("osc2Wave");
    e.pulseWidth = getP ("pw");
    e.osc2Semi   = getP ("osc2Semi");
    e.osc2Detune = getP ("osc2Detune");
    e.oscMix     = getP ("oscMix");
    e.subLevel   = getP ("sub");
    e.noiseLevel = getP ("noise");

    e.uniA   = (int) getP ("uniA");
    e.detA   = getP ("detA");
    e.uniB   = (int) getP ("uniB");
    e.detB   = getP ("detB");
    e.spread = getP ("spread");

    e.fmAmount    = getP ("fm");
    e.fmEnv       = getP ("fmEnv");
    e.ringMod     = getP ("ring");
    e.hardSync    = getP ("sync") > 0.5f;
    e.pitchEnvAmt = getP ("pitchEnv");
    e.pitchDecay  = getP ("pitchDecay");

    e.filterMode = (int) getP ("filterMode");
    e.combMix  = getP ("combMix");
    e.combTune = getP ("combTune");
    e.combFb   = getP ("combFb");
    e.combDamp = getP ("combDamp");

    e.cutoff       = getP ("cutoff");
    e.resonance    = getP ("reso");
    e.drive        = getP ("drive");
    e.filterEnvAmt = getP ("envAmt");
    e.keyTrack     = getP ("keyTrack");

    e.fA = getP ("fA"); e.fD = getP ("fD"); e.fS = getP ("fS"); e.fR = getP ("fR");
    e.aA = getP ("aA"); e.aD = getP ("aD"); e.aS = getP ("aS"); e.aR = getP ("aR");

    auto lfoRate = [&] (const char* sync, const char* div, const char* rate) {
        return getP (sync) > 0.5f ? (float) (hostBpm / 60.0 / divisionBeats ((int) getP (div))) : getP (rate);
    };
    e.lfoWave    = (int) getP ("lfoWave");
    e.lfoRateHz  = lfoRate ("lfoSync", "lfoDiv", "lfoRate");
    e.lfoDepth   = getP ("lfoDepth");
    e.lfoPitch   = getP ("lfoPitch");
    e.lfo2Wave   = (int) getP ("lfo2Wave");
    e.lfo2RateHz = lfoRate ("lfo2Sync", "lfo2Div", "lfo2Rate");

    static const char* srcIds[] = { "m1Src", "m2Src", "m3Src", "m4Src", "m5Src", "m6Src", "m7Src", "m8Src" };
    static const char* dstIds[] = { "m1Dst", "m2Dst", "m3Dst", "m4Dst", "m5Dst", "m6Dst", "m7Dst", "m8Dst" };
    static const char* amtIds[] = { "m1Amt", "m2Amt", "m3Amt", "m4Amt", "m5Amt", "m6Amt", "m7Amt", "m8Amt" };
    for (int i = 0; i < rawtek::kModSlots; ++i)
        e.mod[(size_t) i] = { (int) getP (srcIds[i]), (int) getP (dstIds[i]), getP (amtIds[i]) };

    e.glideTime = getP ("glide") < 0.001f ? 0.0f : getP ("glide");
    e.mono      = (int) getP ("mode") == 0;
    e.velSens   = getP ("velSens");

    engine.setParams (e);
}

void RawTekProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                hostBpm = jlimit (20.0, 400.0, *bpm);

    updateEngineParams();

    if (numSamples > work.getNumSamples())
        work.setSize (2, numSamples, false, false, true);

    float* L = work.getWritePointer (0);
    float* R = work.getWritePointer (1);

    // ---- render con MIDI sample-accurate
    int pos = 0;
    for (const auto meta : midi)
    {
        const int evPos = jlimit (0, numSamples, meta.samplePosition);
        if (evPos > pos) { engine.render (L + pos, R + pos, evPos - pos); pos = evPos; }

        const auto m = meta.getMessage();
        if (m.isNoteOn())                                 engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff())                           engine.noteOff (m.getNoteNumber());
        else if (m.isAllNotesOff() || m.isAllSoundOff())  engine.allNotesOff();
        else if (m.isPitchWheel())                        engine.setPitchBend ((m.getPitchWheelValue() - 8192) / 8192.0f);
        else if (m.isControllerOfType (1))                engine.setModWheel (m.getControllerValue() / 127.0f);
        else if (m.isChannelPressure())                   engine.setAftertouch (m.getChannelPressureValue() / 127.0f);
        else if (m.isAftertouch())                        engine.setAftertouch (m.getAfterTouchValue() / 127.0f);
    }
    if (pos < numSamples) engine.render (L + pos, R + pos, numSamples - pos);
    activeVoices.store (engine.getActiveVoiceCount(), std::memory_order_relaxed);

    // ---- distorsión estéreo oversampleada (4x)
    const auto distType = (rawtek::DistType) (int) getP ("distType");
    const float distAmt = getP ("distAmt");
    const float distMix = getP ("distMix");
    const float osFactor = (float) oversampler->getOversamplingFactor();
    for (int start = 0; start < numSamples; start += maxBlock)
    {
        const int len = jmin (maxBlock, numSamples - start);
        float* chans[] = { L + start, R + start };
        dsp::AudioBlock<float> block (chans, 2, (size_t) len);
        auto up = oversampler->processSamplesUp (block);
        float* dl = up.getChannelPointer (0);
        float* dr = up.getChannelPointer (1);
        for (size_t i = 0; i < up.getNumSamples(); ++i)
        {
            const float xl = dl[i], xr = dr[i];
            dl[i] = xl + (distL.process (xl, distType, distAmt, osFactor) - xl) * distMix;
            dr[i] = xr + (distR.process (xr, distType, distAmt, osFactor) - xr) * distMix;
        }
        oversampler->processSamplesDown (block);
    }

    // ---- chorus -> delay -> reverb
    if (getP ("chMix") > 0.001f)
        chorus.process (L, R, numSamples, getP ("chRate"), getP ("chDepth"), getP ("chMix"));
    if (getP ("dlyMix") > 0.001f)
    {
        const float sec = (float) (divisionBeats ((int) getP ("dlyDiv")) * 60.0 / hostBpm);
        delay.process (L, R, numSamples, jmin (sec, 2.4f), getP ("dlyFb"), getP ("dlyTone"), getP ("dlyMix"), getP ("dlyPing") > 0.5f);
    }
    if (getP ("revMix") > 0.001f)
        reverb.process (L, R, numSamples, getP ("revSize"), getP ("revDamp"), getP ("revPre"), getP ("revLowCut"), getP ("revMix"));

    // ---- DC blocker + volumen
    const float targetGain = Decibels::decibelsToGain (getP ("volume"));
    const float gainStep = (targetGain - lastGain) / (float) jmax (1, numSamples);
    float g = lastGain, peak = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        g += gainStep;
        float* ch[] = { L, R };
        for (int c = 0; c < 2; ++c)
        {
            const float x = ch[c][i];
            const float y = x - dcX1[c] + 0.9975f * dcY1[c];
            dcX1[c] = x; dcY1[c] = y;
            ch[c][i] = y * g;
        }
        peak = jmax (peak, std::abs (L[i]), std::abs (R[i]));
    }
    lastGain = targetGain;
    outputPeak.store (jmax (peak, outputPeak.load (std::memory_order_relaxed) * 0.9f), std::memory_order_relaxed);

    int wp = scopeWritePos.load (std::memory_order_relaxed);
    for (int i = 0; i < numSamples; ++i)
    {
        scopeData[(size_t) wp] = 0.5f * (L[i] + R[i]);
        wp = (wp + 1) & (kScopeSize - 1);
    }
    scopeWritePos.store (wp, std::memory_order_release);

    if (buffer.getNumChannels() >= 2)
    {
        buffer.copyFrom (0, 0, L, numSamples);
        buffer.copyFrom (1, 0, R, numSamples);
        for (int ch = 2; ch < buffer.getNumChannels(); ++ch) buffer.clear (ch, 0, numSamples);
    }
    else if (buffer.getNumChannels() == 1)
    {
        buffer.copyFrom (0, 0, L, numSamples, 0.5f);
        buffer.addFrom (0, 0, R, numSamples, 0.5f);
    }
}

// ============================================================================
void RawTekProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void RawTekProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessorEditor* RawTekProcessor::createEditor() { return new RawTekEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new RawTekProcessor(); }
