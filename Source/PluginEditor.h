#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// Custom LookAndFeel for a warm, minimal aesthetic
//==============================================================================
class WarmLookAndFeel : public juce::LookAndFeel_V4
{
public:
    WarmLookAndFeel();

    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override;
};

//==============================================================================
class WarmSaturationEditor : public juce::AudioProcessorEditor
{
public:
    explicit WarmSaturationEditor (WarmSaturationProcessor&);
    ~WarmSaturationEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    WarmSaturationProcessor& processorRef;
    WarmLookAndFeel warmLookAndFeel;

    // Knobs
    juce::Slider driveKnob;
    juce::Slider outputKnob;
    juce::Slider mixKnob;
    juce::Slider toneKnob;

    // Labels
    juce::Label driveLabel;
    juce::Label outputLabel;
    juce::Label mixLabel;
    juce::Label toneLabel;

    // Parameter attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> toneAttachment;

    // Corner resizer for drag-to-resize
    std::unique_ptr<juce::ResizableCornerComponent> cornerResizer;
    juce::ComponentBoundsConstrainer constrainer;

    void setupKnob (juce::Slider& knob, juce::Label& label, const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WarmSaturationEditor)
};
