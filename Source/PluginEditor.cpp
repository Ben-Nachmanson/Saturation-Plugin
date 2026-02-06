#include "PluginEditor.h"

//==============================================================================
// Colour palette — physical hardware aesthetic
//==============================================================================
namespace Theme
{
    // Wood — warm walnut, brighter and richer
    const juce::Colour woodDark    { 0xFF4A2E18 };   // Walnut base
    const juce::Colour woodMid     { 0xFF5E3A22 };   // Mid walnut
    const juce::Colour woodLight   { 0xFF7A4E30 };   // Lighter grain highlights
    const juce::Colour woodGrain   { 0xFF3A2010 };   // Dark grain lines

    // Matte black faceplate
    const juce::Colour panelBlack  { 0xFF1A1A1A };   // Matte black
    const juce::Colour panelDark   { 0xFF141414 };   // Darker areas
    const juce::Colour panelEdge   { 0xFF2A2A2A };   // Subtle bevel edge

    // Knobs
    const juce::Colour knobMetal   { 0xFF3A3A3A };   // Brushed dark metal
    const juce::Colour knobHighlight{ 0xFF585858 };  // Metal highlight
    const juce::Colour knobShadow  { 0xFF1A1A1A };   // Knob shadow
    const juce::Colour knobRing    { 0xFF2E2E2E };   // Outer ring

    // Accents
    const juce::Colour accentOrange{ 0xFFD4722A };   // Warm orange (slightly muted for realism)
    const juce::Colour cream       { 0xFFD8CCBA };   // Cream pointer
    const juce::Colour textLabel   { 0xFF9A8A78 };   // Stamped label text
    const juce::Colour textDim     { 0xFF5A5046 };   // Subtle text
    const juce::Colour arcBg       { 0xFF0E0E0E };   // Arc background (almost invisible)
}

//==============================================================================
// WarmLookAndFeel — brushed metal knobs with physical depth
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
    const float arcThick = juce::jmax (2.5f, diameter * 0.035f);

    // Arc background
    {
        juce::Path bg;
        bg.addCentredArc (centreX, centreY, radius, radius, 0.0f,
                          rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (Theme::arcBg);
        g.strokePath (bg, juce::PathStrokeType (arcThick,
                      juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Value arc (orange)
    {
        juce::Path val;
        val.addCentredArc (centreX, centreY, radius, radius, 0.0f,
                           rotaryStartAngle, angle, true);
        g.setColour (Theme::accentOrange);
        g.strokePath (val, juce::PathStrokeType (arcThick,
                      juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Drop shadow under knob
    {
        const float shadowR = radius * 0.72f;
        g.setColour (juce::Colour (0x40000000));
        g.fillEllipse (centreX - shadowR + 1.0f, centreY - shadowR + 2.0f,
                       shadowR * 2.0f, shadowR * 2.0f);
    }

    // Knob body — brushed metal look with radial gradient
    {
        const float knobR = radius * 0.68f;

        // Outer ring
        g.setColour (Theme::knobRing);
        g.fillEllipse (centreX - knobR - 2.0f, centreY - knobR - 2.0f,
                       (knobR + 2.0f) * 2.0f, (knobR + 2.0f) * 2.0f);

        // Radial gradient for brushed metal
        juce::ColourGradient metalGrad (Theme::knobHighlight,
                                         centreX - knobR * 0.3f, centreY - knobR * 0.3f,
                                         Theme::knobMetal,
                                         centreX + knobR * 0.5f, centreY + knobR * 0.5f,
                                         true);
        g.setGradientFill (metalGrad);
        g.fillEllipse (centreX - knobR, centreY - knobR,
                       knobR * 2.0f, knobR * 2.0f);

        // Subtle specular highlight (top-left)
        juce::ColourGradient specular (juce::Colour (0x22FFFFFF),
                                        centreX - knobR * 0.4f, centreY - knobR * 0.6f,
                                        juce::Colour (0x00FFFFFF),
                                        centreX, centreY + knobR * 0.2f, true);
        g.setGradientFill (specular);
        g.fillEllipse (centreX - knobR, centreY - knobR,
                       knobR * 2.0f, knobR * 2.0f);

        // Inner edge shadow
        g.setColour (juce::Colour (0x18000000));
        g.drawEllipse (centreX - knobR + 1.0f, centreY - knobR + 1.0f,
                       (knobR - 1.0f) * 2.0f, (knobR - 1.0f) * 2.0f, 1.0f);
    }

    // Pointer indicator (cream line, rounded)
    {
        const float ptrLen = radius * 0.48f;
        const float ptrThick = juce::jmax (2.0f, diameter * 0.028f);

        juce::Path pointer;
        pointer.addRoundedRectangle (-ptrThick * 0.5f, -ptrLen,
                                      ptrThick, ptrLen * 0.55f, ptrThick * 0.4f);
        pointer.applyTransform (juce::AffineTransform::rotation (angle)
                                    .translated (centreX, centreY));

        g.setColour (Theme::cream);
        g.fillPath (pointer);
    }
}

//==============================================================================
// Sizing constants
//==============================================================================
static constexpr int defaultWidth  = 440;
static constexpr int defaultHeight = 380;
static constexpr int minWidth      = 330;
static constexpr int minHeight     = 285;
static constexpr int maxWidth      = 880;
static constexpr int maxHeight     = 760;

//==============================================================================
// Constructor
//==============================================================================
WarmSaturationEditor::WarmSaturationEditor (WarmSaturationProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel (&warmLookAndFeel);

    setupKnob (driveKnob,  driveLabel,  "DRIVE");
    setupKnob (toneKnob,   toneLabel,   "TONE");
    setupKnob (outputKnob, outputLabel, "OUTPUT");
    setupKnob (mixKnob,    mixLabel,    "MIX");

    driveAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "drive", driveKnob);
    toneAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "tone", toneKnob);
    outputAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "output", outputKnob);
    mixAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "mix", mixKnob);

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
// Procedural wood texture generator — dark walnut with vertical grain
//==============================================================================
void WarmSaturationEditor::generateWoodTexture (int width, int height)
{
    woodTexture = juce::Image (juce::Image::ARGB, width, height, true);
    juce::Graphics g (woodTexture);

    // Base gradient (darker at edges, lighter in center for depth)
    juce::ColourGradient base (Theme::woodMid, static_cast<float> (width) * 0.5f, 0.0f,
                                Theme::woodDark, 0.0f, 0.0f, false);
    base.addColour (0.5, Theme::woodLight.withAlpha (0.4f).interpolatedWith (Theme::woodMid, 0.7f));
    g.setGradientFill (base);
    g.fillAll();

    // Draw vertical grain lines
    juce::Random rng (42);  // Fixed seed for consistent look
    for (int i = 0; i < width; i += 2)
    {
        float grainAlpha = rng.nextFloat() * 0.15f + 0.02f;
        float xOffset = static_cast<float> (i) + rng.nextFloat() * 1.5f;

        // Vary grain colour between dark and very dark
        juce::Colour grainCol = Theme::woodGrain.withAlpha (grainAlpha);
        g.setColour (grainCol);

        // Slightly wavy lines for realism
        juce::Path grain;
        grain.startNewSubPath (xOffset, 0.0f);
        for (int yy = 0; yy < height; yy += 8)
        {
            float wave = std::sin (static_cast<float> (yy) * 0.02f + rng.nextFloat() * 0.5f) * 0.8f;
            grain.lineTo (xOffset + wave, static_cast<float> (yy));
        }
        g.strokePath (grain, juce::PathStrokeType (rng.nextFloat() * 1.2f + 0.3f));
    }

    // Wider occasional grain bands
    for (int i = 0; i < 8; ++i)
    {
        float bx = rng.nextFloat() * static_cast<float> (width);
        float bw = rng.nextFloat() * 4.0f + 2.0f;
        g.setColour (Theme::woodGrain.withAlpha (rng.nextFloat() * 0.08f + 0.03f));
        g.fillRect (bx, 0.0f, bw, static_cast<float> (height));
    }

    // Subtle varnish sheen (top-to-bottom gradient)
    juce::ColourGradient varnish (juce::Colour (0x0DFFFFFF), 0.0f, 0.0f,
                                   juce::Colour (0x00000000), 0.0f, static_cast<float> (height) * 0.4f,
                                   false);
    g.setGradientFill (varnish);
    g.fillAll();
}

//==============================================================================
// Procedural matte black panel texture — powder-coated metal feel
//==============================================================================
void WarmSaturationEditor::generatePanelTexture (int width, int height)
{
    panelTexture = juce::Image (juce::Image::ARGB, width, height, true);
    juce::Graphics g (panelTexture);

    // Base matte black
    g.fillAll (Theme::panelBlack);

    // Fine noise grain (powder-coat texture)
    juce::Random rng (99);
    for (int yy = 0; yy < height; ++yy)
    {
        for (int xx = 0; xx < width; xx += 2)
        {
            float noise = rng.nextFloat();
            if (noise > 0.7f)
            {
                float alpha = (noise - 0.7f) * 0.12f;
                juce::Colour dot = (rng.nextFloat() > 0.5f)
                    ? juce::Colour (0xFFFFFFFF).withAlpha (alpha)
                    : juce::Colour (0xFF000000).withAlpha (alpha * 0.8f);
                g.setColour (dot);
                g.fillRect (xx, yy, 1, 1);
            }
        }
    }

    // Very subtle center-to-edge darkening
    juce::ColourGradient vignette (juce::Colour (0x00000000),
                                    static_cast<float> (width) * 0.5f,
                                    static_cast<float> (height) * 0.4f,
                                    juce::Colour (0x18000000),
                                    0.0f, 0.0f, true);
    g.setGradientFill (vignette);
    g.fillAll();
}

//==============================================================================
// Paint — wood side panels, matte faceplate, engraved text
//==============================================================================
void WarmSaturationEditor::paint (juce::Graphics& g)
{
    const int W = getWidth();
    const int H = getHeight();
    const float w = static_cast<float> (W);
    const float h = static_cast<float> (H);

    // Wood panel width: ~12% of total width on each side
    const float woodW = w * 0.12f;
    const float panelX = woodW;
    const float panelW = w - woodW * 2.0f;

    // Regenerate textures if size changed
    if (woodTexture.getWidth() != static_cast<int> (woodW) || woodTexture.getHeight() != H)
        generateWoodTexture (static_cast<int> (woodW), H);

    if (panelTexture.getWidth() != static_cast<int> (panelW) || panelTexture.getHeight() != H)
        generatePanelTexture (static_cast<int> (panelW), H);

    // Draw left wood panel
    g.drawImage (woodTexture, juce::Rectangle<float> (0.0f, 0.0f, woodW, h));

    // Draw right wood panel (mirrored)
    g.drawImage (woodTexture, juce::Rectangle<float> (w - woodW, 0.0f, woodW, h),
                 juce::RectanglePlacement::stretchToFit);

    // Draw center matte panel
    g.drawImage (panelTexture, juce::Rectangle<float> (panelX, 0.0f, panelW, h));

    // Panel edge highlights (bevel effect)
    g.setColour (Theme::panelEdge);
    g.drawVerticalLine (static_cast<int> (panelX), 0.0f, h);
    g.drawVerticalLine (static_cast<int> (panelX + panelW - 1.0f), 0.0f, h);

    // Inner shadow on wood-to-panel transition
    {
        juce::ColourGradient leftShadow (juce::Colour (0x30000000), panelX, 0.0f,
                                          juce::Colour (0x00000000), panelX + 6.0f, 0.0f, false);
        g.setGradientFill (leftShadow);
        g.fillRect (panelX, 0.0f, 6.0f, h);

        juce::ColourGradient rightShadow (juce::Colour (0x30000000), panelX + panelW, 0.0f,
                                           juce::Colour (0x00000000), panelX + panelW - 6.0f, 0.0f, false);
        g.setGradientFill (rightShadow);
        g.fillRect (panelX + panelW - 6.0f, 0.0f, 6.0f, h);
    }

    // Title: "WARM SATURATION" — engraved look (dark shadow + light text)
    const float titleY = h * 0.05f;
    const float titleH = h * 0.06f;

    // Shadow text (offset down-right)
    g.setColour (juce::Colour (0x40000000));
    g.setFont (juce::FontOptions (titleH, juce::Font::bold));
    g.drawText ("WARM SATURATION",
                juce::Rectangle<float> (panelX + 1.0f, titleY + 1.0f, panelW, titleH),
                juce::Justification::centred);

    // Main text
    g.setColour (Theme::cream.withAlpha (0.85f));
    g.drawText ("WARM SATURATION",
                juce::Rectangle<float> (panelX, titleY, panelW, titleH),
                juce::Justification::centred);

    // Subtle line separator under title
    const float lineY = titleY + titleH + h * 0.02f;
    g.setColour (Theme::accentOrange.withAlpha (0.5f));
    g.fillRect (panelX + panelW * 0.2f, lineY, panelW * 0.6f, 1.5f);

    // Subtitle
    g.setColour (Theme::textDim);
    g.setFont (juce::FontOptions (titleH * 0.4f));
    g.drawText ("TUBE SATURATION",
                juce::Rectangle<float> (panelX, lineY + 3.0f, panelW, titleH * 0.5f),
                juce::Justification::centred);

    // Footer
    g.setColour (Theme::textDim.withAlpha (0.35f));
    g.setFont (juce::FontOptions (h * 0.025f));
    g.drawText ("WarmAudio",
                juce::Rectangle<float> (panelX, h - h * 0.055f, panelW, h * 0.04f),
                juce::Justification::centred);

    // Small screws in corners of panel (decorative)
    auto drawScrew = [&] (float sx, float sy)
    {
        const float sr = juce::jmax (2.5f, h * 0.008f);
        // Screw head
        juce::ColourGradient screwGrad (juce::Colour (0xFF444444), sx - sr * 0.3f, sy - sr * 0.3f,
                                         juce::Colour (0xFF222222), sx + sr, sy + sr, true);
        g.setGradientFill (screwGrad);
        g.fillEllipse (sx - sr, sy - sr, sr * 2.0f, sr * 2.0f);
        // Slot
        g.setColour (juce::Colour (0xFF111111));
        g.drawLine (sx - sr * 0.5f, sy, sx + sr * 0.5f, sy, 0.8f);
    };

    const float screwMargin = h * 0.035f;
    drawScrew (panelX + screwMargin, screwMargin);
    drawScrew (panelX + panelW - screwMargin, screwMargin);
    drawScrew (panelX + screwMargin, h - screwMargin);
    drawScrew (panelX + panelW - screwMargin, h - screwMargin);
}

//==============================================================================
// Layout — large DRIVE center, 3 smaller knobs below
//==============================================================================
void WarmSaturationEditor::resized()
{
    const int W = getWidth();
    const int H = getHeight();

    cornerResizer->setBounds (W - 16, H - 16, 16, 16);

    const float woodW   = static_cast<float> (W) * 0.12f;
    const int panelX    = static_cast<int> (woodW);
    const int panelW    = W - panelX * 2;
    const int panelCX   = panelX + panelW / 2;

    const int labelH    = static_cast<int> (H * 0.045f);
    const int textBoxH  = static_cast<int> (H * 0.04f);

    // === DRIVE: large knob, upper-center (below title/subtitle) ===
    const int driveTop   = static_cast<int> (H * 0.25f);
    const int driveSize  = static_cast<int> (H * 0.35f);

    driveKnob.setBounds (panelCX - driveSize / 2, driveTop, driveSize, driveSize);
    driveKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false,
                                driveSize / 2, textBoxH);

    driveLabel.setBounds (panelCX - driveSize / 2, driveTop - labelH - 2,
                          driveSize, labelH);
    driveLabel.setColour (juce::Label::textColourId, Theme::accentOrange);
    driveLabel.setFont (juce::FontOptions (static_cast<float> (labelH) * 0.85f, juce::Font::bold));

    // === Bottom row: TONE, OUTPUT, MIX — evenly spaced across panel ===
    const int smallSize  = static_cast<int> (H * 0.22f);
    const int botRowY    = static_cast<int> (H * 0.67f);
    const int colW       = panelW / 3;

    struct KnobLayout { juce::Slider& knob; juce::Label& label; int col; };
    KnobLayout bottomKnobs[] = {
        { toneKnob,   toneLabel,   0 },
        { outputKnob, outputLabel, 1 },
        { mixKnob,    mixLabel,    2 }
    };

    for (auto& kl : bottomKnobs)
    {
        int cx = panelX + colW * kl.col + colW / 2;

        kl.knob.setBounds (cx - smallSize / 2, botRowY, smallSize, smallSize);
        kl.knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false,
                                  smallSize, textBoxH);

        kl.label.setBounds (cx - colW / 2, botRowY - labelH, colW, labelH);
    }
}
