#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>
#if TEST_FUZZ
using Processor=FUZZAAudioProcessor;
#else
using Processor=DUCKAVERBAudioProcessor;
#endif
// Test-only native editor container. No AudioDeviceManager, audio I/O, MIDI
// device, microphone permission, plugin enumeration or user-profile lookup.
class EditorHarness final:public juce::JUCEApplication,private juce::Timer {
 struct Window final:juce::DocumentWindow {
  explicit Window(Processor& p):DocumentWindow(p.getName()+" - Editor QA",juce::Colours::white,allButtons){setUsingNativeTitleBar(true);setContentOwned(p.createEditor(),true);centreWithSize(getWidth(),getHeight());setVisible(true);}
  void closeButtonPressed() override {juce::JUCEApplication::getInstance()->systemRequestedQuit();}
 };
 std::unique_ptr<Processor> processor;
 std::unique_ptr<Window> window;
 juce::File output;
 void timerCallback() override {
  auto values=std::make_unique<juce::DynamicObject>();
  for(auto* parameter:processor->getParameters())if(auto* id=dynamic_cast<juce::AudioProcessorParameterWithID*>(parameter))values->setProperty(id->paramID,processor->apvts.getRawParameterValue(id->paramID)->load());
  auto state=std::make_unique<juce::DynamicObject>();state->setProperty("testOnly",true);state->setProperty("audioHardwareOpened",false);state->setProperty("parameters",juce::var(values.release()));
  output.getChildFile("parameters.json").replaceWithText(juce::JSON::toString(juce::var(state.release())));
 }
public:
 const juce::String getApplicationName() override {return "Source Edition Editor QA";}
 const juce::String getApplicationVersion() override {return "1.0.0";}
 bool moreThanOneInstanceAllowed() override {return true;}
 void initialise(const juce::String& command) override {
  const auto args=juce::StringArray::fromTokens(command,true);
  juce::String root;for(const auto& arg:args)if(arg.startsWith("--output-dir="))root=arg.fromFirstOccurrenceOf("=",false,false).unquoted();
  if(!juce::File::isAbsolutePath(root)){setApplicationReturnValue(2);quit();return;}
  output=juce::File(root);if(output.createDirectory().failed()){setApplicationReturnValue(3);quit();return;}
  processor=std::make_unique<Processor>();
  const auto saved=output.getChildFile("state.bin");
  if(saved.existsAsFile()){juce::MemoryBlock data;if(saved.loadFileAsData(data))processor->setStateInformation(data.getData(),int(data.getSize()));}
  window=std::make_unique<Window>(*processor);timerCallback();startTimer(200);
 }
 void shutdown() override {
  stopTimer();window.reset();
  if(processor){juce::MemoryBlock data;processor->getStateInformation(data);output.getChildFile("state.bin").replaceWithData(data.getData(),data.getSize());processor.reset();}
 }
 void systemRequestedQuit() override {quit();}
 void anotherInstanceStarted(const juce::String&) override {}
};
START_JUCE_APPLICATION(EditorHarness)
