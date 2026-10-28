#include "PluginProcessor.h"
#include "AllocationProbe.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <atomic>
#include <limits>

#if TEST_FUZZ
using Processor = FUZZAAudioProcessor;
constexpr auto mixId = "MIX";
constexpr auto bypassId = "BYPASS";
#else
using Processor = DUCKAVERBAudioProcessor;
constexpr auto mixId = "space";
constexpr auto bypassId = "bypass";
#endif
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void param(Processor& p, const char* id, float value) {
  auto* v=p.apvts.getParameter(id); require(v!=nullptr,"parameter missing");
  v->setValueNotifyingHost(v->convertTo0to1(value));
}
void setup(Processor& p,double sr,int channels,int block=128) {
  auto layout=p.getBusesLayout();
  layout.inputBuses.set(0,juce::AudioChannelSet::canonicalChannelSet(channels));
  layout.outputBuses.set(0,juce::AudioChannelSet::canonicalChannelSet(channels));
  require(p.setBusesLayout(layout),"mono/stereo layout rejected");
  p.setRateAndBufferSizeDetails(sr,block);p.prepareToPlay(sr,block);
}
void finite(const juce::AudioBuffer<float>& b) {
  for(int c=0;c<b.getNumChannels();++c)for(int i=0;i<b.getNumSamples();++i)
    require(std::isfinite(b.getSample(c,i)),"non-finite output");
}
void dryGateAndBypass() {
 for(double sr:{8000.0,44100.0,48000.0,96000.0})for(int channels:{1,2}) {
  Processor p;setup(p,sr,channels);param(p,mixId,0);
  #if TEST_FUZZ
  param(p,"GATE",100);
  #endif
  juce::AudioBuffer<float>b(channels,257);juce::MidiBuffer midi;
  for(int c=0;c<channels;++c)for(int i=0;i<257;++i)b.setSample(c,i,0.01f*std::sin(float(i)*0.1f));
  juce::AudioBuffer<float>dry(b);p.processBlock(b,midi);
  for(int c=0;c<channels;++c)for(int i=0;i<257;++i)require(b.getSample(c,i)==dry.getSample(c,i),"Mix0 altered dry when gate active");
  param(p,bypassId,1);param(p,mixId,1);p.processBlock(b,midi);
  for(int c=0;c<channels;++c)for(int i=0;i<257;++i)require(b.getSample(c,i)==dry.getSample(c,i),"bypass altered finite input");
 }
}
void invalidStatePreserved() {
 Processor p;param(p,mixId,0.25f);juce::MemoryBlock valid;p.getStateInformation(valid);
 const float savedValue=p.apvts.getRawParameterValue(mixId)->load();
 Processor restored;restored.setStateInformation(valid.getData(),int(valid.getSize()));
 require(std::abs(restored.apvts.getRawParameterValue(mixId)->load()-savedValue)<0.001f,"state roundtrip failed");
 const auto before=p.apvts.getRawParameterValue(mixId)->load();
 p.setStateInformation(nullptr,0);const char bad[]="not-a-state";p.setStateInformation(bad,sizeof(bad));
 auto tree=p.apvts.copyState();tree.getChildWithProperty("id",mixId).setProperty("value","NaN",nullptr);
 juce::MemoryBlock malformed;juce::AudioProcessor::copyXmlToBinary(*tree.createXml(),malformed);
 p.setStateInformation(malformed.getData(),int(malformed.getSize()));
 require(p.apvts.getRawParameterValue(mixId)->load()==before,"invalid state changed valid controls");
 for(auto* v:p.getParameters())require(std::isfinite(v->getValue()),"invalid state introduced NaN");
}
void finiteResetAutomation() {
 for(double sr:{8000.0,44100.0,48000.0,96000.0})for(int channels:{1,2}) {
  Processor p;setup(p,sr,channels,32);juce::MidiBuffer midi;
  juce::AudioBuffer<float>empty(0,0);p.processBlock(empty,midi);
  std::atomic<bool>stop{false};std::thread host([&]{int i=0;while(!stop.load()){param(p,mixId,float(i++%101)/100.0f);}});
  bool ok=true;
  try{for(int block=0;block<64;++block){juce::AudioBuffer<float>b(channels,259);for(int c=0;c<channels;++c)for(int i=0;i<259;++i)b.setSample(c,i,i==0?std::numeric_limits<float>::quiet_NaN():0.2f*std::sin(float(i)*0.03f));p.processBlock(b,midi);finite(b);}}catch(...){ok=false;}
  stop=true;host.join();require(ok,"NaN/oversized block/automation failed");p.reset();
  juce::AudioBuffer<float>silence(channels,512);silence.clear();p.processBlock(silence,midi);finite(silence);
  require(silence.getMagnitude(0,512)<1.0e-7f,"reset retained DSP history");
 }
}
void renderThreadAllocations() {
 #if __APPLE__
 allocationProbe::start();void* volatile calibration=std::malloc(64);std::free(calibration);const auto calibrationCalls=allocationProbe::stop();require(calibrationCalls>0,"allocator probe did not calibrate");
 Processor p;setup(p,48000,2);juce::AudioBuffer<float>b(2,128);b.clear();juce::MidiBuffer midi;
 p.processBlock(b,midi);
 allocationProbe::start();for(int i=0;i<1000;++i)p.processBlock(b,midi);const auto calls=allocationProbe::stop();
 std::cout<<"RENDER_ALLOCATIONS "<<calls<<" blocks=1000\n";require(calls==0,"processBlock allocated on render thread");
 #endif
}
void toneAndClipMatrix() {
 #if TEST_FUZZ
 for(double sr:{8000.0,44100.0,48000.0,96000.0})for(int channels:{1,2})for(int tone=0;tone<3;++tone)for(int clip=0;clip<3;++clip){
 Processor p;setup(p,sr,channels);param(p,"GAIN",100);param(p,"MIX",100);param(p,"TONE_PRESET",float(tone));param(p,"CLIP_MODE",float(clip));
 juce::AudioBuffer<float>b(channels,128);juce::MidiBuffer midi;for(int c=0;c<channels;++c)for(int i=0;i<128;++i)b.setSample(c,i,i%2?0.75f:-0.75f);p.processBlock(b,midi);finite(b);}
 #endif
}
void nativeUtf8ComponentStrings() {
 Processor p;std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());juce::StringArray texts;
 auto collect=[&](auto&& self,juce::Component& component)->void {
 if(auto* l=dynamic_cast<juce::Label*>(&component))texts.add(l->getText());
 if(auto* b=dynamic_cast<juce::TextButton*>(&component))texts.add(b->getButtonText());
 for(auto* child:component.getChildren())self(self,*child);
 };collect(collect,*editor);
 #if TEST_FUZZ
 for(const char* text:{"톤","클리핑","게인","믹스","게이트","바이패스","라이선스와 소스"})
 #else
 for(const char* text:{"공간","바이패스","라이선스와 소스"})
 #endif
 require(texts.contains(juce::String::fromUTF8(text)),"Native component Korean text is not expected UTF-8");
 std::cout<<"UTF8_COMPONENT_MODEL exact Korean strings; actual native pixel/AX QA is separate\n";
}
void wavReadback(const juce::File& out) {
 out.createDirectory();Processor p;setup(p,48000,2);param(p,mixId,
 #if TEST_FUZZ
 100
 #else
 0.8f
 #endif
 );juce::MidiBuffer midi;juce::AudioBuffer<float>wav(2,48000);wav.clear();
 for(int offset=0;offset<48000;offset+=128){int count=std::min(128,48000-offset);juce::AudioBuffer<float>b(2,count);for(int c=0;c<2;++c)for(int i=0;i<count;++i)b.setSample(c,i,0.15f*std::sin(float(offset+i)*float(2*juce::MathConstants<double>::pi*220/48000)));p.processBlock(b,midi);finite(b);for(int c=0;c<2;++c)wav.copyFrom(c,offset,b,c,0,count);}
 juce::WavAudioFormat format;auto file=out.getChildFile("processor-test-tone.wav");file.deleteFile();
 auto stream=file.createOutputStream();require(stream!=nullptr,"WAV stream failed");
 std::unique_ptr<juce::AudioFormatWriter> writer(format.createWriterFor(stream.release(),48000,2,24,{},0));require(writer!=nullptr,"WAV writer failed");require(writer->writeFromAudioSampleBuffer(wav,0,48000),"WAV write failed");writer.reset();
 std::unique_ptr<juce::AudioFormatReader>reader(format.createReaderFor(file.createInputStream().release(),true));require(reader&&reader->lengthInSamples==48000&&reader->numChannels==2&&reader->sampleRate==48000,"WAV metadata readback failed");
 juce::AudioBuffer<float>readback(2,48000);reader->read(&readback,0,48000,0,true,true);finite(readback);require(readback.getMagnitude(0,48000)>0.001f,"WAV silent");
 for(int c=0;c<2;++c)for(int i=0;i<48000;++i)require(std::abs(readback.getSample(c,i)-wav.getSample(c,i))<1.0e-5f,"WAV samples differ");
 std::cout<<"WAV_READBACK samples=48000 channels=2 rate=48000 peak="<<readback.getMagnitude(0,48000)<<"\n";
}
int main(int argc,char**argv) {
 juce::ScopedJuceInitialiser_GUI init;int failed=0;
 auto run=[&](const char*name,auto fn){try{fn();std::cout<<"PASS "<<name<<"\n";}catch(const std::exception&e){++failed;std::cerr<<"FAIL "<<name<<": "<<e.what()<<"\n";}};
 run("dry-gate-and-bypass",dryGateAndBypass);run("invalid-state-preserved",invalidStatePreserved);if(argc<3 || std::string(argv[2])!="red-safe") run("finite-reset-automation",finiteResetAutomation);
 #if !TEST_FUZZ
 run("nonzero-tail",[]{Processor p;require(p.getTailLengthSeconds()>=8.0,"reverb reports no conservative tail");});
 #endif
 run("render-thread-allocations",renderThreadAllocations);run("tone-clip-matrix",toneAndClipMatrix);
 run("native-utf8-component-model",nativeUtf8ComponentStrings);
 run("wav-readback",[&]{wavReadback(juce::File(argc>1?argv[1]:"test-artifacts"));});return failed?1:0;
}
