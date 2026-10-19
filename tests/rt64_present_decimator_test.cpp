// Host test of the RT64 below-native present decimator (header-only).
#include "hle/rt64_present_decimator.h"

#include <cstdio>
#include <cstdlib>

#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, #c); std::exit(1); } } while (0)

static int shown(uint32_t target, uint32_t original, int workloads, bool manual = true) {
    RT64::PresentDecimator d;
    int n = 0;
    for (int i = 0; i < workloads; i++) {
        n += d.shouldPresent(manual, false, target, original) ? 1 : 0;
    }
    return n;
}

int main() {
    // Below native: exact average rate, including non-divisors.
    CHECK(shown(20, 60, 60) == 20);
    CHECK(shown(30, 60, 60) == 30);
    CHECK(shown(45, 60, 60) == 45);
    CHECK(shown(20, 30, 30) == 20);
    // First workload is always shown.
    RT64::PresentDecimator d;
    CHECK(d.shouldPresent(true, false, 20, 60));
    // No decimation: Original, Display/other modes, equal/above native, unknown native, paused.
    CHECK(shown(0, 60, 60) == 60);
    CHECK(shown(30, 60, 60, false) == 60);
    CHECK(shown(60, 60, 60) == 60);
    CHECK(shown(120, 60, 60) == 60);
    CHECK(shown(30, 0, 60) == 60);
    CHECK(d.shouldPresent(true, true, 20, 60));
    // Rate change resyncs and shows immediately.
    RT64::PresentDecimator e;
    e.shouldPresent(true, false, 20, 60);
    CHECK(e.shouldPresent(true, false, 30, 60));
    // Pause resets the cadence: next unpaused workload is shown.
    RT64::PresentDecimator f;
    f.shouldPresent(true, false, 20, 60);
    CHECK(!f.shouldPresent(true, false, 20, 60));
    CHECK(f.shouldPresent(true, true, 20, 60));
    CHECK(f.shouldPresent(true, false, 20, 60));
    std::puts("rt64_present_decimator_test PASS");
    return 0;
}
