#pragma once

#include <JuceHeader.h>
#include <cmath>

//==============================================================================
// Tube-style saturation processor
//
// Emulates the asymmetric soft-clipping behavior of vacuum tubes.
// Real tubes clip positive and negative halves differently, generating
// even-order harmonics (2nd, 4th...) which sound "warm" and "musical."
// The squared term in the transfer function creates this asymmetry.
//==============================================================================
class TubeSaturation
{
public:
    TubeSaturation() = default;

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;

        // Pre-gain (drive)
        preGain.prepare (spec);
        preGain.setRampDurationSeconds (0.02);

        // Post-gain (output level)
        postGain.prepare (spec);
        postGain.setRampDurationSeconds (0.02);

        // Tone filter: single-pole low-pass to tame upper harmonics
        toneFilter.prepare (spec);
        updateToneFilter();

        // Dry buffer for mix blending
        dryBuffer.setSize (static_cast<int> (spec.numChannels),
                           static_cast<int> (spec.maximumBlockSize));
    }

    void reset()
    {
        preGain.reset();
        postGain.reset();
        toneFilter.reset();
    }

    // Set drive amount in dB (0 to 40)
    void setDrive (float driveDb)
    {
        preGain.setGainDecibels (driveDb);
    }

    // Set output level in dB (-24 to +6)
    void setOutput (float outputDb)
    {
        postGain.setGainDecibels (outputDb);
    }

    // Set dry/wet mix (0.0 to 1.0)
    void setMix (float newMix)
    {
        mix = newMix;
    }

    // Set tone filter cutoff in Hz (1000 to 20000)
    void setTone (float frequencyHz)
    {
        toneFrequency = frequencyHz;
        updateToneFilter();
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples  = buffer.getNumSamples();

        // Save dry signal for mix blending
        for (int ch = 0; ch < numChannels; ++ch)
            dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

        // Wrap buffer in a dsp::AudioBlock
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> context (block);

        // Apply drive (pre-gain)
        preGain.process (context);

        // Apply tube-style waveshaping sample by sample
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
            {
                data[i] = tubeWaveshape (data[i]);
            }
        }

        // Apply tone filter (tame harsh upper harmonics)
        juce::dsp::AudioBlock<float> filteredBlock (buffer);
        juce::dsp::ProcessContextReplacing<float> filterContext (filteredBlock);
        toneFilter.process (filterContext);

        // Apply output gain
        juce::dsp::AudioBlock<float> outputBlock (buffer);
        juce::dsp::ProcessContextReplacing<float> outputContext (outputBlock);
        postGain.process (outputContext);

        // Dry/wet mix blending
        if (mix < 1.0f)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* wetData = buffer.getWritePointer (ch);
                const auto* dryData = dryBuffer.getReadPointer (ch);

                for (int i = 0; i < numSamples; ++i)
                {
                    wetData[i] = dryData[i] * (1.0f - mix) + wetData[i] * mix;
                }
            }
        }
    }

private:
    //==========================================================================
    // Tube waveshaping transfer function
    //
    // Combines tanh soft-clipping with an asymmetric squared term:
    //   f(x) = tanh(x) + bias * x^2 / (1 + |x|)
    //
    // The x^2 term is always positive regardless of input sign, creating
    // asymmetry in the transfer curve. This asymmetry generates even-order
    // harmonics (2nd, 4th) which are the signature of tube warmth.
    //
    // The denominator (1 + |x|) prevents the squared term from blowing up
    // at high drive levels.
    //==========================================================================
    static float tubeWaveshape (float x)
    {
        constexpr float bias = 0.15f;  // Controls even-harmonic amount
        const float saturated = std::tanh (x);
        const float evenHarmonics = bias * (x * x) / (1.0f + std::abs (x));
        return saturated + evenHarmonics;
    }

    void updateToneFilter()
    {
        if (sampleRate > 0.0)
        {
            *toneFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass (
                sampleRate, toneFrequency, 0.707f);
        }
    }

    double sampleRate = 44100.0;
    float mix = 1.0f;
    float toneFrequency = 12000.0f;

    juce::dsp::Gain<float> preGain;
    juce::dsp::Gain<float> postGain;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                    juce::dsp::IIR::Coefficients<float>> toneFilter;

    juce::AudioBuffer<float> dryBuffer;
};
