// Regression test for P08 BCF28 helper bindings (0x800BCCC0 and 0x800BCCD8).
// Proves:
// 1. get_function(0x800BCCC0) != lookup_gap_stub
// 2. get_function(0x800BCCD8) != lookup_gap_stub
// 3. 0x800BCCC0 computes exact MIPS semantics: v0 = a0 + a1 * 56 + 0x24
// 4. 0x800BCCD8 computes exact MIPS semantics: v0 = a0 + MEM_BU(4, a0) * 56 + 0x24
// 5. Remaining gap stubs (0x80001998, 0x80031178, 0x80036670, 0x80036728, 0x800979D8) remain bound to no-op gap stubs (v0 = 0).

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "recomp.h"
#include "librecomp/overlays.hpp"

namespace banjo {
void register_boot_lookup_stubs();
}

// Dummy symbols for extern functions referenced by boot_hw_stubs.cpp
extern "C" void func_80013818(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    (void)ctx;
}

extern "C" void osPfsInit_recomp(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    if (ctx) ctx->r2 = 0;
}

static inline gpr kseg0_gpr(uint32_t addr) {
    return static_cast<gpr>(static_cast<int32_t>(addr));
}

int main() {
    std::printf("[bcf28_test] registering lookup stubs...\n");
    banjo::register_boot_lookup_stubs();

    // 8 MiB simulated RDRAM buffer for memory reads
    std::vector<uint8_t> rdram_buffer(8 * 1024 * 1024, 0);
    uint8_t* rdram = rdram_buffer.data();

    // 1. Verify function lookup
    recomp_func_t* f_bccc0 = get_function(static_cast<int32_t>(0x800BCCC0));
    recomp_func_t* f_bccd8 = get_function(static_cast<int32_t>(0x800BCCD8));
    recomp_func_t* f_gap1998 = get_function(static_cast<int32_t>(0x80001998));
    recomp_func_t* f_gap31178 = get_function(static_cast<int32_t>(0x80031178));
    recomp_func_t* f_gap36670 = get_function(static_cast<int32_t>(0x80036670));
    recomp_func_t* f_gap36728 = get_function(static_cast<int32_t>(0x80036728));
    recomp_func_t* f_gap979d8 = get_function(static_cast<int32_t>(0x800979D8));

    assert(f_bccc0 != nullptr);
    assert(f_bccd8 != nullptr);
    assert(f_gap1998 != nullptr);
    assert(f_bccc0 != f_gap1998);
    assert(f_bccd8 != f_gap1998);
    std::printf("[bcf28_test] PASS: 0x800BCCC0 and 0x800BCCD8 are bound to dedicated functions != gap_stub\n");

    // 2. Test 0x800BCCC0 semantics: v0 = a0 + a1 * 56 + 0x24
    struct Bccc0TestCase {
        uint32_t a0;
        uint32_t a1;
        uint32_t expected_v0;
        uint32_t expected_t6;
    };

    const Bccc0TestCase bccc0_cases[] = {
        {0x801D2560u, 0u, 0x801D2584u, 0u},
        {0x801D2560u, 1u, 0x801D25BCu, 56u},
        {0x801D2560u, 2u, 0x801D25F4u, 112u},
        {0x801D2560u, 3u, 0x801D262Cu, 168u}, // The exact case in func_800BCF28 crash
        {0x801D2560u, 13u, 0x801D285Cu, 728u},
        {0x80200000u, 10u, 0x80200254u, 560u},
    };

    for (size_t i = 0; i < sizeof(bccc0_cases) / sizeof(bccc0_cases[0]); ++i) {
        const auto& tc = bccc0_cases[i];
        recomp_context ctx{};
        ctx.r4 = kseg0_gpr(tc.a0);
        ctx.r5 = tc.a1;
        f_bccc0(rdram, &ctx);

        const uint32_t v0 = static_cast<uint32_t>(ctx.r2);
        const uint32_t t6 = static_cast<uint32_t>(ctx.r14);

        if (v0 != tc.expected_v0 || t6 != tc.expected_t6) {
            std::fprintf(stderr,
                         "[bcf28_test] FAIL 0x800BCCC0 case %zu: a0=0x%08X a1=%u -> v0=0x%08X (expected 0x%08X), t6=%u (expected %u)\n",
                         i, tc.a0, tc.a1, v0, tc.expected_v0, t6, tc.expected_t6);
            return 1;
        }
    }
    std::printf("[bcf28_test] PASS: 0x800BCCC0 representative cases passed\n");

    // 3. Test 0x800BCCD8 semantics: v0 = a0 + MEM_BU(4, a0) * 56 + 0x24
    struct Bccd8TestCase {
        uint32_t a0;
        uint8_t byte_at_4;
        uint32_t expected_v0;
        uint32_t expected_t6;
        uint32_t expected_t7;
    };

    const Bccd8TestCase bccd8_cases[] = {
        {0x801D2560u, 0u, 0x801D2584u, 0u, 0u},
        {0x801D2560u, 1u, 0x801D25BCu, 1u, 56u},
        {0x801D2560u, 2u, 0x801D25F4u, 2u, 112u},
        {0x801D2560u, 5u, 0x801D269Cu, 5u, 280u},
        {0x80200000u, 10u, 0x80200254u, 10u, 560u},
    };

    for (size_t i = 0; i < sizeof(bccd8_cases) / sizeof(bccd8_cases[0]); ++i) {
        const auto& tc = bccd8_cases[i];
        // Write byte at offset 4 of a0 in RDRAM
        MEM_BU(4, kseg0_gpr(tc.a0)) = tc.byte_at_4;

        recomp_context ctx{};
        ctx.r4 = kseg0_gpr(tc.a0);
        f_bccd8(rdram, &ctx);

        const uint32_t v0 = static_cast<uint32_t>(ctx.r2);
        const uint32_t t6 = static_cast<uint32_t>(ctx.r14);
        const uint32_t t7 = static_cast<uint32_t>(ctx.r15);

        if (v0 != tc.expected_v0 || t6 != tc.expected_t6 || t7 != tc.expected_t7) {
            std::fprintf(stderr,
                         "[bcf28_test] FAIL 0x800BCCD8 case %zu: a0=0x%08X byte=%u -> v0=0x%08X (expected 0x%08X), t6=%u (expected %u), t7=%u (expected %u)\n",
                         i, tc.a0, tc.byte_at_4, v0, tc.expected_v0, t6, tc.expected_t6, t7, tc.expected_t7);
            return 1;
        }
    }
    std::printf("[bcf28_test] PASS: 0x800BCCD8 representative cases passed\n");

    // 4. Verify remaining gap stubs still write v0 = 0
    recomp_func_t* gap_funcs[] = {f_gap1998, f_gap31178, f_gap36670, f_gap36728, f_gap979d8};
    for (size_t i = 0; i < sizeof(gap_funcs) / sizeof(gap_funcs[0]); ++i) {
        recomp_context ctx{};
        ctx.r2 = 0xDEADBEEF12345678ull;
        gap_funcs[i](rdram, &ctx);
        if (ctx.r2 != 0) {
            std::fprintf(stderr, "[bcf28_test] FAIL gap stub %zu did not clear r2 (got 0x%lx)\n", i, (unsigned long)ctx.r2);
            return 1;
        }
    }
    std::printf("[bcf28_test] PASS: All 5 remaining gap stubs correctly return v0 = 0\n");

    std::printf("[bcf28_test] ALL TESTS PASSED SUCCESSFULLY.\n");
    std::fflush(stdout);
    return 0;
}
