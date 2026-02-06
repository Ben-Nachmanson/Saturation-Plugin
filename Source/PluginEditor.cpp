#include "PluginEditor.h"

//==============================================================================
// Colour palette — Moog-inspired dark panel with orange accents, hi-fi finish
//==============================================================================
namespace Theme
{
    const juce::Colour panelDark      { 0xFF1C1C22 };  // Main panel background
    const juce::Colour panelMid       { 0xFF26262E };  // Section panel backgrounds
    const juce::Colour panelBorder    { 0xFF3A3A44 };  // Subtle section borders
    const juce::Colour knobBody       { 0xFF2A2A32 };  // Knob fill
    const juce::Colour knobRing       { 0xFF3E3E48 };  // Knob outer ring
    const juce::Colour knobTrack      { 0xFF1A1A20 };  // Arc background
    const juce::Colour accentOrange   { 0xFFE8712A };  // Primary accent (Moog orange)
    const juce::Colour accentGlow     { 0xFFFF8C42 };  // Lighter orange glow
    const juce::Colour cream          { 0xFFE8DCC8 };  // Cream white for pointers/values
    const juce::Colour textLabel      { 0xFFB0A898 };  // Muted warm label text
    const juce::Colour textDim        { 0xFF706858 };  // Dimmed text / subtitle
}

//==============================================================================
// WarmLookAndFeel — hi-fi dark knobs with orange arcs
//==============================================================================
WarmLookAndFeel::WarmLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, Theme::cream);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0x00000000));
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0x00000000));
    setColour (juce::Label::textColourId, Theme::cream);
}

void WarmLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                         int x, int y, int width, int height,
                                         float sliderPos,
                                         float rotaryStartAngle,
                                         float rotaryEndAngle,
                                         juce::Slider& /*slider*/)
{
    const float diameter = static_cast<float> (juce::jmin (width, height));
    const float radius   = diameter * 0.40f;
    const float centreX  = static_cast<float> (x) + static_cast<float> (width) * 0.5f;
    const float centreY  = static_cast<float> (y) + static_cast<float> (height) * 0.5f;
    const float angle    = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const float arcThickness = juce::jmax (2.5f, diameter * 0.04f);

    // Background arc (full sweep)
    {
        juce::Path bgArc;
        bgArc.addCentredArc (centreX, centreY, radius, radius, 0.0f,
                             rotaryStartAngle, rotaryEndAngle, true);

        g.setColour (Theme::knobTrack);
        g.strokePath (bgArc, juce::PathStrokeType (arcThickness,
                      juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Value arc (orange glow)
    {
        juce::Path valueArc;
        valueArc.addCentredArc (centreX, centreY, radius, radius, 0.0f,
                                rotaryStartAngle, angle, true);

        g.setColour (Theme::accentOrange);
        g.strokePath (valueArc, juce::PathStrokeType (arcThickness,
                      juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Knob body — dark circle with subtle 3D ring
    {
        const float knobR = radius * 0.68f;

        // Outer ring (slightly lighter)
        g.setColour (Theme::knobRing);
        g.fillEllipse (centreX - knobR - 1.5f, centreY - knobR - 1.5f,
                       (knobR + 1.5f) * 2.0f, (knobR + 1.5f) * 2.0f);

        // Inner body
        g.setColour (Theme::knobBody);
        g.fillEllipse (centreX - knobR, centreY - knobR,
                       knobR * 2.0f, knobR * 2.0f);

        // Top highlight (subtle glossy reflection)
        juce::ColourGradient highlight (juce::Colour (0x18FFFFFF), centreX, centreY - knobR * 0.5f,
                                        juce::Colour (0x00FFFFFF), centreX, centreY + knobR * 0.3f,
                                        false);
        g.setGradientFill (highlight);
        g.fillEllipse (centreX - knobR, centreY - knobR,
                       knobR * 2.0f, knobR * 2.0f);
    }

    // Pointer indicator (cream line)
    {
        const float ptrLen = radius * 0.52f;
        const float ptrThick = juce::jmax (2.0f, diameter * 0.025f);

        juce::Path pointer;
        pointer.addRoundedRectangle (-ptrThick * 0.5f, -ptrLen, ptrThick, ptrLen * 0.6f, ptrThick * 0.3f);
        pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));

        g.setColour (Theme::cream);
        g.fillPath (pointer);
    }

    // Tiny dot at knob center
    {
        const float dotR = juce::jmax (1.5f, diameter * 0.02f);
        g.setColour (Theme::accentOrange.withAlpha (0.6f));
        g.fillEllipse (centreX - dotR, centreY - dotR, dotR * 2.0f, dotR * 2.0f);
    }
}

//==============================================================================
// WarmSaturationEditor
//==============================================================================
static constexpr int defaultWidth  = 480;
static constexpr int defaultHeight = 420;
static constexpr int minWidth      = 360;
static constexpr int minHeight     = 315;
static constexpr int maxWidth      = 960;
static constexpr int maxHeight     = 840;

WarmSaturationEditor::WarmSaturationEditor (WarmSaturationProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel (&warmLookAndFeel);

    // Setup knobs
    setupKnob (driveKnob,   driveLabel,   "DRIVE");
    setupKnob (toneKnob,    toneLabel,    "TONE");
    setupKnob (outputKnob,  outputLabel,  "OUTPUT");
    setupKnob (mixKnob,     mixLabel,     "MIX");
    setupKnob (noiseKnob,   noiseLabel,   "NOISE");
    setupKnob (noiseHPKnob, noiseHPLabel, "NOISE HP");

    // Section labels
    outputSectionLabel.setText ("OUTPUT", juce::dontSendNotification);
    outputSectionLabel.setJustificationType (juce::Justification::centred);
    outputSectionLabel.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    outputSectionLabel.setColour (juce::Label::textColourId, Theme::accentOrange);
    addAndMakeVisible (outputSectionLabel);

    noiseSectionLabel.setText ("NOISE", juce::dontSendNotification);
    noiseSectionLabel.setJustificationType (juce::Justification::centred);
    noiseSectionLabel.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    noiseSectionLabel.setColour (juce::Label::textColourId, Theme::accentOrange);
    addAndMakeVisible (noiseSectionLabel);

    // Attach parameters
    driveAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "drive", driveKnob);
    outputAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "output", outputKnob);
    mixAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "mix", mixKnob);
    toneAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "tone", toneKnob);
    noiseAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "noise", noiseKnob);
    noiseHPAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "noiseHP", noiseHPKnob);

    // Corner resizer
    constrainer.setMinimumSize (minWidth, minHeight);
    constrainer.setMaximumSize (maxWidth, maxHeight);
    constrainer.setFixedAspectRatio (static_cast<double> (defaultWidth)
                                     / static_cast<double> (defaultHeight));

    cornerResizer = std::make_unique<juce::ResizableCornerComponent> (this, &constrainer);
    addAndMakeVisible (*cornerResizer);

    setResizable (true, true);
    setResizeLimits (minWidth, minHeight, maxWidth, maxHeight);
    getConstrainer()->setFixedAspectRatio (static_cast<double> (defaultWidth)
                                           / static_cast<double> (defaultHeight));

    setSize (defaultWidth, defaultHeight);
}

WarmSaturationEditor::~WarmSaturationEditor()
{
    setLookAndFeel (nullptr);
}

void WarmSaturationEditor::setupKnob (juce::Slider& knob, juce::Label& label,
                                        const juce::String& text)
{
    knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
    addAndMakeVisible (knob);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    label.setColour (juce::Label::textColourId, Theme::textLabel);
    addAndMakeVisible (label);
}

//==============================================================================
// Helper: draw a rounded section panel
//==============================================================================
static void drawSectionPanel (juce::Graphics& g, juce::Rectangle<int> bounds, float cornerSize = 6.0f)
{
    g.setColour (Theme::panelMid);
    g.fillRoundedRectangle (bounds.toFloat(), cornerSize);
    g.setColour (Theme::panelBorder);
    g.drawRoundedRectangle (bounds.toFloat().reduced (0.5f), cornerSize, 1.0f);
}

//==============================================================================
void WarmSaturationEditor::paint (juce::Graphics& g)
{
    const auto w = static_cast<float> (getWidth());
    const auto h = static_cast<float> (getHeight());

    // Main panel background with subtle vertical gradient
    juce::ColourGradient bgGrad (Theme::panelDark.brighter (0.04f), 0.0f, 0.0f,
                                  Theme::panelDark, 0.0f, h, false);
    g.setGradientFill (bgGrad);
    g.fillAll();

    // Top accent line (thin orange strip)
    g.setColour (Theme::accentOrange);
    g.fillRect (0.0f, 0.0f, w, 3.0f);

    // Title: "WARM SATURATION"
    const float titleH = h * 0.09f;
    auto titleArea = juce::Rectangle<float> (0.0f, 8.0f, w, titleH);
    g.setColour (Theme::cream);
    g.setFont (juce::FontOptions (titleH * 0.65f, juce::Font::bold));
    g.drawText ("WARM SATURATION", titleArea, juce::Justification::centred);

    // Subtitle
    auto subArea = juce::Rectangle<float> (0.0f, 8.0f + titleH * 0.85f, w, titleH * 0.5f);
    g.setColour (Theme::textDim);
    g.setFont (juce::FontOptions (titleH * 0.3f));
    g.drawText ("TUBE SATURATION", subArea, juce::Justification::centred);

    // --- Section panels ---
    const float margin     = w * 0.03f;
    const float topZone    = h * 0.20f;
    const float midH       = h * 0.42f;
    const float botH       = h * 0.32f;
    const float botY       = topZone + midH + h * 0.01f;
    const float halfW      = (w - margin * 3.0f) * 0.5f;

    // Bottom-left section panel (NOISE)
    auto noisePanel = juce::Rectangle<float> (margin, botY, halfW, botH - margin);
    drawSectionPanel (g, noisePanel.toNearestInt());

    // Bottom-right section panel (OUTPUT)
    auto outputPanel = juce::Rectangle<float> (margin * 2.0f + halfW, botY, halfW, botH - margin);
    drawSectionPanel (g, outputPanel.toNearestInt());

    // Footer branding
    g.setColour (Theme::textDim.withAlpha (0.4f));
    g.setFont (juce::FontOptions (h * 0.025f));
    g.drawText ("WarmAudio", juce::Rectangle<float> (0.0f, h - h * 0.05f, w, h * 0.04f),
                juce::Justification::centred);
}

//==============================================================================
void WarmSaturationEditor::resized()
{
    const int W = getWidth();
    const int H = getHeight();

    // Corner resizer
    const int rz = 16;
    cornerResizer->setBounds (W - rz, H - rz, rz, rz);

    // Layout proportions
    const int margin     = static_cast<int> (W * 0.03f);
    const int topZone    = static_cast<int> (H * 0.20f);      // Title area
    const int midH       = static_cast<int> (H * 0.42f);      // Drive zone
    const int botY       = topZone + midH + static_cast<int> (H * 0.01f);
    const int botH       = static_cast<int> (H * 0.32f) - margin;
    const int halfW      = (W - margin * 3) / 2;
    const int labelH     = static_cast<int> (H * 0.05f);
    const int textBoxH   = static_cast<int> (H * 0.04f);
    const int sectionLblH = static_cast<int> (H * 0.04f);

    // ======================================
    // CENTER: Large DRIVE knob + TONE to the left
    // ======================================
    const int driveCenterX = W / 2;
    const int driveCenterY = topZone + midH / 2;
    const int driveSize    = static_cast<int> (midH * 0.82f);

    driveKnob.setBounds (driveCenterX - driveSize / 2,
                         driveCenterY - driveSize / 2,
                         driveSize, driveSize);
    driveKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false,
                                driveSize / 2, textBoxH);

    driveLabel.setBounds (driveCenterX - driveSize / 2,
                          driveCenterY - driveSize / 2 - labelH - 2,
                          driveSize, labelH);
    driveLabel.setColour (juce::Label::textColourId, Theme::accentOrange);
    driveLabel.setFont (juce::FontOptions (static_cast<float> (labelH) * 0.75f, juce::Font::bold));

    // TONE: to the left of DRIVE
    const int smallKnob   = static_cast<int> (midH * 0.45f);
    const int toneX       = margin + static_cast<int> (halfW * 0.15f);
    const int toneCenterY = driveCenterY + static_cast<int> (midH * 0.05f);

    toneKnob.setBounds (toneX, toneCenterY - smallKnob / 2, smallKnob, smallKnob);
    toneKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false,
                               smallKnob, textBoxH);

    toneLabel.setBounds (toneX, toneCenterY - smallKnob / 2 - labelH,
                         smallKnob, labelH);

    // ======================================
    // BOTTOM-LEFT: NOISE section (Noise + Noise HP)
    // ======================================
    const int secInner    = static_cast<int> (margin * 0.6f);
    const int nlX         = margin;
    const int nlY         = botY;
    const int nlW         = halfW;

    noiseSectionLabel.setBounds (nlX, nlY + secInner / 2, nlW, sectionLblH);

    const int pairKnob    = static_cast<int> ((botH - sectionLblH - secInner * 2) * 0.85f);
    const int pairY       = nlY + sectionLblH + secInner;
    const int colW        = nlW / 2;

    noiseKnob.setBounds (nlX + colW / 2 - pairKnob / 2, pairY, pairKnob, pairKnob);
    noiseKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, pairKnob, textBoxH);
    noiseLabel.setBounds (nlX, pairY - labelH, colW, labelH);

    noiseHPKnob.setBounds (nlX + colW + colW / 2 - pairKnob / 2, pairY, pairKnob, pairKnob);
    noiseHPKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, pairKnob, textBoxH);
    noiseHPLabel.setBounds (nlX + colW, pairY - labelH, colW, labelH);

    // ======================================
    // BOTTOM-RIGHT: OUTPUT section (Output + Mix)
    // ======================================
    const int orX = margin * 2 + halfW;

    outputSectionLabel.setBounds (orX, nlY + secInner / 2, halfW, sectionLblH);

    outputKnob.setBounds (orX + colW / 2 - pairKnob / 2, pairY, pairKnob, pairKnob);
    outputKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, pairKnob, textBoxH);
    outputLabel.setBounds (orX, pairY - labelH, colW, labelH);

    mixKnob.setBounds (orX + colW + colW / 2 - pairKnob / 2, pairY, pairKnob, pairKnob);
    mixKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, pairKnob, textBoxH);
    mixLabel.setBounds (orX + colW, pairY - labelH, colW, labelH);
}
