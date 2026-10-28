cmake_minimum_required(VERSION 3.22)
project(FuzzPedalSource VERSION 1.0.0 LANGUAGES C CXX)
# Source edition safety: build never installs plugins or scans host plugin folders.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)
# JUCE 8.0.16-security; immutable official commit and archive digest.
FetchContent_Declare(juce
  URL https://codeload.github.com/juce-framework/JUCE/tar.gz/ea77444750de45e036e0279bb0291144bf8e3688
  URL_HASH SHA256=79181e00e8511c837983478f561db082ed08028d60c53a0af8ac85ea220299da
  DOWNLOAD_EXTRACT_TIMESTAMP FALSE)
FetchContent_MakeAvailable(juce)
set(source_formats VST3 Standalone)
if(APPLE)
  list(APPEND source_formats AU)
endif()
juce_add_plugin(FuzzPedalSource
  COMPANY_NAME "Sungwoon Jeon"
  BUNDLE_ID "dev.zeztto.fuzzpedal.source"
  PLUGIN_MANUFACTURER_CODE Ztso
  PLUGIN_CODE Fpse
  PRODUCT_NAME "Fuzz Pedal Source"
  IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  MICROPHONE_PERMISSION_ENABLED TRUE
  MICROPHONE_PERMISSION_TEXT "오디오 입력에 이펙트를 적용하려면 입력 접근이 필요합니다."
  COPY_PLUGIN_AFTER_BUILD FALSE
  FORMATS ${source_formats})
target_sources(FuzzPedalSource PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_definitions(FuzzPedalSource PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(FuzzPedalSource PRIVATE juce::juce_audio_utils juce::juce_dsp PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
juce_generate_juce_header(FuzzPedalSource)
include(CTest)

if(BUILD_TESTING)
  add_executable(LocaleStateTests tests/LocaleStateTests.cpp)
  target_compile_definitions(LocaleStateTests PRIVATE TEST_FUZZ=1)
  target_include_directories(LocaleStateTests PRIVATE Source)
  target_link_libraries(LocaleStateTests PRIVATE FuzzPedalSource juce::juce_audio_utils juce::juce_dsp)
  add_test(NAME locale-state-C COMMAND LocaleStateTests C)
  add_test(NAME locale-state-comma COMMAND LocaleStateTests de_DE.UTF-8)
  juce_add_gui_app(EditorHarness PRODUCT_NAME "Fuzz Pedal Source Editor QA" BUNDLE_ID "dev.zeztto.fuzzpedal.source.editorqa")
  target_sources(EditorHarness PRIVATE tests/EditorHarness.cpp)
  target_compile_definitions(EditorHarness PRIVATE TEST_FUZZ=1)
  target_include_directories(EditorHarness PRIVATE Source)
  target_link_libraries(EditorHarness PRIVATE FuzzPedalSource juce::juce_audio_utils juce::juce_dsp)
  add_executable(OriginalProcessorSmokeTests tests/ProcessorSmokeTest.cpp)
  target_link_libraries(OriginalProcessorSmokeTests PRIVATE FuzzPedalSource juce::juce_audio_utils juce::juce_dsp)
  target_include_directories(OriginalProcessorSmokeTests PRIVATE Source)
  add_test(NAME original-three-smoke COMMAND OriginalProcessorSmokeTests)
  add_executable(NativeVst3Host tests/NativeVst3Host.cpp)
  target_compile_definitions(NativeVst3Host PRIVATE JUCE_PLUGINHOST_VST3=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
  target_link_libraries(NativeVst3Host PRIVATE juce::juce_audio_utils)
  add_dependencies(NativeVst3Host FuzzPedalSource_VST3)
  if(APPLE)
    add_test(NAME actual-local-vst3 COMMAND NativeVst3Host "$<TARGET_BUNDLE_DIR:FuzzPedalSource_VST3>")
    add_executable(NativeAuHost tests/NativeAuHost.cpp)
    target_link_libraries(NativeAuHost PRIVATE "-framework AudioUnit" "-framework AudioToolbox" "-framework CoreFoundation")
    add_dependencies(NativeAuHost FuzzPedalSource_AU)
    add_test(NAME actual-local-au COMMAND NativeAuHost "$<TARGET_BUNDLE_DIR:FuzzPedalSource_AU>")
  endif()
  add_library(RenderAllocationProbe SHARED tests/AllocationProbe.cpp)
  set_target_properties(RenderAllocationProbe PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test-only")
  add_executable(DspRegressionTests tests/DspRegressionTests.cpp)
  target_compile_definitions(DspRegressionTests PRIVATE TEST_FUZZ=1)
  target_include_directories(DspRegressionTests PRIVATE Source "${CMAKE_BINARY_DIR}/FuzzPedalSource_artefacts/JuceLibraryCode")
  target_link_libraries(DspRegressionTests PRIVATE RenderAllocationProbe FuzzPedalSource juce::juce_audio_utils juce::juce_dsp)
  add_test(NAME dsp-regressions COMMAND DspRegressionTests "${CMAKE_BINARY_DIR}/test-artifacts")
endif()
