#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

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

    // Knobs (4 total: drive big center, tone/output/mix smaller below)
    juce::Slider driveKnob;
    juce::Slider toneKnob;
    juce::Slider outputKnob;
    juce::Slider mixKnob;

    // Labels
    juce::Label driveLabel;
    juce::Label toneLabel;
    juce::Label outputLabel;
    juce::Label mixLabel;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> toneAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;

    // Resizer
    std::unique_ptr<juce::ResizableCornerComponent> cornerResizer;
    juce::ComponentBoundsConstrainer constrainer;

    // Procedural textures (cached as images for performance)
    juce::Image woodTexture;
    juce::Image panelTexture;

    void setupKnob (juce::Slider& knob, juce::Label& label, const juce::String& text);
    void generateWoodTexture (int width, int height);
    void generatePanelTexture (int width, int height);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WarmSaturationEditor)
};
