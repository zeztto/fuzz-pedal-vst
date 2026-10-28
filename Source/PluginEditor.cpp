// SPDX-License-Identifier: MIT OR AGPL-3.0-only
// Source edition changes Copyright (c) 2026 Sungwoon Jeon
#include "PluginEditor.h"
#include <cmath>
namespace {juce::String utf8(const char* text){return juce::String::fromUTF8(text);}
const juce::Colour ink{0xff20232a},accent{0xffb75616},paper{0xfff8f8f5},line{0xffd8d9d5};}
FUZZAAudioProcessorEditor::FUZZAAudioProcessorEditor(FUZZAAudioProcessor& p):AudioProcessorEditor(&p),processor(p) {
 auto label=[this](juce::Label& l,const juce::String& text){l.setText(text,juce::dontSendNotification);l.setJustificationType(juce::Justification::centred);l.setColour(juce::Label::textColourId,ink);l.setFont(juce::FontOptions(16.0f));addAndMakeVisible(l);};
 auto knob=[&](juce::Slider& s,juce::Label& l,const char* id,const juce::String& title,const juce::String& description){s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,90,26);s.setColour(juce::Slider::rotarySliderFillColourId,accent);s.setColour(juce::Slider::rotarySliderOutlineColourId,line);s.setColour(juce::Slider::thumbColourId,ink);s.setColour(juce::Slider::textBoxTextColourId,ink);s.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);s.setColour(juce::Slider::textBoxOutlineColourId,line);s.setTitle(id);s.setDescription(description);s.setTooltip(description);addAndMakeVisible(s);label(l,title);};
 knob(gainSlider,gainLabel,"Gain",utf8("게인"),utf8("퍼즈 입력 게인 0부터 100"));knob(mixSlider,mixLabel,"Mix",utf8("믹스"),utf8("원음과 퍼즈 믹스. 0에서는 Gate와 무관하게 원음 통과"));knob(gateSlider,gateLabel,"Gate",utf8("게이트"),utf8("퍼즈 경로의 노이즈 게이트. 0에서는 비활성"));
 using A=juce::AudioProcessorValueTreeState::SliderAttachment;
 gainAttachment=std::make_unique<A>(p.apvts,"GAIN",gainSlider);mixAttachment=std::make_unique<A>(p.apvts,"MIX",mixSlider);gateAttachment=std::make_unique<A>(p.apvts,"GATE",gateSlider);
 auto choice=[this](juce::TextButton& b,const juce::String& text,const char* id,int value,int group){b.setButtonText(text);b.setClickingTogglesState(true);b.setRadioGroupId(group);b.setColour(juce::TextButton::buttonColourId,paper);b.setColour(juce::TextButton::buttonOnColourId,accent);b.setColour(juce::TextButton::textColourOffId,ink);b.setColour(juce::TextButton::textColourOnId,juce::Colours::white);b.onClick=[this,id,value]{auto* parameter=processor.apvts.getParameter(id);parameter->beginChangeGesture();parameter->setValueNotifyingHost(parameter->convertTo0to1(float(value)));parameter->endChangeGesture();};addAndMakeVisible(b);};
 choice(warm,utf8("따뜻하게"),"TONE_PRESET",0,1);choice(balanced,utf8("균형"),"TONE_PRESET",1,1);choice(bright,utf8("밝게"),"TONE_PRESET",2,1);choice(hard,utf8("하드"),"CLIP_MODE",0,2);choice(soft,utf8("소프트"),"CLIP_MODE",1,2);choice(asym,utf8("비대칭"),"CLIP_MODE",2,2);
 label(toneLabel,utf8("톤"));label(clipLabel,utf8("클리핑"));label(statusLabel,"");
 bypass.setButtonText(utf8("바이패스"));bypass.setTitle("Bypass");bypass.setClickingTogglesState(true);bypass.setColour(juce::TextButton::buttonColourId,ink);bypass.setColour(juce::TextButton::buttonOnColourId,accent);bypass.setColour(juce::TextButton::textColourOffId,juce::Colours::white);bypass.setColour(juce::TextButton::textColourOnId,juce::Colours::white);addAndMakeVisible(bypass);bypassAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,"BYPASS",bypass);
 legal.setButtonText(utf8("라이선스와 소스"));legal.setColour(juce::TextButton::buttonColourId,paper);legal.setColour(juce::TextButton::textColourOffId,ink);legal.onClick=[this]{juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,utf8("라이선스와 소스"),utf8("Fuzz Pedal Source\nCopyright 2026 Sungwoon Jeon\n\n이 JUCE 결합작업은 AGPL-3.0-only 조건으로 배포합니다. 사용·수정·배포 권리와 소스 제공 조건은 함께 배포한 LICENSE, THIRD_PARTY_NOTICES.md에 있습니다. 보증 없이 제공됩니다. 기존 자체 코드의 MIT 허용은 별도 notice에 보존되어 있습니다.\n\nJUCE Framework: AGPLv3 / commercial dual license\nhttps://www.gnu.org/licenses/agpl-3.0.html"),utf8("닫기"),this);};addAndMakeVisible(legal);
 setSize(540,640);timerCallback();startTimerHz(30);
}
FUZZAAudioProcessorEditor::~FUZZAAudioProcessorEditor(){stopTimer();}
void FUZZAAudioProcessorEditor::timerCallback(){
 auto choice=[this](const char* id,int fallback){const float value=processor.apvts.getRawParameterValue(id)->load();return std::isfinite(value)?juce::jlimit(0,2,juce::roundToInt(value)):fallback;};
 const int tone=choice("TONE_PRESET",1),clip=choice("CLIP_MODE",0);
 warm.setToggleState(tone==0,juce::dontSendNotification);balanced.setToggleState(tone==1,juce::dontSendNotification);bright.setToggleState(tone==2,juce::dontSendNotification);hard.setToggleState(clip==0,juce::dontSendNotification);soft.setToggleState(clip==1,juce::dontSendNotification);asym.setToggleState(clip==2,juce::dontSendNotification);
 statusLabel.setText(processor.apvts.getRawParameterValue("BYPASS")->load()>0.5f?utf8("바이패스 · 원음 통과"):utf8("이펙트 처리"),juce::dontSendNotification);
}
void FUZZAAudioProcessorEditor::paint(juce::Graphics& g){g.fillAll(paper);g.setColour(ink);g.setFont(juce::FontOptions(28.0f,juce::Font::bold));g.drawFittedText("Fuzz Pedal Source",24,20,getWidth()-48,38,juce::Justification::centred,1);g.setFont(juce::FontOptions(15.0f));g.drawFittedText(utf8("퍼즈와 원음의 균형"),24,60,getWidth()-48,24,juce::Justification::centred,1);}
void FUZZAAudioProcessorEditor::resized(){
 auto b=getLocalBounds().reduced(24);b.removeFromTop(80);
 auto row=[&](juce::Label& label,juce::TextButton& a,juce::TextButton& c,juce::TextButton& d){auto area=b.removeFromTop(64);label.setBounds(area.removeFromTop(24));const int w=(area.getWidth()-16)/3;a.setBounds(area.removeFromLeft(w));area.removeFromLeft(8);c.setBounds(area.removeFromLeft(w));area.removeFromLeft(8);d.setBounds(area);};
 row(toneLabel,warm,balanced,bright);b.removeFromTop(12);row(clipLabel,hard,soft,asym);b.removeFromTop(16);
 auto controls=b.removeFromTop(190);const int width=controls.getWidth()/3;
 auto place=[&](juce::Slider& slider,juce::Label& label,juce::Rectangle<int> area){label.setBounds(area.removeFromTop(26));slider.setBounds(area.reduced(8,0));};
 place(gainSlider,gainLabel,controls.removeFromLeft(width));place(mixSlider,mixLabel,controls.removeFromLeft(width));place(gateSlider,gateLabel,controls);b.removeFromTop(16);bypass.setBounds(b.removeFromTop(54).withSizeKeepingCentre(180,48));statusLabel.setBounds(b.removeFromTop(28));legal.setBounds(b.removeFromBottom(28).withSizeKeepingCentre(160,26));
}
