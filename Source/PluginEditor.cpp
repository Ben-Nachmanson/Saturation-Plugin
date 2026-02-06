#include "PluginEditor.h"

//==============================================================================
// Colour palette
//==============================================================================
namespace Theme
{
    const juce::Colour background   { 0xFF1A1A2E };  // Deep dark blue
    const juce::Colour surface      { 0xFF16213E };  // Slightly lighter
    const juce::Colour warmOrange   { 0xFFE8A838 };  // Warm amber/orange
    const juce::Colour warmGlow     { 0xFFF5C361 };  // Lighter warm glow
    const juce::Colour knobTrack    { 0xFF2A2A4A };  // Knob background track
    const juce::Colour textPrimary  { 0xFFE0D5C1 };  // Warm off-white
    const juce::Colour textSecondary{ 0xFF8A8070 };  // Muted warm grey
}

//==============================================================================
// WarmLookAndFeel
//==============================================================================
WarmLookAndFeel::WarmLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, Theme::textPrimary);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0x00000000));
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0x00000000));
    setColour (juce::Label::textColourId, Theme::textPrimary);
}

void WarmLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                         int x, int y, int width, int height,
                                         float sliderPos,
                                         float rotaryStartAngle,
                                         float rotaryEndAngle,
                                         juce::Slider& /*slider*/)
{
    const float radius   = static_cast<float> (juce::jmin (width, height)) * 0.4f;
    const float centreX  = static_cast<float> (x) + static_cast<float> (width) * 0.5f;
    const float centreY  = static_cast<float> (y) + static_cast<float> (height) * 0.5f;
    const float angle    = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Background track (full arc)
    {
        juce::Path bgArc;
        bgArc.addCentredArc (centreX, centreY, radius, radius, 0.0f,
                             rotaryStartAngle, rotaryEndAngle, true);

        g.setColour (Theme::knobTrack);
        g.strokePath (bgArc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    }

    // Value arc (filled portion)
    {
        juce::Path valueArc;
        valueArc.addCentredArc (centreX, centreY, radius, radius, 0.0f,
                                rotaryStartAngle, angle, true);

        g.setColour (Theme::warmOrange);
        g.strokePath (valueArc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
    }

    // Knob body (filled circle)
    {
        const float knobRadius = radius * 0.65f;
        g.setColour (Theme::surface);
        g.fillEllipse (centreX - knobRadius, centreY - knobRadius,
                       knobRadius * 2.0f, knobRadius * 2.0f);

        // Subtle border
        g.setColour (Theme::knobTrack);
        g.drawEllipse (centreX - knobRadius, centreY - knobRadius,
                       knobRadius * 2.0f, knobRadius * 2.0f, 1.5f);
    }

    // Pointer/indicator line
    {
        const float pointerLength = radius * 0.55f;
        const float pointerThickness = 2.5f;

        juce::Path pointer;
        pointer.addRectangle (-pointerThickness * 0.5f, -pointerLength, pointerThickness, pointerLength);

        pointer.applyTransform (juce::AffineTransform::rotation (angle)
                                    .translated (centreX, centreY));

        g.setColour (Theme::warmOrange);
        g.fillPath (pointer);
    }
}

//==============================================================================
// WarmSaturationEditor
//==============================================================================
static constexpr int defaultWidth  = 500;
static constexpr int defaultHeight = 300;
static constexpr int minWidth      = 375;
static constexpr int minHeight     = 225;
static constexpr int maxWidth      = 1000;
static constexpr int maxHeight     = 600;

WarmSaturationEditor::WarmSaturationEditor (WarmSaturationProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel (&warmLookAndFeel);

    // Setup all 5 knobs
    setupKnob (driveKnob,  driveLabel,  "DRIVE");
    setupKnob (outputKnob, outputLabel, "OUTPUT");
    setupKnob (mixKnob,    mixLabel,    "MIX");
    setupKnob (toneKnob,   toneLabel,   "TONE");
    setupKnob (noiseKnob,  noiseLabel,  "NOISE");

    // Attach parameters
    driveAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "drive", driveKnob);
    outputAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "output", outputKnob);
    mixAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "mix", mixKnob);
    toneAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "tone", toneKnob);
    noiseAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "noise", noiseKnob);

    // Setup resizable corner
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
    knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    addAndMakeVisible (knob);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    label.setColour (juce::Label::textColourId, Theme::textSecondary);
    addAndMakeVisible (label);
}

//==============================================================================
void WarmSaturationEditor::paint (juce::Graphics& g)
{
    // Fill background
    g.fillAll (Theme::background);

    // Title
    auto titleArea = getLocalBounds().removeFromTop (proportionOfHeight (0.18f));
    g.setColour (Theme::warmOrange);
    g.setFont (juce::FontOptions (proportionOfHeight (0.07f), juce::Font::bold));
    g.drawText ("WARM SATURATION", titleArea, juce::Justification::centred);

    // Subtle subtitle
    auto subtitleArea = getLocalBounds().removeFromTop (proportionOfHeight (0.24f))
                            .removeFromBottom (proportionOfHeight (0.06f));
    g.setColour (Theme::textSecondary);
    g.setFont (juce::FontOptions (proportionOfHeight (0.035f)));
    g.drawText ("Tube Saturation", subtitleArea, juce::Justification::centred);
}

void WarmSaturationEditor::resized()
{
    auto bounds = getLocalBounds();

    // Position the corner resizer
    const int resizerSize = 16;
    cornerResizer->setBounds (getWidth() - resizerSize, getHeight() - resizerSize,
                              resizerSize, resizerSize);

    // Reserve space for title area
    bounds.removeFromTop (proportionOfHeight (0.28f));

    // Bottom padding
    bounds.removeFromBottom (proportionOfHeight (0.05f));

    // Divide remaining space into 5 equal columns
    const int knobWidth = bounds.getWidth() / 5;
    const int labelHeight = proportionOfHeight (0.06f);

    struct KnobPair { juce::Slider& knob; juce::Label& label; };
    KnobPair knobs[] = {
        { driveKnob,  driveLabel  },
        { outputKnob, outputLabel },
        { mixKnob,    mixLabel    },
        { toneKnob,   toneLabel   },
        { noiseKnob,  noiseLabel  }
    };

    for (auto& kp : knobs)
    {
        auto column = bounds.removeFromLeft (knobWidth);
        auto labelArea = column.removeFromTop (labelHeight);

        kp.label.setBounds (labelArea);
        kp.knob.setBounds (column);

        // Scale text box proportionally
        kp.knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false,
                                  static_cast<int> (knobWidth * 0.8f),
                                  static_cast<int> (proportionOfHeight (0.06f)));
    }
}
