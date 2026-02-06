#pragma once

#include <JuceHeader.h>
#include <cmath>

//==============================================================================
// Tilt EQ — single-knob tone shaping
//
// A first-order shelving filter that pivots around ~800Hz.
// When the control is centered (0.0), the filter is flat/bypassed.
// Turning left (-1.0) cuts highs and boosts lows (warm, dark).
// Turning right (+1.0) boosts highs and cuts lows (bright, present).
//
// This is how most analog "tone" controls work — a single knob that
// shifts the spectral balance. Always audible at every position.
//
// Implementation: first-order shelf using a biquad with coefficients
// derived from the tilt amount and pivot frequency.
//==============================================================================
class TiltEQ
{
public:
    TiltEQ() = default;

    void prepare (double newSampleRate, int numChannels)
    {
        sampleRate = newSampleRate;
        x1.resize (static_cast<size_t> (numChannels), 0.0f);
        y1.resize (static_cast<size_t> (numChannels), 0.0f);
        updateCoefficients();
    }

    void reset()
    {
        std::fill (x1.begin(), x1.end(), 0.0f);
        std::fill (y1.begin(), y1.end(), 0.0f);
    }

    // Set tilt amount: -1.0 (dark) to +1.0 (bright), 0.0 = flat
    void setTilt (float newTilt)
    {
        if (std::abs (newTilt - tilt) > 0.001f)
        {
            tilt = newTilt;
            updateCoefficients();
        }
    }

    float processSample (int channel, float input)
    {
        const auto ch = static_cast<size_t> (channel);
        float output = a0 * input + a1 * x1[ch] - b1 * y1[ch];
        x1[ch] = input;
        y1[ch] = output;
        return output;
    }

private:
    void updateCoefficients()
    {
        // Pivot frequency ~800Hz
        constexpr float pivotHz = 800.0f;
        const float wc = 2.0f * juce::MathConstants<float>::pi * pivotHz
                         / static_cast<float> (sampleRate);

        // Map tilt to gain: ±6dB range
        const float gainDb = tilt * 6.0f;
        const float gain = std::pow (10.0f, gainDb / 20.0f);

        // Compute first-order shelf coefficients
        // Using matched analog prototype: H(s) = (s + wc*g) / (s + wc/g)
        // Bilinear transform gives us the digital coefficients
        const float g = gain;
        const float tanW = std::tan (wc * 0.5f);
        const float t = tanW / g;

        a0 = (tanW * g + 1.0f) / (t + 1.0f);
        a1 = (tanW * g - 1.0f) / (t + 1.0f);
        b1 = (t - 1.0f) / (t + 1.0f);
    }

    double sampleRate = 44100.0;
    float tilt = 0.0f;
    float a0 = 1.0f, a1 = 0.0f, b1 = 0.0f;

    std::vector<float> x1;  // x[n-1] per channel
    std::vector<float> y1;  // y[n-1] per channel
};

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
        numChannels = static_cast<int> (spec.numChannels);

        // Pre-gain (drive)
        preGain.prepare (spec);
        preGain.setRampDurationSeconds (0.02);

        // Post-gain (output level)
        postGain.prepare (spec);
        postGain.setRampDurationSeconds (0.02);

        // Tilt EQ for tone shaping
        tiltEQ.prepare (sampleRate, numChannels);

        // Dry buffer for mix blending
        dryBuffer.setSize (numChannels,
                           static_cast<int> (spec.maximumBlockSize));
    }

    void reset()
    {
        preGain.reset();
        postGain.reset();
        tiltEQ.reset();
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

    // Set tone tilt: -1.0 (dark) to +1.0 (bright), 0.0 = neutral
    void setTone (float toneValue)
    {
        tiltEQ.setTilt (toneValue);
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int channels   = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        // Save dry signal for mix blending
        for (int ch = 0; ch < channels; ++ch)
            dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

        // Wrap buffer in a dsp::AudioBlock
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> context (block);

        // Apply drive (pre-gain)
        preGain.process (context);

        // Apply tube-style waveshaping + tilt EQ per sample
        for (int ch = 0; ch < channels; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
            {
                data[i] = tubeWaveshape (data[i]);
                data[i] = tiltEQ.processSample (ch, data[i]);
            }
        }

        // Apply output gain
        juce::dsp::AudioBlock<float> outputBlock (buffer);
        juce::dsp::ProcessContextReplacing<float> outputContext (outputBlock);
        postGain.process (outputContext);

        // Dry/wet mix blending
        if (mix < 1.0f)
        {
            for (int ch = 0; ch < channels; ++ch)
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
    //==========================================================================
    static float tubeWaveshape (float x)
    {
        constexpr float bias = 0.15f;
        const float saturated = std::tanh (x);
        const float evenHarmonics = bias * (x * x) / (1.0f + std::abs (x));
        return saturated + evenHarmonics;
    }

    double sampleRate = 44100.0;
    int numChannels = 2;
    float mix = 1.0f;

    juce::dsp::Gain<float> preGain;
    juce::dsp::Gain<float> postGain;
    TiltEQ tiltEQ;

    juce::AudioBuffer<float> dryBuffer;
};
