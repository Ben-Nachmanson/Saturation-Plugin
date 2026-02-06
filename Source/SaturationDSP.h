#pragma once

#include <JuceHeader.h>
#include <cmath>

//==============================================================================
// Pink noise generator using the Voss-McCartney algorithm.
//
// White noise has equal energy per Hz, so it sounds harsh and "digital."
// Pink noise (1/f) has equal energy per octave — 3 dB/octave rolloff —
// which matches how we perceive frequency and sounds much more natural,
// like the hiss/hum from real analog circuits.
//
// This implementation layers multiple random sources that update at
// different rates (powers of 2), then sums them. The result approximates
// a 1/f spectral slope without needing an IIR pinking filter.
//==============================================================================
class PinkNoiseGenerator
{
public:
    PinkNoiseGenerator() { reset(); }

    void reset()
    {
        for (int i = 0; i < numRows; ++i)
            rows[i] = 0.0f;

        runningSum = 0.0f;
        counter = 0;
    }

    float nextSample()
    {
        // Find the lowest set bit that changed — determines which row to update
        int lastCounter = counter;
        ++counter;
        int diff = lastCounter ^ counter;

        for (int i = 0; i < numRows; ++i)
        {
            if (diff & (1 << i))
            {
                runningSum -= rows[i];
                rows[i] = random.nextFloat() * 2.0f - 1.0f;
                runningSum += rows[i];
            }
        }

        // Add a white noise component for the highest frequencies
        float white = random.nextFloat() * 2.0f - 1.0f;

        // Normalize: numRows contributors + 1 white noise
        return (runningSum + white) / static_cast<float> (numRows + 1);
    }

private:
    static constexpr int numRows = 12;
    float rows[numRows] {};
    float runningSum = 0.0f;
    int counter = 0;
    juce::Random random;
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

        // Tone filter: single-pole low-pass to tame upper harmonics
        toneFilter.prepare (spec);
        updateToneFilter();

        // Envelope follower state (per channel)
        envelopeState.resize (static_cast<size_t> (numChannels), 0.0f);

        // Compute envelope follower coefficients
        // Attack: very fast (0.5ms) to catch transients tightly
        // Release: moderate (50ms) so noise breathes with the signal naturally
        envelopeAttack  = std::exp (-1.0f / static_cast<float> (sampleRate * 0.0005));
        envelopeRelease = std::exp (-1.0f / static_cast<float> (sampleRate * 0.050));

        // Initialize noise high-pass filter state (one per channel)
        noiseHPState.resize (static_cast<size_t> (numChannels), 0.0f);
        noiseHPPrevInput.resize (static_cast<size_t> (numChannels), 0.0f);
        updateNoiseHPCoefficients();

        // One pink noise generator per channel for uncorrelated stereo noise
        pinkNoise.resize (static_cast<size_t> (numChannels));
        for (auto& gen : pinkNoise)
            gen.reset();

        // Dry buffer for mix blending
        dryBuffer.setSize (numChannels,
                           static_cast<int> (spec.maximumBlockSize));
    }

    void reset()
    {
        preGain.reset();
        postGain.reset();
        toneFilter.reset();
        for (auto& env : envelopeState)
            env = 0.0f;

        for (auto& s : noiseHPState)
            s = 0.0f;

        for (auto& s : noiseHPPrevInput)
            s = 0.0f;

        for (auto& gen : pinkNoise)
            gen.reset();
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

    // Set analog noise amount (0.0 to 1.0)
    void setNoise (float newNoise)
    {
        noiseAmount = newNoise;
    }

    // Set noise high-pass cutoff in Hz (20 to 1000)
    // Filters the noise ONLY — removes low-end rumble without touching audio
    void setNoiseHP (float frequencyHz)
    {
        noiseHPFrequency = frequencyHz;
        updateNoiseHPCoefficients();
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

        // Apply tube-style waveshaping sample by sample
        for (int ch = 0; ch < channels; ++ch)
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

        // Add analog-style noise (after saturation, before output gain)
        if (noiseAmount > 0.0f)
            addAnalogNoise (buffer);

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

    //==========================================================================
    // Analog noise injection
    //
    // Real analog circuits have noise that interacts with the signal:
    //   - A small constant noise floor (thermal noise, always present)
    //   - Signal-dependent noise (tubes generate more noise when driven
    //     harder — the hotter the signal, the more hiss and crackle)
    //
    // The noise is primarily signal-dependent (80%) with a small constant
    // floor (20%), so it breathes dynamically with the music. The envelope
    // follower has fast attack (0.5ms) to catch transients and moderate
    // release (50ms) so the noise decays naturally.
    //
    // A per-sample first-order high-pass filter is applied to the noise
    // BEFORE it's mixed into the audio, removing low-end rumble without
    // touching the signal at all.
    //
    // The noise level maps from 0-100% to roughly -60dB to -20dB.
    //==========================================================================
    void addAnalogNoise (juce::AudioBuffer<float>& buffer)
    {
        const int channels   = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        // Map 0..1 noiseAmount to a dB range: -60dB (silent) to -30dB (present but under the mix)
        // The NOISE knob is the absolute ceiling — signal modulation only shapes
        // the dynamics, it never pushes the noise louder than the knob setting.
        const float noiseGainDb = -60.0f + noiseAmount * 30.0f;
        const float noiseGain   = juce::Decibels::decibelsToGain (noiseGainDb);

        constexpr float floorRatio  = 0.15f;   // Small constant noise floor
        constexpr float signalRatio = 0.85f;   // Primarily signal-dependent

        const float a = noiseHPAlpha;

        for (int ch = 0; ch < channels; ++ch)
        {
            auto* data      = buffer.getWritePointer (ch);
            const auto idx  = static_cast<size_t> (ch);
            float env       = envelopeState[idx];
            float hpY       = noiseHPState[idx];       // y[n-1]
            float hpX       = noiseHPPrevInput[idx];    // x[n-1]

            for (int i = 0; i < numSamples; ++i)
            {
                // Envelope follower: track signal level tightly
                const float absSignal = std::abs (data[i]);
                if (absSignal > env)
                    env = envelopeAttack * env + (1.0f - envelopeAttack) * absSignal;
                else
                    env = envelopeRelease * env + (1.0f - envelopeRelease) * absSignal;

                // Generate pink noise sample (raw, unfiltered)
                const float rawNoise = pinkNoise[idx].nextSample();

                // First-order high-pass filter on noise only:
                //   y[n] = alpha * (y[n-1] + x[n] - x[n-1])
                // Removes low-end rumble below the cutoff frequency
                const float filteredNoise = a * (hpY + rawNoise - hpX);
                hpY = filteredNoise;
                hpX = rawNoise;

                // Scale noise: mostly signal-dependent, small constant floor
                // Clamp envelope to 1.0 so the knob is always the ceiling
                const float clampedEnv = juce::jmin (env, 1.0f);
                const float noiseLevel = noiseGain * (floorRatio + signalRatio * clampedEnv);

                data[i] += filteredNoise * noiseLevel;
            }

            envelopeState[idx]     = env;
            noiseHPState[idx]      = hpY;
            noiseHPPrevInput[idx]  = hpX;
        }
    }

    void updateToneFilter()
    {
        if (sampleRate > 0.0)
        {
            *toneFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass (
                sampleRate, toneFrequency, 0.707f);
        }
    }

    void updateNoiseHPCoefficients()
    {
        if (sampleRate > 0.0)
        {
            // First-order high-pass coefficient
            // alpha = RC / (RC + dt), where RC = 1/(2*pi*fc), dt = 1/sr
            const double rc = 1.0 / (2.0 * juce::MathConstants<double>::pi * static_cast<double> (noiseHPFrequency));
            const double dt = 1.0 / sampleRate;
            noiseHPAlpha = static_cast<float> (rc / (rc + dt));
        }
    }

    double sampleRate = 44100.0;
    int numChannels = 2;
    float mix = 1.0f;
    float toneFrequency = 12000.0f;
    float noiseAmount = 0.0f;
    float noiseHPFrequency = 80.0f;
    float noiseHPAlpha = 0.0f;

    float envelopeAttack  = 0.0f;
    float envelopeRelease = 0.0f;

    juce::dsp::Gain<float> preGain;
    juce::dsp::Gain<float> postGain;

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                    juce::dsp::IIR::Coefficients<float>> toneFilter;

    std::vector<float> envelopeState;
    std::vector<float> noiseHPState;      // y[n-1] per channel for noise high-pass
    std::vector<float> noiseHPPrevInput;  // x[n-1] per channel for noise high-pass
    std::vector<PinkNoiseGenerator> pinkNoise;

    juce::AudioBuffer<float> dryBuffer;
};
