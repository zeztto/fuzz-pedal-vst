#include "PluginProcessor.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
constexpr float tolerance = 1.0e-5f;

void setFloatParameter(FUZZAAudioProcessor& processor, const char* parameterID,
                       float value) {
  auto* parameter = dynamic_cast<juce::AudioParameterFloat*>(
      processor.apvts.getParameter(parameterID));

  if (parameter == nullptr)
    throw std::runtime_error(std::string("Missing float parameter: ") +
                             parameterID);

  *parameter = value;
}

void setBoolParameter(FUZZAAudioProcessor& processor, const char* parameterID,
                      bool value) {
  auto* parameter = dynamic_cast<juce::AudioParameterBool*>(
      processor.apvts.getParameter(parameterID));

  if (parameter == nullptr)
    throw std::runtime_error(std::string("Missing bool parameter: ") +
                             parameterID);

  *parameter = value;
}

void setChoiceParameter(FUZZAAudioProcessor& processor, const char* parameterID,
                        int value) {
  auto* parameter = dynamic_cast<juce::AudioParameterChoice*>(
      processor.apvts.getParameter(parameterID));

  if (parameter == nullptr)
    throw std::runtime_error(std::string("Missing choice parameter: ") +
                             parameterID);

  *parameter = value;
}

bool nearlyEqual(float a, float b) {
  return std::abs(a - b) <= tolerance;
}

bool bypassReturnsOriginalSignal() {
  FUZZAAudioProcessor processor;
  processor.prepareToPlay(44100.0, 8);
  setBoolParameter(processor, "BYPASS", true);

  juce::AudioBuffer<float> buffer(2, 8);
  juce::MidiBuffer midi;

  for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
      buffer.setSample(channel, sample,
                       static_cast<float>((channel + 1) * (sample + 1)) * 0.01f);

  juce::AudioBuffer<float> original(buffer);
  processor.processBlock(buffer, midi);

  for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
      if (!nearlyEqual(buffer.getSample(channel, sample),
                       original.getSample(channel, sample)))
        return false;

  return true;
}

bool gateZeroKeepsDrySignalUnchanged() {
  FUZZAAudioProcessor processor;
  processor.prepareToPlay(44100.0, 8);
  setBoolParameter(processor, "BYPASS", false);
  setFloatParameter(processor, "MIX", 0.0f);
  setFloatParameter(processor, "GATE", 0.0f);

  juce::AudioBuffer<float> buffer(2, 8);
  juce::MidiBuffer midi;

  for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
      buffer.setSample(channel, sample,
                       static_cast<float>((sample % 4) - 1) * 0.05f);

  juce::AudioBuffer<float> original(buffer);
  processor.processBlock(buffer, midi);

  for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
      if (!nearlyEqual(buffer.getSample(channel, sample),
                       original.getSample(channel, sample)))
        return false;

  return true;
}

bool clippingModesProduceFiniteOutput() {
  FUZZAAudioProcessor processor;
  processor.prepareToPlay(48000.0, 16);
  setBoolParameter(processor, "BYPASS", false);
  setFloatParameter(processor, "GAIN", 100.0f);
  setFloatParameter(processor, "MIX", 100.0f);
  setFloatParameter(processor, "GATE", 0.0f);

  for (int mode = 0; mode < 3; ++mode) {
    setChoiceParameter(processor, "CLIP_MODE", mode);

    juce::AudioBuffer<float> buffer(2, 16);
    juce::MidiBuffer midi;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
      for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        buffer.setSample(channel, sample, (sample % 2 == 0) ? 0.7f : -0.7f);

    processor.processBlock(buffer, midi);

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
      for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        if (!std::isfinite(buffer.getSample(channel, sample)))
          return false;
  }

  return true;
}

bool runTest(const char* name, bool (*test)()) {
  if (!test()) {
    std::cerr << "FAIL: " << name << '\n';
    return false;
  }

  std::cout << "PASS: " << name << '\n';
  return true;
}
} // namespace

int main() {
  bool passed = true;

  try {
    passed &= runTest("bypassReturnsOriginalSignal", bypassReturnsOriginalSignal);
    passed &= runTest("gateZeroKeepsDrySignalUnchanged",
                      gateZeroKeepsDrySignalUnchanged);
    passed &= runTest("clippingModesProduceFiniteOutput",
                      clippingModesProduceFiniteOutput);
  } catch (const std::exception& exception) {
    std::cerr << "FAIL: unexpected exception: " << exception.what() << '\n';
    return 1;
  }

  return passed ? 0 : 1;
}
