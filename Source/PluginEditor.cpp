#include "PluginEditor.h"
#include "FontData.h"
#include <complex>

using namespace juce;

namespace panic
{
// ============================================================================
//  Fuentes embebidas
// ============================================================================
static Typeface::Ptr typefaceDisplay()
{
    static Typeface::Ptr t = Typeface::createSystemTypefaceFor (FontData::BebasNeueBold_otf, FontData::BebasNeueBold_otfSize);
    return t;
}
static Typeface::Ptr typefaceMono (bool bold)
{
    static Typeface::Ptr b = Typeface::createSystemTypefaceFor (FontData::JetBrainsMonoBold_ttf, FontData::JetBrainsMonoBold_ttfSize);
    static Typeface::Ptr m = Typeface::createSystemTypefaceFor (FontData::JetBrainsMonoMedium_ttf, FontData::JetBrainsMonoMedium_ttfSize);
    return bold ? b : m;
}

Font display (float h) { return Font (FontOptions (typefaceDisplay()).withHeight (h)); }
Font mono (float h, bool bold) { return Font (FontOptions (typefaceMono (bold)).withHeight (h)); }

// ============================================================================
//  LookAndFeel
// ============================================================================
LookAndFeel::LookAndFeel()
{
    setDefaultSansSerifTypeface (typefaceMono (false));
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Label::textColourId, Pal::text);
    setColour (TextEditor::backgroundColourId, Pal::lcd);
    setColour (TextEditor::textColourId, Pal::yellow);
    setColour (TextEditor::highlightColourId, Pal::yellow.withAlpha (0.3f));
    setColour (TextEditor::outlineColourId, Pal::edge);
    setColour (TextEditor::focusedOutlineColourId, Pal::yellow);
    setColour (CaretComponent::caretColourId, Pal::yellow);
    setColour (ComboBox::textColourId, Pal::text);
    setColour (ComboBox::backgroundColourId, Pal::lcd);
    setColour (ComboBox::outlineColourId, Pal::edge);
    setColour (ComboBox::arrowColourId, Pal::yellow);
    setColour (PopupMenu::backgroundColourId, Colour (0xff11110f));
    setColour (PopupMenu::textColourId, Pal::text);
    setColour (PopupMenu::headerTextColourId, Pal::yellow);
    setColour (PopupMenu::highlightedBackgroundColourId, Pal::yellow);
    setColour (PopupMenu::highlightedTextColourId, Colours::black);
    setColour (TextButton::textColourOffId, Pal::dim);
    setColour (TextButton::textColourOnId, Colours::black);
    setColour (AlertWindow::backgroundColourId, Pal::panel);
    setColour (AlertWindow::textColourId, Pal::text);
    setColour (AlertWindow::outlineColourId, Pal::yellow);
}

void LookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                    float startAngle, float endAngle, Slider& s)
{
    const auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float r = jmin (b.getWidth(), b.getHeight()) * 0.5f - 1.0f;
    const float cx = b.getCentreX(), cy = b.getCentreY();
    const bool enabled = s.isEnabled();
    const bool bipolar = (bool) s.getProperties().getWithDefault ("bipolar", false);
    const float zero = bipolar ? (float) jlimit (0.0, 1.0, s.valueToProportionOfLength (0.0)) : 0.0f;
    const bool hot = s.isMouseOverOrDragging();

    // anillo LED segmentado
    constexpr int segs = 25;
    const float segSpan = (endAngle - startAngle) / (float) segs;
    const float lo = jmin (zero, pos), hi = jmax (zero, pos);
    for (int i = 0; i < segs; ++i)
    {
        const float t0 = (float) i / segs, t1 = (float) (i + 1) / segs;
        const bool lit = enabled && (t1 > lo + 1.0e-4f) && (t0 < hi - 1.0e-4f || (i == 0 && hi <= 1.0e-4f && ! bipolar && false));
        const float a0 = startAngle + i * segSpan + segSpan * 0.18f;
        const float a1 = startAngle + (i + 1) * segSpan - segSpan * 0.18f;
        Path seg;
        seg.addCentredArc (cx, cy, r - 2.5f, r - 2.5f, 0.0f, a0, a1, true);
        g.setColour (lit ? Pal::yellow : Pal::off);
        g.strokePath (seg, PathStrokeType (4.0f, PathStrokeType::mitered, PathStrokeType::butt));
    }
    // halo del segmento activo
    if (enabled && std::abs (pos - zero) > 0.002f)
    {
        const float a = startAngle + pos * (endAngle - startAngle);
        g.setColour (Pal::yellow.withAlpha (hot ? 0.35f : 0.18f));
        g.fillEllipse (cx + std::sin (a) * (r - 2.5f) - 4.0f, cy - std::cos (a) * (r - 2.5f) - 4.0f, 8.0f, 8.0f);
    }

    // cuerpo
    const float br = r - 8.5f;
    g.setGradientFill (ColourGradient (Colour (0xff2a2925), cx, cy - br, Colour (0xff0e0e0d), cx, cy + br, false));
    g.fillEllipse (cx - br, cy - br, br * 2, br * 2);
    g.setColour (Colour (0xff050505));
    g.drawEllipse (cx - br, cy - br, br * 2, br * 2, 1.2f);
    g.setColour (Colour (0x18ffffff));
    g.drawEllipse (cx - br + 1.5f, cy - br + 1.5f, br * 2 - 3, br * 2 - 3, 1.0f);

    // estrías
    g.setColour (Colour (0x10ffffff));
    for (int i = 0; i < 12; ++i)
    {
        const float a = (float) i / 12.0f * MathConstants<float>::twoPi;
        g.drawLine (cx + std::sin (a) * br * 0.72f, cy - std::cos (a) * br * 0.72f,
                    cx + std::sin (a) * br * 0.92f, cy - std::cos (a) * br * 0.92f, 1.0f);
    }

    // indicador
    const float ang = startAngle + pos * (endAngle - startAngle);
    g.setColour (enabled ? (hot ? Pal::yellow : Pal::text) : Pal::dim);
    g.drawLine (cx + std::sin (ang) * br * 0.25f, cy - std::cos (ang) * br * 0.25f,
                cx + std::sin (ang) * br * 0.86f, cy - std::cos (ang) * br * 0.86f, 2.4f);
}

void LookAndFeel::drawLinearSlider (Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                    Slider::SliderStyle, Slider& s)
{
    // barra bipolar para la matriz
    const auto r = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (0.5f);
    g.setColour (Pal::lcd);
    g.fillRect (r);
    g.setColour (s.isMouseOverOrDragging() ? Pal::yellow.withAlpha (0.7f) : Pal::edge);
    g.drawRect (r, 1.0f);
    const float mid = r.getCentreX();
    const float px = jlimit (r.getX(), r.getRight(), pos);
    const auto bar = Rectangle<float> (jmin (mid, px), r.getY() + 3, std::abs (px - mid), r.getHeight() - 6);
    const float v = (float) s.getValue();
    g.setColour (std::abs (v) < 0.001f ? Pal::off : (v > 0 ? Pal::yellow : Pal::red));
    g.fillRect (bar);
    g.setColour (Pal::edge);
    g.drawVerticalLine ((int) mid, r.getY() + 2, r.getBottom() - 2);
    g.setFont (mono (10.0f));
    const String txt = (v > 0 ? "+" : "") + String ((int) std::round (v * 100));
    g.setColour (Pal::text);
    g.drawText (txt, r.toNearestInt(), Justification::centred);
    if (std::abs (v) >= 0.001f)
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (bar.toNearestInt());
        g.setColour (Colours::black);
        g.drawText (txt, r.toNearestInt(), Justification::centred);
    }
}

void LookAndFeel::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& box)
{
    const auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (Pal::lcd);
    g.fillRect (r);
    g.setColour (box.isMouseOver (true) ? Pal::yellow.withAlpha (0.8f) : Pal::edge);
    g.drawRect (r, 1.0f);
    // chevron
    Path p;
    const float ax = (float) w - 11.0f, ay = (float) h * 0.5f;
    p.startNewSubPath (ax - 4, ay - 2); p.lineTo (ax, ay + 2); p.lineTo (ax + 4, ay - 2);
    g.setColour (box.isEnabled() ? Pal::yellow : Pal::dim);
    g.strokePath (p, PathStrokeType (1.6f));
}

void LookAndFeel::positionComboBoxText (ComboBox& box, Label& label)
{
    label.setBounds (5, 0, box.getWidth() - 20, box.getHeight());
    label.setFont (getComboBoxFont (box));
}

void LookAndFeel::drawPopupMenuBackground (Graphics& g, int w, int h)
{
    g.fillAll (Colour (0xff11110f));
    g.setColour (Pal::yellow);
    g.drawRect (0, 0, w, h, 1);
}

void LookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool over, bool down)
{
    const auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    const bool isTab = (bool) b.getProperties().getWithDefault ("tab", false);
    const bool isLed = b.getClickingTogglesState() && ! isTab;

    if (isTab)
    {
        Path p;
        p.startNewSubPath (r.getX(), r.getBottom());
        p.lineTo (r.getX(), r.getY());
        p.lineTo (r.getRight() - 10, r.getY());
        p.lineTo (r.getRight(), r.getY() + 10);
        p.lineTo (r.getRight(), r.getBottom());
        g.setColour (on ? Pal::yellow : (over ? Pal::panelHi : Pal::panel));
        g.fillPath (p);
        g.setColour (on ? Pal::yellow : Pal::edge);
        g.strokePath (p, PathStrokeType (1.0f));
        return;
    }

    g.setColour (down ? Pal::panelHi : Pal::lcd);
    g.fillRect (r);
    g.setColour (over ? Pal::yellow.withAlpha (0.8f) : Pal::edge);
    g.drawRect (r, 1.0f);

    if (isLed)
    {
        const float d = 6.0f, lx = r.getX() + 8, ly = r.getCentreY() - d * 0.5f;
        if (on)
        {
            g.setColour (Pal::yellow.withAlpha (0.25f));
            g.fillEllipse (lx - 3, ly - 3, d + 6, d + 6);
        }
        g.setColour (on ? Pal::yellow : Pal::off);
        g.fillEllipse (lx, ly, d, d);
    }
}

void LookAndFeel::drawButtonText (Graphics& g, TextButton& b, bool over, bool)
{
    const bool on = b.getToggleState();
    const bool isTab = (bool) b.getProperties().getWithDefault ("tab", false);
    const bool isLed = b.getClickingTogglesState() && ! isTab;
    g.setFont (isTab ? display (17.0f) : getTextButtonFont (b, b.getHeight()));
    Colour c = Pal::dim;
    if (isTab)       c = on ? Colours::black : (over ? Pal::text : Pal::dim);
    else if (isLed)  c = on ? Pal::text : Pal::dim;
    else             c = over ? Pal::yellow : Pal::text;
    g.setColour (c);
    auto r = b.getLocalBounds();
    if (isLed) r.removeFromLeft (16);
    g.drawText (b.getButtonText(), r, Justification::centred);
}

// ============================================================================
//  Controles
// ============================================================================
Knob::Knob (AudioProcessorValueTreeState& state, const String& id, const String& text, bool bipolar)
    : label (text.toUpperCase()), attachment (state, id, slider)
{
    slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (MathConstants<float>::pi * 1.22f, MathConstants<float>::pi * 2.78f, true);
    auto* p = state.getParameter (id);
    slider.setDoubleClickReturnValue (true, (double) p->convertFrom0to1 (p->getDefaultValue()));
    slider.getProperties().set ("bipolar", bipolar);
    slider.setMouseDragSensitivity (220);
    slider.onValueChange = [this] { repaint(); };
    slider.addMouseListener (this, false);
    addAndMakeVisible (slider);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop (14);
    r.removeFromBottom (18);
    slider.setBounds (r.withSizeKeepingCentre (jmin (r.getWidth(), r.getHeight()), r.getHeight()));
}

void Knob::paint (Graphics& g)
{
    auto r = getLocalBounds();
    g.setFont (mono (9.5f));
    g.setColour (isEnabled() ? Pal::dim : Pal::off);
    g.drawText (label, r.removeFromTop (14), Justification::centred);

    auto vb = r.removeFromBottom (16).reduced (3, 0).toFloat();
    const bool hot = slider.isMouseOverOrDragging();
    g.setColour (Pal::lcd);
    g.fillRect (vb);
    g.setColour (hot ? Pal::yellow.withAlpha (0.6f) : Colour (0xff23221e));
    g.drawRect (vb, 1.0f);
    g.setFont (mono (10.0f, false));
    g.setColour (hot ? Pal::yellow : Pal::text);
    g.drawText (slider.getTextFromValue (slider.getValue()), vb.toNearestInt(), Justification::centred);
}

Choice::Choice (AudioProcessorValueTreeState& state, const String& id, const String& text)
    : label (text.toUpperCase()),
      attachment ((box.addItemList (state.getParameter (id)->getAllValueStrings(), 1), state), id, box)
{
    addAndMakeVisible (box);
}

void Choice::paint (Graphics& g)
{
    g.setFont (mono (9.5f));
    g.setColour (isEnabled() ? Pal::dim : Pal::off);
    g.drawText (label, getLocalBounds().removeFromTop (13), Justification::centredLeft);
}

void Choice::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop (13);
    box.setBounds (r.removeFromTop (22));
}

Toggle::Toggle (AudioProcessorValueTreeState& state, const String& id, const String& text)
    : attachment (state, id, button)
{
    button.setButtonText (text);
    button.setClickingTogglesState (true);
    addAndMakeVisible (button);
}

MatrixSlot::MatrixSlot (AudioProcessorValueTreeState& state, int i)
    : index (i),
      srcAtt ((src.addItemList (RawTekProcessor::modSourceNames(), 1), state), "m" + String (i) + "Src", src),
      dstAtt ((dst.addItemList (RawTekProcessor::modDestNames(), 1), state), "m" + String (i) + "Dst", dst),
      amtAtt (state, "m" + String (i) + "Amt", amount)
{
    amount.setSliderStyle (Slider::LinearBar);
    amount.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    amount.setDoubleClickReturnValue (true, 0.0);
    amount.setMouseDragSensitivity (180);
    addAndMakeVisible (src);
    addAndMakeVisible (dst);
    addAndMakeVisible (amount);
}

void MatrixSlot::resized()
{
    auto r = getLocalBounds().withSizeKeepingCentre (getWidth(), 24);
    r.removeFromLeft (28);
    src.setBounds (r.removeFromLeft (84));
    r.removeFromLeft (14);
    dst.setBounds (r.removeFromLeft (84));
    r.removeFromLeft (6);
    amount.setBounds (r);
}

void MatrixSlot::paint (Graphics& g)
{
    auto r = getLocalBounds().withSizeKeepingCentre (getWidth(), 24);
    const bool active = src.getSelectedItemIndex() > 0 && dst.getSelectedItemIndex() > 0;
    auto num = r.removeFromLeft (22).toFloat();
    g.setColour (active ? Pal::yellow : Pal::off);
    g.fillRect (num);
    g.setFont (display (16.0f));
    g.setColour (active ? Colours::black : Pal::dim);
    g.drawText (String (index).paddedLeft ('0', 2), num.toNearestInt(), Justification::centred);
    // flecha
    const float ax = (float) src.getRight() + 7.0f, ay = (float) getHeight() * 0.5f;
    Path p; p.startNewSubPath (ax - 4, ay - 3); p.lineTo (ax + 1, ay); p.lineTo (ax - 4, ay + 3);
    g.setColour (active ? Pal::yellow : Pal::dim);
    g.strokePath (p, PathStrokeType (1.5f));
}

// ============================================================================
//  Pantalla CRT
// ============================================================================
void Screen::paint (Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    g.setColour (Colour (0xff070704));
    g.fillRect (r);

    auto area = r.reduced (10.0f);
    auto scopeArea = area.removeFromTop (area.getHeight() * 0.56f);
    area.removeFromTop (6);
    auto info = area.removeFromBottom (18);
    auto filterArea = area;

    // grilla
    g.setColour (Colour (0xff1a1808));
    for (int i = 1; i < 10; ++i)
        g.drawVerticalLine ((int) (scopeArea.getX() + scopeArea.getWidth() * i / 10.0f), scopeArea.getY(), scopeArea.getBottom());
    for (int i = 1; i < 4; ++i)
        g.drawHorizontalLine ((int) (scopeArea.getY() + scopeArea.getHeight() * i / 4.0f), scopeArea.getX(), scopeArea.getRight());

    // ---- osciloscopio (con trigger en cruce por cero)
    constexpr int N = RawTekProcessor::kScopeSize;
    const int wp = proc.scopeWritePos.load (std::memory_order_acquire);
    int start = (wp - 2048 + N) & (N - 1);
    for (int i = 0; i < 1024; ++i)
    {
        const int a = (start + i) & (N - 1), b = (a + 1) & (N - 1);
        if (proc.scopeData[(size_t) a] <= 0.0f && proc.scopeData[(size_t) b] > 0.0f) { start = b; break; }
    }
    for (int i = 0; i < 1024; ++i) snap[(size_t) i] = proc.scopeData[(size_t) ((start + i) & (N - 1))];

    Path trace;
    const int count = 700;
    const float midY = scopeArea.getCentreY(), half = scopeArea.getHeight() * 0.45f;
    for (int i = 0; i < count; ++i)
    {
        const float xx = scopeArea.getX() + scopeArea.getWidth() * (float) i / (count - 1);
        const float yy = midY - jlimit (-1.1f, 1.1f, snap[(size_t) i]) * half;
        if (i == 0) trace.startNewSubPath (xx, yy); else trace.lineTo (xx, yy);
    }
    g.setColour (Pal::yellow.withAlpha (0.12f)); g.strokePath (trace, PathStrokeType (6.0f));
    g.setColour (Pal::yellow.withAlpha (0.30f)); g.strokePath (trace, PathStrokeType (3.0f));
    g.setColour (Pal::yellow);                   g.strokePath (trace, PathStrokeType (1.3f));

    // ---- respuesta del filtro (modelo ladder lineal)
    auto& s = proc.apvts;
    const float fc = s.getRawParameterValue ("cutoff")->load();
    const float k  = s.getRawParameterValue ("reso")->load() * 4.2f;
    const int mode = (int) s.getRawParameterValue ("filterMode")->load();
    g.setColour (Colour (0xff1a1808));
    g.drawRect (filterArea, 1.0f);
    Path curve, fill;
    const int pts = 160;
    for (int i = 0; i < pts; ++i)
    {
        const float t = (float) i / (pts - 1);
        const float f = 20.0f * std::pow (1000.0f, t);
        const std::complex<float> G = 1.0f / (1.0f + std::complex<float> (0.0f, f / fc));
        const auto G2 = G * G, G4 = G2 * G2;
        const auto U = 1.0f / (1.0f + k * G4);
        std::complex<float> H;
        switch (mode)
        {
            case 0:  H = U * G4; break;
            case 1:  H = U * G2; break;
            case 2:  H = 2.0f * U * (G - G2); break;
            default: H = U * (1.0f - 4.0f * G + 6.0f * G2 - 4.0f * G2 * G + G4); break;
        }
        const float db = 20.0f * std::log10 (std::abs (H) * (mode < 3 ? (1.0f + 0.45f * k) : 1.0f) + 1.0e-6f);
        const float yy = jmap (jlimit (-36.0f, 18.0f, db), -36.0f, 18.0f, filterArea.getBottom() - 2, filterArea.getY() + 2);
        const float xx = filterArea.getX() + t * filterArea.getWidth();
        if (i == 0) { curve.startNewSubPath (xx, yy); fill.startNewSubPath (xx, filterArea.getBottom()); }
        curve.lineTo (xx, yy); fill.lineTo (xx, yy);
    }
    fill.lineTo (filterArea.getRight(), filterArea.getBottom()); fill.closeSubPath();
    g.setColour (Pal::yellow.withAlpha (0.10f)); g.fillPath (fill);
    g.setColour (Pal::yellow.withAlpha (0.85f)); g.strokePath (curve, PathStrokeType (1.4f));

    // ---- info
    static const char* modes[] = { "LP24", "LP12", "BP", "HP" };
    g.setFont (mono (10.0f));
    g.setColour (Pal::yellow.withAlpha (0.8f));
    String left = String ("FLT ") + modes[jlimit (0, 3, mode)] + "  " + String ((int) fc) + "HZ";
    g.drawText (left, info.toNearestInt(), Justification::centredLeft);
    const int voices = proc.activeVoices.load();
    const float pk = proc.outputPeak.load();
    String right = "VOICES " + String (voices) + "  OUT " + (pk > 1.0e-5f ? String (Decibels::gainToDecibels (pk), 1) : String ("-INF"));
    g.setColour (pk > 0.98f ? Pal::red : Pal::yellow.withAlpha (0.8f));
    g.drawText (right, info.toNearestInt(), Justification::centredRight);

    // scanlines + viñeta
    g.setColour (Colour (0x16000000));
    for (float yy = r.getY(); yy < r.getBottom(); yy += 3.0f) g.fillRect (r.getX(), yy, r.getWidth(), 1.0f);
    g.setColour (Pal::edge);
    g.drawRect (r, 1.0f);
}

void FxChain::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (4.0f);
    auto& s = proc.apvts;
    struct Stage { const char* name; bool on; };
    const Stage st[] = {
        { "DIST",   s.getRawParameterValue ("distMix")->load() > 0.001f && s.getRawParameterValue ("distAmt")->load() > 0.001f },
        { "CHORUS", s.getRawParameterValue ("chMix")->load() > 0.001f },
        { "DELAY",  s.getRawParameterValue ("dlyMix")->load() > 0.001f },
        { "REVERB", s.getRawParameterValue ("revMix")->load() > 0.001f },
    };
    const float h = r.getHeight() / 4.0f;
    for (int i = 0; i < 4; ++i)
    {
        auto row = Rectangle<float> (r.getX(), r.getY() + i * h, r.getWidth(), h).reduced (0, 3);
        g.setColour (st[i].on ? Pal::yellow : Pal::off);
        g.fillEllipse (row.getX(), row.getCentreY() - 3, 6, 6);
        g.setFont (mono (10.0f));
        g.setColour (st[i].on ? Pal::text : Pal::dim);
        g.drawText (String (i + 1) + "  " + st[i].name, row.withTrimmedLeft (14).toNearestInt(), Justification::centredLeft);
        if (i < 3)
        {
            g.setColour (Pal::edge);
            g.drawVerticalLine ((int) row.getX() + 3, row.getCentreY() + 5, row.getBottom() + 6);
        }
    }
}

void PresetDisplay::paint (Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    g.setColour (Pal::lcd);
    g.fillRect (r);
    g.setColour (isMouseOver() ? Pal::yellow : Pal::edge);
    g.drawRect (r, 1.0f);
    auto inner = r.reduced (10, 2);
    g.setFont (mono (9.5f));
    g.setColour (Pal::yellow.withAlpha (0.55f));
    g.drawText (category.toUpperCase(), inner.removeFromTop (13).toNearestInt(), Justification::centredLeft);
    g.setFont (display (21.0f));
    g.setColour (Pal::yellow);
    g.drawText (name.toUpperCase(), inner.withTrimmedRight (16).toNearestInt(), Justification::centredLeft);
    Path p;
    const float ax = r.getRight() - 14, ay = r.getCentreY();
    p.startNewSubPath (ax - 5, ay - 2.5f); p.lineTo (ax, ay + 2.5f); p.lineTo (ax + 5, ay - 2.5f);
    g.strokePath (p, PathStrokeType (1.8f));
}
} // namespace panic

// ============================================================================
//  Editor
// ============================================================================
RawTekEditor::RawTekEditor (RawTekProcessor& p)
    : AudioProcessorEditor (p), proc (p),
      osc1Wave (p.apvts, "osc1Wave", "Wave"), osc2Wave (p.apvts, "osc2Wave", "Wave"),
      hardSync (p.apvts, "sync", "SYNC"),
      pw (p.apvts, "pw", "PW"), uniA (p.apvts, "uniA", "Unison"), detA (p.apvts, "detA", "Detune"),
      osc2Semi (p.apvts, "osc2Semi", "Semi", true), osc2Detune (p.apvts, "osc2Detune", "Fine"),
      uniB (p.apvts, "uniB", "Unison"), detB (p.apvts, "detB", "Detune"),
      oscMix (p.apvts, "oscMix", "A / B"), sub (p.apvts, "sub", "Sub"), noise (p.apvts, "noise", "Noise"),
      spread (p.apvts, "spread", "Spread"),
      fm (p.apvts, "fm", "FM"), fmEnv (p.apvts, "fmEnv", "FM Env", true), ring (p.apvts, "ring", "Ring"),
      pitchEnv (p.apvts, "pitchEnv", "P.Env", true), pitchDecay (p.apvts, "pitchDecay", "P.Decay"),
      filterMode (p.apvts, "filterMode", "Mode"), voiceMode (p.apvts, "mode", "Voices"),
      cutoff (p.apvts, "cutoff", "Cutoff"), reso (p.apvts, "reso", "Reso"), drive (p.apvts, "drive", "Drive"),
      envAmt (p.apvts, "envAmt", "Env", true), keyTrack (p.apvts, "keyTrack", "Key"), glide (p.apvts, "glide", "Glide"),
      aA (p.apvts, "aA", "A"), aD (p.apvts, "aD", "D"), aS (p.apvts, "aS", "S"), aR (p.apvts, "aR", "R"),
      fA (p.apvts, "fA", "A"), fD (p.apvts, "fD", "D"), fS (p.apvts, "fS", "S"), fR (p.apvts, "fR", "R"),
      combMix (p.apvts, "combMix", "Mix"), combTune (p.apvts, "combTune", "Tune", true),
      combFb (p.apvts, "combFb", "Fdbk"), combDamp (p.apvts, "combDamp", "Damp"),
      lfoWave (p.apvts, "lfoWave", "Wave"), lfoDiv (p.apvts, "lfoDiv", "Div"), lfoSync (p.apvts, "lfoSync", "SYNC"),
      lfoRate (p.apvts, "lfoRate", "Rate"), lfoDepth (p.apvts, "lfoDepth", "> Cut"), lfoPitch (p.apvts, "lfoPitch", "> Pitch"),
      velSens (p.apvts, "velSens", "Velo"), volume (p.apvts, "volume", "Volume"),
      lfo2Wave (p.apvts, "lfo2Wave", "Wave"), lfo2Div (p.apvts, "lfo2Div", "Div"), lfo2Sync (p.apvts, "lfo2Sync", "SYNC"),
      lfo2Rate (p.apvts, "lfo2Rate", "Rate"),
      distType (p.apvts, "distType", "Type"), dlyDiv (p.apvts, "dlyDiv", "Time"), dlyPing (p.apvts, "dlyPing", "PING"),
      distAmt (p.apvts, "distAmt", "Amount"), distMix (p.apvts, "distMix", "Mix"),
      chRate (p.apvts, "chRate", "Rate"), chDepth (p.apvts, "chDepth", "Depth"), chMix (p.apvts, "chMix", "Mix"),
      dlyFb (p.apvts, "dlyFb", "Fdbk"), dlyTone (p.apvts, "dlyTone", "Tone"), dlyMix (p.apvts, "dlyMix", "Mix"),
      revSize (p.apvts, "revSize", "Size"), revDamp (p.apvts, "revDamp", "Damp"), revPre (p.apvts, "revPre", "Pre"),
      revLowCut (p.apvts, "revLowCut", "Low Cut"), revMix (p.apvts, "revMix", "Mix"),
      fxChain (p), screen (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (content);

    for (int i = 1; i <= rawtek::kModSlots; ++i)
        slots.push_back (std::make_unique<panic::MatrixSlot> (p.apvts, i));

    for (Component* c : std::initializer_list<Component*> {
             &osc1Wave, &osc2Wave, &hardSync, &pw, &uniA, &detA, &osc2Semi, &osc2Detune, &uniB, &detB,
             &oscMix, &sub, &noise, &spread, &fm, &fmEnv, &ring, &pitchEnv, &pitchDecay,
             &filterMode, &voiceMode, &cutoff, &reso, &drive, &envAmt, &keyTrack, &glide,
             &aA, &aD, &aS, &aR, &fA, &fD, &fS, &fR, &combMix, &combTune, &combFb, &combDamp,
             &lfoWave, &lfoDiv, &lfoSync, &lfoRate, &lfoDepth, &lfoPitch, &velSens, &volume,
             &screen, &presetDisplay, &prevButton, &nextButton, &saveButton, &sizeButton, &tabMatrix, &tabFx })
        content.addAndMakeVisible (c);

    matrixComps = { &lfo2Wave, &lfo2Div, &lfo2Sync, &lfo2Rate };
    for (auto& s : slots) matrixComps.push_back (s.get());
    fxComps = { &distType, &distAmt, &distMix, &chRate, &chDepth, &chMix, &dlyDiv, &dlyPing, &dlyFb, &dlyTone, &dlyMix,
                &revSize, &revDamp, &revPre, &revLowCut, &revMix, &fxChain };
    for (auto* c : matrixComps) content.addChildComponent (c);
    for (auto* c : fxComps) content.addChildComponent (c);

    for (auto* t : { &tabMatrix, &tabFx })
    {
        t->getProperties().set ("tab", true);
        t->setClickingTogglesState (false);
    }
    tabMatrix.onClick = [this] { setTab (0); };
    tabFx.onClick     = [this] { setTab (1); };

    presetDisplay.onClick = [this] { showPresetMenu(); };
    prevButton.onClick = [this] { stepPreset (-1); };
    nextButton.onClick = [this] { stepPreset (+1); };
    saveButton.onClick = [this] { showSaveDialog(); };
    sizeButton.onClick = [this] {
        static const float sizes[] = { 0.75f, 0.85f, 1.0f, 1.15f, 1.3f };
        int idx = 2;
        for (int i = 0; i < 5; ++i) if (std::abs (sizes[i] - scale) < 0.01f) idx = i;
        applyScale (sizes[(idx + 1) % 5]);
    };

    refreshPresetList();
    setTab ((int) proc.apvts.state.getProperty ("uiTab", 0));

    scale = (float) (double) proc.apvts.state.getProperty ("uiScale", 1.0);
    applyScale (scale);
    timerCallback();
    startTimerHz (15);
}

RawTekEditor::~RawTekEditor() { setLookAndFeel (nullptr); }

void RawTekEditor::applyScale (float s)
{
    scale = jlimit (0.6f, 1.5f, s);
    proc.apvts.state.setProperty ("uiScale", scale, nullptr);
    sizeButton.setButtonText (String (roundToInt (scale * 100)) + "%");
    content.setTransform (AffineTransform::scale (scale));
    setSize (roundToInt (kDesignW * scale), roundToInt (kDesignH * scale));
}

void RawTekEditor::setTab (int t)
{
    currentTab = jlimit (0, 1, t);
    proc.apvts.state.setProperty ("uiTab", currentTab, nullptr);
    tabMatrix.setToggleState (currentTab == 0, dontSendNotification);
    tabFx.setToggleState (currentTab == 1, dontSendNotification);
    for (auto* c : matrixComps) c->setVisible (currentTab == 0);
    for (auto* c : fxComps) c->setVisible (currentTab == 1);
    content.repaint();
}

// ---------------------------------------------------------------------------- presets
void RawTekEditor::refreshPresetList()
{
    presets.clear();
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presets.push_back ({ RawTekProcessor::getProgramShortName (i), RawTekProcessor::getProgramCategory (i), {}, i });

    const auto root = RawTekProcessor::getPresetFolder();
    std::vector<PresetEntry> files;
    if (root.isDirectory())
        for (const auto& entry : RangedDirectoryIterator (root, true, "*.rtkpreset"))
        {
            const auto f = entry.getFile();
            const auto parent = f.getParentDirectory();
            const auto name = f.getFileNameWithoutExtension();
            const auto cat  = parent == root ? String ("User") : parent.getFileName();
            const bool dup = std::any_of (presets.begin(), presets.end(), [&] (const PresetEntry& e) {
                return e.programIndex >= 0 && e.name == name && e.category == cat; });
            if (! dup) files.push_back ({ name, cat, f, -1 });
        }
    std::sort (files.begin(), files.end(), [] (const PresetEntry& a, const PresetEntry& b) {
        if (a.category != b.category) return a.category == "User" ? false : (b.category == "User" ? true : a.category < b.category);
        return a.name.compareNatural (b.name) < 0;
    });
    presets.insert (presets.end(), files.begin(), files.end());
    shownPresetName = {};
}

void RawTekEditor::showPresetMenu()
{
    PopupMenu menu;
    menu.setLookAndFeel (&lnf);
    String cat;
    PopupMenu sub;
    auto flush = [&] { if (cat.isNotEmpty()) menu.addSubMenu (cat.toUpperCase(), sub, true, nullptr, cat == presets[(size_t) jmax (0, currentEntry)].category); };
    for (size_t i = 0; i < presets.size(); ++i)
    {
        if (presets[i].category != cat)
        {
            flush();
            sub = PopupMenu();
            cat = presets[i].category;
        }
        sub.addItem ((int) i + 1, presets[i].name, true, (int) i == currentEntry);
    }
    flush();
    menu.addSeparator();
    menu.addItem (100000, "Refresh preset folder");

    SafePointer<RawTekEditor> safe (this);
    menu.showMenuAsync (PopupMenu::Options().withTargetComponent (&presetDisplay).withMinimumWidth (presetDisplay.getWidth()),
                        [safe] (int r) {
                            if (safe == nullptr || r <= 0) return;
                            if (r == 100000) { safe->refreshPresetList(); return; }
                            safe->selectPreset (r - 1);
                        });
}

void RawTekEditor::selectPreset (int idx)
{
    if (! isPositiveAndBelow (idx, (int) presets.size())) return;
    const auto& e = presets[(size_t) idx];
    if (e.programIndex >= 0) proc.setCurrentProgram (e.programIndex);
    else                     proc.loadPresetFile (e.file);
    currentEntry = idx;
    shownPresetName = {};
    timerCallback();
}

void RawTekEditor::stepPreset (int d)
{
    if (presets.empty()) return;
    const int n = (int) presets.size();
    const int cur = currentEntry < 0 ? (d > 0 ? -1 : 0) : currentEntry;
    selectPreset (((cur + d) % n + n) % n);
}

void RawTekEditor::showSaveDialog()
{
    auto* w = new AlertWindow ("SAVE PRESET", "Preset name (saved in the USER category):", MessageBoxIconType::NoIcon, this);
    w->setLookAndFeel (&lnf);
    w->addTextEditor ("name", proc.getCurrentPresetName());
    w->addButton ("SAVE", 1, KeyPress (KeyPress::returnKey));
    w->addButton ("CANCEL", 0, KeyPress (KeyPress::escapeKey));
    SafePointer<RawTekEditor> safe (this);
    w->enterModalState (true, ModalCallbackFunction::create ([safe, w] (int result) {
        if (result != 1 || safe == nullptr) return;
        const auto name = File::createLegalFileName (w->getTextEditorContents ("name").trim());
        if (name.isEmpty()) return;
        const auto file = RawTekProcessor::getPresetFolder().getChildFile ("User").getChildFile (name + ".rtkpreset");
        if (safe->proc.savePresetFile (file, name, "User"))
        {
            safe->refreshPresetList();
            for (size_t i = 0; i < safe->presets.size(); ++i)
                if (safe->presets[i].file == file) safe->currentEntry = (int) i;
        }
        else
            AlertWindow::showMessageBoxAsync (MessageBoxIconType::WarningIcon, "Panic Raw Tek", "Could not save:\n" + file.getFullPathName());
    }), true);
}

void RawTekEditor::timerCallback()
{
    auto dimIf = [] (Component& c, bool on) { c.setEnabled (on); c.setAlpha (on ? 1.0f : 0.35f); };
    const bool s1 = lfoSync.button.getToggleState(), s2 = lfo2Sync.button.getToggleState();
    dimIf (lfoRate, ! s1); dimIf (lfoDiv, s1);
    dimIf (lfo2Rate, ! s2); dimIf (lfo2Div, s2);
    dimIf (pw, (int) proc.apvts.getRawParameterValue ("osc1Wave")->load() == 2 || (int) proc.apvts.getRawParameterValue ("osc2Wave")->load() == 2);
    dimIf (detA, (int) proc.apvts.getRawParameterValue ("uniA")->load() > 1);
    dimIf (detB, (int) proc.apvts.getRawParameterValue ("uniB")->load() > 1);

    const auto name = proc.getCurrentPresetName();
    if (name != shownPresetName)
    {
        shownPresetName = name;
        if (! (isPositiveAndBelow (currentEntry, (int) presets.size()) && presets[(size_t) currentEntry].name == name))
        {
            currentEntry = -1;
            for (size_t i = 0; i < presets.size(); ++i)
                if (presets[i].name == name) { currentEntry = (int) i; break; }
        }
        presetDisplay.name = name;
        presetDisplay.category = currentEntry >= 0 ? presets[(size_t) currentEntry].category : String ("User");
        presetDisplay.repaint();
    }
}

// ---------------------------------------------------------------------------- layout
static constexpr int kPad = 12, kGap = 8, kKnobW = 62, kSelW = 100, kRowH = 132, kTop = 78;

void RawTekEditor::resized() { content.setBounds (0, 0, kDesignW, kDesignH); layoutContent(); }

void RawTekEditor::layoutContent()
{
    sections.clear();
    const int W = kDesignW;

    // header
    sizeButton.setBounds (W - kPad - 52, 18, 52, 30);
    saveButton.setBounds (sizeButton.getX() - 6 - 56, 18, 56, 30);
    nextButton.setBounds (saveButton.getX() - 10 - 30, 18, 30, 30);
    presetDisplay.setBounds (nextButton.getX() - 4 - 300, 16, 300, 34);
    prevButton.setBounds (presetDisplay.getX() - 4 - 30, 18, 30, 30);

    // section: título + columna de selectores + fila de knobs. Devuelve el ancho.
    auto layout = [&] (const String& idx, const String& title, int x, int y, std::vector<Component*> column,
                       std::vector<Component*> knobs, int forcedW = 0, int tab = -1)
    {
        const int innerW = (column.empty() ? 0 : kSelW) + (int) knobs.size() * kKnobW;
        const int w = forcedW > 0 ? forcedW : innerW + 20;
        const Rectangle<int> area (x, y, w, kRowH);
        sections.push_back ({ idx, title, area, tab });
        auto c = area.reduced (10, 0).withTrimmedTop (32).withTrimmedBottom (6);
        c.removeFromLeft (jmax (0, (c.getWidth() - innerW) / 2));
        if (! column.empty())
        {
            auto col = c.removeFromLeft (kSelW).reduced (4, 0);
            for (auto* comp : column)
            {
                const bool isToggle = dynamic_cast<panic::Toggle*> (comp) != nullptr;
                comp->setBounds (col.removeFromTop (isToggle ? 22 : 35));
                col.removeFromTop (3);
            }
        }
        for (auto* k : knobs) k->setBounds (c.removeFromLeft (kKnobW));
        return w;
    };

    const int R = W - kPad;
    int x = kPad, y = kTop;

    // fila 1
    x += layout ("01", "OSC A", x, y, { &osc1Wave }, { &pw, &uniA, &detA }) + kGap;
    x += layout ("02", "OSC B", x, y, { &osc2Wave, &hardSync }, { &osc2Semi, &osc2Detune, &uniB, &detB }) + kGap;
    x += layout ("03", "MIXER", x, y, {}, { &oscMix, &sub, &noise, &spread }) + kGap;
    const Rectangle<int> screenArea (x, y, R - x, kRowH * 2 + kGap);
    sections.push_back ({ "", "PANIC SCOPE", screenArea, -1 });
    screen.setBounds (screenArea.reduced (8, 0).withTrimmedTop (30).withTrimmedBottom (8));
    const int leftW = x - kGap - kPad;

    // fila 2
    x = kPad; y += kRowH + kGap;
    x += layout ("04", "METAL", x, y, {}, { &fm, &fmEnv, &ring, &pitchEnv, &pitchDecay }) + kGap;
    x += layout ("05", "FILTER", x, y, { &filterMode }, { &cutoff, &reso, &drive, &envAmt, &keyTrack }) + kGap;
    layout ("06", "VOICE", x, y, { &voiceMode }, { &glide }, kPad + leftW - x);

    // fila 3
    x = kPad; y += kRowH + kGap;
    x += layout ("07", "AMP ENV", x, y, {}, { &aA, &aD, &aS, &aR }) + kGap;
    x += layout ("08", "MOD ENV", x, y, {}, { &fA, &fD, &fS, &fR }) + kGap;
    x += layout ("09", "RESONATOR", x, y, {}, { &combMix, &combTune, &combFb, &combDamp }) + kGap;
    x += layout ("10", "LFO 1", x, y, { &lfoWave, &lfoSync, &lfoDiv }, { &lfoRate, &lfoDepth, &lfoPitch }) + kGap;
    layout ("11", "LEVEL", x, y, {}, { &velSens, &volume }, R - x);

    // tabs
    y += kRowH + kGap;
    tabMatrix.setBounds (kPad, y, 150, 26);
    tabFx.setBounds (kPad + 154, y, 130, 26);
    y += 30;

    // pestaña MATRIX
    x = kPad;
    x += layout ("12", "LFO 2", x, y, { &lfo2Wave, &lfo2Sync, &lfo2Div }, { &lfo2Rate }, 0, 0) + kGap;
    {
        const Rectangle<int> area (x, y, R - x, kRowH);
        sections.push_back ({ "13", "MOD MATRIX  //  SOURCE > DESTINATION", area, 0 });
        auto c = area.reduced (12, 0).withTrimmedTop (32).withTrimmedBottom (6);
        const int cw = c.getWidth() / 4, rh = c.getHeight() / 2;
        for (int i = 0; i < (int) slots.size(); ++i)
            slots[(size_t) i]->setBounds (c.getX() + (i % 4) * cw, c.getY() + (i / 4) * rh, cw - 12, rh);
    }

    // pestaña FX
    x = kPad;
    x += layout ("14", "DISTORTION", x, y, { &distType }, { &distAmt, &distMix }, 0, 1) + kGap;
    x += layout ("15", "CHORUS", x, y, {}, { &chRate, &chDepth, &chMix }, 0, 1) + kGap;
    x += layout ("16", "DUB DELAY", x, y, { &dlyDiv, &dlyPing }, { &dlyFb, &dlyTone, &dlyMix }, 0, 1) + kGap;
    x += layout ("17", "REVERB", x, y, {}, { &revSize, &revDamp, &revPre, &revLowCut, &revMix }, 0, 1) + kGap;
    {
        const Rectangle<int> area (x, y, R - x, kRowH);
        sections.push_back ({ "", "CHAIN", area, 1 });
        fxChain.setBounds (area.reduced (14, 0).withTrimmedTop (34).withTrimmedBottom (10));
    }
}

void RawTekEditor::paint (Graphics& g) { g.fillAll (panic::Pal::bg); }

void RawTekEditor::paintContent (Graphics& g)
{
    using namespace panic;
    const int W = kDesignW, H = kDesignH;
    g.fillAll (Pal::bg);

    // textura: trama diagonal muy sutil
    g.setColour (Colour (0x07ffffff));
    for (int i = -H; i < W; i += 6)
        g.drawLine ((float) i, (float) H, (float) (i + H), 0.0f, 0.6f);

    // ---- header
    g.setColour (Colour (0xff0f0f0d));
    g.fillRect (0, 0, W, 64);

    // logo PANIC con fantasma rojo (glitch)
    const auto logoFont = display (54.0f);
    g.setFont (logoFont);
    g.setColour (Pal::red.withAlpha (0.65f));
    g.drawText ("PANIC", 20 - 2, 7 + 2, 200, 54, Justification::centredLeft);
    g.setColour (Pal::yellow);
    g.drawText ("PANIC", 20, 7, 200, 54, Justification::centredLeft);
    const int logoW = (int) GlyphArrangement::getStringWidth (logoFont, "PANIC");

    // bloque RAW TEK inclinado
    {
        const float bx = 20.0f + (float) logoW + 12.0f, by = 18.0f, bw = 104.0f, bh = 30.0f;
        Path p;
        p.startNewSubPath (bx + 8, by); p.lineTo (bx + bw + 8, by); p.lineTo (bx + bw, by + bh); p.lineTo (bx, by + bh); p.closeSubPath();
        g.setColour (Pal::yellow);
        g.fillPath (p);
        g.setFont (display (27.0f));
        g.setColour (Colours::black);
        g.drawText ("RAW TEK", (int) bx + 4, (int) by + 1, (int) bw, (int) bh, Justification::centred);

        g.setFont (mono (9.5f));
        g.setColour (Pal::dim);
        g.drawText ("RAW // HARD GROOVE // HYPNOTIC", (int) (bx + bw + 24), 20, 260, 13, Justification::centredLeft);
        g.drawText ("TECHNO SYNTHESIZER  V3.1", (int) (bx + bw + 24), 34, 260, 13, Justification::centredLeft);
    }

    // cinta de peligro
    {
        Graphics::ScopedSaveState ss (g);
        const Rectangle<int> tape (0, 64, W, 6);
        g.reduceClipRegion (tape);
        g.setColour (Colours::black);
        g.fillRect (tape);
        g.setColour (Pal::yellow);
        for (int xx = -12; xx < W + 12; xx += 14)
        {
            Path s;
            s.addQuadrilateral ((float) xx, 70.0f, (float) xx + 7, 70.0f, (float) xx + 13, 64.0f, (float) xx + 6, 64.0f);
            g.fillPath (s);
        }
    }

    // línea de base de las pestañas
    const int tabY = tabMatrix.getBottom();
    g.setColour (Pal::yellow);
    g.fillRect (kPad, tabY - 1, W - 2 * kPad, 2);

    // ---- secciones
    for (const auto& s : sections)
    {
        if (s.tab >= 0 && s.tab != currentTab) continue;
        const auto r = s.area.toFloat();
        constexpr float ch = 12.0f; // chaflán
        Path p;
        p.startNewSubPath (r.getX(), r.getY());
        p.lineTo (r.getRight() - ch, r.getY());
        p.lineTo (r.getRight(), r.getY() + ch);
        p.lineTo (r.getRight(), r.getBottom());
        p.lineTo (r.getX(), r.getBottom());
        p.closeSubPath();
        g.setGradientFill (ColourGradient (Pal::panelHi, 0, r.getY(), Pal::panel, 0, r.getBottom(), false));
        g.fillPath (p);
        g.setColour (Pal::edge);
        g.strokePath (p, PathStrokeType (1.0f));

        // etiqueta: [NN] TÍTULO
        float tx = r.getX() + 8;
        if (s.index.isNotEmpty())
        {
            const Rectangle<float> tag (tx, r.getY() + 7, 24, 16);
            g.setColour (Pal::yellow);
            g.fillRect (tag);
            g.setFont (display (16.0f));
            g.setColour (Colours::black);
            g.drawText (s.index, tag.toNearestInt(), Justification::centred);
            tx += 30;
        }
        else
        {
            g.setColour (Pal::yellow);
            g.fillRect (tx, r.getY() + 9, 4.0f, 12.0f);
            tx += 10;
        }
        g.setFont (display (19.0f));
        g.setColour (Pal::text);
        g.drawText (s.title, (int) tx, (int) r.getY() + 5, (int) (r.getRight() - tx - 14), 20, Justification::centredLeft);
        g.setColour (Pal::edge);
        g.drawHorizontalLine ((int) r.getY() + 27, r.getX() + 8, r.getRight() - 14);
    }
}
