#include "PluginProcessor.h"
#include <clocale>
#include <cmath>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#if TEST_FUZZ
using Processor=FUZZAAudioProcessor;
constexpr const char* decimalParameter="MIX";
constexpr const char* bypassParameter="BYPASS";
#else
using Processor=DUCKAVERBAudioProcessor;
constexpr const char* decimalParameter="space";
constexpr const char* bypassParameter="bypass";
#endif
using Values=std::map<std::string,float>;
static void require(bool value,const char* error){if(!value)throw std::runtime_error(error);}
static void set(Processor& p,const char* id,float value){auto* parameter=p.apvts.getParameter(id);require(parameter!=nullptr,"parameter missing");parameter->setValueNotifyingHost(parameter->convertTo0to1(value));}
static Values values(Processor& p){Values out;for(auto* parameter:p.getParameters())if(auto* id=dynamic_cast<juce::AudioProcessorParameterWithID*>(parameter))out[id->paramID.toStdString()]=p.apvts.getRawParameterValue(id->paramID)->load();return out;}
static bool same(const Values& a,const Values& b){if(a.size()!=b.size())return false;for(const auto& entry:a){auto found=b.find(entry.first);if(found==b.end()||!std::isfinite(found->second)||std::abs(entry.second-found->second)>1.0e-5f)return false;}return true;}
static void desired(Processor& p){
#if TEST_FUZZ
 set(p,"GAIN",75.5f);set(p,"MIX",37.5f);set(p,"GATE",62.5f);set(p,"TONE_PRESET",2);set(p,"CLIP_MODE",1);
#else
 set(p,"space",0.75f);
#endif
 set(p,bypassParameter,1);
}
static void zero(Processor& p){for(auto* parameter:p.getParameters())parameter->setValueNotifyingHost(0);p.reset();}
int main(int argc,char** argv){
 juce::ScopedJuceInitialiser_GUI gui;
 const std::string requested=argc>1?argv[1]:"C";
 // This executable is always a separate test subprocess. Production code must
 // never call setlocale or alter any host/global locale.
 const char* previousPointer=std::setlocale(LC_NUMERIC,nullptr);
 const std::string previous=previousPointer?previousPointer:"C";
 if(std::setlocale(LC_NUMERIC,requested.c_str())==nullptr){std::cerr<<"FAIL requested subprocess locale unavailable: "<<requested<<"\n";return 2;}
 const std::string activeLocale=std::setlocale(LC_NUMERIC,nullptr);
 int failures=0;
 try{
  Processor p;p.setRateAndBufferSizeDetails(48000,128);p.prepareToPlay(48000,128);desired(p);
  const auto expected=values(p);juce::MemoryBlock saved;p.getStateInformation(saved);
  auto xml=juce::AudioProcessor::getXmlFromBinary(saved.getData(),int(saved.getSize()));require(xml!=nullptr,"actual processor save XML missing");
  const auto valid=juce::ValueTree::fromXml(*xml);require(valid.isValid(),"actual state tree invalid");
  auto run=[&](const std::string& name,auto test){try{test();std::cout<<"PASS "<<name<<" locale="<<requested<<"\n";}catch(const std::exception& error){++failures;std::cerr<<"FAIL "<<name<<" locale="<<requested<<": "<<error.what()<<"\n";}};
  std::cout<<"PROCESS_LOCAL_NUMERIC locale="<<requested<<" decimal="<<std::localeconv()->decimal_point<<" savedDecimal="<<expected.at(decimalParameter)<<"\n";
  run("actual-save-reset-restore-dotted-decimal",[&]{zero(p);require(!same(expected,values(p)),"reset did not change controls before restore");p.setStateInformation(saved.getData(),int(saved.getSize()));const auto actual=values(p);std::cout<<"RESTORE_READBACK expected="<<expected.at(decimalParameter)<<" actual="<<actual.at(decimalParameter)<<"\n";require(same(expected,actual),"actual processor rejected/changed its valid dotted-decimal state");});
  auto apply=[&](const juce::ValueTree& tree){juce::MemoryBlock data;juce::AudioProcessor::copyXmlToBinary(*tree.createXml(),data);p.setStateInformation(data.getData(),int(data.getSize()));};
  run("actual-state-exponent-roundtrip",[&]{auto tree=valid.createCopy();tree.getChildWithProperty("id",decimalParameter).setProperty("value",
#if TEST_FUZZ
   "3.75e1"
#else
   "7.5e-1"
#endif
   ,nullptr);zero(p);apply(tree);require(same(expected,values(p)),"valid complete exponent state did not restore");});
  int rejected=0;
  auto reject=[&](juce::ValueTree tree){
   // Every invalid candidate also carries a valid live-value change. If a
   // malformed payload is mistakenly applied but its bad value rounds to the
   // old value, this sentinel still makes the incorrect acceptance observable.
   for(const char* id:{decimalParameter,bypassParameter}){
    auto child=tree.getChildWithProperty("id",id);
    const auto original=valid.getChildWithProperty("id",id);
    if(child.isValid()&&child["value"].toString()==original["value"].toString())child.setProperty("value",0,nullptr);
   }
   desired(p);const auto before=values(p);apply(tree);require(same(before,values(p)),"invalid whole state altered valid controls");++rejected;
  };
  run("invalid-suffix-nan-inf-range-duplicate-preserved",[&]{
   for(const char* text:{"0.75suffix","0,75","NaN","Inf","-Infinity","","+","-",".","e1","1e","1e999999999999999999999999999999"}){auto tree=valid.createCopy();tree.getChildWithProperty("id",decimalParameter).setProperty("value",text,nullptr);reject(tree);}
   {auto tree=valid.createCopy();tree.getChildWithProperty("id",decimalParameter).setProperty("value",1000,nullptr);reject(tree);}
   {auto tree=valid.createCopy();tree.getChildWithProperty("id",bypassParameter).setProperty("value",0.5,nullptr);reject(tree);}
   {auto tree=valid.createCopy();tree.addChild(tree.getChild(0).createCopy(),-1,nullptr);reject(tree);}
   {auto tree=valid.createCopy();tree.removeChild(0,nullptr);reject(tree);}
   {auto tree=valid.createCopy();tree.getChild(0).setProperty("id","UNKNOWN",nullptr);reject(tree);}
#if TEST_FUZZ
   {auto tree=valid.createCopy();tree.getChildWithProperty("id","TONE_PRESET").setProperty("value",0.5,nullptr);reject(tree);}
#endif
   std::cout<<"INVALID_STATE_READBACK rejected="<<rejected<<" allLiveValuesPreserved=1\n";
  });
  run("host-numeric-locale-unchanged",[&]{const char* current=std::setlocale(LC_NUMERIC,nullptr);require(current&&activeLocale==current,"production state parser changed host numeric locale");});
  p.releaseResources();
 }catch(const std::exception& error){++failures;std::cerr<<"FAIL setup: "<<error.what()<<"\n";}
 std::setlocale(LC_NUMERIC,previous.c_str());return failures?1:0;
}
