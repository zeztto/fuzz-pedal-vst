// SPDX-License-Identifier: MIT OR AGPL-3.0-only
// Source edition changes Copyright (c) 2026 Sungwoon Jeon
#pragma once
#include "PluginProcessor.h"
class FUZZAAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
 explicit FUZZAAudioProcessorEditor(FUZZAAudioProcessor&);
 ~FUZZAAudioProcessorEditor() override;
 void paint(juce::Graphics&) override;
 void resized() override;
private:
 FUZZAAudioProcessor& processor;
 juce::Slider gainSlider,mixSlider,gateSlider;
 juce::Label gainLabel,mixLabel,gateLabel,toneLabel,clipLabel,statusLabel;
 juce::TextButton warm,balanced,bright,hard,soft,asym,bypass,legal;
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment,mixAttachment,gateAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
 juce::TooltipWindow tooltips{this,700};
 void timerCallback() override;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FUZZAAudioProcessorEditor)
};
