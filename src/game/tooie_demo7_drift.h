#pragma once
// TOOIE_DIAG_DEMO7_DRIFT helpers: capture gate and bs-struct layout.
// Layout proof: bs_setState at 0x8009E5D8 (funcs_350.c) does
//   [ps+0x120]->+8 = id; +0 = +4; +4 = +8; +8 = 0   => +0 prev, +4 cur, +8 next.
#include <cstdint>

namespace tooie_demo7_drift {

constexpr int kStage = 0;
constexpr int kSub = 8;
constexpr int kMaxGlidx = 120;  // rows 0..120 inclusive = 121 per cycle

// Write/call trace window (consumed glidx; wider than 35..40 in case phase is off).
constexpr int kTraceLoGlidx = 32;
constexpr int kTraceHiGlidx = 45;
constexpr int kTraceMaxRows = 600;

constexpr uint32_t kBsPrevOff = 0x0;
constexpr uint32_t kBsCurOff = 0x4;
constexpr uint32_t kBsNextOff = 0x8;

constexpr bool in_window(int stage, int sub, int consumed_glidx) {
    return stage == kStage && sub == kSub && consumed_glidx >= 0 &&
           consumed_glidx <= kMaxGlidx;
}

// post = glrecord post index (D_8012D5B0+8); consumed glidx = post-1 (0 when post==0).
constexpr bool in_trace_window(int stage, int sub, int post) {
    const int cg = (post > 0) ? (post - 1) : 0;
    return stage == kStage && sub == kSub && cg >= kTraceLoGlidx && cg <= kTraceHiGlidx;
}

}  // namespace tooie_demo7_drift
