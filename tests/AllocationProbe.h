#pragma once
#include <cstddef>
// Test-only allocator probe lives in a separate dylib so it can observe the
// executable's JUCE render calls. No instrumentation is linked into plugins.
namespace allocationProbe {
void start() noexcept;
std::size_t stop() noexcept;
}
