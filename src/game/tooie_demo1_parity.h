#pragma once
// TOOIE_DIAG_DEMO1_PARITY helpers shared with tests (USER_DEMO1 stage0 sub0 behavior parity).
// Sampling point: ENTRY of func_800151BC (exact boundary, before the original runs), once per
// glrecord post. The oracle mirror (tools/p08_attract_egg_hook.h, P08_DEMO1_PARITY=1) samples at
// PC 0x800151BC with the same fields; the CSV header and row format below are kept identical
// (tests/tooie_demo1_parity_test.cpp compares them with the hook text).
//   consumed = post-1 (D_8012D5B0+8); count = D_8012D5B0+0xC; vi = number of entries of
//   func_8001443C (VI handler) seen since the flag was on. seq = entries of 800151BC seen.
#include <cstdint>

namespace tooie_demo1_parity {

constexpr int kMode = 5;
constexpr int kStage = 0;
constexpr int kSub = 0;
constexpr int kMaxConsumed = 420;  // rows consumed 0..420 = 421 max

constexpr bool in_window(int mode, int stage, int sub, int post) {
    const int consumed = (post > 0) ? (post - 1) : -1;
    return mode == kMode && stage == kStage && sub == kSub && post > 0 && consumed <= kMaxConsumed;
}

constexpr const char* kCsvHeader =
    "phase,pc,seq,elapsed_ns,vi,gs_bits,gs,mode,stage,sub,map,post,consumed,count,"
    "r0,r1,r2,r3,r4,r5,rmx,rmy,bx,by,bang,bdist,yaw,ideal,phys_yaw,yaw_b,"
    "px,py,pz,px_b,py_b,pz_b,ppx,ppy,ppz,vx,vy,vz,vx_b,vz_b,velxz,tspd,tspd_b,accel,mult,ptype,"
    "bs_prev,bs_cur,bs_next,anim_id,anim_timer,anim_dur,anim_phase,camx,camy,camz,camctrl,sel";

constexpr const char* kRowFmt =
    "%s,%08x,%llu,%llu,%llu,%08x,%.9g,%d,%d,%d,%d,%d,%d,%d,"
    "%u,%u,%u,%u,%u,%u,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%08x,"
    "%.9g,%.9g,%.9g,%08x,%08x,%08x,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%08x,%08x,%.9g,%.9g,%08x,%.9g,%.9g,%d,"
    "%d,%d,%d,%d,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%d\n";

}  // namespace tooie_demo1_parity
