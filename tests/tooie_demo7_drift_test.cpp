#include <cstdio>
#include "tooie_demo7_drift.h"

using namespace tooie_demo7_drift;

static_assert(kBsPrevOff == 0 && kBsCurOff == 4 && kBsNextOff == 8, "bs layout prev/cur/next");
static_assert(in_window(0, 8, 0) && in_window(0, 8, 120), "window edges");
static_assert(!in_window(0, 8, 121) && !in_window(0, 8, -1), "outside glidx");
static_assert(!in_window(0, 7, 5) && !in_window(0, 9, 5) && !in_window(1, 8, 5), "stage/sub gate");

static_assert(in_trace_window(0, 8, 33) && in_trace_window(0, 8, 46), "trace window post 33..46");
static_assert(!in_trace_window(0, 8, 32) && !in_trace_window(0, 8, 47), "trace window edges");
static_assert(!in_trace_window(0, 7, 38) && !in_trace_window(1, 8, 38), "trace stage/sub gate");

int main() {
    int rows = 0;
    for (int g = -5; g < 400; ++g) rows += in_window(0, 8, g);
    if (rows != 121) { std::fprintf(stderr, "FAIL rows=%d\n", rows); return 1; }
    int trows = 0;
    for (int p = 0; p < 200; ++p) trows += in_trace_window(0, 8, p);
    if (trows != 14) { std::fprintf(stderr, "FAIL trace rows=%d\n", trows); return 1; }
    std::puts("PASS tooie_demo7_drift_test");
    return 0;
}
