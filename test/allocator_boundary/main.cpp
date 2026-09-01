// Simulates a host replacing operator new/delete without exporting them.
#include <cstdio>
#include <cstdlib>
#include <new>

#include "GameAnalytics/GameAnalytics.h"

namespace
{
    constexpr unsigned long long kMagic = 0x4745554E47494E45ULL;
    constexpr size_t kHeader = 16;
}

void* operator new(size_t n)
{
    auto* p = static_cast<unsigned long long*>(std::malloc(n + kHeader));
    if (!p) throw std::bad_alloc();
    p[0] = kMagic;
    return p + 2;
}
void* operator new[](size_t n) { return operator new(n); }

void operator delete(void* p) noexcept
{
    if (!p) return;
    auto* q = static_cast<unsigned long long*>(p) - 2;
    if (q[0] != kMagic)
    {
        std::fprintf(stderr, "FATAL: freeing pointer %p allocated outside this binary\n", p);
        std::abort();
    }
    q[0] = 0;
    std::free(q);
}
void operator delete[](void* p) noexcept { operator delete(p); }
void operator delete(void* p, size_t) noexcept { operator delete(p); }
void operator delete[](void* p, size_t) noexcept { operator delete(p); }

int main()
{
    using namespace gameanalytics;

    GameAnalytics::setEnabledInfoLog(true);
    GameAnalytics::setEnabledEventSubmission(false);
    GameAnalytics::configureBuild("allocator-boundary-test 1.0");
    GameAnalytics::initialize("00000000000000000000000000000000", "0000000000000000000000000000000000000000");

    GameAnalytics::addDesignEvent("allocator:boundary:check");
    GameAnalytics::addResourceEvent(EGAResourceFlowType::Sink, "gems", -1e20f, "boost", "speed");

    GameAnalytics::onQuit();

    std::printf("ALLOCATOR BOUNDARY TEST OK\n");
    return 0;
}
