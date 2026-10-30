#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

// ============================================================================
//  PANIC RAW TEK — interfaz
//  Estética: hardware industrial, negro + amarillo de emergencia, encoders LED
// ============================================================================
namespace panic
{
namespace Pal
{
    const juce::Colour bg        { 0xff0c0c0b };
    const juce::Colour panel     { 0xff151513 };
    const juce::Colour panelHi   { 0xff1b1b18 };
    const juce::Colour edge      { 0xff2e2d27 };
    const juce::Colour yellow    { 0xffffd60a };
    const juce::Colour yellowDim { 0xff5e5108 };
    const juce::Colour red       { 0xffff3d2e };
    const juce::Colour text      { 0xffebe7da };
    const juce::Colour dim       { 0xff8f8b7d };
    const juce::Colour off       { 0xff2f2e29 };
    const juce::Colour lcd       { 0xff0a0905 };
}

juce::Font display (float height);           // Bebas Neue
juce::Font mono (float height, bool bold = true);  // JetBrains Mono

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return mono (11.0f); }
    juce::Font getPopupMenuFont() override { return mono (12.5f, false); }
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return mono (10.5f); }
    juce::Font getAlertWindowTitleFont() override { return display (24.0f); }
    juce::Font getAlertWindowMessageFont() override { return mono (12.0f, false); }
    juce::Font getAlertWindowFont() override { return mono (12.0f, false); }
};

// Encoder con anillo LED, etiqueta arriba y valor abajo
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState&, const juce::String& paramId, const juce::String& label, bool bipolar = false);
    void paint (juce::Graphics&) override;
    void resized() override;
    juce::Slider slider;
private:
    juce::String label;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
};

class Choice : public juce::Component
{
public:
    Choice (juce::AudioProcessorValueTreeState&, const juce::String& paramId, const juce::String& label);
    void paint (juce::Graphics&) override;
    void resized() override;
    juce::ComboBox box;
private:
    juce::String label;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment attachment;
};

class Toggle : public juce::Component
{
public:
    Toggle (juce::AudioProcessorValueTreeState&, const juce::String& paramId, const juce::String& text);
    void resized() override { button.setBounds (getLocalBounds()); }
    juce::TextButton button;
private:
    juce::AudioProcessorValueTreeState::ButtonAttachment attachment;
};

// Un slot de la matriz: fuente -> destino, cantidad
class MatrixSlot : public juce::Component
{
public:
    MatrixSlot (juce::AudioProcessorValueTreeState&, int index);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    int index;
    juce::ComboBox src, dst;
    juce::Slider amount;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment srcAtt, dstAtt;
    juce::AudioProcessorValueTreeState::SliderAttachment amtAtt;
};

// Pantalla CRT: osciloscopio + curva de filtro + info
class Screen : public juce::Component, private juce::Timer
{
public:
    explicit Screen (RawTekProcessor& p) : proc (p) { startTimerHz (30); }
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override { repaint(); }
    RawTekProcessor& proc;
    std::array<float, 1024> snap {};
};

// Cadena de FX (indicador)
class FxChain : public juce::Component, private juce::Timer
{
public:
    explicit FxChain (RawTekProcessor& p) : proc (p) { startTimerHz (10); }
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override { repaint(); }
    RawTekProcessor& proc;
};

// Barra de presets tipo LCD, abre un menú por categorías
class PresetDisplay : public juce::Component
{
public:
    std::function<void()> onClick;
    juce::String name, category;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }
};
} // namespace panic

// ============================================================================
class RawTekEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit RawTekEditor (RawTekProcessor&);
    ~RawTekEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int kDesignW = 1320, kDesignH = 672;

private:
    struct Content : public juce::Component
    {
        RawTekEditor& owner;
        explicit Content (RawTekEditor& o) : owner (o) {}
        void paint (juce::Graphics& g) override { owner.paintContent (g); }
    };

    void paintContent (juce::Graphics&);
    void layoutContent();
    void timerCallback() override;
    void refreshPresetList();
    void showPresetMenu();
    void selectPreset (int entryIndex);
    void stepPreset (int delta);
    void showSaveDialog();
    void setTab (int t);
    void applyScale (float s);

    struct Section { juce::String index, title; juce::Rectangle<int> area; int tab = -1; };
    struct PresetEntry { juce::String name, category; juce::File file; int programIndex = -1; };

    RawTekProcessor& proc;
    panic::LookAndFeel lnf;
    Content content { *this };
    float scale = 1.0f;

    // OSC A / B / MIX
    panic::Choice osc1Wave, osc2Wave;
    panic::Toggle hardSync;
    panic::Knob pw, uniA, detA, osc2Semi, osc2Detune, uniB, detB, oscMix, sub, noise, spread;
    // METAL / FILTER / VOICE
    panic::Knob fm, fmEnv, ring, pitchEnv, pitchDecay;
    panic::Choice filterMode, voiceMode;
    panic::Knob cutoff, reso, drive, envAmt, keyTrack, glide;
    // ENVS / RESONATOR / LFO1 / LEVEL
    panic::Knob aA, aD, aS, aR, fA, fD, fS, fR;
    panic::Knob combMix, combTune, combFb, combDamp;
    panic::Choice lfoWave, lfoDiv;
    panic::Toggle lfoSync;
    panic::Knob lfoRate, lfoDepth, lfoPitch, velSens, volume;
    // MATRIX tab
    panic::Choice lfo2Wave, lfo2Div;
    panic::Toggle lfo2Sync;
    panic::Knob lfo2Rate;
    std::vector<std::unique_ptr<panic::MatrixSlot>> slots;
    // FX tab
    panic::Choice distType, dlyDiv;
    panic::Toggle dlyPing;
    panic::Knob distAmt, distMix, chRate, chDepth, chMix, dlyFb, dlyTone, dlyMix;
    panic::Knob revSize, revDamp, revPre, revLowCut, revMix;
    panic::FxChain fxChain;

    panic::Screen screen;
    panic::PresetDisplay presetDisplay;
    juce::TextButton prevButton { "<" }, nextButton { ">" }, saveButton { "SAVE" }, sizeButton { "100%" };
    juce::TextButton tabMatrix { "MOD MATRIX" }, tabFx { "FX RACK" };
    int currentTab = 0;

    std::vector<juce::Component*> matrixComps, fxComps;
    std::vector<PresetEntry> presets;
    int currentEntry = -1;
    juce::String shownPresetName;
    std::vector<Section> sections;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RawTekEditor)
};
