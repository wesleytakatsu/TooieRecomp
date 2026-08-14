// Regression test for P08 core1 RA-wrapper native overrides.
//
// Proves:
// 1. 0x800137D4 / 0x800137F4 / 0x80013A5C / 0x80013A7C bind to the Tooie-native
//    strong symbols (get_function after overlay-table-equivalent registration).
// 2. Each wrapper returns normally with ctx->r31 = 0 (the generated-code
//    failure condition) and does not LOOKUP_FUNC(0).
// 3. Representative constants / post-call arithmetic match original MIPS.
// 4. func_80013A5C reloads $f2 AFTER func_80013ABC (helper clobbers $f2).
// 5. func_800139F8 is not overridden (this file never references it).

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "recomp.h"
#include "librecomp/overlays.hpp"

namespace banjo {
void register_boot_lookup_stubs();
}

extern "C" void func_800137D4(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_800137F4(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_80013A5C(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_80013A7C(uint8_t* rdram, recomp_context* ctx);

static int g_13818_calls = 0;
static int g_13abc_calls = 0;
static uint32_t g_13818_f14 = 0;
static uint32_t g_13818_f16 = 0;
static uint32_t g_13abc_f14 = 0;

// Native wrappers must return through the host C call. The generated
// bodies would LOOKUP_FUNC(saved_ra) with r31==0 and abort. A normal
// return that leaves r31 at 0 is the discriminator.

extern "C" void func_80013818(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    ++g_13818_calls;
    g_13818_f14 = ctx->f14.u32l;
    g_13818_f16 = ctx->f16.u32l;
    ctx->f2.fl = 12.5f;
}

extern "C" void func_80013ABC(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    ++g_13abc_calls;
    g_13abc_f14 = ctx->f14.u32l;
    // Match the real helper: it clobbers $f2. A pre-call f2 load would fail.
    ctx->f2.fl = 12345.0f;
    ctx->f0.fl = 2.0f;
}

extern "C" void osPfsInit_recomp(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    if (ctx) {
        ctx->r2 = 0;
    }
}

static inline gpr kseg0_gpr(uint32_t addr) {
    return static_cast<gpr>(static_cast<int32_t>(addr));
}

static bool nearly_eq(float a, float b) {
    return std::fabs(a - b) <= 1.0e-6f;
}

static void seed_word(uint8_t* rdram, uint32_t addr, uint32_t bits) {
    MEM_W(0, kseg0_gpr(addr)) = static_cast<int32_t>(bits);
}

static int fail(const char* msg) {
    std::fprintf(stderr, "[core1_ra_test] FAIL: %s\n", msg);
    return 1;
}

int main() {
    std::printf("[core1_ra_test] registering lookup stubs...\n");
    banjo::register_boot_lookup_stubs();

    // Overlay-table equivalent: recomp_overlays.inl stores
    //   { .func = func_80013xxx, ... }
    // so LOOKUP_FUNC(vram) resolves the strong symbol after load.
    // The unit test does not load core1; register the four addresses
    // to prove the native bodies are the ones get_function would return.
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800137D4), func_800137D4);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800137F4), func_800137F4);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80013A5C), func_80013A5C);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80013A7C), func_80013A7C);

    if (get_function(static_cast<int32_t>(0x800137D4)) != func_800137D4) {
        return fail("0x800137D4 binding is not func_800137D4");
    }
    if (get_function(static_cast<int32_t>(0x800137F4)) != func_800137F4) {
        return fail("0x800137F4 binding is not func_800137F4");
    }
    if (get_function(static_cast<int32_t>(0x80013A5C)) != func_80013A5C) {
        return fail("0x80013A5C binding is not func_80013A5C");
    }
    if (get_function(static_cast<int32_t>(0x80013A7C)) != func_80013A7C) {
        return fail("0x80013A7C binding is not func_80013A7C");
    }
    std::printf("[core1_ra_test] PASS: four wrappers bind to native bodies\n");

    std::vector<uint8_t> rdram_buffer(8 * 1024 * 1024, 0);
    uint8_t* rdram = rdram_buffer.data();

    constexpr uint32_t kF14_137D4 = 0x3F800000u; // 1.0f at 0x800416CC
    constexpr uint32_t kF16_137D4 = 0x40000000u; // 2.0f at 0x800416D0
    constexpr uint32_t kF14_137F4 = 0x40400000u; // 3.0f at 0x800416D4
    constexpr uint32_t kF14_13A5C = 0x3E340000u; // same lui used by 13A7C
    constexpr uint32_t kF2_13A5C = 0x42C80000u;  // 100.0f
    seed_word(rdram, 0x800416CCu, kF14_137D4);
    seed_word(rdram, 0x800416D0u, kF16_137D4);
    seed_word(rdram, 0x800416D4u, kF14_137F4);
    seed_word(rdram, 0x80041700u, kF14_13A5C);
    seed_word(rdram, 0x80041704u, kF2_13A5C);

    struct Case {
        const char* name;
        uint32_t vram;
        recomp_func_t* fn;
        int* helper_calls;
        int expected_helper_delta;
        uint32_t* helper_f14;
        uint32_t expected_f14;
        uint32_t expected_f16; // UINT32_MAX = do not check
        float expected_f0;
        uint32_t expected_f2_bits; // UINT32_MAX = do not check
    };

    const Case cases[] = {
        {"func_800137D4", 0x800137D4u, func_800137D4, &g_13818_calls, 1,
         &g_13818_f14, kF14_137D4, kF16_137D4, 12.5f, UINT32_MAX},
        {"func_800137F4", 0x800137F4u, func_800137F4, &g_13818_calls, 1,
         &g_13818_f14, kF14_137F4, 0x43B40000u, 12.5f, UINT32_MAX},
        {"func_80013A5C", 0x80013A5Cu, func_80013A5C, &g_13abc_calls, 1,
         &g_13abc_f14, kF14_13A5C, UINT32_MAX, 98.0f, kF2_13A5C},
        {"func_80013A7C", 0x80013A7Cu, func_80013A7C, &g_13abc_calls, 1,
         &g_13abc_f14, 0x3E340000u, UINT32_MAX, 88.0f, 0x42B40000u},
    };

    for (const Case& tc : cases) {
        const int helper_before = *tc.helper_calls;
        recomp_context ctx{};
        ctx.r31 = 0;
        ctx.f0.fl = 999.0f;
        ctx.f2.fl = 888.0f;

        tc.fn(rdram, &ctx);

        if (ctx.r31 != 0) {
            std::fprintf(stderr, "[core1_ra_test] FAIL: %s wrote r31 (0x%llx)\n",
                         tc.name, static_cast<unsigned long long>(ctx.r31));
            return 1;
        }
        if (*tc.helper_calls != helper_before + tc.expected_helper_delta) {
            std::fprintf(stderr, "[core1_ra_test] FAIL: %s helper call count %d (expected %d)\n",
                         tc.name, *tc.helper_calls, helper_before + tc.expected_helper_delta);
            return 1;
        }
        if (*tc.helper_f14 != tc.expected_f14) {
            std::fprintf(stderr, "[core1_ra_test] FAIL: %s helper f14=0x%08X (expected 0x%08X)\n",
                         tc.name, *tc.helper_f14, tc.expected_f14);
            return 1;
        }
        if (tc.expected_f16 != UINT32_MAX && g_13818_f16 != tc.expected_f16) {
            std::fprintf(stderr, "[core1_ra_test] FAIL: %s helper f16=0x%08X (expected 0x%08X)\n",
                         tc.name, g_13818_f16, tc.expected_f16);
            return 1;
        }
        if (!nearly_eq(ctx.f0.fl, tc.expected_f0)) {
            std::fprintf(stderr, "[core1_ra_test] FAIL: %s f0=%f (expected %f)\n",
                         tc.name, ctx.f0.fl, tc.expected_f0);
            return 1;
        }
        if (tc.expected_f2_bits != UINT32_MAX && ctx.f2.u32l != tc.expected_f2_bits) {
            std::fprintf(stderr, "[core1_ra_test] FAIL: %s f2=0x%08X (expected 0x%08X)\n",
                         tc.name, ctx.f2.u32l, tc.expected_f2_bits);
            return 1;
        }

        // Same body via get_function, still with r31=0.
        recomp_func_t* looked_up = get_function(static_cast<int32_t>(tc.vram));
        if (looked_up != tc.fn) {
            std::fprintf(stderr, "[core1_ra_test] FAIL: %s lookup pointer mismatch\n", tc.name);
            return 1;
        }
        recomp_context ctx2{};
        ctx2.r31 = 0;
        looked_up(rdram, &ctx2);
        if (ctx2.r31 != 0 || !nearly_eq(ctx2.f0.fl, tc.expected_f0)) {
            std::fprintf(stderr, "[core1_ra_test] FAIL: %s lookup-path result mismatch\n", tc.name);
            return 1;
        }
        std::printf("[core1_ra_test] PASS: %s r31=0 host call + lookup path\n", tc.name);
    }

    std::printf("[core1_ra_test] ALL TESTS PASSED SUCCESSFULLY.\n");
    std::fflush(stdout);
    return 0;
}
