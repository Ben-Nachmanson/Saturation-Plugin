#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
WarmSaturationProcessor::WarmSaturationProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

WarmSaturationProcessor::~WarmSaturationProcessor() {}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
WarmSaturationProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "drive", 1 },
        "Drive",
        juce::NormalisableRange<float> (0.0f, 40.0f, 0.1f, 0.5f),
        10.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 1) + " dB"; },
        nullptr));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "output", 1 },
        "Output",
        juce::NormalisableRange<float> (-24.0f, 6.0f, 0.1f),
        0.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (value, 1) + " dB"; },
        nullptr));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 },
        "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        100.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) { return juce::String (static_cast<int> (value)) + "%"; },
        nullptr));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "tone", 1 },
        "Tone",
        juce::NormalisableRange<float> (1000.0f, 20000.0f, 1.0f, 0.3f),
        12000.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [] (float value, int)
        {
            if (value >= 1000.0f)
                return juce::String (value / 1000.0f, 1) + " kHz";
            return juce::String (static_cast<int> (value)) + " Hz";
        },
        nullptr));

    return { params.begin(), params.end() };
}

//==============================================================================
void WarmSaturationProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels      = static_cast<juce::uint32> (getTotalNumOutputChannels());

    saturation.prepare (spec);
}

void WarmSaturationProcessor::releaseResources()
{
    saturation.reset();
}

bool WarmSaturationProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Support mono and stereo only
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Input must match output
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void WarmSaturationProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any extra output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Read parameter values
    float driveVal  = apvts.getRawParameterValue ("drive")->load();
    float outputVal = apvts.getRawParameterValue ("output")->load();
    float mixVal    = apvts.getRawParameterValue ("mix")->load() / 100.0f;
    float toneVal   = apvts.getRawParameterValue ("tone")->load();

    // Update DSP parameters
    saturation.setDrive (driveVal);
    saturation.setOutput (outputVal);
    saturation.setMix (mixVal);
    saturation.setTone (toneVal);

    // Process audio
    saturation.process (buffer);
}

//==============================================================================
juce::AudioProcessorEditor* WarmSaturationProcessor::createEditor()
{
    return new WarmSaturationEditor (*this);
}

bool WarmSaturationProcessor::hasEditor() const { return true; }

//==============================================================================
const juce::String WarmSaturationProcessor::getName() const { return JucePlugin_Name; }
bool WarmSaturationProcessor::acceptsMidi() const { return false; }
bool WarmSaturationProcessor::producesMidi() const { return false; }
bool WarmSaturationProcessor::isMidiEffect() const { return false; }
double WarmSaturationProcessor::getTailLengthSeconds() const { return 0.0; }

//==============================================================================
int WarmSaturationProcessor::getNumPrograms() { return 1; }
int WarmSaturationProcessor::getCurrentProgram() { return 0; }
void WarmSaturationProcessor::setCurrentProgram (int) {}
const juce::String WarmSaturationProcessor::getProgramName (int) { return {}; }
void WarmSaturationProcessor::changeProgramName (int, const juce::String&) {}

//==============================================================================
void WarmSaturationProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void WarmSaturationProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
// This creates the plugin instance
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WarmSaturationProcessor();
}
