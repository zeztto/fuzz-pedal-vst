// SPDX-License-Identifier: MIT OR AGPL-3.0-only
// Source edition changes Copyright (c) 2026 Sungwoon Jeon
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>
#include <sstream>
#include <locale>
#include <set>

// State restore runs outside processBlock. Reject the complete incoming state
// before replacing any live APVTS values, including unknown/duplicate controls.
inline bool isValidParameterState(const juce::ValueTree& tree,
                                  juce::AudioProcessorValueTreeState& apvts,
                                  juce::AudioProcessor& processor) {
  if (!tree.isValid() || tree.getType()!=apvts.state.getType()
      || tree.getNumProperties()!=0
      || tree.getNumChildren()!=processor.getParameters().size()) return false;
  std::set<juce::String> seen;
  for (const auto& child:tree) {
    if (!child.hasType("PARAM") || child.getNumChildren()!=0
        || child.getNumProperties()!=2 || !child.hasProperty("id")
        || !child.hasProperty("value")) return false;
    const auto id=child["id"].toString();
    auto* parameter=apvts.getParameter(id);
    if (!parameter || !seen.insert(id).second) return false;
    auto text=child["value"].toString().trim();
    if (text.isEmpty()) return false;
    const auto utf8=text.toStdString();
    // State parsing may run inside a host with any LC_NUMERIC. The classic
    // locale belongs to this stream only; never change the host global locale.
    std::istringstream parser(utf8);
    parser.imbue(std::locale::classic());
    double value=0;
    parser >> std::noskipws >> value;
    const auto& range=parameter->getNormalisableRange();
    if (!parser || !parser.eof() || !std::isfinite(value)
        || value<range.start || value>range.end) return false;
    if ((dynamic_cast<juce::AudioParameterChoice*>(parameter)
         || dynamic_cast<juce::AudioParameterBool*>(parameter))
        && value!=std::floor(value)) return false;
  }
  return true;
}
