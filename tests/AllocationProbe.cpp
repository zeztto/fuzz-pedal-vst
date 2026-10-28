#include "AllocationProbe.h"
#include <cstdlib>
namespace { thread_local bool active=false;thread_local std::size_t calls=0; }
void allocationProbe::start() noexcept {calls=0;active=true;}
std::size_t allocationProbe::stop() noexcept {active=false;return calls;}
#if __APPLE__
static void* probeMalloc(std::size_t n) {if(active)++calls;return std::malloc(n);}
static void* probeCalloc(std::size_t n,std::size_t s) {if(active)++calls;return std::calloc(n,s);}
static void* probeRealloc(void* p,std::size_t n) {if(active)++calls;return std::realloc(p,n);}
__attribute__((used)) static const struct {const void* replacement;const void* original;} interposers[]
__attribute__((section("__DATA,__interpose")))={
 {reinterpret_cast<const void*>(&probeMalloc),reinterpret_cast<const void*>(&malloc)},
 {reinterpret_cast<const void*>(&probeCalloc),reinterpret_cast<const void*>(&calloc)},
 {reinterpret_cast<const void*>(&probeRealloc),reinterpret_cast<const void*>(&realloc)}};
#endif
