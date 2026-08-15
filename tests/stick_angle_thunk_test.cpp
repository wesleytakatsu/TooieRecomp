// Focused test for the P08 func_80013B70 stick-angle secondary-entry override.
//
// Calls the actual Tooie-native func_80013B70 body. Does not reimplement the
// shared-tail formula in the assertions: cardinals are the MIPS immediate
// constants, diagonals go through a stub of func_80013ABC so the test can
// observe the y/distance contract and the 180/360 quadrant adjust.

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "recomp.h"
#include "librecomp/overlays.hpp"

namespace banjo {
void register_boot_lookup_stubs();
}

extern "C" void func_80013B70(uint8_t* rdram, recomp_context* ctx);

static int g_13abc_calls = 0;
static float g_13abc_f12 = 0.0f;
static uint32_t g_13abc_f14 = 0;
static uint32_t g_13abc_f16 = 0;

// Stand-in for the core1 table helper. Sign-preserving 45° lets the shared
// tail's 180-x / 360+x adjust produce the four diagonal quadrants without
// duplicating func_80013B70.
extern "C" void func_80013ABC(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    ++g_13abc_calls;
    g_13abc_f12 = ctx->f12.fl;
    g_13abc_f14 = ctx->f14.u32l;
    g_13abc_f16 = ctx->f16.u32l;
    ctx->f0.fl = (ctx->f12.fl < 0.0f) ? -45.0f : 45.0f;
}

extern "C" void osPfsInit_recomp(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    if (ctx) {
        ctx->r2 = 0;
    }
}

static bool nearly_eq(float a, float b) {
    return std::fabs(a - b) <= 1.0e-5f;
}

static uint32_t float_bits(float v) {
    uint32_t bits = 0;
    std::memcpy(&bits, &v, sizeof(bits));
    return bits;
}

static int fail(const char* msg) {
    std::fprintf(stderr, "[stick_angle_test] FAIL: %s\n", msg);
    return 1;
}

static void call_13b70(uint8_t* rdram, float y, float x, float distance, recomp_context* ctx) {
    *ctx = recomp_context{};
    ctx->r1 = static_cast<gpr>(static_cast<int32_t>(0xDEADBEEF));
    ctx->r31 = 0;
    ctx->f0.fl = 999.0f;
    ctx->f12.fl = y;
    ctx->f14.fl = x;
    ctx->r6 = float_bits(distance);
}

int main() {
    std::printf("[stick_angle_test] registering lookup stubs...\n");
    banjo::register_boot_lookup_stubs();

    // Overlay-table equivalent: recomp_overlays.inl stores
    //   { .func = func_80013B70, ... }
    // The unit test does not load core1; register the VRAM so get_function
    // returns the strong native body. Production does not add_loaded_function.
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80013B70), func_80013B70);
    if (get_function(static_cast<int32_t>(0x80013B70)) != func_80013B70) {
        return fail("LOOKUP 0x80013B70 is not the native func_80013B70");
    }
    std::printf("[stick_angle_test] PASS: LOOKUP_80013B70_RESOLVES_TO_NATIVE\n");

    std::vector<uint8_t> rdram_buffer(1024 * 1024, 0);
    uint8_t* rdram = rdram_buffer.data();
    recomp_context ctx{};

    struct Cardinal {
        const char* name;
        float y;
        float x;
        float expected;
        int helper_calls;
        uint32_t expected_r1;
    };
    const Cardinal cardinals[] = {
        {"RIGHT", 0.0f, 1.0f, 0.0f, 0, 0x43340000u},
        {"UP", 1.0f, 0.0f, 90.0f, 0, 0x42B40000u},
        {"LEFT", 0.0f, -1.0f, 180.0f, 0, 0x43340000u},
        {"DOWN", -1.0f, 0.0f, 270.0f, 0, 0x43870000u},
        {"CENTER", 0.0f, 0.0f, 0.0f, 0, 0x43340000u},
    };

    for (const Cardinal& tc : cardinals) {
        const int helpers_before = g_13abc_calls;
        call_13b70(rdram, tc.y, tc.x, 1.0f, &ctx);
        func_80013B70(rdram, &ctx);
        if (ctx.r31 != 0) {
            std::fprintf(stderr, "[stick_angle_test] FAIL: %s wrote r31\n", tc.name);
            return 1;
        }
        if (g_13abc_calls != helpers_before + tc.helper_calls) {
            std::fprintf(stderr, "[stick_angle_test] FAIL: %s called func_80013ABC\n", tc.name);
            return 1;
        }
        if (!nearly_eq(ctx.f0.fl, tc.expected)) {
            std::fprintf(stderr, "[stick_angle_test] FAIL: %s f0=%f expected %f\n",
                         tc.name, ctx.f0.fl, tc.expected);
            return 1;
        }
        const uint32_t actual_r1 = static_cast<uint32_t>(ctx.r1);
        if (actual_r1 != tc.expected_r1) {
            std::fprintf(stderr, "[stick_angle_test] FAIL: %s r1=0x%08X expected 0x%08X (delay-slot clobber)\n",
                         tc.name, actual_r1, tc.expected_r1);
            return 1;
        }
        if (nearly_eq(ctx.f0.fl, 1.0f)) {
            return fail("cardinal result collapsed to stick distance");
        }
        std::printf("[stick_angle_test] PASS: %s = %.1f (r1=0x%08X)\n", tc.name, ctx.f0.fl, actual_r1);
    }

    struct Diagonal {
        const char* name;
        float y;
        float x;
        float expected;
    };
    const Diagonal diagonals[] = {
        {"NE", 1.0f, 1.0f, 45.0f},
        {"NW", 1.0f, -1.0f, 135.0f},
        {"SW", -1.0f, -1.0f, 225.0f},
        {"SE", -1.0f, 1.0f, 315.0f},
    };

    for (const Diagonal& tc : diagonals) {
        const int helpers_before = g_13abc_calls;
        const float dist = std::sqrt(tc.y * tc.y + tc.x * tc.x);
        call_13b70(rdram, tc.y, tc.x, dist, &ctx);
        func_80013B70(rdram, &ctx);
        if (g_13abc_calls != helpers_before + 1) {
            std::fprintf(stderr, "[stick_angle_test] FAIL: %s did not call func_80013ABC\n",
                         tc.name);
            return 1;
        }
        if (g_13abc_f14 != 0x3E340000u) {
            std::fprintf(stderr, "[stick_angle_test] FAIL: %s helper f14=0x%08X\n",
                         tc.name, g_13abc_f14);
            return 1;
        }
        if (!nearly_eq(ctx.f0.fl, tc.expected)) {
            std::fprintf(stderr, "[stick_angle_test] FAIL: %s f0=%f expected %f\n",
                         tc.name, ctx.f0.fl, tc.expected);
            return 1;
        }
        if (nearly_eq(ctx.f0.fl, dist)) {
            return fail("diagonal result collapsed to stick distance");
        }
        std::printf("[stick_angle_test] PASS: %s = %.1f (not distance %.5f)\n",
                    tc.name, ctx.f0.fl, dist);
    }

    // Third-argument contract: same (y, x), two supplied distances.
    // Shared tail does f12 = y / f16 before func_80013ABC. A full 13B7C call
    // would recompute hypot and ignore r6, so both cases would see the same f12.
    call_13b70(rdram, 3.0f, 4.0f, 10.0f, &ctx);
    const int helpers_before = g_13abc_calls;
    func_80013B70(rdram, &ctx);
    if (g_13abc_calls != helpers_before + 1) {
        return fail("third-arg case did not call func_80013ABC");
    }
    if (!nearly_eq(g_13abc_f12, 0.3f)) {
        std::fprintf(stderr, "[stick_angle_test] FAIL: y/r6 f12=%f expected 0.3\n",
                     g_13abc_f12);
        return 1;
    }
    if (g_13abc_f16 != float_bits(10.0f)) {
        std::fprintf(stderr, "[stick_angle_test] FAIL: f16=0x%08X not supplied r6\n",
                     g_13abc_f16);
        return 1;
    }
    const float doubled_f12 = g_13abc_f12;

    call_13b70(rdram, 3.0f, 4.0f, 5.0f, &ctx);
    func_80013B70(rdram, &ctx);
    if (!nearly_eq(g_13abc_f12, 0.6f)) {
        std::fprintf(stderr, "[stick_angle_test] FAIL: hypot-length f12=%f expected 0.6\n",
                     g_13abc_f12);
        return 1;
    }
    if (nearly_eq(doubled_f12, g_13abc_f12)) {
        return fail("third argument was ignored (same y/f16 as recomputed hypot)");
    }
    std::printf("[stick_angle_test] PASS: third-argument r6/f16 contract (0.3 vs 0.6)\n");

    // Non-unit / small magnitude still uses supplied r6, not a recomputed length.
    call_13b70(rdram, 0.02f, 0.02f, 0.08f, &ctx);
    func_80013B70(rdram, &ctx);
    if (!nearly_eq(g_13abc_f12, 0.25f)) {
        std::fprintf(stderr, "[stick_angle_test] FAIL: small-mag f12=%f expected 0.25\n",
                     g_13abc_f12);
        return 1;
    }
    std::printf("[stick_angle_test] PASS: small-magnitude uses supplied distance\n");

    // Same body via get_function.
    recomp_func_t* looked_up = get_function(static_cast<int32_t>(0x80013B70));
    call_13b70(rdram, 0.0f, 1.0f, 1.0f, &ctx);
    looked_up(rdram, &ctx);
    if (!nearly_eq(ctx.f0.fl, 0.0f) || ctx.r31 != 0) {
        return fail("lookup-path RIGHT mismatch");
    }
    std::printf("[stick_angle_test] PASS: lookup path RIGHT = 0\n");

    std::printf("[stick_angle_test] ALL TESTS PASSED SUCCESSFULLY.\n");
    std::fflush(stdout);
    return 0;
}
