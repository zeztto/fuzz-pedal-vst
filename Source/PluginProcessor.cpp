// SPDX-License-Identifier: MIT OR AGPL-3.0-only
// Source edition changes Copyright (c) 2026 Sungwoon Jeon
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "StateValidation.h"
#include <cmath>

FUZZAAudioProcessor::FUZZAAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
#endif
      apvts(*this, nullptr, "Parameters", createParameterLayout()) {
  raw={apvts.getRawParameterValue("BYPASS"),apvts.getRawParameterValue("GAIN"),
       apvts.getRawParameterValue("TONE_PRESET"),apvts.getRawParameterValue("MIX"),
       apvts.getRawParameterValue("GATE"),apvts.getRawParameterValue("CLIP_MODE")};
}

FUZZAAudioProcessor::~FUZZAAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout
FUZZAAudioProcessor::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID{"BYPASS", 1}, "Bypass", false));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"GAIN", 1}, "Gain",
      juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 0.5f), 50.0f));

  // Tone Preset: 0 = Warm (800Hz), 1 = Balanced (2000Hz), 2 = Bright (4500Hz)
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID{"TONE_PRESET", 1}, "Tone Preset",
      juce::StringArray{"Warm", "Balanced", "Bright"}, 1));

  // Mix: Dry/Wet blend (0% = dry, 100% = wet)
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"MIX", 1}, "Mix",
      juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f));

  // Gate: Noise gate threshold (0% = off, higher = more aggressive)
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"GATE", 1}, "Gate",
      juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));

  // Clipping mode: 0 = Hard, 1 = Soft, 2 = Asymmetric
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID{"CLIP_MODE", 1}, "Clip Mode",
      juce::StringArray{"Hard", "Soft", "Asymmetric"}, 0));

  return layout;
}

const juce::String FUZZAAudioProcessor::getName() const {
  return JucePlugin_Name;
}

bool FUZZAAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool FUZZAAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool FUZZAAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
  return true;
#else
  return false;
#endif
}

double FUZZAAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int FUZZAAudioProcessor::getNumPrograms() {
  return 1; // NB: some hosts don't cope very well if you tell them there are 0
            // programs, so this should be at least 1, even if you're not really
            // implementing programs.
}

int FUZZAAudioProcessor::getCurrentProgram() { return 0; }

void FUZZAAudioProcessor::setCurrentProgram(int index) { juce::ignoreUnused(index); }

const juce::String FUZZAAudioProcessor::getProgramName(int index) {
  juce::ignoreUnused(index);
  return {};
}

void FUZZAAudioProcessor::changeProgramName(int index,
                                            const juce::String &newName) {
  juce::ignoreUnused(index, newName);
}

void FUZZAAudioProcessor::prepareToPlay(double sampleRate,int samplesPerBlock) {
  processingSampleRate=std::isfinite(sampleRate)&&sampleRate>=1000&&sampleRate<=768000?sampleRate:44100;
  const juce::dsp::ProcessSpec spec{processingSampleRate,static_cast<juce::uint32>(juce::jmax(1,samplesPerBlock)),1};
  toneFilterLeft.prepare(spec);toneFilterRight.prepare(spec);
  toneFilterLeft.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
  toneFilterRight.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
  inputAttack=static_cast<float>(std::exp(-1.0/(processingSampleRate*0.005)));
  inputRelease=static_cast<float>(std::exp(-1.0/(processingSampleRate*0.05)));
  outputAttack=static_cast<float>(std::exp(-1.0/(processingSampleRate*0.02)));
  outputRelease=static_cast<float>(std::exp(-1.0/(processingSampleRate*0.5)));
  reset();
}
void FUZZAAudioProcessor::reset() {
  toneFilterLeft.reset();toneFilterRight.reset();
  inputGateGainLeft=inputGateGainRight=outputGateGainLeft=outputGateGainRight=0;
}
void FUZZAAudioProcessor::releaseResources() {reset();}
juce::AudioProcessorParameter* FUZZAAudioProcessor::getBypassParameter() const {
  return apvts.getParameter("BYPASS");
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FUZZAAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
#if JucePlugin_IsMidiEffect
  juce::ignoreUnused(layouts);
  return true;
#else
  // This is the place where you check if the layout is supported.
  // In this template code we only support mono or stereo.
  // Some plugin hosts, such as certain GarageBand versions, will only
  // load plugins that support stereo bus layouts.
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;
#endif

  return true;
#endif
}
#endif

void FUZZAAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& midi) {
  juce::ScopedNoDenormals noDenormals;juce::ignoreUnused(midi);
  const int channels=juce::jmin(2,buffer.getNumChannels(),getTotalNumInputChannels());
  const int count=buffer.getNumSamples();
  if(channels<=0||count<=0)return;
  for(int channel=channels;channel<buffer.getNumChannels();++channel)buffer.clear(channel,0,count);
  auto read=[](std::atomic<float>* value,float low,float high,float fallback) noexcept {
    const float x=value->load(std::memory_order_relaxed);
    return std::isfinite(x)?juce::jlimit(low,high,x):fallback;
  };
  const bool bypass=read(raw.bypass,0,1,0)>0.5f;
  const float gain=juce::jmax(1.0f,read(raw.gain,0,100,50));
  const float makeup=2.0f/std::sqrt(gain);
  const float mix=read(raw.mix,0,100,100)/100.0f;
  const float threshold=read(raw.gate,0,100,0)*0.001f;
  const int tone=juce::roundToInt(read(raw.tone,0,2,1));
  const int clip=juce::roundToInt(read(raw.clip,0,2,0));
  const float frequencies[]{800,2000,4500};
  const float cutoff=juce::jmin(frequencies[tone],static_cast<float>(processingSampleRate*0.45));
  toneFilterLeft.setCutoffFrequency(cutoff);toneFilterRight.setCutoffFrequency(cutoff);
  for(int channel=0;channel<channels;++channel) {
    auto* data=buffer.getWritePointer(channel);
    auto& inGate=channel==0?inputGateGainLeft:inputGateGainRight;
    auto& outGate=channel==0?outputGateGainLeft:outputGateGainRight;
    auto& filter=channel==0?toneFilterLeft:toneFilterRight;
    if(threshold==0){inGate=1;outGate=1;}
    for(int sample=0;sample<count;++sample) {
      const float dry=std::isfinite(data[sample])?data[sample]:0.0f;
      if(bypass){data[sample]=dry;continue;}
      float wet=dry;
      if(threshold>0){
        const float target=std::abs(dry)>threshold?1.0f:0.0f;
        const float alpha=target>inGate?inputAttack:inputRelease;
        inGate=alpha*inGate+(1-alpha)*target;wet*=inGate;
      }
      wet*=gain;
      if(clip==1)wet=std::tanh(wet);
      else if(clip==2&&wet>0)wet=std::tanh(wet*1.5f);
      else wet=juce::jlimit(-1.0f,1.0f,wet);
      wet=filter.processSample(0,wet)*makeup;
      if(!std::isfinite(wet)){filter.reset();wet=0;}
      if(threshold>0){
        const float target=std::abs(wet)>threshold*0.5f?1.0f:0.0f;
        const float alpha=target>outGate?outputAttack:outputRelease;
        outGate=alpha*outGate+(1-alpha)*target;wet*=outGate;
      }
      // Gate only affects the wet path. Mix 0 is exact finite dry, even at Gate 100.
      data[sample]=mix==0?dry:dry*(1-mix)+wet*mix;
    }
  }
}

bool FUZZAAudioProcessor::hasEditor() const {
  return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *FUZZAAudioProcessor::createEditor() {
  return new FUZZAAudioProcessorEditor(*this);
}

void FUZZAAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {
  // You should use this method to store your parameters in the memory block.
  // You could do that either as raw data, or use the XML or ValueTree classes
  // as intermediaries
  auto state = apvts.copyState();
  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void FUZZAAudioProcessor::setStateInformation(const void* data,int sizeInBytes) {
  if(data==nullptr||sizeInBytes<=0||sizeInBytes>65536)return;
  auto xml=getXmlFromBinary(data,sizeInBytes);
  if(!xml)return;
  const auto tree=juce::ValueTree::fromXml(*xml);
  if(isValidParameterState(tree,apvts,*this))apvts.replaceState(tree);
}

// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new FUZZAAudioProcessor();
}
