#include <juce_audio_utils/juce_audio_utils.h>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void check(bool v,const char* error){if(!v)throw std::runtime_error(error);}
int main(int argc,char**argv){
 juce::ScopedJuceInitialiser_GUI init;
 try{
  check(argc==2,"Expected one explicit local VST3 path");
  juce::File file(argv[1]);check(file.exists(),"Local VST3 missing");
  // Explicit file only. No searchPathsForPlugins, KnownPluginList scan or
  // AudioDeviceManager is constructed by this offline host.
  juce::VST3PluginFormat format;juce::OwnedArray<juce::PluginDescription> types;
  format.findAllTypesForFile(types,file.getFullPathName());check(types.size()==1,"Expected one effect from exact local bundle");
  juce::String error;auto plugin=format.createInstanceFromDescription(*types[0],48000,256,error);
  if(!plugin)throw std::runtime_error(error.toStdString());
  plugin->setPlayConfigDetails(2,2,48000,256);plugin->prepareToPlay(48000,256);
  juce::MidiBuffer midi;juce::AudioBuffer<float> b(2,256);
  auto fill=[&]{for(int c=0;c<2;++c)for(int i=0;i<256;++i)b.setSample(c,i,0.1f*std::sin(float(i)*0.07f));};fill();plugin->processBlock(b,midi);
  for(int c=0;c<2;++c)for(int i=0;i<256;++i)check(std::isfinite(b.getSample(c,i)),"Non-finite VST3 output");
  juce::AudioProcessorParameter* bypass=nullptr;for(auto* p:plugin->getParameters())if(p->getName(128).containsIgnoreCase("bypass"))bypass=p;
  check(bypass!=nullptr,"Actual host bypass parameter missing");bypass->setValueNotifyingHost(1);
  fill();juce::AudioBuffer<float> dry(b);plugin->processBlock(b,midi);
  for(int c=0;c<2;++c)for(int i=0;i<256;++i)check(std::abs(b.getSample(c,i)-dry.getSample(c,i))<1.0e-6f,"Native VST3 bypass changed dry");
  juce::MemoryBlock state;plugin->getStateInformation(state);check(state.getSize()>0,"Native VST3 state empty");plugin->setStateInformation(state.getData(),int(state.getSize()));
  const char invalid[]="invalid-state";plugin->setStateInformation(invalid,sizeof(invalid));check(std::isfinite(bypass->getValue()),"Malformed native state introduced NaN");
  plugin->releaseResources();std::cout<<"PASS actual-local-vst3 parameters="<<plugin->getParameters().size()<<" finite/bypass/state; audioDevicesOpened=0 pluginDirectoriesScanned=0\n";return 0;
 }catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}
}
