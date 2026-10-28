#include <AudioUnit/AudioUnit.h>
#include <CoreFoundation/CoreFoundation.h>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>
static void check(bool value,const char* error){if(!value)throw std::runtime_error(error);}
static void ok(OSStatus status,const char* operation){if(status!=noErr)throw std::runtime_error(std::string(operation)+" status="+std::to_string(status));}
static OSType fourcc(CFStringRef text){char value[8]{};check(text&&CFStringGetCString(text,value,sizeof(value),kCFStringEncodingASCII)&&CFStringGetLength(text)==4,"Invalid local AU code");return (uint32_t(uint8_t(value[0]))<<24)|(uint32_t(uint8_t(value[1]))<<16)|(uint32_t(uint8_t(value[2]))<<8)|uint8_t(value[3]);}
static OSStatus input(void*,AudioUnitRenderActionFlags*,const AudioTimeStamp* timestamp,UInt32,UInt32 frames,AudioBufferList* buffers){
 for(UInt32 c=0;c<buffers->mNumberBuffers;++c){auto* samples=static_cast<float*>(buffers->mBuffers[c].mData);for(UInt32 i=0;i<frames;++i)samples[i]=0.1f*std::sin(float(timestamp->mSampleTime+i)*0.07f);}return noErr;
}
int main(int argc,char**argv){
 AudioComponentInstance unit=nullptr;CFBundleRef bundle=nullptr;CFURLRef url=nullptr;
 try{
  check(argc==2,"Expected one exact local .component bundle path");
  url=CFURLCreateFromFileSystemRepresentation(nullptr,reinterpret_cast<const UInt8*>(argv[1]),std::char_traits<char>::length(argv[1]),true);bundle=CFBundleCreate(nullptr,url);check(bundle&&CFBundleLoadExecutable(bundle),"Could not load local AU executable");
  auto array=static_cast<CFArrayRef>(CFBundleGetValueForInfoDictionaryKey(bundle,CFSTR("AudioComponents")));check(array&&CFArrayGetCount(array)==1,"Local AU metadata missing");
  auto info=static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(array,0));auto get=[&](CFStringRef key){return static_cast<CFStringRef>(CFDictionaryGetValue(info,key));};
  AudioComponentDescription desc{};desc.componentType=fourcc(get(CFSTR("type")));desc.componentSubType=fourcc(get(CFSTR("subtype")));desc.componentManufacturer=fourcc(get(CFSTR("manufacturer")));
  auto factory=reinterpret_cast<AudioComponentFactoryFunction>(CFBundleGetFunctionPointerForName(bundle,get(CFSTR("factoryFunction"))));check(factory,"Local AU factory missing");
  // Register this exact in-memory factory in this test process. Never enumerate
  // AudioComponentFindNext, run auval or copy/install a component globally.
  auto component=AudioComponentRegister(&desc,CFSTR("Source Edition Offline QA"),0x10000,factory);check(component,"Process-local AU register failed");ok(AudioComponentInstanceNew(component,&unit),"instance");
  AudioStreamBasicDescription stream{};stream.mSampleRate=48000;stream.mFormatID=kAudioFormatLinearPCM;stream.mFormatFlags=kAudioFormatFlagsNativeFloatPacked|kAudioFormatFlagIsNonInterleaved;stream.mBytesPerPacket=4;stream.mFramesPerPacket=1;stream.mBytesPerFrame=4;stream.mChannelsPerFrame=2;stream.mBitsPerChannel=32;
  ok(AudioUnitSetProperty(unit,kAudioUnitProperty_StreamFormat,kAudioUnitScope_Input,0,&stream,sizeof(stream)),"input format");ok(AudioUnitSetProperty(unit,kAudioUnitProperty_StreamFormat,kAudioUnitScope_Output,0,&stream,sizeof(stream)),"output format");
  UInt32 maximum=256;ok(AudioUnitSetProperty(unit,kAudioUnitProperty_MaximumFramesPerSlice,kAudioUnitScope_Global,0,&maximum,sizeof(maximum)),"max frames");AURenderCallbackStruct callback{input,nullptr};ok(AudioUnitSetProperty(unit,kAudioUnitProperty_SetRenderCallback,kAudioUnitScope_Input,0,&callback,sizeof(callback)),"input callback");ok(AudioUnitInitialize(unit),"initialize");
  UInt32 size=0;Boolean writable=false;ok(AudioUnitGetPropertyInfo(unit,kAudioUnitProperty_ParameterList,kAudioUnitScope_Global,0,&size,&writable),"parameter list size");check(size>=2*sizeof(AudioUnitParameterID),"AU parameters missing");
  std::vector<AudioUnitParameterID> parameters(size/sizeof(AudioUnitParameterID));ok(AudioUnitGetProperty(unit,kAudioUnitProperty_ParameterList,kAudioUnitScope_Global,0,parameters.data(),&size),"parameter list");
  struct StereoBuffers {UInt32 count;AudioBuffer channels[2];} buffers{};buffers.count=2;float left[256]{},right[256]{};buffers.channels[0]={1,sizeof(left),left};buffers.channels[1]={1,sizeof(right),right};AudioTimeStamp timestamp{};timestamp.mFlags=kAudioTimeStampSampleTimeValid;AudioUnitRenderActionFlags flags=0;
  ok(AudioUnitRender(unit,&flags,&timestamp,0,256,reinterpret_cast<AudioBufferList*>(&buffers)),"render");for(float value:left)check(std::isfinite(value),"AU left non-finite");for(float value:right)check(std::isfinite(value),"AU right non-finite");
  UInt32 bypass=1;ok(AudioUnitSetProperty(unit,kAudioUnitProperty_BypassEffect,kAudioUnitScope_Global,0,&bypass,sizeof(bypass)),"bypass");timestamp.mSampleTime=256;ok(AudioUnitRender(unit,&flags,&timestamp,0,256,reinterpret_cast<AudioBufferList*>(&buffers)),"bypass render");for(int i=0;i<256;++i)check(std::abs(left[i]-0.1f*std::sin(float(256+i)*0.07f))<1.0e-6f,"AU bypass changed dry");
  CFPropertyListRef state=nullptr;size=sizeof(state);ok(AudioUnitGetProperty(unit,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&state,&size),"state get");check(state,"AU state empty");ok(AudioUnitSetProperty(unit,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&state,sizeof(state)),"state restore");CFRelease(state);
  ok(AudioUnitUninitialize(unit),"uninitialize");AudioComponentInstanceDispose(unit);unit=nullptr;CFRelease(bundle);CFRelease(url);std::cout<<"PASS actual-local-AU parameters="<<parameters.size()<<" finite/bypass/state; processLocalFactoryOnly=1 audioDevicesOpened=0 globalScan=0\n";return 0;
 }catch(const std::exception&e){if(unit)AudioComponentInstanceDispose(unit);if(bundle)CFRelease(bundle);if(url)CFRelease(url);std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}
}
