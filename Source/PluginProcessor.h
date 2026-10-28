// SPDX-License-Identifier: MIT OR AGPL-3.0-only
// Source edition changes Copyright (c) 2026 Sungwoon Jeon
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class FUZZAAudioProcessor  : public juce::AudioProcessor
{
public:
    FUZZAAudioProcessor();
    ~FUZZAAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;
    juce::AudioProcessorParameter* getBypassParameter() const override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    struct RawParameters {
        std::atomic<float>* bypass{}; std::atomic<float>* gain{};
        std::atomic<float>* tone{}; std::atomic<float>* mix{};
        std::atomic<float>* gate{}; std::atomic<float>* clip{};
    } raw;
    double processingSampleRate=44100.0;
    float inputAttack=0, inputRelease=0, outputAttack=0, outputRelease=0;

    // Tone filter (low-pass filter for each channel)
    juce::dsp::StateVariableTPTFilter<float> toneFilterLeft;
    juce::dsp::StateVariableTPTFilter<float> toneFilterRight;

    // Gate gain state for smoothing (Envelope Follower)
    // Input Gate (Tight)
    float inputGateGainLeft = 0.0f;
    float inputGateGainRight = 0.0f;

    // Output Suppressor (Smooth/Sustain)
    float outputGateGainLeft = 0.0f;
    float outputGateGainRight = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FUZZAAudioProcessor)
};
