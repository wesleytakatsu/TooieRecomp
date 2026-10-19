// Strong overrides for boot-segment libultra that touch unmapped N64 MMIO
// (SI/PI at 0xA48xxxxx / 0xA46xxxxx). RecompiledFuncs symbols are weak under
// clang RECOMP_FUNC; these replace them at link time.
//
// Also registers LOOKUP targets that N64Recomp never emitted (unnamed helpers).

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fenv.h>

#include "recomp.h"
#include "funcs.h"
#include "librecomp/game.hpp"
#include "librecomp/addresses.hpp"
#include "librecomp/overlays.hpp"
#include "ultramodern/ultramodern.hpp"
#include "ultramodern/config.hpp"
#include "recompui/recompui.h"
#include "tooie_window.h"
#include "tooie_diag.h"
#include "tooie_demo7_drift.h"
#include "tooie_demo1_parity.h"
#include "tooie_demo1_projectile.h"
#include "generated/overlay_dispatch.inc.h"

namespace banjo {
void register_boot_lookup_stubs();
}

extern "C" void __wrap_func_800E0B8C(uint8_t* rdram, recomp_context* ctx);
extern "C" void __wrap_func_800E0E94(uint8_t* rdram, recomp_context* ctx);
extern "C" void __wrap_func_800AE8A8(uint8_t* rdram, recomp_context* ctx);
extern "C" void __wrap_func_800AE820(uint8_t* rdram, recomp_context* ctx);
extern "C" void __wrap_func_800E2AA4(uint8_t* rdram, recomp_context* ctx);
extern "C" void __wrap_func_800DE2A4(uint8_t* rdram, recomp_context* ctx);
extern "C" void __wrap_func_800DE498(uint8_t* rdram, recomp_context* ctx);
extern "C" void __wrap_func_800DCF48(uint8_t* rdram, recomp_context* ctx);
extern "C" void __wrap_func_800ADE2C(uint8_t* rdram, recomp_context* ctx);
extern "C" void __wrap_func_800E2180(uint8_t* rdram, recomp_context* ctx);

// MEM_* expects KSEG0 addresses sign-extended into gpr (same as recomp codegen).
static inline gpr kseg0_gpr(uint32_t addr) {
    return static_cast<gpr>(static_cast<int32_t>(addr));
}

// KSEG1 RDRAM base + signed 16-bit MIPS immediate → guest KSEG0 for MEM_*.
// Mirrors: lui $t, 0xA030 ; lw $t, imm16($t)
static constexpr uint32_t kseg1_base_imm16_to_kseg0(uint32_t kseg1_base, uint16_t imm16) {
    const uint32_t kseg1 = kseg1_base + static_cast<uint32_t>(static_cast<int32_t>(static_cast<int16_t>(imm16)));
    return 0x80000000u | (kseg1 & 0x1FFFFFFFu);
}

// Sparse boot/IPL3 residue expected by func_8001DE64 (retail probe compares these
// words). Static recomp does not inherit that residual guest state automatically.
// Values match the opcodes the probe XORs against; exact N64 writer unknown.
// Restores guest RDRAM state — does not bypass the probe.
static void tooie_seed_ipl3_boot_residue(uint8_t* rdram) {
    constexpr uint32_t kBaseA030 = 0xA0300000u;
    constexpr uint32_t kAddr0 = kseg1_base_imm16_to_kseg0(kBaseA030, 0xB1F4u); // 0x802FB1F4
    constexpr uint32_t kAddr1 = kseg1_base_imm16_to_kseg0(kBaseA030, 0xE1C0u); // 0x802FE1C0
    static_assert(kAddr0 == 0x802FB1F4u);
    static_assert(kAddr1 == 0x802FE1C0u);
    MEM_W(0, kseg0_gpr(kAddr0)) = static_cast<int32_t>(0xAD090010);
    MEM_W(0, kseg0_gpr(kAddr1)) = static_cast<int32_t>(0xAD170014);
}

// Declared in librecomp overlays.cpp
extern "C" recomp_func_t* get_function(int32_t addr);
extern "C" void load_overlays(uint32_t rom, int32_t ram_addr, uint32_t size);
extern "C" void unload_overlays(int32_t ram_addr, uint32_t size);
extern "C" void func_80013818(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_80013ABC(uint8_t* rdram, recomp_context* ctx);
extern "C" void bakey_held(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_800BCE84(uint8_t* rdram, recomp_context* ctx);
extern "C" void bainput_isEnabled(uint8_t* rdram, recomp_context* ctx);
extern "C" void bainput_enable(uint8_t* rdram, recomp_context* ctx);
extern "C" void bainput_init(uint8_t* rdram, recomp_context* ctx);
extern "C" void bainput_should_rotate_camera_left(uint8_t* rdram, recomp_context* ctx);
extern "C" void bainput_should_rotate_camera_right(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_800A4338(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_80110EFC(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_80110E00(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_801111D0(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_80112D90(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_800CAA24(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_800CACEC(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_800E3E8C(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_8011447C(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_800B97A4(uint8_t* rdram, recomp_context* ctx);

// Declared in syscall_handler.cpp
extern "C" void tooie_invalidate_resident_overlay(gpr overlay_ptr);

namespace {

constexpr uint32_t k1_to_phys(uint32_t addr) {
    return addr & 0x1FFFFFFFu;
}

// osPiRawStartDma(direction, cartAddr, dramAddr, size)
// direction 0 = cart→RDRAM (OS_READ), 1 = RDRAM→cart (OS_WRITE)
void pi_raw_start_dma_impl(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t direction = static_cast<uint32_t>(ctx->r4);
    const uint32_t cart_addr = static_cast<uint32_t>(ctx->r5);
    const gpr dram_addr = ctx->r6;
    const uint32_t size = static_cast<uint32_t>(ctx->r7);

    // Cart address may be a bare ROM offset or already OR'd with osRomBase (0xB0000000).
    uint32_t physical = k1_to_phys(cart_addr);
    if (physical < recomp::rom_base) {
        physical |= recomp::rom_base;
    }

    if (direction == 0) {
        recomp::do_rom_read(rdram, dram_addr, physical, size);
    } else {
        // Boot does not need cart writes; ignore.
    }
    ctx->r2 = 0;
}

} // namespace

extern "C" void boot___osSiDeviceBusy(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    ctx->r2 = 0; // not busy
}

extern "C" void boot___osSiRawReadIo(uint8_t* rdram, recomp_context* ctx) {
    gpr data_ptr = ctx->r5;
    if (data_ptr != 0) {
        MEM_W(0, data_ptr) = 0;
    }
    (void)rdram;
    ctx->r2 = 0;
}

extern "C" void boot___osSiRawWriteIo(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    ctx->r2 = 0;
}

// osInitialize only programs MI/PI/SI MMIO; ultramodern owns the host side.
extern "C" void boot_osInitialize(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    (void)ctx;
}

extern "C" void boot___osPiRawStartDma(uint8_t* rdram, recomp_context* ctx) {
    pi_raw_start_dma_impl(rdram, ctx);
}

extern "C" void boot_osPiGetStatus(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    ctx->r2 = 0; // DMA complete / not busy
}

extern "C" void boot___osPiGetAccess(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    (void)ctx;
}

extern "C" void boot___osPiRelAccess(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    (void)ctx;
}

extern "C" void boot___osEPiRawStartDma(uint8_t* rdram, recomp_context* ctx) {
    // a0=handle, a1=mb direction pattern varies; Tooie boot uses PiRaw primarily.
    // Fall back to treating a1/a2/a3 like raw if needed later.
    (void)rdram;
    ctx->r2 = 0;
}

extern "C" void boot___osEPiRawReadIo(uint8_t* rdram, recomp_context* ctx) {
    gpr data_ptr = ctx->r6;
    if (data_ptr != 0) {
        MEM_W(0, data_ptr) = 0;
    }
    (void)rdram;
    ctx->r2 = 0;
}

extern "C" void boot___osEPiRawWriteIo(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    ctx->r2 = 0;
}

// Core1 contains a few unnamed leaf functions that N64Recomp could not emit
// because they have no symbols. These three are used heavily by camera, actor,
// and animation code. Their ROM implementations normalize degrees and sample a
// sine table; native libm is equivalent at the game-facing precision.
static void tooie_wrap_degrees(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    const float input = ctx->f12.fl;
    float result = std::fmod(input, 360.0f);
    if (result < 0.0f) {
        result += 360.0f;
    }
    ctx->f0.fl = result;
    tooie_diag::note_math("wrap_deg", input, result);
}

static void tooie_cos_degrees(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    constexpr float kDegreesToRadians = 0.01745329251994329577f;
    const float input = ctx->f12.fl;
    const float result = std::cos(input * kDegreesToRadians);
    ctx->f0.fl = result;
    tooie_diag::note_math("cos_deg", input, result);
}

static void tooie_sin_degrees(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    constexpr float kDegreesToRadians = 0.01745329251994329577f;
    const float input = ctx->f12.fl;
    const float result = std::sin(input * kDegreesToRadians);
    ctx->f0.fl = result;
    tooie_diag::note_math("sin_deg", input, result);
}

// Core2 unnamed button data accessor (__get_button_data_ptr(arg0, arg1)).
// MIPS:
//   sll   t6, a1, 3
//   subu  t6, t6, a1    ; t6 = a1 * 7
//   sll   t6, t6, 3     ; t6 = a1 * 56
//   addu  v0, a0, t6    ; v0 = a0 + a1 * 56
//   jr    ra
//   addiu v0, v0, 36    ; v0 = a0 + a1 * 56 + 0x24 (36)
static void tooie_helper_800BCCC0(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    const int32_t a0 = static_cast<int32_t>(ctx->r4);
    const int32_t a1 = static_cast<int32_t>(ctx->r5);
    const uint32_t t6 = static_cast<uint32_t>((a1 << 3) - a1) << 3; // a1 * 56
    const int32_t v0 = static_cast<int32_t>(static_cast<uint32_t>(a0) + t6 + 36u);
    ctx->r14 = static_cast<gpr>(static_cast<int32_t>(t6));
    ctx->r2 = static_cast<gpr>(v0);
}

// Core2 unnamed stick data accessor (__get_stick_data_ptr(arg0)).
// MIPS:
//   lbu   t6, 4(a0)     ; t6 = *(uint8_t*)(a0 + 4)
//   sll   t7, t6, 3
//   subu  t7, t7, t6    ; t7 = t6 * 7
//   sll   t7, t7, 3     ; t7 = t6 * 56
//   addu  v0, a0, t7    ; v0 = a0 + t6 * 56
//   jr    ra
//   addiu v0, v0, 36    ; v0 = a0 + t6 * 56 + 0x24 (36)
static void tooie_helper_800BCCD8(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t t6 = static_cast<uint32_t>(MEM_BU(4, ctx->r4));
    const uint32_t t7 = ((t6 << 3) - t6) << 3; // t6 * 56
    const int32_t a0 = static_cast<int32_t>(ctx->r4);
    const int32_t v0 = static_cast<int32_t>(static_cast<uint32_t>(a0) + t7 + 36u);
    ctx->r14 = static_cast<gpr>(t6);
    ctx->r15 = static_cast<gpr>(static_cast<int32_t>(t7));
    ctx->r2 = static_cast<gpr>(v0);
}

// LOOKUP_FUNC targets with no recompiled symbol (N64Recomp gaps). No-op stubs
// keep boot/runtime from aborting for still-unknown helpers only.
// DO NOT put real libultra entry points here — that overwrites the func_map and
// deadlocks the scheduler (e.g. osDpGetStatus/osYieldThread were wrongly stubbed).
template <int32_t Address>
static void lookup_gap_stub(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    (void)Address;
    ctx->r2 = 0;
}

static bool tooie_is_guest(uint32_t a);
static bool tooie_ud4_writer_window(uint8_t* rdram, int* glidx_out);
static bool tooie_ud5_p60_writer_window(uint8_t* rdram, int* glidx_out);
static bool tooie_ud5_bainput_window(uint8_t* rdram, int* glidx_out);


static void tooie_bainput_func_80097990(uint8_t* rdram, gpr self, uint32_t input_id, uint32_t val) {
    const uint32_t self_u = static_cast<uint32_t>(self);
    if (!tooie_is_guest(self_u)) {
        return;
    }
    const uint32_t input_addr = static_cast<uint32_t>(MEM_W(0xA4, kseg0_gpr(self_u)));
    if (!tooie_is_guest(input_addr)) {
        return;
    }
    const gpr input_p = kseg0_gpr(input_addr);
    const uint32_t old_word4 = static_cast<uint32_t>(MEM_W(0x4, input_p));
    const uint32_t mask = 1u << (input_id & 0x1Fu);
    const uint32_t new_word4 = (val != 0) ? (old_word4 | mask) : (old_word4 & ~mask);
    MEM_W(0x4, input_p) = static_cast<int32_t>(new_word4);
}

static gpr tooie_bainput_func_80097978(uint8_t* rdram, gpr self, uint32_t input_id) {
    const uint32_t self_u = static_cast<uint32_t>(self);
    if (!tooie_is_guest(self_u)) {
        return 0;
    }
    const uint32_t input_addr = static_cast<uint32_t>(MEM_W(0xA4, kseg0_gpr(self_u)));
    if (!tooie_is_guest(input_addr)) {
        return 0;
    }
    const gpr input_p = kseg0_gpr(input_addr);
    const uint32_t word4 = static_cast<uint32_t>(MEM_W(0x4, input_p));
    const uint32_t mask = 1u << (input_id & 0x1Fu);
    const uint32_t ret_v0 = word4 & mask;
    return static_cast<gpr>(static_cast<int64_t>(static_cast<int32_t>(ret_v0)));
}

// 0x800979D8 is a real static bainput helper. It has no public decomp symbol,
// but generated callers must observe its stateful contract.
static void tooie_bainput_func_800979D8(uint8_t* rdram, recomp_context* ctx) {
    const auto sign_extend_guest = [](int32_t value) -> gpr {
        return static_cast<gpr>(static_cast<int64_t>(value));
    };
    const gpr self = sign_extend_guest(static_cast<int32_t>(ctx->r4));
    const uint32_t button_id = static_cast<uint32_t>(ctx->r5);
    const uint32_t input_id = static_cast<uint32_t>(ctx->r6);

    int trace_glidx = -1;
    const bool trace = tooie_ud5_bainput_window(rdram, &trace_glidx);
    const uint32_t self_u = static_cast<uint32_t>(self);
    static uint32_t s_rt_979d8_seq = 0;
    const uint32_t cur_seq = ++s_rt_979d8_seq;
    uint32_t key = 0;
    uint32_t input = 0;
    uint32_t state = 0;
    uint32_t prev_state = 0;
    uint32_t button_map = 0;
    uint32_t button_word = 0;
    uint32_t enabled_word0 = 0;
    uint32_t enabled_word4 = 0;
    const uint32_t caller_ra = static_cast<uint32_t>(ctx->r31);
    if (trace && tooie_is_guest(self_u)) {
        const gpr self_p = kseg0_gpr(self_u);
        key = static_cast<uint32_t>(MEM_W(0x40, self_p));
        input = static_cast<uint32_t>(MEM_W(0xA4, self_p));
        if (tooie_is_guest(key)) {
            const gpr key_p = kseg0_gpr(key);
            prev_state = static_cast<uint32_t>(MEM_BU(0x18 + button_id, key_p));
            state = static_cast<uint32_t>(MEM_W(0x0, key_p));
        }
        if (tooie_is_guest(state)) {
            const gpr state_p = kseg0_gpr(state);
            button_map = static_cast<uint32_t>(MEM_BU(button_id, state_p));
            button_word = static_cast<uint32_t>(MEM_W(
                0x24 + (button_map * 56u), state_p));
        }
        if (tooie_is_guest(input)) {
            enabled_word0 = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(input)));
            enabled_word4 = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(input)));
        }
        fprintf(stderr,
                "[ud5-bainput-979d8] enter runtime=RT seq=%u glidx=%d self=%08x btn=%u in_id=%u "
                "key=%08x state=%08x prev=%u map=%u button_word=%08x "
                "input=%08x enabled_word0=%08x enabled_word4=%08x ra=%08x\n",
                cur_seq, trace_glidx, self_u, button_id, input_id, key, state, prev_state,
                button_map, button_word, input, enabled_word0, enabled_word4, caller_ra);
        fflush(stderr);
    }
    // The original helper delegates the input-state query to bakey_held
    // (0x800919F4).
    ctx->r4 = self;
    ctx->r5 = static_cast<gpr>(button_id);
    bakey_held(rdram, ctx);
    const bool held = ctx->r2 != 0;
    if (trace) {
        fprintf(stderr, "[ud5-host-step] runtime=RT seq=%u glidx=%d step=held_evaluated held=%u ret_r2=%08x\n",
                cur_seq, trace_glidx, held ? 1u : 0u, static_cast<uint32_t>(ctx->r2));
        fflush(stderr);
    }

    if (!held) {
        if (trace) {
            fprintf(stderr, "[ud5-host-step] runtime=RT seq=%u glidx=%d step=branch_held taken=0 (held==false) path=UNHELD\n", cur_seq, trace_glidx);
            fprintf(stderr, "[ud5-host-step] runtime=RT seq=%u glidx=%d step=calling_func_80097990_1\n", cur_seq, trace_glidx);
            fflush(stderr);
        }
        tooie_bainput_func_80097990(rdram, self, input_id, 1);
        ctx->r2 = 0;
        if (trace) {
            uint32_t post_w0 = 0, post_w4 = 0;
            if (tooie_is_guest(input)) {
                post_w0 = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(input)));
                post_w4 = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(input)));
            }
            fprintf(stderr,
                    "[ud5-enable-write-helper] runtime=RT seq=%u glidx=%d self=%08x input=%08x id=%u "
                    "arg=1 pc=800979B0 ra=%08x old0=%08x new0=%08x old4=%08x new4=%08x\n",
                    cur_seq, trace_glidx, self_u, input, input_id, caller_ra,
                    enabled_word0, post_w0, enabled_word4, post_w4);
            fprintf(stderr, "[ud5-host-step] runtime=RT seq=%u glidx=%d step=final_return v0=0\n", cur_seq, trace_glidx);
            fprintf(stderr,
                    "[ud5-bainput-979d8] leave runtime=RT seq=%u glidx=%d v0=0 (raw=00000000)\n",
                    cur_seq, trace_glidx);
            fflush(stderr);
        }
        return;
    }

    // Held path: query enabled mask via bainput_isEnabled (input+0)
    ctx->r4 = self;
    ctx->r5 = static_cast<gpr>(input_id);
    bainput_isEnabled(rdram, ctx);
    const gpr enabled = ctx->r2;

    if (trace) {
        fprintf(stderr,
                "[ud5-bainput-979d8] state runtime=RT seq=%u glidx=%d held=1 enabled=%08x\n",
                cur_seq, trace_glidx, static_cast<uint32_t>(enabled));
        fprintf(stderr, "[ud5-host-step] runtime=RT seq=%u glidx=%d step=isenabled_first_result enabled=%08x bit0=%u\n",
                cur_seq, trace_glidx, static_cast<uint32_t>(enabled), static_cast<uint32_t>(enabled) & 1u);
        fflush(stderr);
    }

    if (enabled == 0) {
        if (trace) {
            fprintf(stderr, "[ud5-host-step] runtime=RT seq=%u glidx=%d step=calling_func_80097990_0 (enabled==0) path=HELD_DISABLED\n", cur_seq, trace_glidx);
            fflush(stderr);
        }
        tooie_bainput_func_80097990(rdram, self, input_id, 0);
        if (trace) {
            uint32_t post_w0 = 0, post_w4 = 0;
            if (tooie_is_guest(input)) {
                post_w0 = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(input)));
                post_w4 = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(input)));
            }
            fprintf(stderr,
                    "[ud5-enable-write-helper] runtime=RT seq=%u glidx=%d self=%08x input=%08x id=%u "
                    "arg=0 pc=800979CC ra=%08x old0=%08x new0=%08x old4=%08x new4=%08x\n",
                    cur_seq, trace_glidx, self_u, input, input_id, caller_ra,
                    enabled_word0, post_w0, enabled_word4, post_w4);
            fflush(stderr);
        }
    } else {
        if (trace) {
            fprintf(stderr, "[ud5-host-step] runtime=RT seq=%u glidx=%d step=skip_enable (enabled!=0) path=HELD_ENABLED\n", cur_seq, trace_glidx);
            fflush(stderr);
        }
    }

    // Query active bit via func_80097978 (input+4)
    ctx->r2 = tooie_bainput_func_80097978(rdram, self, input_id);
    if (trace) {
        fprintf(stderr, "[ud5-host-step] runtime=RT seq=%u glidx=%d step=func_80097978_result v0=%u (raw=%08x)\n",
                cur_seq, trace_glidx, static_cast<uint32_t>(ctx->r2), static_cast<uint32_t>(ctx->r2));
        fprintf(stderr,
                "[ud5-bainput-979d8] leave runtime=RT seq=%u glidx=%d v0=%u (raw=%08x)\n",
                cur_seq, trace_glidx, static_cast<uint32_t>(ctx->r2), static_cast<uint32_t>(ctx->r2));
        fflush(stderr);
    }
}

// Truly unknown / not yet implemented helpers (not standard libultra).
static constexpr int32_t kLookupGapAddrs[] = {
    static_cast<int32_t>(0x80001998),
    static_cast<int32_t>(0x80031178),
    static_cast<int32_t>(0x80036670),
    static_cast<int32_t>(0x80036728),
};

static recomp_func_t* const kLookupGapFuncs[] = {
    lookup_gap_stub<static_cast<int32_t>(0x80001998)>,
    lookup_gap_stub<static_cast<int32_t>(0x80031178)>,
    lookup_gap_stub<static_cast<int32_t>(0x80036670)>,
    lookup_gap_stub<static_cast<int32_t>(0x80036728)>,
};

// Host libultra implementations (librecomp). Declared so the linker pulls them in
// and we can install them into the LOOKUP map at Tooie VRAMs.
extern "C" void osDpGetStatus_recomp(uint8_t* rdram, recomp_context* ctx);
extern "C" void osDpSetStatus_recomp(uint8_t* rdram, recomp_context* ctx);
extern "C" void osYieldThread_recomp(uint8_t* rdram, recomp_context* ctx);
extern "C" void __osPiGetAccess_recomp(uint8_t* rdram, recomp_context* ctx);
extern "C" void __osPiRelAccess_recomp(uint8_t* rdram, recomp_context* ctx);
extern "C" void osPfsInit_recomp(uint8_t* rdram, recomp_context* ctx);
extern "C" void osContInit_recomp(uint8_t* rdram, recomp_context* ctx);
extern "C" void osContStartReadData_recomp(uint8_t* rdram, recomp_context* ctx);
extern "C" void osContGetReadData_recomp(uint8_t* rdram, recomp_context* ctx);

// Diagnostic wrap only: call the shared runtime writer, then sample guest
// OSContPad + the func_80015D60 scale word. No behavioral change when
// TOOIE_DIAG_INPUT is unset.
static void tooie_osContGetReadData(uint8_t* rdram, recomp_context* ctx) {
    osContGetReadData_recomp(rdram, ctx);
    if (!tooie_diag::input_enabled()) {
        return;
    }
    const gpr pad = static_cast<gpr>(ctx->r4);
    const int8_t sx = MEM_B(2, pad);
    const int8_t sy = MEM_B(3, pad);
    const uint16_t buttons = static_cast<uint16_t>(MEM_HU(0, pad));
    const int32_t scale = MEM_W(0, kseg0_gpr(0x8007DB54u));
    const gpr remap = kseg0_gpr(0x80079AF8u);
    auto bits_f32 = [](int32_t bits) {
        float f = 0.0f;
        const uint32_t u = static_cast<uint32_t>(bits);
        std::memcpy(&f, &u, sizeof(f));
        return f;
    };
    const float remap_x = bits_f32(MEM_W(8, remap));
    const float remap_y = bits_f32(MEM_W(12, remap));
    tooie_diag::note_guest_pad(sx, sy, buttons, scale, remap_x, remap_y);
}
extern "C" void osContSetCh_recomp(uint8_t* rdram, recomp_context* ctx);
extern "C" void __osMotorAccess_recomp(uint8_t* rdram, recomp_context* ctx);
extern "C" void osMotorInit_recomp(uint8_t* rdram, recomp_context* ctx);
extern "C" void osEepromProbe_recomp(uint8_t* rdram, recomp_context* ctx);
extern "C" void osEepromRead_recomp(uint8_t* rdram, recomp_context* ctx);
extern "C" void osEepromWrite_recomp(uint8_t* rdram, recomp_context* ctx);
extern "C" void osEepromLongRead_recomp(uint8_t* rdram, recomp_context* ctx);
extern "C" void osEepromLongWrite_recomp(uint8_t* rdram, recomp_context* ctx);
extern "C" void osSendMesg_recomp(uint8_t* rdram, recomp_context* ctx);
extern "C" void osRecvMesg_recomp(uint8_t* rdram, recomp_context* ctx);

// Lightweight host stubs for SI/VI helpers without a librecomp body yet.
// RT64 consumes display lists immediately even while librecomp's synthetic
// DPC_STATUS_FREEZE bit is set. Tooie's scheduler assumes real RDP semantics:
// a frozen RDP cannot complete the next list until VI clears the freeze. Mask
// that bookkeeping-only bit for this game so task submission records the real
// completion state instead of losing the next frame-ending DP interrupt.
static void tooie_osDpGetStatus(uint8_t* rdram, recomp_context* ctx) {
    osDpGetStatus_recomp(rdram, ctx);
    ctx->r2 &= ~static_cast<gpr>(0x2u); // DPC_STATUS_FREEZE
}

static void tooie_osViGetCurrentLine(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    ctx->r2 = 0;
}
static void tooie_osSiAccessNoop(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    (void)ctx;
    ctx->r2 = 0;
}

// CIC-NUS-6105 C/R — X-Scale algorithm (mupen64plus n64_cic_nus_6105.c).
// chl/rsp are 30 nybbles; call with len = 30 (CHL_LEN-2).
static void n64_cic_nus_6105(const uint8_t* chl, uint8_t* rsp, int len) {
    static const uint8_t lut0[16] = {
        0x4, 0x7, 0xA, 0x7, 0xE, 0x5, 0xE, 0x1,
        0xC, 0xF, 0x8, 0xF, 0x6, 0x3, 0x6, 0x9,
    };
    static const uint8_t lut1[16] = {
        0x4, 0x1, 0xA, 0x7, 0xE, 0x5, 0xE, 0x1,
        0xC, 0x9, 0x8, 0x5, 0x6, 0x3, 0xC, 0x9,
    };
    uint8_t key = 0xB;
    const uint8_t* lut = lut0;
    for (int i = 0; i < len; i++) {
        rsp[i] = static_cast<uint8_t>((key + 5 * chl[i]) & 0xF);
        key = lut[rsp[i]];
        const int sgn = (rsp[i] >> 3) & 0x1;
        const int mag = ((sgn == 1) ? ~rsp[i] : rsp[i]) & 0x7;
        int mod = (mag % 3 == 1) ? sgn : 1 - sgn;
        if (lut == lut1 && (rsp[i] == 0x1 || rsp[i] == 0x9)) {
            mod = 1;
        }
        if (lut == lut1 && (rsp[i] == 0xB || rsp[i] == 0xE)) {
            mod = 0;
        }
        lut = (mod == 1) ? lut1 : lut0;
    }
}

// Host-side 64-byte PIF-RAM mirror for CIC/private PIF traffic. Normal
// controller polling is routed through the osCont* host hooks below.
static uint8_t g_pif_ram[64]{};

// CIC6105 / gldbDll use __osSiRawStartDma then osRecvMesg(BLOCK) on the SI event
// queue. A pure no-op never posts SI completion → permanent hang in gldbDll
// (stack gsworld→gsmaker→gldb). Maintain a PIF-RAM image + 6105 C/R so gldb
// asset decrypt gets a real key (wrong key → inflate crash).
static void tooie___osSiRawStartDma(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t direction = static_cast<uint32_t>(ctx->r4); // 0=PIF→RDRAM, 1=RDRAM→PIF
    const uint32_t buf = static_cast<uint32_t>(ctx->r5);
    const gpr dram = kseg0_gpr(buf);

    if (buf != 0u) {
        if (direction == 1u) {
            // RDRAM → PIF
            for (int i = 0; i < 64; i++) {
                g_pif_ram[i] = MEM_B(i, dram);
            }
            // Tooie CIC path (cic_send_challenge) packs challenge at +0x30 and often
            // leaves cmd as 0xFF/0xFE rather than standard bit 0x02. Detect either.
            const bool challenge_cmd = (g_pif_ram[0x3F] & 0x02u) != 0u;
            const bool cic_buffer = (buf == 0x8007D850u) || challenge_cmd;
            if (cic_buffer) {
                uint8_t chl[30];
                uint8_t rsp[30];
                for (int i = 0; i < 15; i++) {
                    chl[i * 2 + 0] = static_cast<uint8_t>((g_pif_ram[0x30 + i] >> 4) & 0xF);
                    chl[i * 2 + 1] = static_cast<uint8_t>(g_pif_ram[0x30 + i] & 0xF);
                }
                n64_cic_nus_6105(chl, rsp, 30); // CHL_LEN(0x20)-2
                // Match mupen process_cic_challenge: clear 0x2e/0x2f, repack response.
                g_pif_ram[0x2e] = 0;
                g_pif_ram[0x2f] = 0;
                for (int i = 0; i < 15; i++) {
                    g_pif_ram[0x30 + i] =
                        static_cast<uint8_t>((rsp[i * 2] << 4) | (rsp[i * 2 + 1] & 0xF));
                }
                g_pif_ram[0x3F] = static_cast<uint8_t>((g_pif_ram[0x3F] & ~0x02u) | 0x80u);
                if (tooie_diag::enabled()) {
                    static unsigned s_cic_log = 4;
                    if (s_cic_log > 0) {
                        --s_cic_log;
                        std::fprintf(stderr,
                                     "[tooie-diag] CIC6105 C/R on SI write buf=0x%08x cmd=0x%02x\n",
                                     buf, g_pif_ram[0x3F]);
                        std::fflush(stderr);
                    }
                }
            }
        } else {
            // PIF → RDRAM
            for (int i = 0; i < 64; i++) {
                MEM_B(i, dram) = g_pif_ram[i];
            }
        }
    }

    ultramodern::send_si_message();
    ctx->r2 = 0;
}

// __osGetSR_2 reads CP0 Status. N64Recomp cannot obtain the host's emulated CP0
// value here, but the scheduler requires the RCP interrupt bit to remain enabled;
// returning zero enters func_800149BC's intentional fatal DP-flush loop.
static void tooie___osGetSR(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    ctx->r2 = 0x1000; // SR_IBIT5
}

struct UltraLookupHook {
    int32_t vram;
    recomp_func_t* func;
    const char* name;
};

// osYieldThread_recomp is assert(false) in librecomp — use a host no-op instead.
static void tooie_osYieldThread(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    (void)ctx;
}

static const UltraLookupHook kUltraLookupHooks[] = {
    { static_cast<int32_t>(0x8001DCA0), tooie___osGetSR, "__osGetSR_2" },
    { static_cast<int32_t>(0x8002EF70), osDpSetStatus_recomp, "osDpSetStatus" },
    { static_cast<int32_t>(0x8002F240), tooie_osDpGetStatus, "osDpGetStatus" },
    { static_cast<int32_t>(0x8002FE70), tooie_osYieldThread, "osYieldThread" },
    { static_cast<int32_t>(0x8002FF60), tooie_osViGetCurrentLine, "osViGetCurrentLine" },
    // Keep controller polling and rumble on the same host path as BanjoRecomp.
    // Tooie's raw SI hook below is deliberately retained only for CIC-6105 PIF
    // traffic; ordinary input must reach RecompFrontend's profile callbacks.
    { static_cast<int32_t>(0x8002FF90), osContStartReadData_recomp, "osContStartReadData" },
    { static_cast<int32_t>(0x80030014), tooie_osContGetReadData, "osContGetReadData" },
    // JOY thread (func_800166DC): if osEepromProbe < EEPROM_TYPE_16K it forces
    // D_80079B90[0].errno = 8 and never calls osContGetReadData → title/save
    // select ignore pads. Must return host Eep16k (GameEntry.save_type).
    { static_cast<int32_t>(0x80030170), osEepromProbe_recomp, "osEepromProbe" },
    { static_cast<int32_t>(0x800301E0), osContInit_recomp, "osContInit" },
    { static_cast<int32_t>(0x80030550), osContSetCh_recomp, "osContSetCh" },
    { static_cast<int32_t>(0x80030640), __osMotorAccess_recomp, "__osMotorAccess" },
    { static_cast<int32_t>(0x800308B4), osMotorInit_recomp, "osMotorInit" },
    { static_cast<int32_t>(0x80030A10), osPfsInit_recomp, "osPfsInit" },
    { static_cast<int32_t>(0x8002DE70), osEepromLongRead_recomp, "osEepromLongRead" },
    { static_cast<int32_t>(0x8002DF00), osEepromLongWrite_recomp, "osEepromLongWrite" },
    { static_cast<int32_t>(0x80032B10), osEepromRead_recomp, "osEepromRead" },
    { static_cast<int32_t>(0x80032D60), osEepromWrite_recomp, "osEepromWrite" },
    { static_cast<int32_t>(0x800319F0), tooie_osSiAccessNoop, "__osSiGetAccess" },
    { static_cast<int32_t>(0x80031A34), tooie_osSiAccessNoop, "__osSiRelAccess" },
    { static_cast<int32_t>(0x80031A60), tooie___osSiRawStartDma, "__osSiRawStartDma" },
    { static_cast<int32_t>(0x80031B60), __osPiGetAccess_recomp, "__osPiGetAccess" },
    { static_cast<int32_t>(0x80031BA4), __osPiRelAccess_recomp, "__osPiRelAccess" },
};

// Tooie runtime currently ships the *decompressed* image used by N64Recomp.
// Original boot DMAs a compressed core1 blob from cart and inflates it — that
// path crashes when the ROM bytes are already plain MIPS. Replace the entry
// with a recomp-native load of core1 (mirror BK's boot___osInitialize_common patch).
extern "C" void func_80000450(uint8_t* rdram, recomp_context* ctx) {
    constexpr uint32_t kCore1Rom = 0x01E29B60u;
    constexpr int32_t kCore1Vram = static_cast<int32_t>(0x80012030u);
    constexpr uint32_t kCore1Size = 0x00031350u;

    // Preserve stack frame semantics lightly: original saves ra at sp-0x20.
    ctx->r29 = ADD32(ctx->r29, -0x20);
    MEM_W(0x14, ctx->r29) = ctx->r31;

    boot_osInitialize(rdram, ctx);

    // One-shot sparse IPL3/boot residue for func_8001DE64 (before core1).
    tooie_seed_ipl3_boot_residue(rdram);

    // Copy core1 image into RDRAM for data/rodata; register native funcs.
    recomp::do_rom_read(rdram, kCore1Vram, recomp::rom_base + kCore1Rom, kCore1Size);
    load_overlays(kCore1Rom, kCore1Vram, kCore1Size);

    // Enter core1 (same target as original jalr 0x80012030).
    recomp_func_t* core1_entry = recomp::overlays::get_func_by_section_rom_function_vram(
        kCore1Rom, static_cast<uint32_t>(kCore1Vram));
    if (core1_entry == nullptr) {
        // Fallback: address map after load_overlays.
        core1_entry = get_function(kCore1Vram);
    }
    if (core1_entry != nullptr) {
        core1_entry(rdram, ctx);
    } else {
        std::fprintf(stderr, "[tooie] boot: failed to resolve core1 entry 0x%08x\n",
                     static_cast<unsigned>(kCore1Vram));
    }

    // Re-install ultra LOOKUP hooks after load_overlays may have touched the map.
    for (const UltraLookupHook& hook : kUltraLookupHooks) {
        recomp::overlays::add_loaded_function(hook.vram, hook.func);
    }

    ctx->r31 = MEM_W(ctx->r29, 0x14);
    ctx->r29 = ADD32(ctx->r29, 0x20);
}

// void rom_read_word(u32 rom_offset, u32* out)
// Original does cart PIO via PI MMIO (0xB0000000 | offset). Host path uses ROM image.
extern "C" void rom_read_word(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t rom_offset = static_cast<uint32_t>(ctx->r4);
    const gpr out_ptr = kseg0_gpr(static_cast<uint32_t>(ctx->r5));
    const uint32_t physical = recomp::rom_base + (rom_offset & 0x0FFFFFFFu);
    if (out_ptr != 0) {
        recomp::do_rom_pio(rdram, out_ptr, physical);
    }
    ctx->r2 = 0;
}

// Fixed-bank loader (core2 etc.): original always DMA+inflate compressed blobs.
// With decompressed.us.z64 those offsets are already plain MIPS — skip inflate.
// Signature (MIPS ABI):
//   void func_80019EC0(s32 index, void* ovl_start, void* ovl_end,
//                      void* rom_start, void* rom_end, ...);
extern "C" void func_80019EC0(uint8_t* rdram, recomp_context* ctx) {
    // Caller stack layout (sp not yet adjusted by our body):
    //   0x10 rom_end, 0x14 text_start, 0x18 text_end, 0x1C rodata_start,
    //   0x20 data_end, 0x24 bss_start, 0x28 bss_end
    const uint32_t ovl_start = static_cast<uint32_t>(ctx->r5);
    const uint32_t rom_start = static_cast<uint32_t>(ctx->r7);
    const uint32_t rom_end = static_cast<uint32_t>(MEM_W(0x10, ctx->r29));
    const uint32_t bss_start = static_cast<uint32_t>(MEM_W(0x24, ctx->r29));
    const uint32_t bss_end = static_cast<uint32_t>(MEM_W(0x28, ctx->r29));

    if (rom_end <= rom_start || ovl_start == 0) {
        std::fprintf(stderr,
                     "[tooie] func_80019EC0: bad ranges rom=%x..%x vram=%x\n",
                     rom_start, rom_end, ovl_start);
        ctx->r2 = 0;
        return;
    }
    const uint32_t size = rom_end - rom_start;
    const gpr vram = kseg0_gpr(ovl_start);

    std::fprintf(stderr, "[tooie] func_80019EC0: load rom=0x%x size=0x%x -> vram=0x%x\n",
                 rom_start, size, ovl_start);

    recomp::do_rom_read(rdram, vram, recomp::rom_base + (rom_start & 0x0FFFFFFFu), size);
    load_overlays(rom_start & 0x0FFFFFFFu, static_cast<int32_t>(ovl_start), size);

    if (bss_start != 0 && bss_end > bss_start) {
        const uint32_t bss_size = bss_end - bss_start;
        for (uint32_t i = 0; i + 3 < bss_size; i += 4) {
            MEM_W(static_cast<int32_t>(i), kseg0_gpr(bss_start)) = 0;
        }
    }
    banjo::register_boot_lookup_stubs();
    ctx->r2 = 0;
}

// void rom_dma_read(void* vaddr, u32 rom_addr, u32 size)
// Original chunks via osPiStartDma + message queues. Host: direct ROM copy.
extern "C" void rom_dma_read(uint8_t* rdram, recomp_context* ctx) {
    // MEM_* requires KSEG0 addrs sign-extended into 64-bit gpr.
    const gpr dram_addr = kseg0_gpr(static_cast<uint32_t>(ctx->r4));
    const uint32_t rom_offset = static_cast<uint32_t>(ctx->r5) & 0x0FFFFFFFu;
    const uint32_t size = static_cast<uint32_t>(ctx->r6);
    if (tooie_diag::enabled()) {
        static unsigned s_romdma_log = 32;
        if (s_romdma_log > 0) {
            --s_romdma_log;
            std::fprintf(stderr,
                         "[tooie-diag] rom_dma_read vaddr=0x%08x rom=0x%08x size=0x%x\n",
                         static_cast<uint32_t>(ctx->r4), rom_offset, size);
            std::fflush(stderr);
        }
    }
    const auto rom = recomp::get_rom();
    if (size == 0 || dram_addr == 0) {
        ctx->r2 = 0;
        return;
    }
    if (rom_offset + size > rom.size()) {
        std::fprintf(stderr, "[tooie] rom_dma_read OOB rom=0x%x size=0x%x rom_size=0x%zx\n",
                     rom_offset, size, rom.size());
        ctx->r2 = -1;
        return;
    }
    recomp::do_rom_read(rdram, dram_addr, recomp::rom_base + rom_offset, size);
    ctx->r2 = 0;
}

// ---------------------------------------------------------------------------
// Overlay table + dynamic ovl_load (decompressed ROM image)
// ---------------------------------------------------------------------------
// table_addr@0x50E0 still points at compressed overlay_table base (0x1E899B0).
// On decompressed.us.z64 the usable table is at overlay_table_ROM_START.
static constexpr uint32_t kOvlTableRom = 0x01F00020u;
static constexpr uint32_t kOvlTableVram = 0x80137390u;
static constexpr uint32_t kOvlTableSize = 0x00000DE0u;
static constexpr uint32_t kAddr_D_80126730 = 0x80126730u;
static constexpr uint32_t kAddr_D_80126738 = 0x80126738u;
static constexpr uint32_t kAddr_D_80117C60 = 0x80117C60u;
static constexpr uint32_t kAddr_D_80117C64 = 0x80117C64u;
static constexpr uint32_t kAddr_D_80126CB8 = 0x80126CB8u;
static constexpr uint32_t kAddr_D_80126CBC = 0x80126CBCu;

// Overlay package layout (matches func_80082088 / overlay_processor):
// Header(16) + OverlayFields(0x28) + entrypoints[ne] + name + pad4 + relocs[nr]
// + pad4 + unkC*4 + pad16 → text.
static uint32_t overlay_text_offset(const uint8_t* pkg) {
    const uint16_t ne = (uint16_t(pkg[8]) << 8) | pkg[9];
    const uint16_t nr = (uint16_t(pkg[10]) << 8) | pkg[11];
    const uint16_t unk_c = (uint16_t(pkg[12]) << 8) | pkg[13];
    const uint8_t name_len = pkg[14];
    uint32_t off = 0x38u; // header + OverlayFields
    off += uint32_t(ne) * 4u;
    off += name_len;
    off = (off + 3u) & ~3u;
    off += uint32_t(nr) * 2u;
    off = (off + 3u) & ~3u;
    off += uint32_t(unk_c) * 4u;
    off = (off + 15u) & ~15u;
    return off;
}

// void func_800815CC(void) — init overlay loader globals.
// Only rewrite the ROM table base; keep the rest of the original recompiled body
// by re-running the same stores the asm does (counters already 0 in BSS).
extern "C" void func_800815CC(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    (void)ctx;
    // Decompressed image: use overlay_table_ROM_START, not compressed cart base from 0x50E0.
    MEM_W(0, kseg0_gpr(kAddr_D_80126730)) = static_cast<int32_t>(kOvlTableRom);
    MEM_W(0, kseg0_gpr(kAddr_D_80117C60)) = 0;
    MEM_W(0, kseg0_gpr(kAddr_D_80117C64)) = 0;
    MEM_W(0, kseg0_gpr(kAddr_D_80126CB8)) = 0;
    std::fprintf(stderr, "[tooie] func_800815CC: D_80126730=0x%08x\n", kOvlTableRom);
    std::fflush(stderr);
}

// Host ovl_load for the decompressed ROM image.
// Mirrors banjo-tooie-Recomp/src/core2/overlays.c (uncompressed path) and then
// registers native recomp functions at the actual heap text address.
extern "C" void ovl_load(uint8_t* rdram, recomp_context* ctx) {
    const int32_t overlay_index = static_cast<int32_t>(ctx->r4);
    const gpr arg2 = ctx->r6;

    uint32_t table_base = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(kAddr_D_80126730)));
    if (table_base == 0u || table_base == 0x1E899B0u) {
        table_base = kOvlTableRom;
        MEM_W(0, kseg0_gpr(kAddr_D_80126730)) = static_cast<int32_t>(table_base);
    }

    if (overlay_index <= 0) {
        ctx->r2 = 0;
        return;
    }

    const auto rom = recomp::get_rom();
    const uint32_t off_a = table_base + uint32_t(overlay_index - 1) * 4u;
    const uint32_t off_b = table_base + uint32_t(overlay_index) * 4u;
    if (off_b + 4u > rom.size()) {
        std::fprintf(stderr, "[tooie] ovl_load: table OOB index=%d base=0x%x\n",
                     overlay_index, table_base);
        ctx->r2 = 0;
        return;
    }
    auto be32 = [&](uint32_t o) -> uint32_t {
        return (uint32_t(rom[o]) << 24) | (uint32_t(rom[o + 1]) << 16) |
               (uint32_t(rom[o + 2]) << 8) | uint32_t(rom[o + 3]);
    };
    const uint32_t start_off = be32(off_a);
    const uint32_t end_off = be32(off_b);
    if (end_off <= start_off) {
        ctx->r2 = 0;
        return;
    }
    const uint32_t pkg_rom = table_base + start_off;
    const uint32_t pkg_size = end_off - start_off;
    if (pkg_rom + 16u > rom.size() || pkg_rom + pkg_size > rom.size()) {
        std::fprintf(stderr, "[tooie] ovl_load: pkg OOB idx=%d\n", overlay_index);
        ctx->r2 = 0;
        return;
    }

    const uint8_t* pkg = rom.data() + pkg_rom;
    const uint8_t flags = pkg[15];
    const uint16_t bss_units = (uint16_t(pkg[6]) << 8) | pkg[7];
    const int32_t bss_size = int32_t(int16_t(bss_units)) * 0x10;
    const uint32_t text_off = overlay_text_offset(pkg);
    const uint32_t text_size = (uint32_t((uint16_t(pkg[0]) << 8) | pkg[1])) * 0x10u;

    if (flags & 0x80u) {
        // Compressed packages need inflate+CRC/XOR. Loading them raw corrupts
        // their header and text, so require the supported decompressed image.
        std::fprintf(stderr,
                     "[tooie] ovl_load: idx=%d is compressed; decompressed.us.z64 required\n",
                     overlay_index);
        ctx->r2 = 0;
        return;
    }

    const int32_t alloc_size = int32_t(pkg_size) + (bss_size > 0 ? bss_size : 0);
    recomp_context call = *ctx;
    call.r4 = static_cast<gpr>(alloc_size);
    call.r5 = (arg2 || (flags & 8u) || MEM_W(0, kseg0_gpr(kAddr_D_80126CB8))) ? 1 : 0;
    LOOKUP_FUNC(0x8001ACEC)(rdram, &call); // heap_alloc_sided
    const gpr overlay_ptr = call.r2;
    if (overlay_ptr == 0) {
        std::fprintf(stderr, "[tooie] ovl_load: alloc failed idx=%d size=%d\n",
                     overlay_index, alloc_size);
        ctx->r2 = 0;
        return;
    }

    // Original order: heap_alloc → func_8001B3B0 → rom_dma_read → ovl_clear_bss.
    call = *ctx;
    call.r4 = overlay_ptr;
    LOOKUP_FUNC(0x8001B3B0)(rdram, &call);

    recomp::do_rom_read(rdram, overlay_ptr, recomp::rom_base + pkg_rom, pkg_size);

    call = *ctx;
    call.r4 = 0;
    call.r5 = overlay_ptr;
    LOOKUP_FUNC(0x80082380)(rdram, &call); // ovl_clear_bss

    const int32_t load_count = MEM_W(0, kseg0_gpr(kAddr_D_80117C60));
    MEM_W(load_count * 4, kseg0_gpr(kAddr_D_80126738)) = static_cast<int32_t>(overlay_ptr);
    // fields.load_index at +0x30
    MEM_H(0x30, overlay_ptr) = static_cast<int16_t>(load_count);
    MEM_W(0, kseg0_gpr(kAddr_D_80117C60)) = load_count + 1;

    // Register the heap copy for native dynamic LOOKUPs.  Keep fields.text_start
    // package-relative here; func_80082088 relocates it exactly once below.
    const gpr text_start = overlay_ptr + static_cast<gpr>(static_cast<int32_t>(text_off));

    uint32_t mapping_rom = pkg_rom + text_off;
    uint32_t mapping_size = text_size;
    if (overlay_index <= static_cast<int32_t>(kTooieOverlayCount)) {
        const TooieOverlayDispatchEntry& dispatch = g_tooie_overlay_dispatch[overlay_index];
        if (dispatch.entrypoints != nullptr && dispatch.size != 0u) {
            mapping_rom = dispatch.rom_addr;
            mapping_size = dispatch.size;
        }
    }

    if (text_size > 0 && text_off + text_size <= pkg_size) {
        // N64Recomp requires the full registered section range here. Package
        // text_size may omit trailing data (for example 0x230 vs section 0x240),
        // which otherwise makes load_overlays select no section at all.
        load_overlays(mapping_rom, static_cast<int32_t>(text_start), mapping_size);
        std::fprintf(stderr,
                     "[tooie] ovl_load: idx=%d pkg=0x%x +0x%x -> ram=0x%x text=0x%x map=0x%x\n",
                     overlay_index, pkg_rom, text_off,
                     static_cast<uint32_t>(text_start), text_size, mapping_size);
        std::fflush(stderr);
    }

    ctx->r2 = overlay_ptr;
}

// Host-aware unload. The game-side bookkeeping and syscall restoration stay the
// same, but librecomp must also forget native functions registered at this heap
// address before the allocation can be reused.
extern "C" void ovl_unload(uint8_t* rdram, recomp_context* ctx) {
    const gpr overlay_ptr = ctx->r4;
    if (overlay_ptr == 0) {
        ctx->r2 = 0;
        return;
    }

    const uint8_t flags = static_cast<uint8_t>(MEM_B(0x0F, overlay_ptr));
    if (flags & 0x04u) {
        ctx->r2 = 0;
        return;
    }
    const int32_t load_count = MEM_W(0, kseg0_gpr(kAddr_D_80117C60));
    const int32_t load_index = int16_t(MEM_H(0x30, overlay_ptr));
    if (load_count <= 0 || load_index < 0 || load_index >= load_count) {
        std::fprintf(stderr,
                     "[tooie] ovl_unload: invalid list state ptr=0x%x index=%d count=%d\n",
                     static_cast<uint32_t>(overlay_ptr), load_index, load_count);
        ctx->r2 = 0;
        return;
    }

    const bool reset_bss = (flags & 0x01u) != 0 && MEM_H(0x06, overlay_ptr) != 0;
    recomp_context call = *ctx;
    call.r4 = overlay_ptr;
    call.r5 = reset_bss ? 1 : 0;
    LOOKUP_FUNC(0x80082428)(rdram, &call); // ovl_reset_syscalls

    const int32_t text_start = MEM_W(0x34, overlay_ptr);
    uint32_t mapping_size = uint32_t(uint16_t(MEM_H(0x00, overlay_ptr))) * 0x10u;
    const uint32_t overlay_index = uint16_t(MEM_HU(0x2C, overlay_ptr));
    if (overlay_index > 0u && overlay_index <= kTooieOverlayCount) {
        const TooieOverlayDispatchEntry& dispatch = g_tooie_overlay_dispatch[overlay_index];
        if (dispatch.entrypoints != nullptr && dispatch.size != 0u) {
            mapping_size = dispatch.size;
        }
    }
    if (text_start != 0 && mapping_size != 0) {
        unload_overlays(text_start, mapping_size);
    }
    std::fprintf(stderr,
                 "[tooie] ovl_unload: idx=%u ptr=0x%x text=0x%x size=0x%x\n",
                 overlay_index, static_cast<uint32_t>(overlay_ptr),
                 static_cast<uint32_t>(text_start), mapping_size);
    std::fflush(stderr);

    const int32_t new_count = load_count - 1;
    const int32_t swapped = MEM_W(new_count * 4, kseg0_gpr(kAddr_D_80126738));
    MEM_W(load_index * 4, kseg0_gpr(kAddr_D_80126738)) = swapped;
    if (swapped != 0) {
        MEM_H(0x30, static_cast<gpr>(swapped)) = static_cast<int16_t>(load_index);
    }
    MEM_W(new_count * 4, kseg0_gpr(kAddr_D_80126738)) = 0;
    MEM_W(0, kseg0_gpr(kAddr_D_80117C60)) = new_count;

    call = *ctx;
    call.r4 = overlay_ptr;
    LOOKUP_FUNC(0x8001B084)(rdram, &call); // heap_free

    // The package heap block is freed; drop any cached fast-path entry so a
    // later syscall dispatch reloads the overlay instead of reusing the freed
    // region (which the heap may have reallocated to another overlay).
    tooie_invalidate_resident_overlay(overlay_ptr);

    ctx->r2 = 1;
}

// Moving a loaded block without atomically relocating librecomp's function map
// leaves stale native entrypoints. Explicit unloads are host-aware above; defer
// opportunistic heap compaction until a host relocation API is available.
extern "C" void defragment_overlays(uint8_t* rdram, recomp_context* ctx) {
    static bool s_reported = false;
    MEM_W(0, kseg0_gpr(kAddr_D_80126CBC)) = 0;
    if (!s_reported) {
        std::fprintf(stderr,
                     "[tooie] defragment_overlays: host-safe no-op (native mappings stay fixed)\n");
        std::fflush(stderr);
        s_reported = true;
    }
    (void)rdram;
    (void)ctx;
}

// Host-safe replacement for func_80082088: original polls PI MMIO (0xA4600010) and
// reads cart as KSEG1, which crash under recomp MEM_*. Keep entrypoint relocation
// and text_start bookkeeping; native code is already registered in ovl_load.
extern "C" void func_80082088(uint8_t* rdram, recomp_context* ctx) {
    const gpr overlay = ctx->r4;
    const gpr forced_text = ctx->r5; // a1: optional precomputed text base

    if (overlay == 0) {
        return;
    }

    // Build a host view of the package header and reproduce the original layout:
    // entrypoints, byte metadata, 16-bit relocations, 32-bit relocations, text.
    uint8_t hdr[16];
    for (int i = 0; i < 16; i++) {
        hdr[i] = static_cast<uint8_t>(MEM_BU(i, overlay));
    }
    const uint32_t text_off = overlay_text_offset(hdr);
    const int32_t num_entry = static_cast<int32_t>((hdr[8] << 8) | hdr[9]);
    const int32_t reloc16_count = static_cast<int16_t>((hdr[10] << 8) | hdr[11]);
    const int32_t reloc32_count = static_cast<int16_t>((hdr[12] << 8) | hdr[13]);
    const int32_t text_size = int32_t((uint16_t(hdr[0]) << 8) | hdr[1]) * 0x10;

    const gpr text_storage = overlay + static_cast<gpr>(static_cast<int32_t>(text_off));
    const gpr text_start =
        forced_text != 0 ? forced_text
                         : text_storage;
    // The decompressed packages retain link-time 0x80800000 addresses in their
    // relocation targets. Relocate by the load delta, not by the absolute heap
    // base used by the original compressed-package path.
    const gpr relocation_delta = SUB32(
        text_start, static_cast<gpr>(static_cast<int32_t>(0x80800000u)));

    // Relocate entrypoint table: package offsets → absolute VRAM.
    gpr ep = ADD32(overlay, 0x38);
    const gpr ep_end = ADD32(ep, S32(num_entry << 2));
    while (ep != ep_end) {
        const int32_t rel = MEM_W(0, ep);
        MEM_W(0, ep) = static_cast<int32_t>(ADD32(rel, text_start));
        ep = ADD32(ep, 4);
    }

    // Original does fields.text_start += text_start.
    MEM_W(0x34, overlay) = static_cast<int32_t>(
        ADD32(MEM_W(0x34, overlay), text_start));

    // The short relocation stream is XOR-obfuscated with the low halfword of a
    // ROM word selected by overlay index.  This remains true in the decompressed
    // runtime image; only the package payload compression was removed.
    const uint32_t overlay_index = uint16_t(MEM_HU(0x2C, overlay));
    const auto rom = recomp::get_rom();
    const uint32_t key_off = 0x40u + overlay_index * 4u;
    uint16_t candidate_xor_key = 0;
    if (key_off + 4u <= rom.size()) {
        candidate_xor_key = static_cast<uint16_t>((uint32_t(rom[key_off + 2]) << 8) |
                                                 uint32_t(rom[key_off + 3]));
    }

    const gpr entry_end = overlay + 0x38 + static_cast<gpr>(num_entry * 4);
    const gpr reloc16 = (entry_end + static_cast<gpr>(hdr[14]) + 3) & ~gpr(3);
    const gpr reloc32 = (reloc16 + static_cast<gpr>(reloc16_count * 2) + 3) & ~gpr(3);

    auto relocation_stream_valid = [&](uint16_t key) {
        for (int32_t i = 0; i < reloc16_count; ++i) {
            const uint16_t encoded = uint16_t(MEM_HU(i * 2, reloc16)) ^ key;
            if (uint32_t(encoded & ~3u) + 4u > uint32_t(text_size)) {
                return false;
            }
        }
        return true;
    };

    // The decompressed ROM has already decoded these entries. Fall back to the
    // cartridge XOR key only when the raw stream cannot describe this text.
    const bool raw_relocations_valid = relocation_stream_valid(0);
    const bool keyed_relocations_valid = relocation_stream_valid(candidate_xor_key);
    const uint16_t xor_key = (!raw_relocations_valid && keyed_relocations_valid)
                                 ? candidate_xor_key
                                 : 0;

    auto apply_relocations = [&](gpr list, int32_t count, int stride, bool decode_xor) {
        gpr pending_hi = 0;
        for (int32_t i = 0; i < count; ++i, list = ADD32(list, stride)) {
            uint32_t encoded = stride == 2 ? uint16_t(MEM_HU(0, list))
                                           : static_cast<uint32_t>(MEM_W(0, list));
            if (decode_xor) {
                encoded = uint16_t(encoded) ^ xor_key;
            }

            const uint32_t type = encoded & 3u;
            const gpr target = ADD32(text_storage,
                                     static_cast<int32_t>(encoded & ~3u));
            switch (type) {
                case 0: { // R_MIPS_32
                    MEM_W(0, target) = static_cast<int32_t>(
                        ADD32(MEM_W(0, target), relocation_delta));
                    break;
                }
                case 1: { // R_MIPS_26
                    const uint32_t instruction = static_cast<uint32_t>(MEM_W(0, target));
                    const uint32_t relocated = instruction +
                                               (static_cast<uint32_t>(relocation_delta) >> 2);
                    MEM_W(0, target) = static_cast<int32_t>(
                        (instruction & 0xFC000000u) | (relocated & 0x03FFFFFFu));
                    break;
                }
                case 2: // R_MIPS_HI16; paired with the next LO16.
                    pending_hi = target;
                    break;
                case 3: { // R_MIPS_LO16
                    const int32_t old_lo = int16_t(MEM_H(2, target));
                    const uint32_t sum = static_cast<uint32_t>(old_lo + relocation_delta);
                    MEM_H(2, target) = static_cast<int16_t>(sum);
                    if (pending_hi != 0) {
                        const int32_t old_hi = int16_t(MEM_H(2, pending_hi));
                        const uint32_t carry = (sum >> 15) & 1u;
                        const uint32_t upper = static_cast<uint32_t>(relocation_delta) >> 16;
                        MEM_H(2, pending_hi) = static_cast<int16_t>(
                            old_hi + static_cast<int32_t>(carry + upper));
                        pending_hi = 0;
                    }
                    break;
                }
            }
        }
    };

    if (reloc16_count > 0) {
        apply_relocations(reloc16, reloc16_count, 2, true);
    }
    if (reloc32_count > 0) {
        apply_relocations(reloc32, reloc32_count, 4, false);
    }

    // osWritebackDCache(text, size) + companion — host-safe via existing LOOKUP.
    recomp_context call = *ctx;
    call.r4 = text_start;
    call.r5 = static_cast<gpr>(text_size);
    LOOKUP_FUNC(0x8002E660)(rdram, &call);
    LOOKUP_FUNC(0x80030AB0)(rdram, &call);

    (void)rdram;
}

// sinf wrapper in core1 uses a non-standard return sequence:
//   move a1, ra; jal func_80013818; jr a1
// Direct calls emitted by N64Recomp do not materialize the MIPS return address
// in ctx->r31 because the host C call already models jal/return. Consequently
// the generated wrapper copies a zero ra into a1 and performs LOOKUP_FUNC(0).
// Keep the original constants and approximation helper, but return through the
// host call normally instead of replaying the synthetic MIPS return jump.
extern "C" void func_800137F4(uint8_t* rdram, recomp_context* ctx) {
    const float input = ctx->f12.fl;
    ctx->r1 = S32(0x8004 << 16);
    ctx->f14.u32l = MEM_W(ctx->r1, 0x16D4);
    ctx->r1 = S32(0x43B4 << 16);
    ctx->f16.u32l = ctx->r1;
    func_80013818(rdram, ctx);
    ctx->f0.fl = ctx->f2.fl;
    tooie_diag::note_math("800137F4", input, ctx->f0.fl);
}

// Companion core1 RA wrappers with the same non-standard return pattern.
// Generated bodies copy unmaterialized ctx->r31 and LOOKUP_FUNC the saved
// register (a1/a3). Strong-symbol interposition is sufficient: Clang
// RECOMP_FUNC is weak, and recomp_overlays.inl binds .func = func_80013xxx
// by symbol name, so LOOKUP_FUNC(vram) and direct calls both resolve here.
//
// 0x800137D4: move a1, ra; jal func_80013818; jr a1 / mov.s $f0, $f2
extern "C" void func_800137D4(uint8_t* rdram, recomp_context* ctx) {
    const float input = ctx->f12.fl;
    ctx->r1 = S32(0x8004 << 16);
    ctx->f14.u32l = MEM_W(ctx->r1, 0x16CC);
    ctx->f16.u32l = MEM_W(ctx->r1, 0x16D0);
    func_80013818(rdram, ctx);
    ctx->f0.fl = ctx->f2.fl;
    tooie_diag::note_math("800137D4", input, ctx->f0.fl);
}

// 0x80013A5C: move a3, ra; jal func_80013ABC (delay: lwc1 $f14);
//             lwc1 $f2 AFTER the helper (which clobbers $f2); jr a3 / sub.s
extern "C" void func_80013A5C(uint8_t* rdram, recomp_context* ctx) {
    const float input = ctx->f12.fl;
    ctx->r1 = S32(0x8004 << 16);
    ctx->f14.u32l = MEM_W(ctx->r1, 0x1700);
    func_80013ABC(rdram, ctx);
    ctx->r1 = S32(0x8004 << 16);
    ctx->f2.u32l = MEM_W(ctx->r1, 0x1704);
    ctx->f0.fl = ctx->f2.fl - ctx->f0.fl;
    tooie_diag::note_math("80013A5C", input, ctx->f0.fl);
}

// 0x80013A7C: move a3, ra; jal func_80013ABC; mtc1 90.0, $f2; jr a3 / sub.s
extern "C" void func_80013A7C(uint8_t* rdram, recomp_context* ctx) {
    const float input = ctx->f12.fl;
    ctx->r1 = S32(0x3E34 << 16);
    ctx->f14.u32l = ctx->r1;
    func_80013ABC(rdram, ctx);
    ctx->r1 = S32(0x42B4 << 16);
    ctx->f2.u32l = ctx->r1;
    ctx->f0.fl = ctx->f2.fl - ctx->f0.fl;
    tooie_diag::note_math("80013A7C", input, ctx->f0.fl);
}

// func_80013B70 is a 12-byte secondary entry listed in tooie.us.rev0.recomp.toml
// stubs[], so N64Recomp emits an empty body. Original MIPS:
//   80013B70: mtc1  a2, $f16
//   80013B74: j     80013B90          ; func_80013B7C + 0x14
//   80013B78: nop
// The shared tail expects $f16 to already hold the caller-supplied distance.
// Calling the normal func_80013B7C entry is NOT equivalent: that preamble
// overwrites $f16 with sqrt(y*y + x*x) and discards $a2. Real callers pass a
// precomputed hypot that can differ from that fresh sqrt by add-order /
// rounding (bastick: x*x+y*y; 13B7C: y*y then x*x). Strong-symbol
// interposition only — the overlay table already stores .func = func_80013B70
// by name.
extern "C" void func_80013B70(uint8_t* rdram, recomp_context* ctx) {
    const float input_y = ctx->f12.fl;
    const float input_x = ctx->f14.fl;
    const uint32_t caller_ra = static_cast<uint32_t>(ctx->r31);

    // mtc1 $a2, $f16
    ctx->f16.u32l = static_cast<uint32_t>(ctx->r6);
    const float input_mag = ctx->f16.fl;

    // 0x80013B90 shared tail
    ctx->f10.u32l = 0;
    ctx->f18.fl = ctx->f14.fl;

    const bool y_eq_zero = (ctx->f10.fl == ctx->f12.fl);
    if (!y_eq_zero) {
        // bc1fl delay: c.eq.s $f10, $f14
        const bool x_eq_zero = (ctx->f10.fl == ctx->f14.fl);
        if (!x_eq_zero) {
            // bc1fl delay: lui $at, 0x3E34
            ctx->r1 = S32(0x3E34 << 16);
            ctx->f14.u32l = ctx->r1;
            ctx->r7 = ctx->r31 | 0;
            ctx->f12.fl = ctx->f12.fl / ctx->f16.fl;
            func_80013ABC(rdram, ctx);
            ctx->r31 = ctx->r7 | 0;
            // bc1f is not "likely"; delay lui $at, 0x4334 always runs
            ctx->r1 = S32(0x4334 << 16);
            if (ctx->f18.fl < ctx->f10.fl) {
                ctx->f2.u32l = ctx->r1;
                ctx->f0.fl = ctx->f2.fl - ctx->f0.fl;
            }
            ctx->r1 = S32(0x43B4 << 16);
            if (ctx->f0.fl < ctx->f10.fl) {
                ctx->f2.u32l = ctx->r1;
                ctx->f0.fl = ctx->f2.fl + ctx->f0.fl;
            }
            tooie_diag::note_math("80013B70", input_y, ctx->f0.fl);
            tooie_diag::note_13b70(caller_ra, input_y, input_x, input_mag, ctx->f0.fl);
            return;
        }
        // y != 0, x == 0
        if (ctx->f10.fl < ctx->f12.fl) {
            ctx->r1 = S32(0x42B4 << 16); // 90.0
            ctx->f0.u32l = ctx->r1;
            tooie_diag::note_math("80013B70", input_y, ctx->f0.fl);
            tooie_diag::note_13b70(caller_ra, input_y, input_x, input_mag, ctx->f0.fl);
            return;
        }
        ctx->r1 = S32(0x4387 << 16); // 270.0
        ctx->f0.u32l = ctx->r1;
        tooie_diag::note_math("80013B70", input_y, ctx->f0.fl);
        tooie_diag::note_13b70(caller_ra, input_y, input_x, input_mag, ctx->f0.fl);
        return;
    }

    // y == 0
    ctx->r1 = S32(0x4334 << 16); // original bc1t delay-slot LUI (0x43340000)
    if (ctx->f10.fl <= ctx->f14.fl) {
        ctx->f0.fl = ctx->f10.fl; // 0.0
        tooie_diag::note_math("80013B70", input_y, ctx->f0.fl);
        tooie_diag::note_13b70(caller_ra, input_y, input_x, input_mag, ctx->f0.fl);
        return;
    }
    ctx->f0.u32l = ctx->r1; // 180.0
    tooie_diag::note_math("80013B70", input_y, ctx->f0.fl);
    tooie_diag::note_13b70(caller_ra, input_y, input_x, input_mag, ctx->f0.fl);
}

// Submit one unit of background TIDY work. The original uses a blocking send
// to a one-message queue, but host-side VI delivery can revisit MAIN before the
// lower-priority TIDY thread drains the previous request. Blocking here stalls
// every subsequent frame; duplicate TIDY requests are safely coalescible.
extern "C" void func_80013488(uint8_t* rdram, recomp_context* ctx) {
    recomp_func_t* should_submit = get_function(static_cast<int32_t>(0x80012530u));
    if (should_submit == nullptr) {
        ctx->r2 = 0;
        return;
    }

    should_submit(rdram, ctx);
    if (static_cast<int32_t>(ctx->r2) != 1) {
        return;
    }

    ctx->r4 = kseg0_gpr(0x800769E0u);
    ctx->r5 = 0;
    ctx->r6 = 0; // OS_MESG_NOBLOCK: retain at most one pending TIDY request.
    osSendMesg_recomp(rdram, ctx);

    if (static_cast<int32_t>(ctx->r2) != 0 && tooie_diag::enabled()) {
        static bool s_reported_coalesced_tidy = false;
        if (!s_reported_coalesced_tidy) {
            std::fprintf(stderr,
                         "[tooie-diag] TIDY queue full; coalescing duplicate request\n");
            std::fflush(stderr);
            s_reported_coalesced_tidy = true;
        }
    }
}

// Companion TIDY handshake used after MAIN finishes its side of the frame.
// Keep this one coalesced as well; waiting for the lower-priority worker here
// has the same host scheduling deadlock as the request queue above.
extern "C" void func_800134C4(uint8_t* rdram, recomp_context* ctx) {
    recomp_func_t* should_submit = get_function(static_cast<int32_t>(0x80012530u));
    if (should_submit == nullptr) {
        ctx->r2 = 0;
        return;
    }

    should_submit(rdram, ctx);
    if (static_cast<int32_t>(ctx->r2) != 1) {
        return;
    }

    ctx->r4 = kseg0_gpr(0x80076A00u);
    ctx->r5 = 0;
    ctx->r6 = 0; // OS_MESG_NOBLOCK
    osSendMesg_recomp(rdram, ctx);

    if (static_cast<int32_t>(ctx->r2) != 0 && tooie_diag::enabled()) {
        static bool s_reported_coalesced_tidy_reply = false;
        if (!s_reported_coalesced_tidy_reply) {
            std::fprintf(stderr,
                         "[tooie-diag] TIDY reply queue full; coalescing duplicate\n");
            std::fflush(stderr);
            s_reported_coalesced_tidy_reply = true;
        }
    }
}

// Scheduler handshake (func_80013D10): send event to D_800783D0 and, for event 3,
// block on D_80078438 until func_80014258 acknowledges it. This matches the game;
// the __osGetSR_2 hook above keeps the scheduler out of its fatal DP-flush loop.
static constexpr uint32_t kSchedEventQ = 0x800783D0u;
static constexpr uint32_t kSchedReplyQ = 0x80078438u;
static constexpr uint32_t kAddr_D_80078F0C = 0x80078F0Cu;

extern "C" void func_80013D10(uint8_t* rdram, recomp_context* ctx) {
    const int32_t msg = static_cast<int32_t>(ctx->r4);
    if (msg == 3) {
        MEM_W(0, kseg0_gpr(kAddr_D_80078F0C)) = 0x1E;
        static bool s_cleared_dp_freeze = false;
        if (!s_cleared_dp_freeze) {
            recomp_context call = *ctx;
            call.r4 = 0x4; // DPC_CLR_FREEZE
            osDpSetStatus_recomp(rdram, &call);
            s_cleared_dp_freeze = true;
        }
    }

    recomp_context call = *ctx;
    call.r4 = kseg0_gpr(kSchedEventQ);
    call.r5 = static_cast<gpr>(msg);
    call.r6 = 1; // OS_MESG_BLOCK
    osSendMesg_recomp(rdram, &call);
    const int32_t send_result = static_cast<int32_t>(call.r2);

    if (msg == 3) {
        recomp_context reply = *ctx;
        reply.r4 = kseg0_gpr(kSchedReplyQ);
        reply.r5 = 0;
        reply.r6 = 1; // OS_MESG_BLOCK
        osRecvMesg_recomp(rdram, &reply);
        std::fprintf(stderr, "[tooie] func_80013D10: event 3 scheduler ack\n");
        MEM_W(0, kseg0_gpr(kAddr_D_80078F0C)) = 0;
        std::fflush(stderr);
        tooie_diag::note_sched_ack();
    } else if (send_result != 0) {
        std::fprintf(stderr,
                     "[tooie] func_80013D10: scheduler queue full for event %d\n",
                     msg);
        std::fflush(stderr);
    }
}

// Rare anti-tamper / probe: original uses KSEG1 RDRAM (0xA030xxxx). recomp MEM_* only
// maps KSEG0 (0x80xxxxxx). Replay with cached addresses.
extern "C" void func_8001DE64(uint8_t* rdram, recomp_context* ctx) {
    ctx->r29 = ADD32(ctx->r29, -0x28);
    MEM_W(0x14, ctx->r29) = ctx->r31;
    MEM_W(0x28, ctx->r29) = ctx->r4;

    // Original: lui 0xA030; lw imm16 (signed). KSEG1 -> phys -> KSEG0 for MEM_*.
    constexpr uint32_t kProbeBase = 0xA0300000u;
    constexpr uint32_t kProbeAddr0 = kseg1_base_imm16_to_kseg0(kProbeBase, 0xB1F4u); // 0x802FB1F4
    constexpr uint32_t kProbeAddr1 = kseg1_base_imm16_to_kseg0(kProbeBase, 0xE1C0u); // 0x802FE1C0
    const int32_t word0 = MEM_W(0, kseg0_gpr(kProbeAddr0));
    const int32_t word1 = MEM_W(0, kseg0_gpr(kProbeAddr1));
    MEM_W(0x1C, ctx->r29) = word0;
    MEM_W(0x18, ctx->r29) = word1;

    const int32_t tv_type = MEM_W(0x300, kseg0_gpr(0x80000000u));
    if (tv_type == 0) {
        LOOKUP_FUNC(0x80012598)(rdram, ctx);
        ctx->r4 = ctx->r2;
        ctx->r5 = 0;
        LOOKUP_FUNC(0x8002EC20)(rdram, ctx);
    }

    const int32_t t1 = MEM_W(0x1C, ctx->r29);
    const int32_t t3 = MEM_W(0x18, ctx->r29);
    const int32_t combined = (t1 ^ 0xAD090010) | (t3 ^ 0xAD170014);
    MEM_B(-0x2487, kseg0_gpr(0x80080000u)) = static_cast<uint8_t>(combined);

    LOOKUP_FUNC(0x8001DEE8)(rdram, ctx);

    ctx->r31 = MEM_W(0x14, ctx->r29);
    ctx->r29 = ADD32(ctx->r29, 0x28);
}

// Per-guest-logic-tick demo-path sampler (TOOIE_DIAG_DEMO_PATH=1).
//
// The attract gameplay demo (mode==5) is driven by the glrecord overlay
// (overlay 698 entrypoint 0): each dispatch consumes one 6-byte recorded-pad
// sample from D_8012D5B0 and writes OSContPad. That dispatch is the TRUE
// recorded-input consumer and one guest logic tick during the walk.
//
// This discriminator resolves, on the first Wooded Hollow recorded-pad UP
// hold: tspd semantics vs final |vel_xz|, glrecord sample cadence per logic
// tick, accel ramp trajectory, and the local dt used by the position
// integrator.
extern "C" void tooie_demo_path_tick(uint8_t* rdram, uint64_t tick) {
    // Player 0 struct D_80135490[0]; its phys is at +0xC8.
    const gpr plist = kseg0_gpr(0x80135490u);
    const uint32_t ps_addr = static_cast<uint32_t>(MEM_W(0, plist));
    if (ps_addr == 0) {
        return;
    }
    const gpr ps = kseg0_gpr(ps_addr);

    auto bits_f32 = [](int32_t bits) -> float {
        float f = 0.0f;
        const uint32_t u = static_cast<uint32_t>(bits);
        std::memcpy(&f, &u, sizeof(f));
        return f;
    };

    // Mode/map: D_80127630.unk2 @ +0x2, map word @ +0x10.
    const int mode_v = int(MEM_H(0x2, kseg0_gpr(0x80127630u)));
    const int map_v = int(MEM_H(0x10, kseg0_gpr(0x80127630u)));
    if (mode_v != 5) {
        // Only the mode-5 attract gameplay demo is the selected clip.
        return;
    }

    // OSContPad for controller 0: buttons @ +0, stick_x @ +2, stick_y @ +3.
    const gpr pad = kseg0_gpr(0x80079B40u);
    const int8_t pad_x = int8_t(MEM_B(2, pad));
    const int8_t pad_y = int8_t(MEM_B(3, pad));

    // Remapped stick floats D_80079AF8+8/+C.
    const gpr remap = kseg0_gpr(0x80079AF8u);
    const float rmx = bits_f32(MEM_W(8, remap));
    const float rmy = bits_f32(MEM_W(12, remap));

    // Position pointer at ps+0xE4, yaw struct at +0xF8, physics at +0xC8,
    // bastick at +0x128.
    const uint32_t pos_addr = static_cast<uint32_t>(MEM_W(0xE4, ps));
    const uint32_t yaw_addr = static_cast<uint32_t>(MEM_W(0xF8, ps));
    const uint32_t phys_addr = static_cast<uint32_t>(MEM_W(0xC8, ps));
    const uint32_t stick_addr = static_cast<uint32_t>(MEM_W(0x120, ps)); // bs_state
    const uint32_t ba_stick_addr = static_cast<uint32_t>(MEM_W(0x128, ps));

    auto is_guest = [](uint32_t a) {
        return (a >= 0x80000000u) && ((a + 0x80000000u) < 0x00800000u) && ((a & 3u) == 0u);
    };

    float px = 0, py = 0, pz = 0;
    float yaw = 0, yaw_ideal = 0;
    float vx = 0, vy = 0, vz = 0, tspd = 0;
    float bstick_x = 0, bstick_y = 0, bstick_ang = 0, bstick_dist = 0;
    float mult = 0;
    float gx = 0, gy = 0, gz = 0;
    float tvx = 0, tvz = 0, phys_yaw = 0;
    float accel = 0, ptype = 0;
    float ppx = 0, ppy = 0, ppz = 0;
    int32_t bs_id = -1;
    if (is_guest(pos_addr)) {
        const gpr posp = kseg0_gpr(pos_addr);
        px = bits_f32(MEM_W(0x0, posp));
        py = bits_f32(MEM_W(0x4, posp));
        pz = bits_f32(MEM_W(0x8, posp));
    }
    if (is_guest(yaw_addr)) {
        const gpr yawp = kseg0_gpr(yaw_addr);
        yaw = bits_f32(MEM_W(0x0, yawp));
        yaw_ideal = bits_f32(MEM_W(0x4, yawp));
    }
    if (is_guest(phys_addr)) {
        const gpr ph = kseg0_gpr(phys_addr);
        ptype = bits_f32(MEM_W(0x00, ph));
        ppx = bits_f32(MEM_W(0x04, ph));
        ppy = bits_f32(MEM_W(0x08, ph));
        ppz = bits_f32(MEM_W(0x0C, ph));
        vx = bits_f32(MEM_W(0x10, ph));
        vy = bits_f32(MEM_W(0x14, ph));
        vz = bits_f32(MEM_W(0x18, ph));
        tvx = bits_f32(MEM_W(0x1C, ph));
        tvz = bits_f32(MEM_W(0x24, ph));
        accel = bits_f32(MEM_W(0x44, ph));
        phys_yaw = bits_f32(MEM_W(0x40, ph));
        tspd = bits_f32(MEM_W(0x3C, ph));
        mult = bits_f32(MEM_W(0x50, ph));
        gx = bits_f32(MEM_W(0x64, ph));
        gy = bits_f32(MEM_W(0x68, ph));
        gz = bits_f32(MEM_W(0x6C, ph));
    }
    if (is_guest(ba_stick_addr)) {
        const gpr st = kseg0_gpr(ba_stick_addr);
        bstick_x = bits_f32(MEM_W(0x54, st)); // bastick value[0]
        bstick_y = bits_f32(MEM_W(0x58, st)); // bastick value[1]
        bstick_ang = bits_f32(MEM_W(0x5C, st));
        bstick_dist = bits_f32(MEM_W(0x60, st));
    }
    if (is_guest(stick_addr)) {
        bs_id = static_cast<int32_t>(MEM_W(0, kseg0_gpr(stick_addr)));
    }

    // glrecord: D_8012D5B0 +0x0 = sample base, +0x8 = current index, +0xC = count.
    const gpr gl = kseg0_gpr(0x8012D5B0u);
    const uint32_t gl_base = static_cast<uint32_t>(MEM_W(0, gl));
    const int32_t gl_idx = static_cast<int32_t>(MEM_W(0x8, gl));
    const int32_t gl_count = static_cast<int32_t>(MEM_W(0xC, gl));
    int8_t g_rec_x = 0, g_rec_y = 0;
    uint16_t g_rec_btn = 0;
    if (is_guest(gl_base)) {
        const gpr base = kseg0_gpr(gl_base + static_cast<uint32_t>(gl_idx) * 6u);
        g_rec_x = int8_t(MEM_B(0, base));
        g_rec_y = int8_t(MEM_B(1, base));
        g_rec_btn = static_cast<uint16_t>(MEM_HU(2, base));
    }

    // Local movement dt: func_800D8FF8 -> D_8012C760 (gamespeed, min(dcount*1/60, 0.05)).
    const float gamespeed = bits_f32(MEM_W(0, kseg0_gpr(0x8012C760u)));
    const int32_t gfcnt = static_cast<int32_t>(MEM_W(0, kseg0_gpr(0x8012C764u)));

    // Per-tick position delta against previous sampled guest logic tick.
    static bool s_have_prev = false;
    static float s_prev_px = 0, s_prev_pz = 0;
    static int32_t s_prev_glidx = -1;
    static uint64_t s_emitted = 0;
    static bool s_moving = false;

    const float dx = s_have_prev ? px - s_prev_px : 0.0f;
    const float dz = s_have_prev ? pz - s_prev_pz : 0.0f;
    const float dposxz = std::sqrt(dx * dx + dz * dz);
    const float velxz = std::sqrt(vx * vx + vz * vz);
    const int32_t gl_adv = s_have_prev ? gl_idx - s_prev_glidx : 0;

    // Update prev state every guest tick (also during idle) so the walk-start
    // delta/cadence becomes measurable.
    if (s_have_prev) {
        s_prev_glidx = gl_idx;
    }
    s_have_prev = true;

    // First-straight window only: emit every guest tick once the recorded-pad
    // walk begins (gl_adv > 0), up to 240 ticks. Do not run into later analog
    // turns in the recorded demo.
    if (gl_adv > 0 || dposxz > 1.0f) {
        s_moving = true;
    }
    if (!s_moving) {
        return; // pre-walk idle: skip
    }
    if (s_emitted >= 240) {
        return;
    }
    ++s_emitted;

    std::fprintf(stderr,
                 "[demo-tick] tick=%llu map=%d glidx=%d adv=%d count=%d ptype=%.0f "
                 "sample=(%d,%d,%04x) pad=(%d,%d) remap=(%.2f,%.2f) "
                 "bstick=(%.2f,%.2f) bang=%.1f bdist=%.2f "
                 "ideal=%.1f yaw=%.1f phys_yaw=%.1f tspd=%.1f mult=%.4f accel=%.2f "
                 "vel=(%.1f,%.1f,%.1f) velxz=%.1f "
                 "tvel=(%.1f,%.1f,%.1f) tvelxz=%.1f "
                 "dpos=(%.2f,%.2f) dposxz=%.1f "
                 "pos=(%.1f,%.1f,%.1f) ppos=(%.1f,%.1f,%.1f) gs=%.5f fc=%d bs=%d goto=(%.1f,%.1f,%.1f) n=%llu\n",
                 static_cast<unsigned long long>(tick), map_v, gl_idx, gl_adv, gl_count, ptype,
                 (int)g_rec_x, (int)g_rec_y, (unsigned)g_rec_btn,
                 (int)pad_x, (int)pad_y, rmx, rmy,
                 bstick_x, bstick_y, bstick_ang, bstick_dist,
                 yaw_ideal, yaw, phys_yaw, tspd, mult, accel,
                 vx, vy, vz, velxz,
                 tvx, 0.0f, tvz, std::sqrt(tvx * tvx + tvz * tvz),
                 dx, dz, dposxz,
                 px, py, pz, ppx, ppy, ppz, gamespeed, gfcnt, bs_id, gx, gy, gz,
                 static_cast<unsigned long long>(s_emitted));
    std::fflush(stderr);

    s_prev_px = px;
    s_prev_pz = pz;
}

// Compact first-shot window (TOOIE_DIAG_SHOT=1). Keyed by glrecord sample,
// not host gfx. Dumps player + nearby actors around the first C-up fire.
extern "C" void tooie_shot_tick(uint8_t* rdram, uint64_t tick) {
    if (!tooie_diag::shot_enabled()) {
        return;
    }
    const gpr plist = kseg0_gpr(0x80135490u);
    const uint32_t ps_addr = static_cast<uint32_t>(MEM_W(0, plist));
    if (ps_addr == 0) {
        return;
    }
    const gpr ps = kseg0_gpr(ps_addr);
    auto bits_f32 = [](int32_t bits) -> float {
        float f = 0.0f;
        const uint32_t u = static_cast<uint32_t>(bits);
        std::memcpy(&f, &u, sizeof(f));
        return f;
    };
    auto is_guest = [](uint32_t a) {
        return (a >= 0x80000000u) && ((a + 0x80000000u) < 0x00800000u) && ((a & 3u) == 0u);
    };

    const int mode_v = int(MEM_H(0x2, kseg0_gpr(0x80127630u)));
    const int map_v = int(MEM_H(0x10, kseg0_gpr(0x80127630u)));
    if (mode_v != 5) {
        return;
    }

    const uint32_t pos_addr = static_cast<uint32_t>(MEM_W(0xE4, ps));
    const uint32_t yaw_addr = static_cast<uint32_t>(MEM_W(0xF8, ps));
    const uint32_t phys_addr = static_cast<uint32_t>(MEM_W(0xC8, ps));
    const uint32_t bs_addr = static_cast<uint32_t>(MEM_W(0x120, ps));
    float px = 0, py = 0, pz = 0, yaw = 0, yaw_ideal = 0, vx = 0, vz = 0, tspd = 0;
    int32_t bs_id = -1;
    if (is_guest(pos_addr)) {
        const gpr posp = kseg0_gpr(pos_addr);
        px = bits_f32(MEM_W(0x0, posp));
        py = bits_f32(MEM_W(0x4, posp));
        pz = bits_f32(MEM_W(0x8, posp));
    }
    if (is_guest(yaw_addr)) {
        const gpr yawp = kseg0_gpr(yaw_addr);
        yaw = bits_f32(MEM_W(0x0, yawp));
        yaw_ideal = bits_f32(MEM_W(0x4, yawp));
    }
    if (is_guest(phys_addr)) {
        const gpr ph = kseg0_gpr(phys_addr);
        vx = bits_f32(MEM_W(0x10, ph));
        vz = bits_f32(MEM_W(0x18, ph));
        tspd = bits_f32(MEM_W(0x3C, ph));
    }
    if (is_guest(bs_addr)) {
        bs_id = static_cast<int32_t>(MEM_W(0, kseg0_gpr(bs_addr)));
    }

    const gpr gl = kseg0_gpr(0x8012D5B0u);
    const uint32_t gl_base = static_cast<uint32_t>(MEM_W(0, gl));
    const int32_t gl_idx = static_cast<int32_t>(MEM_W(0x8, gl));
    const int32_t gl_count = static_cast<int32_t>(MEM_W(0xC, gl));
    int8_t rec_x = 0, rec_y = 0;
    uint16_t rec_btn = 0;
    if (is_guest(gl_base)) {
        const gpr base = kseg0_gpr(gl_base + static_cast<uint32_t>(gl_idx) * 6u);
        rec_x = int8_t(MEM_B(0, base));
        rec_y = int8_t(MEM_B(1, base));
        rec_btn = static_cast<uint16_t>(MEM_HU(2, base));
    }

    static uint16_t s_prev_btn = 0;
    static int32_t s_prev_glidx = -1;
    static int32_t s_fire_glidx = -1;
    static uint64_t s_fire_tick = 0;
    static bool s_done = false;
    static int s_after = 0;
    if (s_done) {
        return;
    }

    const bool cup_now = (rec_btn & 0x0008u) != 0;
    const bool cup_prev = (s_prev_btn & 0x0008u) != 0;
    const int32_t gl_adv = (s_prev_glidx < 0) ? 0 : (gl_idx - s_prev_glidx);
    if (s_fire_glidx < 0 && cup_now && !cup_prev) {
        s_fire_glidx = gl_idx;
        s_fire_tick = tick;
        std::fprintf(stderr,
                     "[shot-fire] FIRST_CUP_EDGE tick=%llu glidx=%d adv=%d sample=(%d,%d,%04x) "
                     "pos=(%.2f,%.2f,%.2f) velxz=%.2f yaw=%.2f ideal=%.2f tspd=%.1f bs=%d map=%d count=%d\n",
                     static_cast<unsigned long long>(tick), gl_idx, gl_adv,
                     (int)rec_x, (int)rec_y, (unsigned)rec_btn,
                     px, py, pz, std::sqrt(vx * vx + vz * vz),
                     yaw, yaw_ideal, tspd, bs_id, map_v, gl_count);
    }

    const bool in_window = (s_fire_glidx >= 0) && (gl_idx >= s_fire_glidx - 16) && (s_after <= 80);
    // Also pre-arm 16 samples before the known first fire (glidx 322) so we
    // capture the approach even before the edge is seen.
    const bool pre_window = (s_fire_glidx < 0) && (gl_idx >= 306) && (gl_idx <= 360);
    if (in_window || pre_window) {
        std::fprintf(stderr,
                     "[shot-tick] tick=%llu glidx=%d adv=%d btn=%04x stick=(%d,%d) "
                     "pos=(%.2f,%.2f,%.2f) vel=(%.2f,%.2f) velxz=%.2f yaw=%.2f ideal=%.2f "
                     "tspd=%.1f bs=%d map=%d\n",
                     static_cast<unsigned long long>(tick), gl_idx, gl_adv, (unsigned)rec_btn,
                     (int)rec_x, (int)rec_y, px, py, pz, vx, vz,
                     std::sqrt(vx * vx + vz * vz), yaw, yaw_ideal, tspd, bs_id, map_v);

        const gpr actors_vec_sym = kseg0_gpr(0x80136EE0u);
        const uint32_t vec_addr = static_cast<uint32_t>(MEM_W(0, actors_vec_sym));
        if (is_guest(vec_addr)) {
            const gpr vec = kseg0_gpr(vec_addr);
            const uint32_t elem_size = static_cast<uint32_t>(MEM_W(0x00, vec));
            const uint32_t begin = static_cast<uint32_t>(MEM_W(0x04, vec));
            const uint32_t end = static_cast<uint32_t>(MEM_W(0x08, vec));
            if (elem_size == 0x9Cu && is_guest(begin) && is_guest(end) && end >= begin) {
                const uint32_t count = (end - begin) / elem_size;
                for (uint32_t i = 0; i < count && i < 64; ++i) {
                    const uint32_t act_addr = begin + i * 0x9Cu;
                    const gpr act = kseg0_gpr(act_addr);
                    const float ax = bits_f32(MEM_W(0x04, act));
                    const float ay = bits_f32(MEM_W(0x08, act));
                    const float az = bits_f32(MEM_W(0x0C, act));
                    const float dx = ax - px;
                    const float dy = ay - py;
                    const float dz = az - pz;
                    const float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
                    // After fire, dump every actor so the intended target is
                    // not lost to a radius filter. Before fire, keep nearby.
                    if (s_fire_glidx < 0 && dist > 1200.0f) {
                        continue;
                    }
                    const float ry = bits_f32(MEM_W(0x48, act));
                    const uint32_t sys = static_cast<uint32_t>(MEM_W(0x10, act));
                    const uint32_t d0 = static_cast<uint32_t>(MEM_W(0x18, act));
                    const uint32_t idw = static_cast<uint32_t>(MEM_W(0x6C, act));
                    const uint32_t marker_id = idw & 0x1FFu;
                    std::fprintf(stderr,
                                 "[shot-act] tick=%llu glidx=%d i=%u act=%08x pos=(%.2f,%.2f,%.2f) "
                                 "d=%.2f ry=%.1f sys=%08x d0=%08x mid=%u idw=%08x\n",
                                 static_cast<unsigned long long>(tick), gl_idx, i, act_addr,
                                 ax, ay, az, dist, ry, sys, d0, marker_id, idw);
                }
            }
        }
        if (s_fire_glidx >= 0 && gl_idx >= s_fire_glidx) {
            ++s_after;
            if (s_after > 80) {
                s_done = true;
                std::fprintf(stderr, "[shot-fire] WINDOW_DONE tick=%llu fire_tick=%llu fire_glidx=%d\n",
                             static_cast<unsigned long long>(tick),
                             static_cast<unsigned long long>(s_fire_tick), s_fire_glidx);
            }
        }
    }

    s_prev_btn = rec_btn;
    s_prev_glidx = gl_idx;
}

extern "C" void tooie_shot_note_overlay(uint8_t* rdram, void* ctx_void,
                                       uint32_t ovl, uint32_t ep, int leaving) {
    if (!tooie_diag::shot_enabled()) {
        return;
    }
    auto* ctx = static_cast<recomp_context*>(ctx_void);
    // baeggsetup=126, baeggaim=127, baeggcursor=128, baeggfire=129,
    // cheggclockwork=534, cheggfire=535, chegggolden=536, chegggrenade=537,
    // cheggice=538, cheggnormal=539, cheggproximity=540, cheggsplode=552,
    // sueggmisc=837, suegg=841, sueggmove=842
    const bool interesting =
        (ovl == 126u) || (ovl == 127u) || (ovl == 128u) || (ovl == 129u) ||
        (ovl >= 534u && ovl <= 540u) || (ovl == 552u) ||
        (ovl == 837u) || (ovl == 841u) || (ovl == 842u);
    if (!interesting) {
        return;
    }
    auto bits_f32 = [](int32_t bits) -> float {
        float f = 0.0f;
        const uint32_t u = static_cast<uint32_t>(bits);
        std::memcpy(&f, &u, sizeof(f));
        return f;
    };
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t a1 = static_cast<uint32_t>(ctx->r5);
    const uint32_t a2 = static_cast<uint32_t>(ctx->r6);
    const uint32_t a3 = static_cast<uint32_t>(ctx->r7);
    const bool hit_probe = leaving && (ovl == 841u || ovl == 842u || ovl == 837u) &&
                           (ep == 3u || ep == 8u || ep == 9u || ep == 12u || ep == 13u ||
                            ep == 14u || ep == 23u);
    const int mode_v = int(MEM_H(0x2, kseg0_gpr(0x80127630u)));
    const int map_v = int(MEM_H(0x10, kseg0_gpr(0x80127630u)));
    const int stage_v = int(MEM_BU(0x0, kseg0_gpr(0x80135470u)));
    const int sub_v = int(MEM_BU(0x1, kseg0_gpr(0x80135470u)));
    int32_t egg_slot = -1;
    const uint32_t ps_addr = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(0x80135490u)));
    if ((ps_addr >= 0x80000000u) && ((ps_addr + 0x80000000u) < 0x00800000u) && ((ps_addr & 3u) == 0u)) {
        egg_slot = static_cast<int32_t>(MEM_W(0x64, kseg0_gpr(ps_addr)));
    }
    const uint32_t egg_tbl = 0x80117E20u;
    const int egg_enum = (egg_slot >= 0 && egg_slot < 8)
                             ? int(MEM_BU(egg_slot, kseg0_gpr(egg_tbl)))
                             : -1;
    const char* egg_name = "?";
    switch (egg_enum) {
    case 1: egg_name = "BLUE"; break;
    case 2: egg_name = "FIRE"; break;
    case 3: egg_name = "GRENADE"; break;
    case 4: egg_name = "CLOCKWORK"; break;
    case 5: egg_name = "ICE"; break;
    case 6: egg_name = "GOLDEN"; break;
    case 7: egg_name = "PROXIMITY"; break;
    default: break;
    }
    std::fprintf(stderr,
                 "[shot-ovl] %s ovl=%u ep=%u a0=%08x a1=%08x a2=%08x/%.3f a3=%08x/%.3f v0=%08x%s "
                 "mode=%d map=%d stage=%d sub=%d egg_slot=%d egg_enum=%d egg=%s\n",
                 leaving ? "leave" : "enter", ovl, ep, a0, a1, a2, bits_f32(int32_t(a2)),
                 a3, bits_f32(int32_t(a3)), static_cast<uint32_t>(ctx->r2),
                 (hit_probe && ctx->r2 != 0) ? " HIT" : "",
                 mode_v, map_v, stage_v, sub_v, egg_slot, egg_enum, egg_name);
    // projectile actor overlays: a0 is typically the actor.
    if ((ovl >= 534u && ovl <= 540u) || ovl == 552u) {
        if ((a0 >= 0x80000000u) && ((a0 + 0x80000000u) < 0x00800000u) && ((a0 & 3u) == 0u)) {
            const gpr act = kseg0_gpr(a0);
            const float ax = bits_f32(MEM_W(0x04, act));
            const float ay = bits_f32(MEM_W(0x08, act));
            const float az = bits_f32(MEM_W(0x0C, act));
            const float ry = bits_f32(MEM_W(0x48, act));
            const uint32_t idw = static_cast<uint32_t>(MEM_W(0x6C, act));
            const char* spawn_name = "?";
            switch (ovl) {
            case 534: spawn_name = "cheggclockwork"; break;
            case 535: spawn_name = "cheggfire"; break;
            case 536: spawn_name = "chegggolden"; break;
            case 537: spawn_name = "chegggrenade"; break;
            case 538: spawn_name = "cheggice"; break;
            case 539: spawn_name = "cheggnormal"; break;
            case 540: spawn_name = "cheggproximity"; break;
            case 552: spawn_name = "cheggsplode"; break;
            default: break;
            }
            std::fprintf(stderr,
                         "[shot-egg] %s ovl=%u ep=%u name=%s act=%08x pos=(%.2f,%.2f,%.2f) "
                         "ry=%.2f mid=%u actor_id=%u egg_slot=%d egg=%s map=%d sub=%d\n",
                         leaving ? "leave" : "enter", ovl, ep, spawn_name, a0, ax, ay, az, ry,
                         idw & 0x1FFu, idw & 0x1FFu, egg_slot, egg_name, map_v, sub_v);
        }
    }
    std::fflush(stderr);
}

// USER_DEMO_4 follow-camera window (TOOIE_DIAG_ATTRACT=1): CA8B4 first-seed
// recurrence trace at glidx 0..42 (idle slew + walk start). Logging only.
static bool tooie_ud4_cam_window(uint8_t* rdram, int* glidx_out) {
    if (!tooie_diag::attract_enabled()) {
        return false;
    }
    const int mode_v = int(MEM_H(0x2, kseg0_gpr(0x80127630u)));
    const int sub_v = int(MEM_BU(0x1, kseg0_gpr(0x80135470u)));
    if (mode_v != 5 || sub_v != 4) {
        return false;
    }
    const int32_t post_glidx = static_cast<int32_t>(MEM_W(0x8, kseg0_gpr(0x8012D5B0u)));
    const int32_t glidx = (post_glidx > 0) ? (post_glidx - 1) : 0;
    if (glidx_out) {
        *glidx_out = int(glidx);
    }
    return glidx >= 0 && glidx <= 42;
}

static float tooie_bits_f32(int32_t bits) {
    float f = 0.0f;
    const uint32_t u = static_cast<uint32_t>(bits);
    std::memcpy(&f, &u, sizeof(f));
    return f;
}

static bool tooie_is_guest(uint32_t a) {
    return (a >= 0x80000000u) && ((a + 0x80000000u) < 0x00800000u) && ((a & 3u) == 0u);
}

static uint32_t tooie_ud4_seq() {
    static uint32_t s = 0;
    return ++s;
}

static void tooie_ud4_dump_player(uint8_t* rdram, FILE* fp) {
    const uint32_t ps_addr = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(0x80135490u)));
    if (!tooie_is_guest(ps_addr)) {
        fprintf(fp, " player=none");
        return;
    }
    const gpr ps = kseg0_gpr(ps_addr);
    const uint32_t pos_addr = static_cast<uint32_t>(MEM_W(0xE4, ps));
    const uint32_t yaw_addr = static_cast<uint32_t>(MEM_W(0xF8, ps));
    const int32_t sel = static_cast<int32_t>(MEM_W(0x184, ps));
    float px = 0, py = 0, pz = 0, yaw = 0, ideal = 0;
    if (tooie_is_guest(pos_addr)) {
        const gpr posp = kseg0_gpr(pos_addr);
        px = tooie_bits_f32(MEM_W(0x0, posp));
        py = tooie_bits_f32(MEM_W(0x4, posp));
        pz = tooie_bits_f32(MEM_W(0x8, posp));
    }
    if (tooie_is_guest(yaw_addr)) {
        const gpr yawp = kseg0_gpr(yaw_addr);
        yaw = tooie_bits_f32(MEM_W(0x0, yawp));
        ideal = tooie_bits_f32(MEM_W(0x4, yawp));
    }
    uint32_t pxb = 0, pyb = 0, pzb = 0, yawb = 0, idb = 0;
    if (tooie_is_guest(pos_addr)) {
        const gpr posp = kseg0_gpr(pos_addr);
        pxb = static_cast<uint32_t>(MEM_W(0x0, posp));
        pyb = static_cast<uint32_t>(MEM_W(0x4, posp));
        pzb = static_cast<uint32_t>(MEM_W(0x8, posp));
    }
    if (tooie_is_guest(yaw_addr)) {
        const gpr yawp = kseg0_gpr(yaw_addr);
        yawb = static_cast<uint32_t>(MEM_W(0x0, yawp));
        idb = static_cast<uint32_t>(MEM_W(0x4, yawp));
    }
    fprintf(fp,
            " sel=%d pos=(%.6f,%.6f,%.6f) pos_bits=%08x,%08x,%08x "
            "yaw=%.6f ideal=%.6f yaw_bits=%08x ideal_bits=%08x",
            sel, px, py, pz, pxb, pyb, pzb, yaw, ideal, yawb, idb);
}

static uint32_t tooie_ud4_sel_rec(uint8_t* rdram) {
    const uint32_t ps_addr = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(0x80135490u)));
    if (!tooie_is_guest(ps_addr)) {
        return 0;
    }
    const int32_t sel = static_cast<int32_t>(MEM_W(0x184, kseg0_gpr(ps_addr)));
    if (sel < 0 || sel >= 8) {
        return 0;
    }
    const uint32_t rec =
        static_cast<uint32_t>(MEM_W(sel * 4, kseg0_gpr(0x80127708u)));
    return tooie_is_guest(rec) ? rec : 0;
}

// CA8B4 executed reads: record[+0x00,+0x04,+0x08] eye and [+0x18,+0x1C,+0x20] src2.
static void tooie_ud4_dump_ca8b4_reads(uint8_t* rdram, FILE* fp, uint32_t rec) {
    if (!tooie_is_guest(rec)) {
        fprintf(fp, " rec=none");
        return;
    }
    const gpr p = kseg0_gpr(rec);
    const uint32_t e0 = static_cast<uint32_t>(MEM_W(0x00, p));
    const uint32_t e1 = static_cast<uint32_t>(MEM_W(0x04, p));
    const uint32_t e2 = static_cast<uint32_t>(MEM_W(0x08, p));
    const uint32_t s0 = static_cast<uint32_t>(MEM_W(0x18, p));
    const uint32_t s1 = static_cast<uint32_t>(MEM_W(0x1C, p));
    const uint32_t s2 = static_cast<uint32_t>(MEM_W(0x20, p));
    fprintf(fp,
            " rec=%08x eye_in=%08x,%08x,%08x src2_in=%08x,%08x,%08x "
            "eye=(%.6f,%.6f,%.6f) src2=(%.6f,%.6f,%.6f)",
            rec, e0, e1, e2, s0, s1, s2,
            tooie_bits_f32(int32_t(e0)), tooie_bits_f32(int32_t(e1)),
            tooie_bits_f32(int32_t(e2)), tooie_bits_f32(int32_t(s0)),
            tooie_bits_f32(int32_t(s1)), tooie_bits_f32(int32_t(s2)));
}

static int tooie_ud4_ca8b4_ord(int glidx) {
    static int last = -1;
    static int ord = 0;
    if (glidx != last) {
        last = glidx;
        ord = 0;
    }
    return ++ord;
}

extern "C" void func_800CA628(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_8010FC38(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_8010FE14(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_800CA8B4(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_800E1A58(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_80112A84(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_80112D08(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_80110588(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_800EE7F8(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_80116218(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_80116024(uint8_t* rdram, recomp_context* ctx);
extern "C" void yaw_setIdeal(uint8_t* rdram, recomp_context* ctx);
static uint32_t s_rt_cam1c_tracked_val = 0xFFFFFFFFu;
static uint32_t s_rt_node34_tracked_val = 0xFFFFFFFFu;
static uint32_t s_rt_node3c_tracked_val = 0xFFFFFFFFu;
static uint32_t s_rt_src_mem_a = 0;
static uint32_t s_rt_src_base_a = 0;
static uint32_t s_rt_src_mem_a_bits = 0;
static unsigned s_rt_cam_upd_ord = 0;
static unsigned s_rt_efa4c_ord = 0;
static unsigned s_rt_10970_ord = 0;
static unsigned s_rt_fe34_ord = 0;
static int s_rt_heading_glidx = -1;

extern "C" void func_8010FDEC(uint8_t* rdram, recomp_context* ctx);
extern "C" void _plcamera_entrypoint_13(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_800EFA4C(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_80110970(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_8010FE34(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_800A9318(uint8_t* rdram, recomp_context* ctx);

static void tooie_check_node3c_mutation(uint8_t* rdram, const char* context, uint32_t pc) {
    int glidx = -1;
    if (!tooie_ud5_bainput_window(rdram, &glidx) && !tooie_ud5_p60_writer_window(rdram, &glidx)) {
        return;
    }
    if (glidx < 22 || glidx > 27) {
        return;
    }
    uint32_t node = 0;
    const uint32_t ps = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(0x80135490u)));
    if (tooie_is_guest(ps)) {
        const uint32_t sel = static_cast<uint32_t>(MEM_W(0x184, kseg0_gpr(ps)));
        if (sel <= 7) {
            node = static_cast<uint32_t>(MEM_W(sel * 4, kseg0_gpr(0x80127728u)));
        }
    }
    if (tooie_is_guest(s_rt_src_mem_a)) {
        const uint32_t latched = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(s_rt_src_mem_a)));
        if (latched != s_rt_src_mem_a_bits) {
            fprintf(stderr,
                    "[ud5-watch-src-mem-a] runtime=RT glidx=%d pc=%08x context=%s src_base=%08x src_mem_a=%08x old_bits=%08x (%.6f) new_bits=%08x (%.6f)\n",
                    glidx, pc, context, s_rt_src_base_a, s_rt_src_mem_a,
                    s_rt_src_mem_a_bits, tooie_bits_f32(int32_t(s_rt_src_mem_a_bits)),
                    latched, tooie_bits_f32(int32_t(latched)));
            fflush(stderr);
            s_rt_src_mem_a_bits = latched;
        }
    }
    if (!tooie_is_guest(node)) {
        return;
    }
    const uint32_t cur_val = static_cast<uint32_t>(MEM_W(0x3C, kseg0_gpr(node)));
    if (cur_val != s_rt_node3c_tracked_val) {
        fprintf(stderr,
                "[ud5-watch-node3c] runtime=RT glidx=%d pc=%08x context=%s node=%08x target=%08x old_val=%08x (%.6f) new_val=%08x (%.6f)\n",
                glidx, pc, context, node, node + 0x3Cu, s_rt_node3c_tracked_val, tooie_bits_f32(int32_t(s_rt_node3c_tracked_val)),
                cur_val, tooie_bits_f32(int32_t(cur_val)));
        fflush(stderr);
        s_rt_node3c_tracked_val = cur_val;
    }
}

static bool tooie_ud5_heading_window(uint8_t* rdram, int* glidx_out) {
    int glidx = -1;
    const bool win = tooie_ud5_bainput_window(rdram, &glidx) || tooie_ud5_p60_writer_window(rdram, &glidx);
    if (glidx_out) {
        *glidx_out = glidx;
    }
    return win && glidx >= 22 && glidx <= 27;
}

static void tooie_ud5_reset_heading_ords(int glidx) {
    if (glidx != s_rt_heading_glidx) {
        s_rt_heading_glidx = glidx;
        s_rt_cam_upd_ord = 0;
        s_rt_efa4c_ord = 0;
        s_rt_10970_ord = 0;
        s_rt_fe34_ord = 0;
    }
}

// === USER_DEMO_5 MAP=296 CLEAN SEMANTIC JOIN & CALL-PRESENCE MODULE (RT64) ===
static uint32_t s_rt_trace_epoch = 0;
static uint32_t s_rt_ud5_map296_epoch = 0;
static bool s_rt_in_map296 = false;
static uint64_t s_rt_logic_tick = 0;
static uint32_t s_rt_cam_update_ord = 0;
static uint32_t s_rt_cam_call_seq = 0;
static int s_rt_last_logged_rec_ord = -1;

static inline void tooie_rt_dump_checkpoint(uint8_t* rdram, const char* phase) {
    if (!s_rt_in_map296) return;
    uint32_t cam_obj = 0, cam_sub = 0, node = 0;
    const gpr p_cam_list = kseg0_gpr(0x80127728u);
    cam_obj = static_cast<uint32_t>(MEM_W(0, p_cam_list));
    if (tooie_is_guest(cam_obj)) {
        cam_sub = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(cam_obj)));
    }
    if (tooie_is_guest(cam_sub)) {
        node = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(cam_sub)));
    }
    uint32_t sub_80 = 0, sub_84 = 0, sub_88 = 0;
    if (tooie_is_guest(cam_sub)) {
        sub_80 = static_cast<uint32_t>(MEM_W(0x80, kseg0_gpr(cam_sub)));
        sub_84 = static_cast<uint32_t>(MEM_W(0x84, kseg0_gpr(cam_sub)));
        sub_88 = static_cast<uint32_t>(MEM_W(0x88, kseg0_gpr(cam_sub)));
    }
    uint32_t node_34 = 0, node_3c = 0, node_50 = 0, node_58 = 0;
    if (tooie_is_guest(node)) {
        node_34 = static_cast<uint32_t>(MEM_W(0x34, kseg0_gpr(node)));
        node_3c = static_cast<uint32_t>(MEM_W(0x3C, kseg0_gpr(node)));
        node_50 = static_cast<uint32_t>(MEM_W(0x50, kseg0_gpr(node)));
        node_58 = static_cast<uint32_t>(MEM_W(0x58, kseg0_gpr(node)));
    }
    fprintf(stderr,
            "[ud5-checkpoint] runtime=RT phase=%s tick=%llu rec_ord=%d cam_obj=%08x cam_sub=%08x node=%08x sub_80=%08x sub_84=%08x (%.6f) sub_88=%08x node_34=%08x node_3c=%08x node_50=%08x (%.6f) node_58=%08x (%.6f)\n",
            phase, (unsigned long long)s_rt_logic_tick, s_rt_last_logged_rec_ord,
            cam_obj, cam_sub, node, sub_80, sub_84, tooie_bits_f32(int32_t(sub_84)), sub_88,
            node_34, node_3c, node_50, tooie_bits_f32(int32_t(node_50)), node_58, tooie_bits_f32(int32_t(node_58)));
    fflush(stderr);
}

static inline bool tooie_rt_check_map_entry(uint8_t* rdram) {
    if (s_rt_trace_epoch == 0) {
        s_rt_trace_epoch = 0x54524143; // 'TRAC'
    }
    const int mode = int(MEM_H(0x2, kseg0_gpr(0x80127630u)));
    const int map = int(MEM_H(0x10, kseg0_gpr(0x80127630u)));
    const int stage = int(MEM_BU(0x0, kseg0_gpr(0x80135470u)));
    const int sub = int(MEM_BU(0x1, kseg0_gpr(0x80135470u)));
    const bool in_target = (mode == 5 && stage == 0 && sub == 5 && map == 296);
    if (in_target && !s_rt_in_map296) {
        s_rt_ud5_map296_epoch++;
        s_rt_in_map296 = true;
        s_rt_last_logged_rec_ord = -1;
        fprintf(stderr,
                "[ud5-epoch-entry] runtime=RT epoch=%u ud5epoch=%u stage=%d sub=%d map=%d mode=%d logic_tick=%llu\n",
                s_rt_trace_epoch, s_rt_ud5_map296_epoch, stage, sub, map, mode, (unsigned long long)s_rt_logic_tick);
        fflush(stderr);
        tooie_rt_dump_checkpoint(rdram, "map-entry");
    } else if (!in_target && s_rt_in_map296) {
        s_rt_in_map296 = false;
    }
    return in_target;
}

static inline void tooie_rt_on_logic_tick(uint8_t* rdram) {
    tooie_rt_check_map_entry(rdram);
    s_rt_logic_tick++;
    s_rt_cam_update_ord = 0;
    s_rt_cam_call_seq = 0;

    if (!s_rt_in_map296) return;

    const gpr gl = kseg0_gpr(0x8012D5B0u);
    const uint32_t gl_base = static_cast<uint32_t>(MEM_W(0, gl));
    const int32_t cur_idx = static_cast<int32_t>(MEM_W(0x8, gl));

    if (tooie_is_guest(gl_base)) {
        const int raw_ord = static_cast<int>(cur_idx);
        const int rec_ord = (raw_ord >= 1000) ? (raw_ord - 1012) : raw_ord;
        const uint32_t rec_ptr = gl_base + static_cast<uint32_t>(rec_ord) * 6u;
        const gpr base = kseg0_gpr(rec_ptr);
        const int8_t sx = int8_t(MEM_B(0, base));
        const int8_t sy = int8_t(MEM_B(1, base));
        const uint16_t btn = static_cast<uint16_t>(MEM_HU(2, base));
        const uint8_t b0 = uint8_t(MEM_B(0, base));
        const uint8_t b1 = uint8_t(MEM_B(1, base));
        const uint8_t b2 = uint8_t(MEM_B(2, base));
        const uint8_t b3 = uint8_t(MEM_B(3, base));
        const uint8_t b4 = uint8_t(MEM_B(4, base));
        const uint8_t b5 = uint8_t(MEM_B(5, base));

        if (rec_ord != s_rt_last_logged_rec_ord) {
            s_rt_last_logged_rec_ord = rec_ord;
            fprintf(stderr,
                    "[ud5-record] runtime=RT epoch=%u ud5epoch=%u stage=0 sub=5 map=296 rec_ord=%d rec_ptr=%08x raw6=%02x%02x%02x%02x%02x%02x stick=(%d,%d) btn=%04x logic_tick=%llu\n",
                    s_rt_trace_epoch, s_rt_ud5_map296_epoch, rec_ord, rec_ptr,
                    b0, b1, b2, b3, b4, b5,
                    int(sx), int(sy), unsigned(btn), (unsigned long long)s_rt_logic_tick);
            fflush(stderr);
            if (rec_ord == 0) {
                tooie_rt_dump_checkpoint(rdram, "r0-record-consume");
            }
        }
    }
}

static inline void tooie_rt_on_cam_call(uint8_t* rdram, recomp_context* ctx, uint32_t pc, const char* fn_name, uint32_t static_callsite) {
    if (!s_rt_in_map296) return;
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    const uint32_t callsite_pc = (ra >= 8) ? (ra - 8) : static_callsite;
    s_rt_cam_call_seq++;

    const gpr gl = kseg0_gpr(0x8012D5B0u);
    const uint32_t gl_base = static_cast<uint32_t>(MEM_W(0, gl));
    const int32_t cur_idx = static_cast<int32_t>(MEM_W(0x8, gl));
    const int raw_ord = static_cast<int>(cur_idx);
    const int rec_ord = (raw_ord >= 1000) ? (raw_ord - 1012) : raw_ord;

    char raw_hex[16] = "000000000000";
    if (tooie_is_guest(gl_base)) {
        const uint32_t rec_ptr = gl_base + static_cast<uint32_t>(rec_ord) * 6u;
        const gpr base = kseg0_gpr(rec_ptr);
        snprintf(raw_hex, sizeof(raw_hex), "%02x%02x%02x%02x%02x%02x",
                 uint8_t(MEM_B(0, base)), uint8_t(MEM_B(1, base)),
                 uint8_t(MEM_B(2, base)), uint8_t(MEM_B(3, base)),
                 uint8_t(MEM_B(4, base)), uint8_t(MEM_B(5, base)));
    }

    fprintf(stderr,
            "[ud5-cam-call] runtime=RT epoch=%u ud5epoch=%u stage=0 sub=5 map=296 rec_ord=%d raw6=%s logic_tick=%llu cam_ord=%u fn=%s pc=%08x callsite_pc=%08x ra=%08x call_seq=%u\n",
            s_rt_trace_epoch, s_rt_ud5_map296_epoch, rec_ord, raw_hex,
            (unsigned long long)s_rt_logic_tick, s_rt_cam_update_ord, fn_name, pc, callsite_pc, ra, s_rt_cam_call_seq);
    fflush(stderr);
}

static inline void tooie_rt_on_ca628(uint8_t* rdram, recomp_context* ctx) {
    if (!s_rt_in_map296) return;
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t a1 = static_cast<uint32_t>(ctx->r5);
    const uint32_t a2 = static_cast<uint32_t>(ctx->r6);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    const uint32_t callsite_pc = (ra >= 8) ? (ra - 8) : 0x8010FE24u;

    uint32_t old_1c = 0, src2_1 = 0;
    if (tooie_is_guest(a0)) {
        old_1c = static_cast<uint32_t>(MEM_W(0x1C, kseg0_gpr(a0)));
    }
    if (tooie_is_guest(a2)) {
        src2_1 = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(a2)));
    }

    const gpr gl = kseg0_gpr(0x8012D5B0u);
    const uint32_t gl_base = static_cast<uint32_t>(MEM_W(0, gl));
    const int32_t cur_idx = static_cast<int32_t>(MEM_W(0x8, gl));
    const int raw_ord = static_cast<int>(cur_idx);
    const int rec_ord = (raw_ord >= 1000) ? (raw_ord - 1012) : raw_ord;

    char raw_hex[16] = "000000000000";
    if (tooie_is_guest(gl_base)) {
        const uint32_t rec_ptr = gl_base + static_cast<uint32_t>(rec_ord) * 6u;
        const gpr base = kseg0_gpr(rec_ptr);
        snprintf(raw_hex, sizeof(raw_hex), "%02x%02x%02x%02x%02x%02x",
                 uint8_t(MEM_B(0, base)), uint8_t(MEM_B(1, base)),
                 uint8_t(MEM_B(2, base)), uint8_t(MEM_B(3, base)),
                 uint8_t(MEM_B(4, base)), uint8_t(MEM_B(5, base)));
    }

    fprintf(stderr,
            "[ud5-ca628] runtime=RT epoch=%u ud5epoch=%u stage=0 sub=5 map=296 rec_ord=%d raw6=%s logic_tick=%llu cam_ord=%u dst=%08x src=%08x src2=%08x old_1c=%08x (%.6f) src2_1=%08x (%.6f) callsite_pc=%08x ra=%08x\n",
            s_rt_trace_epoch, s_rt_ud5_map296_epoch, rec_ord, raw_hex,
            (unsigned long long)s_rt_logic_tick, s_rt_cam_update_ord, a0, a1, a2, old_1c, tooie_bits_f32(int32_t(old_1c)), src2_1, tooie_bits_f32(int32_t(src2_1)), callsite_pc, ra);
    fflush(stderr);
}

static void tooie_wrap_800a8e9c(uint8_t* rdram, recomp_context* ctx) {
    if (s_rt_in_map296) {
        s_rt_cam_update_ord++;
        if (s_rt_last_logged_rec_ord == 0 && s_rt_cam_update_ord == 1) {
            tooie_rt_dump_checkpoint(rdram, "r0-cam-entry");
        }
        tooie_rt_on_cam_call(rdram, ctx, 0x800A8E9Cu, "func_800A8E9C", 0x80800488u);
    }
    func_800A8E9C(rdram, ctx);
}

static void tooie_wrap_func_80116218(uint8_t* rdram, recomp_context* ctx) {
    if (s_rt_in_map296) {
        const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
        const uint32_t a1 = static_cast<uint32_t>(ctx->r5);
        uint32_t in_yaw = 0, node = 0, node_50 = 0, node_58 = 0;
        if (tooie_is_guest(a1)) {
            in_yaw = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(a1)));
        }
        if (tooie_is_guest(a0)) {
            node = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(a0)));
        }
        if (tooie_is_guest(node)) {
            node_50 = static_cast<uint32_t>(MEM_W(0x50, kseg0_gpr(node)));
            node_58 = static_cast<uint32_t>(MEM_W(0x58, kseg0_gpr(node)));
        }
        fprintf(stderr,
                "[ud5-116218-calc] runtime=RT tick=%llu rec_ord=%d s0=%08x s1=%08x in_yaw=%08x (%.6f) node=%08x node_50=%08x (%.6f) node_58=%08x (%.6f)\n",
                (unsigned long long)s_rt_logic_tick, s_rt_last_logged_rec_ord,
                a0, a1, in_yaw, tooie_bits_f32(int32_t(in_yaw)), node, node_50, tooie_bits_f32(int32_t(node_50)), node_58, tooie_bits_f32(int32_t(node_58)));
        fflush(stderr);
    }
    func_80116218(rdram, ctx);
    if (s_rt_in_map296) {
        const uint32_t a1 = static_cast<uint32_t>(ctx->r5);
        uint32_t new_yaw = 0;
        if (tooie_is_guest(a1)) {
            new_yaw = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(a1)));
        }
        fprintf(stderr,
                "[ud5-116380-store] runtime=RT tick=%llu rec_ord=%d addr=%08x input_f18=%08x (%.6f)\n",
                (unsigned long long)s_rt_logic_tick, s_rt_last_logged_rec_ord,
                a1 + 4u, new_yaw, tooie_bits_f32(int32_t(new_yaw)));
        fflush(stderr);
    }
}

static void tooie_wrap_800efa4c(uint8_t* rdram, recomp_context* ctx) {
    if (s_rt_in_map296) {
        tooie_rt_on_cam_call(rdram, ctx, 0x800EFA4Cu, "func_800EFA4C", 0x801109DCu);
    }
    func_800EFA4C(rdram, ctx);
}

static void tooie_wrap_80110970(uint8_t* rdram, recomp_context* ctx) {
    if (s_rt_in_map296) {
        tooie_rt_on_cam_call(rdram, ctx, 0x80110970u, "func_80110970", 0x8010FEA4u);
    }
    func_80110970(rdram, ctx);
}

static void tooie_wrap_8010fe34(uint8_t* rdram, recomp_context* ctx) {
    if (s_rt_in_map296) {
        tooie_rt_on_cam_call(rdram, ctx, 0x8010FE34u, "func_8010FE34", 0x800A934Cu);
    }
    func_8010FE34(rdram, ctx);
}

static void tooie_wrap_800a9318(uint8_t* rdram, recomp_context* ctx) {
    if (s_rt_in_map296) {
        tooie_rt_on_cam_call(rdram, ctx, 0x800A9318u, "func_800A9318", 0x800A512Cu);
    }
    func_800A9318(rdram, ctx);
}

static void tooie_wrap_8010fdec(uint8_t* rdram, recomp_context* ctx) {
    func_8010FDEC(rdram, ctx);
}

static void tooie_wrap_plcamera13(uint8_t* rdram, recomp_context* ctx) {
    _plcamera_entrypoint_13(rdram, ctx);
}

static void tooie_wrap_ca628(uint8_t* rdram, recomp_context* ctx) {
    if (s_rt_in_map296) {
        tooie_rt_on_ca628(rdram, ctx);
    }
    func_800CA628(rdram, ctx);
}

static void tooie_wrap_8010fc38(uint8_t* rdram, recomp_context* ctx) {
    if (s_rt_in_map296) {
        tooie_rt_on_cam_call(rdram, ctx, 0x8010FC38u, "func_8010FC38", 0x800A8EC8u);
    }
    func_8010FC38(rdram, ctx);
}

static void tooie_wrap_8010fe14(uint8_t* rdram, recomp_context* ctx) {
    if (s_rt_in_map296) {
        tooie_rt_on_cam_call(rdram, ctx, 0x8010FE14u, "func_8010FE14", 0x801109ECu);
    }
    func_8010FE14(rdram, ctx);
}

static void tooie_wrap_ca8b4(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win = tooie_ud4_cam_window(rdram, &glidx) ||
                     tooie_ud5_bainput_window(rdram, &glidx) ||
                     tooie_ud5_p60_writer_window(rdram, &glidx);
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t a1 = static_cast<uint32_t>(ctx->r5);
    const uint32_t a2 = static_cast<uint32_t>(ctx->r6);
    const uint32_t a3 = static_cast<uint32_t>(ctx->r7);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    int ord = 0;
    if (win) {
        static int s_ca8b4_n = 0;
        const int first = (s_ca8b4_n++ == 0) ? 1 : 0;
        ord = tooie_ud4_ca8b4_ord(glidx);
        fprintf(stderr,
                "[ud5-ca8b4] enter seq=%u glidx=%d ord=%d first=%d ra=%08x "
                "a0=%08x a1=%08x a2=%08x a3=%08x f4=%08x f6=%08x",
                tooie_ud4_seq(), glidx, ord, first, ra, a0, a1, a2, a3,
                ctx->f4.u32l, ctx->f6.u32l);
        tooie_ud4_dump_ca8b4_reads(rdram, stderr, a0);
        tooie_ud4_dump_player(rdram, stderr);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
    func_800CA8B4(rdram, ctx);
    if (win && tooie_is_guest(a1) && tooie_is_guest(a2)) {
        const uint32_t e0 = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(a1)));
        const uint32_t e1 = static_cast<uint32_t>(MEM_W(4, kseg0_gpr(a1)));
        const uint32_t e2 = static_cast<uint32_t>(MEM_W(8, kseg0_gpr(a1)));
        const uint32_t s0 = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(a2)));
        const uint32_t s1 = static_cast<uint32_t>(MEM_W(4, kseg0_gpr(a2)));
        const uint32_t s2 = static_cast<uint32_t>(MEM_W(8, kseg0_gpr(a2)));
        fprintf(stderr,
                "[ud5-ca8b4] leave seq=%u glidx=%d ord=%d eye_out=%08x,%08x,%08x "
                "src2_out=%08x,%08x,%08x eye=(%.6f,%.6f,%.6f) src2=(%.6f,%.6f,%.6f) "
                "f4=%08x f6=%08x\n",
                tooie_ud4_seq(), glidx, ord, e0, e1, e2, s0, s1, s2,
                tooie_bits_f32(int32_t(e0)), tooie_bits_f32(int32_t(e1)),
                tooie_bits_f32(int32_t(e2)), tooie_bits_f32(int32_t(s0)),
                tooie_bits_f32(int32_t(s1)), tooie_bits_f32(int32_t(s2)),
                ctx->f4.u32l, ctx->f6.u32l);
        fflush(stderr);
    }
}

static void tooie_wrap_e1a58(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win = tooie_ud4_cam_window(rdram, &glidx) ||
                     tooie_ud5_bainput_window(rdram, &glidx) ||
                     tooie_ud5_p60_writer_window(rdram, &glidx);
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t a1 = static_cast<uint32_t>(ctx->r5);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    if (win && tooie_is_guest(a0) && tooie_is_guest(a1)) {
        fprintf(stderr,
                "[ud5-e1a58] enter runtime=RT seq=%u glidx=%d ra=%08x a0=%08x a1=%08x "
                "v0=(%.6f,%.6f,%.6f) v1=(%.6f,%.6f,%.6f) v1_1_bits=%08x v1_2_bits=%08x\n",
                tooie_ud4_seq(), glidx, ra, a0, a1,
                tooie_bits_f32(MEM_W(0, kseg0_gpr(a0))), tooie_bits_f32(MEM_W(4, kseg0_gpr(a0))),
                tooie_bits_f32(MEM_W(8, kseg0_gpr(a0))),
                tooie_bits_f32(MEM_W(0, kseg0_gpr(a1))), tooie_bits_f32(MEM_W(4, kseg0_gpr(a1))),
                tooie_bits_f32(MEM_W(8, kseg0_gpr(a1))),
                static_cast<uint32_t>(MEM_W(4, kseg0_gpr(a1))),
                static_cast<uint32_t>(MEM_W(8, kseg0_gpr(a1))));
        fflush(stderr);
    }
    func_800E1A58(rdram, ctx);
    if (win && tooie_is_guest(a0) && tooie_is_guest(a1)) {
        fprintf(stderr,
                "[ud5-e1a58] leave runtime=RT seq=%u glidx=%d v0=(%.6f,%.6f,%.6f) v1=(%.6f,%.6f,%.6f) "
                "v1_1_bits=%08x (%.6f) v1_2_bits=%08x (%.6f)\n",
                tooie_ud4_seq(), glidx,
                tooie_bits_f32(MEM_W(0, kseg0_gpr(a0))), tooie_bits_f32(MEM_W(4, kseg0_gpr(a0))),
                tooie_bits_f32(MEM_W(8, kseg0_gpr(a0))),
                tooie_bits_f32(MEM_W(0, kseg0_gpr(a1))), tooie_bits_f32(MEM_W(4, kseg0_gpr(a1))),
                tooie_bits_f32(MEM_W(8, kseg0_gpr(a1))),
                static_cast<uint32_t>(MEM_W(4, kseg0_gpr(a1))),
                tooie_bits_f32(MEM_W(4, kseg0_gpr(a1))),
                static_cast<uint32_t>(MEM_W(8, kseg0_gpr(a1))),
                tooie_bits_f32(MEM_W(8, kseg0_gpr(a1))));
        fflush(stderr);
    }
}

static void tooie_wrap_yaw_set_ideal(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    if (tooie_ud4_cam_window(rdram, &glidx)) {
        fprintf(stderr, "[ud4-yawideal] enter seq=%u glidx=%d ra=%08x a0=%08x a1=%.6f bits=%08x\n",
                tooie_ud4_seq(), glidx, static_cast<uint32_t>(ctx->r31),
                static_cast<uint32_t>(ctx->r4), tooie_bits_f32(int32_t(ctx->r5)),
                static_cast<uint32_t>(ctx->r5));
        fflush(stderr);
    }
    yaw_setIdeal(rdram, ctx);
}

static int s_ud4_np_active = 0;
static int s_ud4_np_glidx = -1;
static uint32_t s_ud4_np_node = 0;
static uint32_t s_ud4_np_frame = 0;
static int s_ud4_np_call_n = 0;
static int s_ud4_in_set74 = 0;
static int s_ud4_in_set80 = 0;
static int s_ud4_set74_n = 0;
static int s_ud4_set80_n = 0;
static int s_ud4_seen_haunted = 0;

static int tooie_ud4_first_tick_env();
static int tooie_ud4_walkstart_env();

static bool tooie_ud4_haunted(uint8_t* rdram, int* glidx_out) {
    if (!tooie_diag::attract_enabled()) {
        return false;
    }
    const int mode_v = int(MEM_H(0x2, kseg0_gpr(0x80127630u)));
    const int map_v = int(MEM_H(0x10, kseg0_gpr(0x80127630u)));
    const int sub_v = int(MEM_BU(0x1, kseg0_gpr(0x80135470u)));
    const int32_t post_glidx = static_cast<int32_t>(MEM_W(0x8, kseg0_gpr(0x8012D5B0u)));
    const int32_t glidx = (post_glidx > 0) ? (post_glidx - 1) : 0;
    if (glidx_out) {
        *glidx_out = int(glidx);
    }
    if (map_v == 235 || (mode_v == 5 && sub_v == 4)) {
        s_ud4_seen_haunted = 1;
    }
    // Latch through the mode=3 warp-in → mode=2 node bind → mode=5 first
    // ncbapivot ep3 interval. map_v is 0 during that bind.
    return s_ud4_seen_haunted != 0;
}

static bool tooie_ud5_bainput_window(uint8_t* rdram, int* glidx_out) {
    if (!tooie_diag::attract_enabled()) {
        return false;
    }
    const gpr state = kseg0_gpr(0x80127630u);
    const gpr attract = kseg0_gpr(0x80135470u);
    const gpr sync = kseg0_gpr(0x8012D5B0u);
    const int mode_v = int(MEM_H(0x2, state));
    const int sub_v = int(MEM_BU(0x1, attract));
    const int32_t post_glidx = static_cast<int32_t>(MEM_W(0x8, sync));
    const int32_t raw_glidx = (post_glidx > 0) ? (post_glidx - 1) : 0;
    const int32_t glidx = (raw_glidx >= 1000) ? (raw_glidx - 1012) : raw_glidx;
    if (glidx_out) {
        *glidx_out = int(glidx);
    }
    return mode_v == 5 && sub_v == 5 && glidx >= 0 && glidx <= 35;
}

static bool tooie_ud4_writer_window(uint8_t* rdram, int* glidx_out) {
    const bool first_tick = tooie_ud4_first_tick_env() != 0;
    const bool walkstart = tooie_ud4_walkstart_env() != 0;
    if (!first_tick && !walkstart) {
        return false;
    }
    if (!tooie_ud4_haunted(rdram, glidx_out)) {
        return false;
    }
    if (walkstart) {
        const int glidx = glidx_out ? *glidx_out : -1;
        return glidx >= 36 && glidx <= 42;
    }
    return true;
}

static void tooie_ud4_np_call(const char* name, uint32_t addr, uint8_t* rdram,
                             recomp_context* ctx, int entering);

static void tooie_ud4_dump_vec3(uint8_t* rdram, FILE* fp, const char* tag, uint32_t addr) {
    if (!tooie_is_guest(addr)) {
        fprintf(fp, " %s=none", tag);
        return;
    }
    const gpr p = kseg0_gpr(addr);
    fprintf(fp, " %s=%08x,%08x,%08x", tag,
            static_cast<uint32_t>(MEM_W(0, p)),
            static_cast<uint32_t>(MEM_W(4, p)),
            static_cast<uint32_t>(MEM_W(8, p)));
}

static void tooie_ud4_dump_node_fields(uint8_t* rdram, FILE* fp, uint32_t node) {
    if (!tooie_is_guest(node)) {
        fprintf(fp, " node=none");
        return;
    }
    const gpr n = kseg0_gpr(node);
    const uint32_t p4 = static_cast<uint32_t>(MEM_W(0x4, n));
    const uint32_t p28 = static_cast<uint32_t>(MEM_W(0x28, n));
    const uint32_t p30 = static_cast<uint32_t>(MEM_W(0x30, n));
    const uint32_t p5c = static_cast<uint32_t>(MEM_W(0x5C, n));
    uint32_t p5c0 = 0;
    if (tooie_is_guest(p5c)) {
        p5c0 = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(p5c)));
    }
    uint32_t h2e = 0;
    if (tooie_is_guest(p4)) {
        h2e = static_cast<uint16_t>(MEM_HU(0x2E, kseg0_gpr(p4)));
    }
    const uint32_t p14 = static_cast<uint32_t>(MEM_W(0x14, n));
    const uint32_t p3c = static_cast<uint32_t>(MEM_W(0x3C, n));
    const uint32_t p4c = static_cast<uint32_t>(MEM_W(0x4C, n));
    const uint32_t p60 = static_cast<uint32_t>(MEM_W(0x60, n));
    fprintf(fp, " node=%08x +4=%08x +14=%08x +28=%08x +30=%08x +3c=%08x +4c=%08x +5c=%08x +60=%08x *5c=%08x +4.2e=%04x",
            node, p4, p14, p28, p30, p3c, p4c, p5c, p60, p5c0, h2e);
    tooie_ud4_dump_vec3(rdram, fp, "n74", node + 0x74u);
    tooie_ud4_dump_vec3(rdram, fp, "n80", node + 0x80u);
    if (tooie_is_guest(p30)) {
        tooie_ud4_dump_vec3(rdram, fp, "tgt", p30 + 0x4u);
    }
    if (tooie_is_guest(p14)) {
        const gpr p = kseg0_gpr(p14);
        fprintf(fp, " p14c=%08x p14_10=%08x",
                static_cast<uint32_t>(MEM_W(0xC, p)),
                static_cast<uint32_t>(MEM_W(0x10, p)));
    }
    if (tooie_is_guest(p3c)) {
        const gpr p = kseg0_gpr(p3c);
        fprintf(fp, " p3c0=%08x p3c4=%08x p3c8=%08x p3c10=%02x p3c11=%02x p3c12=%02x p3c1c=%02x",
                static_cast<uint32_t>(MEM_W(0x0, p)),
                static_cast<uint32_t>(MEM_W(0x4, p)),
                static_cast<uint32_t>(MEM_W(0x8, p)),
                static_cast<uint32_t>(MEM_BU(0x10, p)),
                static_cast<uint32_t>(MEM_BU(0x11, p)),
                static_cast<uint32_t>(MEM_BU(0x12, p)),
                static_cast<uint32_t>(MEM_BU(0x1C, p)));
    }
    if (tooie_is_guest(p4c)) {
        fprintf(fp, " p4cC=%08x", static_cast<uint32_t>(MEM_W(0xC, kseg0_gpr(p4c))));
    }
    if (tooie_is_guest(p60)) {
        const gpr p = kseg0_gpr(p60);
        fprintf(fp, " p60_0=%08x p60_4=%08x p60_8=%08x",
                static_cast<uint32_t>(MEM_W(0x0, p)),
                static_cast<uint32_t>(MEM_W(0x4, p)),
                static_cast<uint32_t>(MEM_W(0x8, p)));
    }
}

static void tooie_ud4_dump_p60_object(uint8_t* rdram, FILE* fp, uint32_t node) {
    if (!tooie_is_guest(node)) {
        fprintf(fp, " p60=none");
        return;
    }
    const uint32_t p60 = static_cast<uint32_t>(MEM_W(0x60, kseg0_gpr(node)));
    if (!tooie_is_guest(p60)) {
        fprintf(fp, " p60=%08x object=none", p60);
        return;
    }
    const gpr p = kseg0_gpr(p60);
    fprintf(fp,
            " p60=%08x p60_0=%08x p60_4=%08x p60_8=%08x p60_c=%04x p60_e=%04x"
            " p60_10=%08x p60_14=%08x p60_18=%08x p60_1c=%08x p60_20=%08x",
            p60,
            static_cast<uint32_t>(MEM_W(0x00, p)),
            static_cast<uint32_t>(MEM_W(0x04, p)),
            static_cast<uint32_t>(MEM_W(0x08, p)),
            static_cast<uint32_t>(MEM_HU(0x0C, p)),
            static_cast<uint32_t>(MEM_HU(0x0E, p)),
            static_cast<uint32_t>(MEM_W(0x10, p)),
            static_cast<uint32_t>(MEM_W(0x14, p)),
            static_cast<uint32_t>(MEM_W(0x18, p)),
            static_cast<uint32_t>(MEM_W(0x1C, p)),
            static_cast<uint32_t>(MEM_W(0x20, p)));
}

static void tooie_ud4_dump_np_frame(uint8_t* rdram, FILE* fp) {
    if (!tooie_is_guest(s_ud4_np_frame)) {
        fprintf(fp, " frame=none");
        return;
    }
    const gpr f = kseg0_gpr(s_ud4_np_frame);
    fprintf(fp, " frame=%08x sp38=%08x sp3c=%08x sp40=%08x sp44=%08x",
            s_ud4_np_frame,
            static_cast<uint32_t>(MEM_W(0x38, f)),
            static_cast<uint32_t>(MEM_W(0x3C, f)),
            static_cast<uint32_t>(MEM_W(0x40, f)),
            static_cast<uint32_t>(MEM_W(0x44, f)));
    tooie_ud4_dump_vec3(rdram, fp, "sp28", s_ud4_np_frame + 0x28u);
    tooie_ud4_dump_vec3(rdram, fp, "sp34", s_ud4_np_frame + 0x34u);
    tooie_ud4_dump_vec3(rdram, fp, "sp48", s_ud4_np_frame + 0x48u);
    tooie_ud4_dump_vec3(rdram, fp, "sp54", s_ud4_np_frame + 0x54u);
    tooie_ud4_dump_vec3(rdram, fp, "sp60", s_ud4_np_frame + 0x60u);
    tooie_ud4_dump_vec3(rdram, fp, "sp6c", s_ud4_np_frame + 0x6Cu);
}

static void tooie_ud4_dump_mode(uint8_t* rdram, FILE* fp, int glidx) {
    const int mode_v = int(MEM_H(0x2, kseg0_gpr(0x80127630u)));
    const int map_v = int(MEM_H(0x10, kseg0_gpr(0x80127630u)));
    const int stage_v = int(MEM_BU(0x0, kseg0_gpr(0x80135470u)));
    const int sub_v = int(MEM_BU(0x1, kseg0_gpr(0x80135470u)));
    fprintf(fp, " glidx=%d mode=%d map=%d stage=%d sub=%d", glidx, mode_v, map_v, stage_v, sub_v);
}

static void tooie_wrap_func_80110770(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win = tooie_ud4_writer_window(rdram, &glidx) || (s_ud4_np_active != 0);
    const uint32_t node = static_cast<uint32_t>(ctx->r4);
    const uint32_t src = static_cast<uint32_t>(ctx->r5);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    int ord = 0;
    if (win) {
        ord = ++s_ud4_set74_n;
        fprintf(stderr,
                "[ud4-set74] enter ord=%d seq=%u ra=%08x node=%08x src=%08x",
                ord, tooie_ud4_seq(), ra, node, src);
        tooie_ud4_dump_mode(rdram, stderr, glidx);
        tooie_ud4_dump_vec3(rdram, stderr, "dest_before", node + 0x74u);
        tooie_ud4_dump_vec3(rdram, stderr, "src", src);
        tooie_ud4_dump_node_fields(rdram, stderr, node);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
    s_ud4_in_set74 += 1;
    func_80110770(rdram, ctx);
    s_ud4_in_set74 -= 1;
    if (win) {
        fprintf(stderr, "[ud4-set74] leave ord=%d seq=%u node=%08x", ord, tooie_ud4_seq(), node);
        tooie_ud4_dump_vec3(rdram, stderr, "dest_after", node + 0x74u);
        tooie_ud4_dump_vec3(rdram, stderr, "src", src);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
}

static void tooie_wrap_func_80110790(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win = tooie_ud4_writer_window(rdram, &glidx) || (s_ud4_np_active != 0);
    const uint32_t node = static_cast<uint32_t>(ctx->r4);
    const uint32_t src = static_cast<uint32_t>(ctx->r5);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    int ord = 0;
    if (win) {
        ord = ++s_ud4_set80_n;
        fprintf(stderr,
                "[ud4-set80] enter ord=%d seq=%u ra=%08x node=%08x src=%08x",
                ord, tooie_ud4_seq(), ra, node, src);
        tooie_ud4_dump_mode(rdram, stderr, glidx);
        tooie_ud4_dump_vec3(rdram, stderr, "dest_before", node + 0x80u);
        tooie_ud4_dump_vec3(rdram, stderr, "src", src);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
    s_ud4_in_set80 += 1;
    tooie_ud4_np_call("80110790", 0x80110790u, rdram, ctx, 1);
    func_80110790(rdram, ctx);
    tooie_ud4_np_call("80110790", 0x80110790u, rdram, ctx, 0);
    s_ud4_in_set80 -= 1;
    if (win) {
        fprintf(stderr, "[ud4-set80] leave ord=%d seq=%u node=%08x", ord, tooie_ud4_seq(), node);
        tooie_ud4_dump_vec3(rdram, stderr, "dest_after", node + 0x80u);
        tooie_ud4_dump_vec3(rdram, stderr, "src", src);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
}

static void tooie_wrap_func_800EE7F8(uint8_t* rdram, recomp_context* ctx) {
    if (s_rt_in_map296) {
        const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
        const uint32_t a1 = static_cast<uint32_t>(ctx->r5);
        uint32_t w0 = 0, w1 = 0, w2 = 0;
        if (tooie_is_guest(a1)) {
            w0 = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(a1)));
            w1 = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(a1)));
            w2 = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(a1)));
        }
        fprintf(stderr,
                "[ud5-ee7f8-copy] runtime=RT tick=%llu rec_ord=%d dst=%08x src=%08x pitch=%08x yaw=%08x (%.6f) roll=%08x\n",
                (unsigned long long)s_rt_logic_tick, s_rt_last_logged_rec_ord,
                a0, a1, w0, w1, tooie_bits_f32(int32_t(w1)), w2);
        fflush(stderr);
    }
    int glidx = -1;
    const bool writer = (s_ud4_in_set74 || s_ud4_in_set80) &&
                        (tooie_ud4_writer_window(rdram, &glidx) || (s_ud4_np_active != 0));
    const uint32_t dst = static_cast<uint32_t>(ctx->r4);
    const uint32_t src = static_cast<uint32_t>(ctx->r5);
    if (writer) {
        fprintf(stderr,
                "[ud4-ee7f8] enter seq=%u ra=%08x dst=%08x src=%08x set74=%d set80=%d",
                tooie_ud4_seq(), static_cast<uint32_t>(ctx->r31), dst, src,
                s_ud4_in_set74, s_ud4_in_set80);
        tooie_ud4_dump_vec3(rdram, stderr, "dst_pre", dst);
        tooie_ud4_dump_vec3(rdram, stderr, "src", src);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
    tooie_ud4_np_call("800EE7F8", 0x800EE7F8u, rdram, ctx, 1);
    func_800EE7F8(rdram, ctx);
    tooie_ud4_np_call("800EE7F8", 0x800EE7F8u, rdram, ctx, 0);
    if (writer) {
        fprintf(stderr, "[ud4-ee7f8] leave seq=%u dst=%08x", tooie_ud4_seq(), dst);
        tooie_ud4_dump_vec3(rdram, stderr, "dst_post", dst);
        tooie_ud4_dump_vec3(rdram, stderr, "src", src);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
}

static void tooie_wrap_func_801108A0(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win = tooie_ud4_writer_window(rdram, &glidx);
    const uint32_t node = static_cast<uint32_t>(ctx->r4);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    uint32_t rec64 = 0;
    if (win) {
        if (tooie_is_guest(node)) {
            rec64 = static_cast<uint32_t>(MEM_W(0x64, kseg0_gpr(node)));
        }
        fprintf(stderr,
                "[ud4-108a0] enter seq=%u ra=%08x node=%08x rec64=%08x",
                tooie_ud4_seq(), ra, node, rec64);
        tooie_ud4_dump_mode(rdram, stderr, glidx);
        tooie_ud4_dump_node_fields(rdram, stderr, node);
        tooie_ud4_dump_ca8b4_reads(rdram, stderr, tooie_ud4_sel_rec(rdram));
        fprintf(stderr, "\n");
        fflush(stderr);
    }
    func_801108A0(rdram, ctx);
    if (win) {
        fprintf(stderr, "[ud4-108a0] leave seq=%u node=%08x", tooie_ud4_seq(), node);
        tooie_ud4_dump_node_fields(rdram, stderr, node);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
}

static void tooie_wrap_func_80112D08(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win = tooie_ud5_p60_writer_window(rdram, &glidx);
    const uint32_t node = static_cast<uint32_t>(ctx->r4);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    const uint32_t a1 = static_cast<uint32_t>(ctx->r5);
    const uint32_t a2 = static_cast<uint32_t>(ctx->r6);
    const uint32_t a3 = static_cast<uint32_t>(ctx->r7);
    if (win) {
        fprintf(stderr,
                "[ud5-p60-write] enter seq=%u glidx=%d pc=80112D08 ra=%08x"
                " node=%08x a1_mode=%u a2_targ=%08x/%.6f a3_step=%08x/%.6f",
                tooie_ud4_seq(), glidx, ra, node,
                a1, a2, tooie_bits_f32(a2), a3, tooie_bits_f32(a3));
        tooie_ud4_dump_p60_object(rdram, stderr, node);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
    func_80112D08(rdram, ctx);
    if (win) {
        fprintf(stderr,
                "[ud5-p60-write] leave seq=%u glidx=%d pc=80112D08 node=%08x",
                tooie_ud4_seq(), glidx, node);
        tooie_ud4_dump_p60_object(rdram, stderr, node);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
}

static void tooie_wrap_func_80112A84(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win = tooie_ud5_p60_writer_window(rdram, &glidx);
    const uint32_t node = static_cast<uint32_t>(ctx->r4);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    if (win) {
        uint32_t p60 = 0;
        uint16_t p60_c = 0;
        if (tooie_is_guest(node)) {
            p60 = static_cast<uint32_t>(MEM_W(0x60, kseg0_gpr(node)));
            if (tooie_is_guest(p60)) {
                p60_c = static_cast<uint16_t>(MEM_HU(0x0C, kseg0_gpr(p60)));
            }
        }
        fprintf(stderr,
                "[ud5-p60-writer-2a84] enter seq=%u glidx=%d pc=80112A84 ra=%08x node=%08x p60_c_predicate=%04x",
                tooie_ud4_seq(), glidx, ra, node, p60_c);
        tooie_ud4_dump_p60_object(rdram, stderr, node);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
    func_80112A84(rdram, ctx);
    if (win) {
        fprintf(stderr,
                "[ud5-p60-writer-2a84] leave seq=%u glidx=%d pc=80112A84 node=%08x",
                tooie_ud4_seq(), glidx, node);
        tooie_ud4_dump_p60_object(rdram, stderr, node);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
}

static void tooie_wrap_func_80112D90(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win = tooie_ud5_p60_writer_window(rdram, &glidx);
    const uint32_t node = static_cast<uint32_t>(ctx->r4);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    const uint32_t a1 = static_cast<uint32_t>(ctx->r5);
    const uint32_t a2 = static_cast<uint32_t>(ctx->r6);
    const uint32_t a3 = static_cast<uint32_t>(ctx->r7);
    const uint32_t sp30 = static_cast<uint32_t>(MEM_W(0x30, kseg0_gpr(ctx->r29)));
    if (win) {
        fprintf(stderr,
                "[ud5-p60-set-c-2d90] enter seq=%u glidx=%d pc=80112D90 ra=%08x node=%08x a1=%08x a2=%08x a3=%08x sp30=%08x",
                tooie_ud4_seq(), glidx, ra, node, a1, a2, a3, sp30);
        tooie_ud4_dump_p60_object(rdram, stderr, node);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
    func_80112D90(rdram, ctx);
    if (win) {
        fprintf(stderr,
                "[ud5-p60-set-c-2d90] leave seq=%u glidx=%d pc=80112D90 node=%08x",
                tooie_ud4_seq(), glidx, node);
        tooie_ud4_dump_p60_object(rdram, stderr, node);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
}

static void tooie_wrap_func_80110588(uint8_t* rdram, recomp_context* ctx) {
    if (s_rt_in_map296) {
        const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
        uint32_t sub_84 = 0, node = 0, node_50 = 0, node_58 = 0;
        if (tooie_is_guest(a0)) {
            sub_84 = static_cast<uint32_t>(MEM_W(0x84, kseg0_gpr(a0)));
            node = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(a0)));
        }
        if (tooie_is_guest(node)) {
            node_50 = static_cast<uint32_t>(MEM_W(0x50, kseg0_gpr(node)));
            node_58 = static_cast<uint32_t>(MEM_W(0x58, kseg0_gpr(node)));
        }
        fprintf(stderr,
                "[ud5-110588-entry] runtime=RT tick=%llu rec_ord=%d a0=%08x sub_84=%08x (%.6f) node=%08x node_50=%08x (%.6f) node_58=%08x (%.6f)\n",
                (unsigned long long)s_rt_logic_tick, s_rt_last_logged_rec_ord,
                a0, sub_84, tooie_bits_f32(int32_t(sub_84)), node, node_50, tooie_bits_f32(int32_t(node_50)), node_58, tooie_bits_f32(int32_t(node_58)));
        fflush(stderr);
    }
    int glidx = -1;
    const bool win = tooie_ud5_p60_writer_window(rdram, &glidx);
    const uint32_t node = static_cast<uint32_t>(ctx->r4);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    if (win) {
        uint8_t f70 = 0, f72 = 0;
        if (tooie_is_guest(node)) {
            f70 = static_cast<uint8_t>(MEM_BU(0x70, kseg0_gpr(node)));
            f72 = static_cast<uint8_t>(MEM_BU(0x72, kseg0_gpr(node)));
        }
        fprintf(stderr,
                "[ud5-p60-gate-0588] enter seq=%u glidx=%d pc=80110588 ra=%08x node=%08x f70=%u f72=%u",
                tooie_ud4_seq(), glidx, ra, node, f70, f72);
        tooie_ud4_dump_p60_object(rdram, stderr, node);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
    func_80110588(rdram, ctx);
    if (win) {
        fprintf(stderr,
                "[ud5-p60-gate-0588] leave seq=%u glidx=%d pc=80110588 node=%08x",
                tooie_ud4_seq(), glidx, node);
        tooie_ud4_dump_p60_object(rdram, stderr, node);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
}

static void tooie_wrap_func_8010FFB0(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win = tooie_ud4_writer_window(rdram, &glidx);
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t a1 = static_cast<uint32_t>(ctx->r5);
    const uint32_t a2 = static_cast<uint32_t>(ctx->r6);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    uint32_t rec = 0;
    if (win) {
        if (tooie_is_guest(a0)) {
            rec = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(a0)));
        }
        fprintf(stderr,
                "[ud4-ffb0] enter seq=%u ra=%08x a0=%08x rec=%08x a1=%08x a2=%08x",
                tooie_ud4_seq(), ra, a0, rec, a1, a2);
        tooie_ud4_dump_mode(rdram, stderr, glidx);
        tooie_ud4_dump_ca8b4_reads(rdram, stderr, rec);
        tooie_ud4_dump_vec3(rdram, stderr, "a1_pre", a1);
        tooie_ud4_dump_vec3(rdram, stderr, "a2_pre", a2);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
    func_8010FFB0(rdram, ctx);
    if (win) {
        fprintf(stderr, "[ud4-ffb0] leave seq=%u a1=%08x a2=%08x", tooie_ud4_seq(), a1, a2);
        tooie_ud4_dump_vec3(rdram, stderr, "a1_post", a1);
        tooie_ud4_dump_vec3(rdram, stderr, "a2_post", a2);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
}

static void tooie_wrap_bainput_should_rotate_camera_right(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win4 = tooie_ud4_writer_window(rdram, &glidx);
    const bool win5 = tooie_ud5_bainput_window(rdram, &glidx) || tooie_ud5_p60_writer_window(rdram, &glidx);
    const uint32_t self = static_cast<uint32_t>(ctx->r4);
    if (win4 || win5) {
        fprintf(stderr, "[%s-cam-chain] enter 80097B94 (rotate_camera_right) glidx=%d self=%08x\n",
                win4 ? "ud4" : "ud5", glidx, self);
        fflush(stderr);
    }
    bainput_should_rotate_camera_right(rdram, ctx);
    if (win4 || win5) {
        fprintf(stderr, "[%s-cam-chain] leave 80097B94 (rotate_camera_right) glidx=%d v0=%08x\n",
                win4 ? "ud4" : "ud5", glidx, static_cast<uint32_t>(ctx->r2));
        fflush(stderr);
    }
}

static void tooie_wrap_bainput_should_rotate_camera_left(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win = tooie_ud4_writer_window(rdram, &glidx) ||
                     tooie_ud5_bainput_window(rdram, &glidx);
    const uint32_t self = static_cast<uint32_t>(ctx->r4);
    if (win) {
        fprintf(stderr, "[ud5-cam-chain] enter 80097B70 (rotate_camera_left) glidx=%d self=%08x\n",
                glidx, self);
        fflush(stderr);
    }
    bainput_should_rotate_camera_left(rdram, ctx);
    if (win) {
        const uint32_t v0 = static_cast<uint32_t>(ctx->r2);
        fprintf(stderr, "[ud5-cam-chain] leave 80097B70 (rotate_camera_left) glidx=%d v0=%u (raw=%08x)\n",
                glidx, v0, v0);
        fprintf(stderr, "[ud5-branch-800A4928] glidx=%d pc=800A4928 v0_before=%u (raw=%08x) taken=%u target=800A4940\n",
                glidx, v0, v0, (v0 == 0) ? 1u : 0u);
        fflush(stderr);
    }
}

static void tooie_wrap_func_800BCE84(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win = tooie_ud4_writer_window(rdram, &glidx) ||
                     tooie_ud5_bainput_window(rdram, &glidx);
    const uint32_t obj = static_cast<uint32_t>(ctx->r4);
    const uint32_t button = static_cast<uint32_t>(ctx->r5);
    uint32_t ring = 0;
    uint32_t raw = 0;
    if (win && tooie_is_guest(obj)) {
        const gpr obj_p = kseg0_gpr(obj);
        ring = static_cast<uint32_t>(MEM_BU(0x8 + button, obj_p));
        raw = static_cast<uint32_t>(MEM_W(
            0x24 + ring * 56u + button * 4u, obj_p));
        fprintf(stderr,
                "[ud5-bce84] enter glidx=%d obj=%08x btn=%u ring=%u raw=%08x masked=%u\n",
                glidx, obj, button, ring, raw, raw & 1u);
        fflush(stderr);
    }
    func_800BCE84(rdram, ctx);
    if (win) {
        fprintf(stderr, "[ud5-bce84] leave glidx=%d obj=%08x btn=%u ret=%08x\n",
                glidx, obj, button, static_cast<uint32_t>(ctx->r2));
        fflush(stderr);
    }
}

static void tooie_wrap_bainput_enable(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win = tooie_ud5_bainput_window(rdram, &glidx) ||
                     tooie_ud5_p60_writer_window(rdram, &glidx);
    const uint32_t self = static_cast<uint32_t>(ctx->r4);
    const uint32_t input_id = static_cast<uint32_t>(ctx->r5);
    const uint32_t enable = static_cast<uint32_t>(ctx->r6);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    uint32_t old_word = 0;
    uint32_t inp_ptr = 0;
    if (win && tooie_is_guest(self)) {
        inp_ptr = static_cast<uint32_t>(MEM_W(0xA4, kseg0_gpr(self)));
        if (tooie_is_guest(inp_ptr)) {
            old_word = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(inp_ptr)));
        }
    }
    bainput_enable(rdram, ctx);
    if (win) {
        uint32_t new_word = 0;
        if (tooie_is_guest(inp_ptr)) {
            new_word = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(inp_ptr)));
        }
        const uint32_t old_bit0 = old_word & 1u;
        const uint32_t new_bit0 = new_word & 1u;
        fprintf(stderr,
                "[ud5-enable] runtime=RT seq=%u glidx=%d self=%08x inp_ptr=%08x in_id=%u en=%u old_w=%08x new_w=%08x bit0_old=%u bit0_new=%u ra=%08x\n",
                tooie_ud4_seq(), glidx, self, inp_ptr, input_id, enable, old_word, new_word, old_bit0, new_bit0, ra);
        fflush(stderr);
    }
}

static void tooie_wrap_bainput_isEnabled(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win = tooie_ud5_bainput_window(rdram, &glidx) ||
                     tooie_ud5_p60_writer_window(rdram, &glidx);
    const uint32_t self = static_cast<uint32_t>(ctx->r4);
    const uint32_t input_id = static_cast<uint32_t>(ctx->r5);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    bainput_isEnabled(rdram, ctx);
    if (win) {
        const uint32_t ret_v0 = static_cast<uint32_t>(ctx->r2);
        uint32_t enable_word = 0;
        if (tooie_is_guest(self)) {
            const uint32_t inp_ptr = static_cast<uint32_t>(MEM_W(0xA4, kseg0_gpr(self)));
            if (tooie_is_guest(inp_ptr)) {
                enable_word = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(inp_ptr)));
            }
        }
        fprintf(stderr,
                "[ud5-isenabled] runtime=RT seq=%u glidx=%d self=%08x in_id=%u en_w=%08x bit0=%u ret_v0=%08x ra=%08x\n",
                tooie_ud4_seq(), glidx, self, input_id, enable_word, enable_word & 1u, ret_v0, ra);
        fflush(stderr);
    }
}

static void tooie_wrap_bainput_init(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win = tooie_ud5_bainput_window(rdram, &glidx) ||
                     tooie_ud5_p60_writer_window(rdram, &glidx);
    const uint32_t self = static_cast<uint32_t>(ctx->r4);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    bainput_init(rdram, ctx);
    if (win) {
        fprintf(stderr,
                "[ud5-enable-init] runtime=RT seq=%u glidx=%d self=%08x ra=%08x\n",
                tooie_ud4_seq(), glidx, self, ra);
        fflush(stderr);
    }
}

static void tooie_wrap_func_80097990(uint8_t* rdram, recomp_context* ctx) {
    const auto sign_extend_guest = [](int32_t value) -> gpr {
        return static_cast<gpr>(static_cast<int64_t>(value));
    };
    const gpr self = sign_extend_guest(static_cast<int32_t>(ctx->r4));
    const uint32_t input_id = static_cast<uint32_t>(ctx->r5);
    const uint32_t val = static_cast<uint32_t>(ctx->r6);
    tooie_bainput_func_80097990(rdram, self, input_id, val);
}

static void tooie_wrap_func_80097978(uint8_t* rdram, recomp_context* ctx) {
    const auto sign_extend_guest = [](int32_t value) -> gpr {
        return static_cast<gpr>(static_cast<int64_t>(value));
    };
    const gpr self = sign_extend_guest(static_cast<int32_t>(ctx->r4));
    const uint32_t input_id = static_cast<uint32_t>(ctx->r5);
    ctx->r2 = tooie_bainput_func_80097978(rdram, self, input_id);
}

static void tooie_wrap_func_800A4338(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win4 = tooie_ud4_writer_window(rdram, &glidx);
    const bool win5 = tooie_ud5_p60_writer_window(rdram, &glidx);
    if (win4 || win5) {
        fprintf(stderr, "[%s-cam-chain] enter 800A4338 glidx=%d a0=%08x a1=%08x a2=%08x\n",
                win4 ? "ud4" : "ud5", glidx, static_cast<uint32_t>(ctx->r4), static_cast<uint32_t>(ctx->r5), static_cast<uint32_t>(ctx->r6));
        fflush(stderr);
    }
    func_800A4338(rdram, ctx);
}

static void tooie_wrap_func_80110EFC(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win4 = tooie_ud4_writer_window(rdram, &glidx);
    const bool win5 = tooie_ud5_p60_writer_window(rdram, &glidx);
    if (win4 || win5) {
        fprintf(stderr, "[%s-cam-chain] enter 80110EFC glidx=%d node=%08x a1=%08x\n",
                win4 ? "ud4" : "ud5", glidx, static_cast<uint32_t>(ctx->r4), static_cast<uint32_t>(ctx->r5));
        fflush(stderr);
    }
    func_80110EFC(rdram, ctx);
}

static void tooie_wrap_func_80110E00(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win4 = tooie_ud4_writer_window(rdram, &glidx);
    const bool win5 = tooie_ud5_p60_writer_window(rdram, &glidx);
    if (win4 || win5) {
        fprintf(stderr, "[%s-cam-chain] enter 80110E00 glidx=%d node=%08x a1=%08x\n",
                win4 ? "ud4" : "ud5", glidx, static_cast<uint32_t>(ctx->r4), static_cast<uint32_t>(ctx->r5));
        fflush(stderr);
    }
    func_80110E00(rdram, ctx);
}

static void tooie_wrap_func_801111D0(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win4 = tooie_ud4_writer_window(rdram, &glidx);
    const bool win5 = tooie_ud5_p60_writer_window(rdram, &glidx);
    if (win4 || win5) {
        const float f12 = ctx->f12.fl;
        const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
        const uint32_t ra = static_cast<uint32_t>(ctx->r31);
        fprintf(stderr, "[ud5-111d0] enter seq=%u glidx=%d pc=801111D0 ra=%08x node=%08x delta_yaw=%.4f (bits=%08x)\n",
                tooie_ud4_seq(), glidx, ra, a0, f12, ctx->f12.u32l);
        fflush(stderr);
    }
    func_801111D0(rdram, ctx);
    if (win4 || win5) {
        const uint32_t v0 = static_cast<uint32_t>(ctx->r2);
        fprintf(stderr, "[ud5-111d0] leave seq=%u glidx=%d pc=801111D0 ret_v0=%u\n",
                tooie_ud4_seq(), glidx, v0);
        fflush(stderr);
    }
}

static void tooie_wrap_func_80112E84(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win = tooie_ud5_p60_writer_window(rdram, &glidx);
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    if (win) {
        fprintf(stderr, "[ud5-12e84] enter seq=%u glidx=%d pc=80112E84 ra=%08x a0=%08x\n",
                tooie_ud4_seq(), glidx, ra, a0);
        fflush(stderr);
    }
    func_80112E84(rdram, ctx);
    if (win) {
        const uint32_t v0 = static_cast<uint32_t>(ctx->r2);
        fprintf(stderr, "[ud5-12e84] leave seq=%u glidx=%d pc=80112E84 ret_v0=%u\n",
                tooie_ud4_seq(), glidx, v0);
        fflush(stderr);
    }
}

static void tooie_wrap_func_8011349C(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win = tooie_ud5_p60_writer_window(rdram, &glidx);
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t a1 = static_cast<uint32_t>(ctx->r5);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    if (win) {
        uint32_t tx = 0, ty = 0, tz = 0;
        uint32_t ex = 0, ey = 0, ez = 0;
        if (tooie_is_guest(a0)) {
            tx = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(a0)));
            ty = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(a0)));
            tz = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(a0)));
        }
        if (tooie_is_guest(a1)) {
            ex = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(a1)));
            ey = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(a1)));
            ez = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(a1)));
        }
        fprintf(stderr,
                "[ud5-1349c] enter seq=%u glidx=%d pc=8011349C ra=%08x a0=%08x target=(%08x,%08x,%08x / %.6f,%.6f,%.6f) a1=%08x cand_eye=(%08x,%08x,%08x / %.6f,%.6f,%.6f)\n",
                tooie_ud4_seq(), glidx, ra, a0,
                tx, ty, tz, tooie_bits_f32(tx), tooie_bits_f32(ty), tooie_bits_f32(tz),
                a1,
                ex, ey, ez, tooie_bits_f32(ex), tooie_bits_f32(ey), tooie_bits_f32(ez));
        fflush(stderr);
    }
    func_8011349C(rdram, ctx);
    if (win) {
        const uint32_t v0 = static_cast<uint32_t>(ctx->r2);
        fprintf(stderr,
                "[ud5-1349c] leave seq=%u glidx=%d pc=8011349C ret_v0=%u\n",
                tooie_ud4_seq(), glidx, v0);
        fflush(stderr);
    }
}

static void tooie_wrap_func_800FAB50(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win = tooie_ud5_p60_writer_window(rdram, &glidx);
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t a1 = static_cast<uint32_t>(ctx->r5);
    const uint32_t a2 = static_cast<uint32_t>(ctx->r6);
    const uint32_t a3 = static_cast<uint32_t>(ctx->r7);
    const uint32_t sp10 = static_cast<uint32_t>(MEM_W(0x10, kseg0_gpr(ctx->r29)));
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    if (win) {
        uint32_t a0x = 0, a0y = 0, a0z = 0;
        uint32_t a1x = 0, a1y = 0, a1z = 0;
        if (tooie_is_guest(a0)) {
            a0x = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(a0)));
            a0y = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(a0)));
            a0z = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(a0)));
        }
        if (tooie_is_guest(a1)) {
            a1x = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(a1)));
            a1y = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(a1)));
            a1z = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(a1)));
        }
        fprintf(stderr,
                "[ud5-fab50] enter seq=%u glidx=%d pc=800FAB50 ra=%08x a0=%08x=(%08x,%08x,%08x / %.6f,%.6f,%.6f) a1=%08x=(%08x,%08x,%08x / %.6f,%.6f,%.6f) a2=%08x a3=%08x radius=%08x/%.6f\n",
                tooie_ud4_seq(), glidx, ra, a0,
                a0x, a0y, a0z, tooie_bits_f32(a0x), tooie_bits_f32(a0y), tooie_bits_f32(a0z),
                a1,
                a1x, a1y, a1z, tooie_bits_f32(a1x), tooie_bits_f32(a1y), tooie_bits_f32(a1z),
                a2, a3, sp10, tooie_bits_f32(sp10));
        fflush(stderr);
    }
    func_800FAB50(rdram, ctx);
    if (win) {
        const uint32_t v0 = static_cast<uint32_t>(ctx->r2);
        fprintf(stderr,
                "[ud5-fab50] leave seq=%u glidx=%d pc=800FAB50 ret_v0=%08x\n",
                tooie_ud4_seq(), glidx, v0);
        fflush(stderr);
    }
}

static void tooie_wrap_func_800C6A7C(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win = tooie_ud5_p60_writer_window(rdram, &glidx);
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t a1 = static_cast<uint32_t>(ctx->r5);
    const uint32_t a2 = static_cast<uint32_t>(ctx->r6);
    const uint32_t a3 = static_cast<uint32_t>(ctx->r7);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    if (win) {
        uint32_t a0x = 0, a0y = 0, a0z = 0;
        uint32_t a1x = 0, a1y = 0, a1z = 0;
        if (tooie_is_guest(a0)) {
            a0x = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(a0)));
            a0y = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(a0)));
            a0z = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(a0)));
        }
        if (tooie_is_guest(a1)) {
            a1x = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(a1)));
            a1y = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(a1)));
            a1z = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(a1)));
        }
        fprintf(stderr,
                "[ud5-c6a7c] enter seq=%u glidx=%d pc=800C6A7C ra=%08x a0=%08x=(%08x,%08x,%08x / %.6f,%.6f,%.6f) a1=%08x=(%08x,%08x,%08x / %.6f,%.6f,%.6f) a2=%08x a3=%08x\n",
                tooie_ud4_seq(), glidx, ra, a0,
                a0x, a0y, a0z, tooie_bits_f32(a0x), tooie_bits_f32(a0y), tooie_bits_f32(a0z),
                a1,
                a1x, a1y, a1z, tooie_bits_f32(a1x), tooie_bits_f32(a1y), tooie_bits_f32(a1z),
                a2, a3);
        fflush(stderr);
    }
    func_800C6A7C(rdram, ctx);
    if (win) {
        const uint32_t v0 = static_cast<uint32_t>(ctx->r2);
        fprintf(stderr,
                "[ud5-c6a7c] leave seq=%u glidx=%d pc=800C6A7C ret_v0=%08x\n",
                tooie_ud4_seq(), glidx, v0);
        fflush(stderr);
    }
}


extern "C" void tooie_ud4_cam_ovl(uint8_t* rdram, recomp_context* ctx, uint32_t ovl,
                                 uint32_t ep, int leaving) {
    int glidx = -1;
    if (!tooie_ud4_writer_window(rdram, &glidx)) {
        return;
    }
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    fprintf(stderr,
            "[ud4-cam-ovl] %s seq=%u ovl=%u ep=%u ra=%08x a0=%08x",
            leaving ? "leave" : "enter", tooie_ud4_seq(), ovl, ep,
            static_cast<uint32_t>(ctx->r31), a0);
    tooie_ud4_dump_mode(rdram, stderr, glidx);
    tooie_ud4_dump_node_fields(rdram, stderr, a0);
    tooie_ud4_dump_ca8b4_reads(rdram, stderr, tooie_ud4_sel_rec(rdram));
    fprintf(stderr, "\n");
    fflush(stderr);
}

// Guest FCR31 is not stored in recomp_context. Host fegetround maps to MIPS RM;
// mips3_float_mode is CP1 FR, not FCSR.FS; C/Enable/Cause/FS are unemulated.
static void tooie_ud4_dump_fcsr(recomp_context* ctx, FILE* fp) {
    uint32_t mips_rm = 0xFFu;
    switch (fegetround()) {
    case FE_TONEAREST: mips_rm = 0; break;
    case FE_TOWARDZERO: mips_rm = 1; break;
    case FE_UPWARD: mips_rm = 2; break;
    case FE_DOWNWARD: mips_rm = 3; break;
    default: break;
    }
    fprintf(fp,
            " fcr31=UNEMULATED rm=%d mips_rm=%u mips3=%u status=%08x",
            fegetround(), mips_rm, unsigned(ctx->mips3_float_mode),
            static_cast<uint32_t>(ctx->status_reg));
}

static void tooie_ud4_dump_80112c3c_sel(uint8_t* rdram, FILE* fp) {
    if (!tooie_is_guest(s_ud4_np_node)) {
        return;
    }
    const uint32_t p60 = static_cast<uint32_t>(MEM_W(0x60, kseg0_gpr(s_ud4_np_node)));
    if (!tooie_is_guest(p60)) {
        fprintf(fp, " 80112C3C_sel p60=none");
        return;
    }
    const gpr p = kseg0_gpr(p60);
    const uint32_t w8 = static_cast<uint32_t>(MEM_W(0x8, p));
    const uint32_t t7 = w8 - 1u;
    fprintf(fp,
            " 80112C3C_sel p60=%08x p60_0=%08x p60_4=%08x p60_8=%08x t7=%08x sltiu5=%d",
            p60,
            static_cast<uint32_t>(MEM_W(0x0, p)),
            static_cast<uint32_t>(MEM_W(0x4, p)),
            w8, t7, t7 < 5u ? 1 : 0);
}

static void tooie_ud4_np_call(const char* name, uint32_t addr, uint8_t* rdram,
                             recomp_context* ctx, int entering) {
    if (!s_ud4_np_active) {
        return;
    }
    const int n = entering ? ++s_ud4_np_call_n : s_ud4_np_call_n;
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t a1 = static_cast<uint32_t>(ctx->r5);
    const uint32_t a2 = static_cast<uint32_t>(ctx->r6);
    const uint32_t a3 = static_cast<uint32_t>(ctx->r7);
    fprintf(stderr,
            "[ud4-np-call] %s n=%d name=%s pc=%08x glidx=%d ra=%08x "
            "a0=%08x a1=%08x a2=%08x a3=%08x v0=%08x "
            "f0=%08x f2=%08x f12=%08x f14=%08x",
            entering ? "enter" : "leave", n, name, addr, s_ud4_np_glidx,
            static_cast<uint32_t>(ctx->r31), a0, a1, a2, a3,
            static_cast<uint32_t>(ctx->r2),
            ctx->f0.u32l, ctx->f2.u32l, ctx->f12.u32l, ctx->f14.u32l);
    tooie_ud4_dump_fcsr(ctx, stderr);
    if (entering && addr == 0x80112C3Cu) {
        tooie_ud4_dump_80112c3c_sel(rdram, stderr);
    }
    tooie_ud4_dump_vec3(rdram, stderr, "va0", a0);
    tooie_ud4_dump_vec3(rdram, stderr, "va1", a1);
    tooie_ud4_dump_vec3(rdram, stderr, "va2", a2);
    tooie_ud4_dump_np_frame(rdram, stderr);
    tooie_ud4_dump_node_fields(rdram, stderr, s_ud4_np_node);
    tooie_ud4_dump_ca8b4_reads(rdram, stderr, tooie_ud4_sel_rec(rdram));
    fprintf(stderr, "\n");
    fflush(stderr);
}

static bool tooie_ud5_combinator_window(uint8_t* rdram, int* glidx_out) {
    const int mode_v = int(MEM_H(0x2, kseg0_gpr(0x80127630u)));
    const int stage_v = int(MEM_BU(0x0, kseg0_gpr(0x80135470u)));
    const int sub_v = int(MEM_BU(0x1, kseg0_gpr(0x80135470u)));
    if (mode_v != 5 || stage_v != 0 || sub_v != 5) {
        return false;
    }
    const int32_t post_glidx = static_cast<int32_t>(MEM_W(0x8, kseg0_gpr(0x8012D5B0u)));
    const int32_t glidx = (post_glidx > 0) ? (post_glidx - 1) : 0;
    const int32_t rel_glidx = (glidx >= 1000) ? (glidx - 1012) : glidx;
    if (glidx_out) {
        *glidx_out = int(rel_glidx);
    }
    return rel_glidx >= 0 && rel_glidx <= 28;
}

static bool tooie_ud5_p60_writer_window(uint8_t* rdram, int* glidx_out) {
    const int mode_v = int(MEM_H(0x2, kseg0_gpr(0x80127630u)));
    const int stage_v = int(MEM_BU(0x0, kseg0_gpr(0x80135470u)));
    const int sub_v = int(MEM_BU(0x1, kseg0_gpr(0x80135470u)));
    if (mode_v != 5 || stage_v != 0 || sub_v != 5) {
        return false;
    }
    const int32_t post_glidx = static_cast<int32_t>(MEM_W(0x8, kseg0_gpr(0x8012D5B0u)));
    const int32_t glidx = (post_glidx > 0) ? (post_glidx - 1) : 0;
    const int32_t rel_glidx = (glidx >= 1000) ? (glidx - 1012) : glidx;
    if (glidx_out) {
        *glidx_out = int(rel_glidx);
    }
    return rel_glidx >= 0 && rel_glidx <= 28;
}

#define TOOIE_UD4_WRAP(sym, addr, tag)                                                    \
    static void tooie_wrap_##sym(uint8_t* rdram, recomp_context* ctx) {                    \
        tooie_ud4_np_call(tag, addr, rdram, ctx, 1);                                      \
        sym(rdram, ctx);                                                                  \
        tooie_ud4_np_call(tag, addr, rdram, ctx, 0);                                      \
    }

TOOIE_UD4_WRAP(func_8011607C, 0x8011607Cu, "8011607C")
TOOIE_UD4_WRAP(func_80113978, 0x80113978u, "80113978")
TOOIE_UD4_WRAP(func_80112524, 0x80112524u, "80112524")
TOOIE_UD4_WRAP(func_801107F0, 0x801107F0u, "801107F0")
TOOIE_UD4_WRAP(func_800EFB24, 0x800EFB24u, "800EFB24")
TOOIE_UD4_WRAP(func_800EEF94, 0x800EEF94u, "800EEF94")

static void tooie_wrap_func_800F1A88(uint8_t* rdram, recomp_context* ctx) {
    tooie_ud4_np_call("800F1A88", 0x800F1A88u, rdram, ctx, 1);
    int glidx = -1;
    const bool win5 = tooie_ud5_combinator_window(rdram, &glidx);
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t a1 = static_cast<uint32_t>(ctx->r5);
    if (win5 && tooie_is_guest(a0)) {
        const uint32_t vx = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(a0)));
        const uint32_t vy = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(a0)));
        const uint32_t vz = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(a0)));
        std::fprintf(stderr,
                     "[ud5-f1a88] enter glidx=%d pc=800f1a88 ra=%08x in_vec=(%08x,%08x,%08x / %.6f,%.6f,%.6f)\n",
                     glidx, static_cast<uint32_t>(ctx->r31), vx, vy, vz,
                     tooie_bits_f32(vx), tooie_bits_f32(vy), tooie_bits_f32(vz));
        std::fflush(stderr);
    }
    func_800F1A88(rdram, ctx);
    tooie_ud4_np_call("800F1A88", 0x800F1A88u, rdram, ctx, 0);
    if (win5 && tooie_is_guest(a1)) {
        const uint32_t pitch = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(a1)));
        const uint32_t init_yaw = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(a1)));
        std::fprintf(stderr,
                     "[ud5-f1a88] leave glidx=%d pc=800f1a88 pitch=%08x/%.6f init_yaw=%08x/%.6f\n",
                     glidx, pitch, tooie_bits_f32(pitch), init_yaw, tooie_bits_f32(init_yaw));
        std::fflush(stderr);
    }
}

TOOIE_UD4_WRAP(func_80115358, 0x80115358u, "80115358")
TOOIE_UD4_WRAP(func_80115750, 0x80115750u, "80115750")

static void tooie_wrap_func_80112C3C(uint8_t* rdram, recomp_context* ctx) {
    tooie_ud4_np_call("80112C3C", 0x80112C3Cu, rdram, ctx, 1);
    int glidx = -1;
    const bool win5 = tooie_ud5_combinator_window(rdram, &glidx);
    if (win5) {
        const uint32_t node = static_cast<uint32_t>(ctx->r4);
        const uint32_t a1_bits = static_cast<uint32_t>(ctx->r5);
        uint32_t p60 = 0, p60_0 = 0, p60_4 = 0, p60_8 = 0;
        if (tooie_is_guest(node)) {
            p60 = static_cast<uint32_t>(MEM_W(0x60, kseg0_gpr(node)));
            if (tooie_is_guest(p60)) {
                p60_0 = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(p60)));
                p60_4 = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(p60)));
                p60_8 = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(p60)));
            }
        }
        std::fprintf(stderr,
                     "[ud5-2c3c] enter glidx=%d pc=80112c3c ra=%08x s0=%08x in_yaw=%08x/%.6f "
                     "p60=%08x p60_8=%08x p60_0=%08x/%.6f p60_4=%08x\n",
                     glidx, static_cast<uint32_t>(ctx->r31), node, a1_bits, tooie_bits_f32(a1_bits),
                     p60, p60_8, p60_0, tooie_bits_f32(p60_0), p60_4);
        std::fflush(stderr);
    }
    func_80112C3C(rdram, ctx);
    tooie_ud4_np_call("80112C3C", 0x80112C3Cu, rdram, ctx, 0);
    if (win5) {
        const uint32_t f0_bits = ctx->f0.u32l;
        std::fprintf(stderr,
                     "[ud5-2c3c] leave glidx=%d pc=80112c3c out_f0=%08x/%.6f (OPERAND_A)\n",
                     glidx, f0_bits, tooie_bits_f32(f0_bits));
        std::fflush(stderr);
    }
}

static void tooie_wrap_func_800F1738(uint8_t* rdram, recomp_context* ctx) {
    tooie_ud4_np_call("800F1738", 0x800F1738u, rdram, ctx, 1);
    int glidx = -1;
    const bool win5 = tooie_ud5_combinator_window(rdram, &glidx);
    if (win5) {
        std::fprintf(stderr,
                     "[ud5-f1738] enter glidx=%d pc=800f1738 cur=%08x/%.6f targ=%08x/%.6f step=%08x\n",
                     glidx, ctx->f12.u32l, ctx->f12.fl, ctx->f14.u32l, ctx->f14.fl, static_cast<uint32_t>(ctx->r6));
        std::fflush(stderr);
    }
    func_800F1738(rdram, ctx);
    tooie_ud4_np_call("800F1738", 0x800F1738u, rdram, ctx, 0);
    if (win5) {
        std::fprintf(stderr,
                     "[ud5-f1738] leave glidx=%d pc=800f1738 out=%08x/%.6f\n",
                     glidx, ctx->f0.u32l, ctx->f0.fl);
        std::fflush(stderr);
    }
}

static void tooie_wrap_func_800F1DCC(uint8_t* rdram, recomp_context* ctx) {
    tooie_ud4_np_call("800F1DCC", 0x800F1DCCu, rdram, ctx, 1);
    int glidx = -1;
    const bool win5 = tooie_ud5_combinator_window(rdram, &glidx);
    if (win5) {
        std::fprintf(stderr,
                     "[ud5-f1dcc] enter glidx=%d pc=800f1dcc a0=%08x/%.6f a1=%08x/%.6f\n",
                     glidx, ctx->f12.u32l, ctx->f12.fl, ctx->f14.u32l, ctx->f14.fl);
        std::fflush(stderr);
    }
    func_800F1DCC(rdram, ctx);
    tooie_ud4_np_call("800F1DCC", 0x800F1DCCu, rdram, ctx, 0);
    if (win5) {
        std::fprintf(stderr,
                     "[ud5-f1dcc] leave glidx=%d pc=800f1dcc out=%08x/%.6f\n",
                     glidx, ctx->f0.u32l, ctx->f0.fl);
        std::fflush(stderr);
    }
}

static void tooie_wrap_func_801125E8(uint8_t* rdram, recomp_context* ctx) {
    tooie_ud4_np_call("801125E8", 0x801125E8u, rdram, ctx, 1);
    int glidx = -1;
    const bool win5 = tooie_ud5_combinator_window(rdram, &glidx);
    if (win5) {
        const uint32_t node = static_cast<uint32_t>(ctx->r4);
        uint32_t t6 = 0, t6_c = 0;
        if (tooie_is_guest(node)) {
            t6 = static_cast<uint32_t>(MEM_W(0x4C, kseg0_gpr(node)));
            if (tooie_is_guest(t6)) {
                t6_c = static_cast<uint32_t>(MEM_W(0xC, kseg0_gpr(t6)));
            }
        }
        std::fprintf(stderr,
                     "[ud5-25e8] enter glidx=%d pc=801125e8 ra=%08x s0=%08x t6=%08x t6_c=%08x/%.6f\n",
                     glidx, static_cast<uint32_t>(ctx->r31), node, t6, t6_c, tooie_bits_f32(t6_c));
        std::fflush(stderr);
    }
    func_801125E8(rdram, ctx);
    tooie_ud4_np_call("801125E8", 0x801125E8u, rdram, ctx, 0);
    if (win5) {
        const uint32_t f0_bits = ctx->f0.u32l;
        std::fprintf(stderr,
                     "[ud5-25e8] leave glidx=%d pc=801125e8 out_f0=%08x/%.6f (OPERAND_B)\n",
                     glidx, f0_bits, tooie_bits_f32(f0_bits));
        std::fflush(stderr);
    }
}

static uint32_t s_rt_ud5_ee780_ord = 0;
static uint32_t s_rt_ud5_eebf0_ord = 0;

static void tooie_wrap_func_800EEBF0(uint8_t* rdram, recomp_context* ctx) {
    tooie_ud4_np_call("800EEBF0", 0x800EEBF0u, rdram, ctx, 1);
    int glidx = -1;
    const bool win5 = tooie_ud5_combinator_window(rdram, &glidx);
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t a1 = static_cast<uint32_t>(ctx->r5);
    const uint32_t a2 = static_cast<uint32_t>(ctx->r6);
    const uint32_t a3 = static_cast<uint32_t>(ctx->r7);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    uint32_t uord = 0;
    if (win5) {
        uord = ++s_rt_ud5_eebf0_ord;
    }
    if (win5 && tooie_is_guest(a0) && tooie_is_guest(a1)) {
        const uint32_t ox = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(a0)));
        const uint32_t oy = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(a0)));
        const uint32_t oz = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(a0)));
        const uint32_t sx = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(a1)));
        const uint32_t sy = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(a1)));
        const uint32_t sz = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(a1)));
        std::fprintf(stderr,
                     "[ud5-add194] glidx=%d pc=80800194 opA_prod=80112c3c opB_prod=801125e8 "
                     "pitch=%08x/%.6f yaw_sum=%08x/%.6f radius=%08x/%.6f stored_sp38=%08x/%.6f\n",
                     glidx, sx, tooie_bits_f32(sx), sy, tooie_bits_f32(sy),
                     a2, tooie_bits_f32(a2), sy, tooie_bits_f32(sy));
        std::fprintf(stderr,
                     "[ud5-eebf0] enter glidx=%d ord=%u pc=800eebf0 ra=%08x "
                     "a0=%08x a1=%08x a2=%08x/%.6f a3=%08x "
                     "old=(%08x,%08x,%08x / %.6f,%.6f,%.6f) "
                     "src_dir=(%08x,%08x,%08x / %.6f,%.6f,%.6f)\n",
                     glidx, uord, ra, a0, a1, a2, tooie_bits_f32(a2), a3,
                     ox, oy, oz, tooie_bits_f32(ox), tooie_bits_f32(oy), tooie_bits_f32(oz),
                     sx, sy, sz, tooie_bits_f32(sx), tooie_bits_f32(sy), tooie_bits_f32(sz));
        std::fflush(stderr);
    }
    func_800EEBF0(rdram, ctx);
    tooie_ud4_np_call("800EEBF0", 0x800EEBF0u, rdram, ctx, 0);
    if (win5 && tooie_is_guest(a0)) {
        const uint32_t dx = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(a0)));
        const uint32_t dy = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(a0)));
        const uint32_t dz = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(a0)));
        std::fprintf(stderr,
                     "[ud5-eebf0] leave glidx=%d ord=%u dest=%08x "
                     "out=(%08x,%08x,%08x / %.6f,%.6f,%.6f)\n",
                     glidx, uord, a0, dx, dy, dz, tooie_bits_f32(dx), tooie_bits_f32(dy), tooie_bits_f32(dz));
        std::fflush(stderr);
    }
}

static void tooie_wrap_func_800EE780(uint8_t* rdram, recomp_context* ctx) {
    tooie_ud4_np_call("800EE780", 0x800EE780u, rdram, ctx, 1);
    int glidx = -1;
    const bool win5 = tooie_ud5_combinator_window(rdram, &glidx);
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t a1 = static_cast<uint32_t>(ctx->r5);
    const uint32_t a2 = static_cast<uint32_t>(ctx->r6);
    const uint32_t a3 = static_cast<uint32_t>(ctx->r7);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    uint32_t uord = 0;
    if (win5) {
        uord = ++s_rt_ud5_ee780_ord;
    }
    if (win5 && tooie_is_guest(a0) && tooie_is_guest(a1) && tooie_is_guest(a2)) {
        const uint32_t ox = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(a0)));
        const uint32_t oy = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(a0)));
        const uint32_t oz = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(a0)));
        const uint32_t t0 = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(a1)));
        const uint32_t t1 = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(a1)));
        const uint32_t t2 = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(a1)));
        const uint32_t d0 = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(a2)));
        const uint32_t d1 = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(a2)));
        const uint32_t d2 = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(a2)));
        std::fprintf(stderr,
                     "[ud5-ee780] enter glidx=%d ord=%u pc=800ee780 ra=%08x "
                     "a0=%08x a1=%08x a2=%08x a3=%08x "
                     "old=(%08x,%08x,%08x / %.6f,%.6f,%.6f) "
                     "src_a1=(%08x,%08x,%08x / %.6f,%.6f,%.6f) "
                     "src_a2=(%08x,%08x,%08x / %.6f,%.6f,%.6f)\n",
                     glidx, uord, ra, a0, a1, a2, a3,
                     ox, oy, oz, tooie_bits_f32(ox), tooie_bits_f32(oy), tooie_bits_f32(oz),
                     t0, t1, t2, tooie_bits_f32(t0), tooie_bits_f32(t1), tooie_bits_f32(t2),
                     d0, d1, d2, tooie_bits_f32(d0), tooie_bits_f32(d1), tooie_bits_f32(d2));
        std::fflush(stderr);
    }
    func_800EE780(rdram, ctx);
    tooie_ud4_np_call("800EE780", 0x800EE780u, rdram, ctx, 0);
    if (win5 && tooie_is_guest(a0)) {
        const uint32_t e0 = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(a0)));
        const uint32_t e1 = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(a0)));
        const uint32_t e2 = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(a0)));
        std::fprintf(stderr,
                     "[ud5-ee780] leave glidx=%d ord=%u dest=%08x "
                     "out=(%08x,%08x,%08x / %.6f,%.6f,%.6f)\n",
                     glidx, uord, a0, e0, e1, e2, tooie_bits_f32(e0), tooie_bits_f32(e1), tooie_bits_f32(e2));
        std::fflush(stderr);
    }
}

static void tooie_wrap_func_800EFD24(uint8_t* rdram, recomp_context* ctx) {
    tooie_ud4_np_call("800EFD24", 0x800EFD24u, rdram, ctx, 1);
    int glidx = -1;
    const bool win5 = tooie_ud5_combinator_window(rdram, &glidx);
    if (win5) {
        std::fprintf(stderr,
                     "[ud5-efd24] enter glidx=%d pc=800efd24 in_angle=%08x/%.6f\n",
                     glidx, ctx->f12.u32l, ctx->f12.fl);
        std::fflush(stderr);
    }
    func_800EFD24(rdram, ctx);
    tooie_ud4_np_call("800EFD24", 0x800EFD24u, rdram, ctx, 0);
    if (win5) {
        std::fprintf(stderr,
                     "[ud5-efd24] leave glidx=%d pc=800efd24 out_angle=%08x/%.6f\n",
                     glidx, ctx->f0.u32l, ctx->f0.fl);
        std::fflush(stderr);
    }
}

static void tooie_wrap_func_800EF214(uint8_t* rdram, recomp_context* ctx) {
    tooie_ud4_np_call("800EF214", 0x800EF214u, rdram, ctx, 1);
    int glidx = -1;
    const bool win5 = tooie_ud5_combinator_window(rdram, &glidx);
    const uint32_t dst = static_cast<uint32_t>(ctx->r4);
    if (win5) {
        const uint32_t p_bits = static_cast<uint32_t>(ctx->r5);
        const uint32_t y_bits = static_cast<uint32_t>(ctx->r6);
        const uint32_t r_bits = static_cast<uint32_t>(ctx->r7);
        std::fprintf(stderr,
                     "[ud5-ef214] enter glidx=%d pc=800ef214 dst=%08x pitch=%08x/%.6f yaw=%08x/%.6f radius=%08x/%.6f\n",
                     glidx, dst, p_bits, tooie_bits_f32(p_bits), y_bits, tooie_bits_f32(y_bits), r_bits, tooie_bits_f32(r_bits));
        std::fflush(stderr);
    }
    func_800EF214(rdram, ctx);
    tooie_ud4_np_call("800EF214", 0x800EF214u, rdram, ctx, 0);
    if (win5 && tooie_is_guest(dst)) {
        const uint32_t dx = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(dst)));
        const uint32_t dy = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(dst)));
        const uint32_t dz = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(dst)));
        std::fprintf(stderr,
                     "[ud5-ef214] leave glidx=%d pc=800ef214 dst=%08x out_cart=(%08x,%08x,%08x / %.6f,%.6f,%.6f)\n",
                     glidx, dst, dx, dy, dz, tooie_bits_f32(dx), tooie_bits_f32(dy), tooie_bits_f32(dz));
        std::fflush(stderr);
    }
}

TOOIE_UD4_WRAP(func_80013B70, 0x80013B70u, "80013B70")
TOOIE_UD4_WRAP(func_80013B7C, 0x80013B7Cu, "80013B7C")
TOOIE_UD4_WRAP(func_800137AC, 0x800137ACu, "800137AC")
TOOIE_UD4_WRAP(func_800137C4, 0x800137C4u, "800137C4")
TOOIE_UD4_WRAP(func_800137F4, 0x800137F4u, "800137F4")
TOOIE_UD4_WRAP(func_80013788, 0x80013788u, "80013788")
TOOIE_UD4_WRAP(func_80013818, 0x80013818u, "80013818")
TOOIE_UD4_WRAP(func_80111FD4, 0x80111FD4u, "80111FD4")
TOOIE_UD4_WRAP(func_801138A0, 0x801138A0u, "801138A0")
TOOIE_UD4_WRAP(func_801138F4, 0x801138F4u, "801138F4")
TOOIE_UD4_WRAP(func_80110F44, 0x80110F44u, "80110F44")
TOOIE_UD4_WRAP(func_80115580, 0x80115580u, "80115580")
TOOIE_UD4_WRAP(func_80110A68, 0x80110A68u, "80110A68")
TOOIE_UD4_WRAP(func_80111310, 0x80111310u, "80111310")
TOOIE_UD4_WRAP(func_800A516C, 0x800A516Cu, "800A516C")

extern "C" void sqrtf_recomp(uint8_t* rdram, recomp_context* ctx);
static void tooie_wrap_sqrtf(uint8_t* rdram, recomp_context* ctx) {
    tooie_ud4_np_call("sqrtf", 0x8002DCA0u, rdram, ctx, 1);
    sqrtf_recomp(rdram, ctx);
    tooie_ud4_np_call("sqrtf", 0x8002DCA0u, rdram, ctx, 0);
}

static int tooie_ud4_first_tick_env() {
    static int s = -1;
    if (s < 0) {
        const char* raw = std::getenv("TOOIE_UD4_FIRST_TICK");
        s = (raw != nullptr && raw[0] != '\0' && raw[0] != '0') ? 1 : 0;
    }
    return s;
}

static int tooie_ud4_walkstart_env() {
    static int s = -1;
    if (s < 0) {
        const char* raw = std::getenv("TOOIE_UD4_WALKSTART");
        s = (raw != nullptr && raw[0] != '\0' && raw[0] != '0') ? 1 : 0;
    }
    return s;
}

extern "C" void tooie_ud4_ncbapivot_ep3(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    if (!tooie_ud4_writer_window(rdram, &glidx)) {
        return;
    }
    static int s_ncbapivot_n = 0;
    const int first = (s_ncbapivot_n++ == 0) ? 1 : 0;
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t sp = static_cast<uint32_t>(ctx->r29);
    s_ud4_np_node = a0;
    s_ud4_np_glidx = glidx;
    s_ud4_np_frame = sp - 0x78u;
    s_ud4_np_call_n = 0;
    // First ncbapivot_entrypoint_3 in the haunted latch is the mode=2 glidx=0 bind tick.
    s_ud4_np_active = first || tooie_ud4_walkstart_env() ? 1 : 0;
    fprintf(stderr,
            "[ud4-ncbapivot] enter seq=%u glidx=%d first=%d ra=%08x a0=%08x "
            "a1=%08x a2=%08x a3=%08x sp=%08x frame=%08x f0=%08x f12=%08x",
            tooie_ud4_seq(), glidx, first, static_cast<uint32_t>(ctx->r31), a0,
            static_cast<uint32_t>(ctx->r5), static_cast<uint32_t>(ctx->r6),
            static_cast<uint32_t>(ctx->r7), sp, s_ud4_np_frame,
            ctx->f0.u32l, ctx->f12.u32l);
    tooie_ud4_dump_fcsr(ctx, stderr);
    tooie_ud4_dump_mode(rdram, stderr, glidx);
    tooie_ud4_dump_node_fields(rdram, stderr, a0);
    tooie_ud4_dump_ca8b4_reads(rdram, stderr, tooie_ud4_sel_rec(rdram));
    tooie_ud4_dump_player(rdram, stderr);
    fprintf(stderr, "\n");
    fflush(stderr);
}

extern "C" void tooie_ud4_ncbapivot_ep3_leave(uint8_t* rdram, recomp_context* ctx) {
    if (s_ud4_np_glidx < 0) {
        return;
    }
    int glidx = s_ud4_np_glidx;
    fprintf(stderr,
            "[ud4-ncbapivot] leave seq=%u glidx=%d active=%d v0=%08x f0=%08x",
            tooie_ud4_seq(), glidx, s_ud4_np_active, static_cast<uint32_t>(ctx->r2),
            ctx->f0.u32l);
    tooie_ud4_dump_fcsr(ctx, stderr);
    tooie_ud4_dump_np_frame(rdram, stderr);
    tooie_ud4_dump_node_fields(rdram, stderr, s_ud4_np_node);
    tooie_ud4_dump_ca8b4_reads(rdram, stderr, tooie_ud4_sel_rec(rdram));
    tooie_ud4_dump_player(rdram, stderr);
    fprintf(stderr, "\n");
    fflush(stderr);
    const int was_active = s_ud4_np_active;
    s_ud4_np_active = 0;
    s_ud4_np_glidx = -1;
    if (was_active && tooie_ud4_first_tick_env()) {
        fprintf(stderr, "[ud4-ncbapivot] FIRST_TICK_STOP after glidx=%d\n", glidx);
        fflush(stderr);
        std::_Exit(0);
    }
}

// Full Wooded Hollow gameplay demo synchronization diagnostic (TOOIE_DIAG_DEMO_SYNC=1).
// Emits one line per glrecord dispatch across the entire demo run.
extern "C" void tooie_demo_sync_tick(uint8_t* rdram, uint64_t tick) {
    if (!tooie_diag::demo_sync_enabled()) {
        return;
    }
    const int mode_v = int(MEM_H(0x2, kseg0_gpr(0x80127630u)));
    const int map_v = int(MEM_H(0x10, kseg0_gpr(0x80127630u)));
    if (mode_v != 5) {
        return;
    }

    auto bits_f32 = [](int32_t bits) -> float {
        float f = 0.0f;
        const uint32_t u = static_cast<uint32_t>(bits);
        std::memcpy(&f, &u, sizeof(f));
        return f;
    };
    auto is_guest = [](uint32_t a) {
        return (a >= 0x80000000u) && ((a + 0x80000000u) < 0x00800000u) && ((a & 3u) == 0u);
    };

    const gpr plist = kseg0_gpr(0x80135490u);
    const uint32_t ps_addr = static_cast<uint32_t>(MEM_W(0, plist));
    if (ps_addr == 0 || !is_guest(ps_addr)) {
        return;
    }
    const gpr ps = kseg0_gpr(ps_addr);

    const uint32_t pos_addr = static_cast<uint32_t>(MEM_W(0xE4, ps));
    const uint32_t yaw_addr = static_cast<uint32_t>(MEM_W(0xF8, ps));
    const uint32_t phys_addr = static_cast<uint32_t>(MEM_W(0xC8, ps));
    const uint32_t bs_addr = static_cast<uint32_t>(MEM_W(0x120, ps));
    const uint32_t ba_stick_addr = static_cast<uint32_t>(MEM_W(0x128, ps));

    float px = 0, py = 0, pz = 0;
    float yaw = 0, yaw_ideal = 0;
    float vx = 0, vy = 0, vz = 0, tspd = 0;
    float bstick_x = 0, bstick_y = 0, bstick_ang = 0, bstick_dist = 0;
    float mult = 0, accel = 0, phys_yaw = 0;
    float ppx = 0, ppy = 0, ppz = 0;
    int32_t bs_id = -1;

    if (is_guest(pos_addr)) {
        const gpr posp = kseg0_gpr(pos_addr);
        px = bits_f32(MEM_W(0x0, posp));
        py = bits_f32(MEM_W(0x4, posp));
        pz = bits_f32(MEM_W(0x8, posp));
    }
    if (is_guest(yaw_addr)) {
        const gpr yawp = kseg0_gpr(yaw_addr);
        yaw = bits_f32(MEM_W(0x0, yawp));
        yaw_ideal = bits_f32(MEM_W(0x4, yawp));
    }
    if (is_guest(phys_addr)) {
        const gpr ph = kseg0_gpr(phys_addr);
        ppx = bits_f32(MEM_W(0x04, ph));
        ppy = bits_f32(MEM_W(0x08, ph));
        ppz = bits_f32(MEM_W(0x0C, ph));
        vx = bits_f32(MEM_W(0x10, ph));
        vy = bits_f32(MEM_W(0x14, ph));
        vz = bits_f32(MEM_W(0x18, ph));
        tspd = bits_f32(MEM_W(0x3C, ph));
        phys_yaw = bits_f32(MEM_W(0x40, ph));
        accel = bits_f32(MEM_W(0x44, ph));
        mult = bits_f32(MEM_W(0x50, ph));
    }
    if (is_guest(ba_stick_addr)) {
        const gpr st = kseg0_gpr(ba_stick_addr);
        bstick_x = bits_f32(MEM_W(0x54, st));
        bstick_y = bits_f32(MEM_W(0x58, st));
        bstick_ang = bits_f32(MEM_W(0x5C, st));
        bstick_dist = bits_f32(MEM_W(0x60, st));
    }
    if (is_guest(bs_addr)) {
        bs_id = static_cast<int32_t>(MEM_W(0, kseg0_gpr(bs_addr)));
    }

    const gpr remap = kseg0_gpr(0x80079AF8u);
    const float rmx = bits_f32(MEM_W(8, remap));
    const float rmy = bits_f32(MEM_W(12, remap));

    const gpr gl = kseg0_gpr(0x8012D5B0u);
    const uint32_t gl_base = static_cast<uint32_t>(MEM_W(0, gl));
    const int32_t post_glidx = static_cast<int32_t>(MEM_W(0x8, gl));
    const int32_t gl_count = static_cast<int32_t>(MEM_W(0xC, gl));
    const int32_t consumed_glidx = (post_glidx > 0) ? (post_glidx - 1) : 0;
    int8_t rec_x = 0, rec_y = 0;
    uint16_t rec_btn = 0;
    if (is_guest(gl_base)) {
        const gpr base = kseg0_gpr(gl_base + static_cast<uint32_t>(consumed_glidx) * 6u);
        rec_x = int8_t(MEM_B(0, base));
        rec_y = int8_t(MEM_B(1, base));
        rec_btn = static_cast<uint16_t>(MEM_HU(2, base));
    }

    const float gamespeed = bits_f32(MEM_W(0, kseg0_gpr(0x8012C760u)));
    const int32_t gfcnt = static_cast<int32_t>(MEM_W(0, kseg0_gpr(0x8012C764u)));
    const uint64_t host_gfx = tooie_diag::gfx_ticks();

    static bool s_have_t0 = false;
    static std::chrono::steady_clock::time_point s_t0;
    if (!s_have_t0) {
        s_t0 = std::chrono::steady_clock::now();
        s_have_t0 = true;
    }
    const auto wall_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - s_t0).count();

    static int32_t s_prev_glidx = -1;
    const int32_t gl_adv = (s_prev_glidx < 0) ? 0 : (post_glidx - s_prev_glidx);
    s_prev_glidx = post_glidx;

    float cx = 0, cy = 0, cz = 0, cam_yaw = 0;
    const gpr cam_vp_sym = kseg0_gpr(0x8012D500u);
    const uint32_t cam_vp_addr = static_cast<uint32_t>(MEM_W(0, cam_vp_sym));
    if (is_guest(cam_vp_addr)) {
        const gpr vp = kseg0_gpr(cam_vp_addr);
        cx = bits_f32(MEM_W(0x0, vp));
        cy = bits_f32(MEM_W(0x4, vp));
        cz = bits_f32(MEM_W(0x8, vp));
        const float m00 = bits_f32(MEM_W(0xA4 + 0x00, vp));
        const float m02 = bits_f32(MEM_W(0xA4 + 0x08, vp));
        cam_yaw = std::atan2(-m02, m00) * (180.0f / 3.14159265f);
        if (cam_yaw < 0.0f) cam_yaw += 360.0f;
    }

    float dig_x = 0, dig_y = 0, dig_z = 0;
    float act2_x = 0, act2_y = 0, act2_z = 0;
    uint32_t act2_mid = 0, act2_sys = 0;
    const gpr actors_vec_sym = kseg0_gpr(0x80136EE0u);
    const uint32_t vec_addr = static_cast<uint32_t>(MEM_W(0, actors_vec_sym));
    if (is_guest(vec_addr)) {
        const gpr vec = kseg0_gpr(vec_addr);
        const uint32_t elem_size = static_cast<uint32_t>(MEM_W(0x00, vec));
        const uint32_t begin = static_cast<uint32_t>(MEM_W(0x04, vec));
        const uint32_t end = static_cast<uint32_t>(MEM_W(0x08, vec));
        if (elem_size == 0x9Cu && is_guest(begin) && is_guest(end) && end >= begin) {
            const uint32_t count = (end - begin) / elem_size;
            for (uint32_t i = 0; i < count && i < 64; ++i) {
                const uint32_t act_addr = begin + i * 0x9Cu;
                const gpr act = kseg0_gpr(act_addr);
                const uint32_t idw = static_cast<uint32_t>(MEM_W(0x6C, act));
                const uint32_t marker_id = idw & 0x1FFu;
                if (marker_id == 337u) {
                    dig_x = bits_f32(MEM_W(0x04, act));
                    dig_y = bits_f32(MEM_W(0x08, act));
                    dig_z = bits_f32(MEM_W(0x0C, act));
                } else if (act2_mid == 0 && marker_id != 0 && marker_id != 1u) {
                    act2_mid = marker_id;
                    act2_sys = static_cast<uint32_t>(MEM_W(0x10, act));
                    act2_x = bits_f32(MEM_W(0x04, act));
                    act2_y = bits_f32(MEM_W(0x08, act));
                    act2_z = bits_f32(MEM_W(0x0C, act));
                }
            }
        }
    }

    const float velxz = std::sqrt(vx * vx + vz * vz);

    const int stage_v = int(MEM_BU(0x0, kseg0_gpr(0x80135470u)));
    const int sub_v = int(MEM_BU(0x1, kseg0_gpr(0x80135470u)));
    int32_t egg_slot = -1;
    int egg_enum = -1;
    egg_slot = static_cast<int32_t>(MEM_W(0x64, ps));
    if (egg_slot >= 0 && egg_slot < 8) {
        egg_enum = int(MEM_BU(egg_slot, kseg0_gpr(0x80117E20u)));
    }
    int32_t cam_sel = -1;
    float camctrl = 0.0f;
    int cam_mode = -1;
    cam_sel = static_cast<int32_t>(MEM_W(0x184, ps));
    if (cam_sel >= 0 && cam_sel < 8) {
        const uint32_t rec_addr =
            static_cast<uint32_t>(MEM_W(cam_sel * 4, kseg0_gpr(0x80127708u)));
        if (is_guest(rec_addr)) {
            const gpr rec = kseg0_gpr(rec_addr);
            camctrl = bits_f32(MEM_W(0x1C, rec));
            cam_mode = int(MEM_H(0x16, rec));
            if (tooie_diag::attract_enabled() && stage_v == 0 && sub_v == 4 &&
                consumed_glidx >= 0 && consumed_glidx <= 42) {
                std::fprintf(stderr,
                             "[ud4-rec] glidx=%d rec=%08x eye=(%.6f,%.6f,%.6f) "
                             "eye_bits=%08x,%08x,%08x d=(%.6f,%.6f,%.6f) "
                             "camctrl=%.6f bits=%08x src2_in=%08x,%08x,%08x "
                             "p20=(%.6f,%.6f,%.6f) p2c=(%.6f,%.6f,%.6f)\n",
                             consumed_glidx, rec_addr,
                             bits_f32(MEM_W(0x00, rec)), bits_f32(MEM_W(0x04, rec)),
                             bits_f32(MEM_W(0x08, rec)),
                             static_cast<uint32_t>(MEM_W(0x00, rec)),
                             static_cast<uint32_t>(MEM_W(0x04, rec)),
                             static_cast<uint32_t>(MEM_W(0x08, rec)),
                             bits_f32(MEM_W(0x0C, rec)), bits_f32(MEM_W(0x10, rec)),
                             bits_f32(MEM_W(0x14, rec)), camctrl,
                             static_cast<uint32_t>(MEM_W(0x1C, rec)),
                             static_cast<uint32_t>(MEM_W(0x18, rec)),
                             static_cast<uint32_t>(MEM_W(0x1C, rec)),
                             static_cast<uint32_t>(MEM_W(0x20, rec)),
                             bits_f32(MEM_W(0x20, rec)), bits_f32(MEM_W(0x24, rec)),
                             bits_f32(MEM_W(0x28, rec)),
                             bits_f32(MEM_W(0x2C, rec)), bits_f32(MEM_W(0x30, rec)),
                             bits_f32(MEM_W(0x34, rec)));
            }
        }
    }

    int16_t anim_id = -1;
    float anim_timer = 0.0f, anim_dur = 0.0f, anim_phase = 0.0f;
    uint8_t anim_type = 0, anim_flags = 0;
    const uint32_t anim_p = static_cast<uint32_t>(MEM_W(0x04, ps));
    if (is_guest(anim_p)) {
        const uint32_t anctrl_p = static_cast<uint32_t>(MEM_W(0x00, kseg0_gpr(anim_p)));
        if (is_guest(anctrl_p)) {
            const gpr ac = kseg0_gpr(anctrl_p);
            anim_dur = bits_f32(MEM_W(0x0C, ac));
            anim_timer = bits_f32(MEM_W(0x2C, ac));
            anim_type = static_cast<uint8_t>(MEM_BU(0x1A, ac));
            anim_flags = static_cast<uint8_t>(MEM_BU(0x1B, ac));
            anim_id = static_cast<int16_t>(MEM_H(0x34, ac));
            anim_phase = (anim_dur > 0.0f) ? (anim_timer / anim_dur) : 0.0f;
        }
    }

    std::fprintf(stderr,
                 "[demo-sync] glidx=%d post=%d tick=%llu wall_ms=%lld gfx=%llu adv=%d "
                 "pad=(%d,%d,%04x) remap=(%.2f,%.2f) bstick=(%.2f,%.2f,%.1f,%.2f) "
                 "bs=%d pos=(%.2f,%.2f,%.2f) ppos=(%.2f,%.2f,%.2f) "
                 "vel=(%.2f,%.2f,%.2f) velxz=%.2f tspd=%.1f "
                 "yaw=%.2f ideal=%.2f phys_yaw=%.2f mult=%.4f accel=%.2f "
                 "cam=(%.2f,%.2f,%.2f,%.1f) camctrl=%.4f cammode=%d sel=%d "
                 "anim=(%d,%.3f,%.3f,%d,0x%02x,%.3f) "
                 "egg_slot=%d egg_enum=%d stage=%d sub=%d "
                 "digger=(%.2f,%.2f,%.2f) act2=(%u,%08x,%.2f,%.2f,%.2f) "
                 "gs=%.5f fc=%d cnt=%d map=%d\n",
                 consumed_glidx, post_glidx, static_cast<unsigned long long>(tick), static_cast<long long>(wall_ms),
                 static_cast<unsigned long long>(host_gfx), gl_adv,
                 (int)rec_x, (int)rec_y, (unsigned)rec_btn, rmx, rmy,
                 bstick_x, bstick_y, bstick_ang, bstick_dist,
                 bs_id, px, py, pz, ppx, ppy, ppz,
                 vx, vy, vz, velxz, tspd,
                 yaw, yaw_ideal, phys_yaw, mult, accel,
                 cx, cy, cz, cam_yaw, camctrl, cam_mode, cam_sel,
                 anim_id, anim_timer, anim_dur, int(anim_type), unsigned(anim_flags), anim_phase,
                 egg_slot, egg_enum, stage_v, sub_v,
                 dig_x, dig_y, dig_z, act2_mid, act2_sys, act2_x, act2_y, act2_z,
                 gamespeed, gfcnt, gl_count, map_v);
    std::fflush(stderr);
}

// Demo7 drift diagnostic (TOOIE_DIAG_DEMO7_DRIFT=1), default OFF, isolated from
// TOOIE_DIAG_DEMO_SYNC / movement diagnostics. Called at the leave of glrecord
// overlay 698 ep0: the recorded pad for consumed_glidx was already read, the
// guest player update for that frame has not run yet (values = state at start
// of the interval, same convention as [demo-sync]). Player slot 0 only, no
// actor scan. One row per consumed glidx, max 121 rows per cycle.
// bs ids: prev/cur/next are +0/+4/+8 of *(player+0x120) (see tooie_demo7_drift.h).
// bstick ang is 90 when stick is neutral (not a real heading).
// Counts glrecord overlay-698 ep0 leaves (all stages) while the flag is on; used by [demo7-trace].
static uint64_t s_d7_logic_tick = 0;
extern "C" void tooie_demo7_drift_tick(uint8_t* rdram) {
    ++s_d7_logic_tick;
    const int stage_v = int(MEM_BU(0x0, kseg0_gpr(0x80135470u)));
    const int sub_v = int(MEM_BU(0x1, kseg0_gpr(0x80135470u)));
    static int32_t s_last_glidx = -1;
    if (stage_v != tooie_demo7_drift::kStage || sub_v != tooie_demo7_drift::kSub) {
        s_last_glidx = -1;
        return;
    }
    auto bits_f32 = [](int32_t bits) -> float {
        float f = 0.0f;
        const uint32_t u = static_cast<uint32_t>(bits);
        std::memcpy(&f, &u, sizeof(f));
        return f;
    };
    auto is_guest = [](uint32_t a) {
        return (a >= 0x80000000u) && ((a + 0x80000000u) < 0x00800000u) && ((a & 3u) == 0u);
    };
    const gpr gl = kseg0_gpr(0x8012D5B0u);
    const uint32_t gl_base = static_cast<uint32_t>(MEM_W(0, gl));
    const int32_t post = static_cast<int32_t>(MEM_W(0x8, gl));
    const int32_t cg = (post > 0) ? (post - 1) : 0;
    if (!tooie_demo7_drift::in_window(stage_v, sub_v, cg) || cg == s_last_glidx) {
        return;
    }
    const uint32_t ps_addr = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(0x80135490u)));
    if (!is_guest(ps_addr)) {
        return;
    }
    s_last_glidx = cg;
    const gpr ps = kseg0_gpr(ps_addr);
    const uint32_t pos_a = static_cast<uint32_t>(MEM_W(0xE4, ps));
    const uint32_t phys_a = static_cast<uint32_t>(MEM_W(0xC8, ps));
    const uint32_t bs_a = static_cast<uint32_t>(MEM_W(0x120, ps));
    const uint32_t st_a = static_cast<uint32_t>(MEM_W(0x128, ps));
    float px = 0, py = 0, pz = 0, ppx = 0, ppy = 0, ppz = 0, vx = 0, vy = 0, vz = 0, tspd = 0;
    float bsx = 0, bsy = 0, bsang = 0, bsd = 0;
    int32_t bs_prev = -1, bs_cur = -1, bs_next = -1, ptype = -1;
    if (is_guest(pos_a)) {
        const gpr p = kseg0_gpr(pos_a);
        px = bits_f32(MEM_W(0x0, p)); py = bits_f32(MEM_W(0x4, p)); pz = bits_f32(MEM_W(0x8, p));
    }
    if (is_guest(phys_a)) {
        const gpr p = kseg0_gpr(phys_a);
        ptype = static_cast<int32_t>(MEM_W(0x0, p));
        ppx = bits_f32(MEM_W(0x04, p)); ppy = bits_f32(MEM_W(0x08, p)); ppz = bits_f32(MEM_W(0x0C, p));
        vx = bits_f32(MEM_W(0x10, p)); vy = bits_f32(MEM_W(0x14, p)); vz = bits_f32(MEM_W(0x18, p));
        tspd = bits_f32(MEM_W(0x3C, p));
    }
    if (is_guest(bs_a)) {
        const gpr b = kseg0_gpr(bs_a);
        bs_prev = static_cast<int32_t>(MEM_W(tooie_demo7_drift::kBsPrevOff, b));
        bs_cur = static_cast<int32_t>(MEM_W(tooie_demo7_drift::kBsCurOff, b));
        bs_next = static_cast<int32_t>(MEM_W(tooie_demo7_drift::kBsNextOff, b));
    }
    if (is_guest(st_a)) {
        const gpr s = kseg0_gpr(st_a);
        bsx = bits_f32(MEM_W(0x54, s)); bsy = bits_f32(MEM_W(0x58, s));
        bsang = bits_f32(MEM_W(0x5C, s)); bsd = bits_f32(MEM_W(0x60, s));
    }
    int rec_x = 0, rec_y = 0, rec_btn = 0;
    if (is_guest(gl_base)) {
        const gpr r = kseg0_gpr(gl_base + static_cast<uint32_t>(cg) * 6u);
        rec_x = int8_t(MEM_B(0, r)); rec_y = int8_t(MEM_B(1, r)); rec_btn = int(MEM_HU(2, r));
    }
    const gpr remap = kseg0_gpr(0x80079AF8u);
    int16_t anim_id = -1;
    float anim_timer = 0.0f, anim_dur = 0.0f;
    const uint32_t anim_p = static_cast<uint32_t>(MEM_W(0x04, ps));
    if (is_guest(anim_p)) {
        const uint32_t ac_p = static_cast<uint32_t>(MEM_W(0x00, kseg0_gpr(anim_p)));
        if (is_guest(ac_p)) {
            const gpr ac = kseg0_gpr(ac_p);
            anim_dur = bits_f32(MEM_W(0x0C, ac));
            anim_timer = bits_f32(MEM_W(0x2C, ac));
            anim_id = static_cast<int16_t>(MEM_H(0x34, ac));
        }
    }
    float cx = 0, cy = 0, cz = 0;
    const uint32_t vp_a = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(0x8012D500u)));
    if (is_guest(vp_a)) {
        const gpr vp = kseg0_gpr(vp_a);
        cx = bits_f32(MEM_W(0x0, vp)); cy = bits_f32(MEM_W(0x4, vp)); cz = bits_f32(MEM_W(0x8, vp));
    }
    const float gs = bits_f32(MEM_W(0, kseg0_gpr(0x8012C760u)));
    std::fprintf(stderr,
                 "[demo7-drift] stage=%d sub=%d glidx=%d post=%d pad=(%d,%d,%04x) "
                 "remap=(%.2f,%.2f) bstick=(%.2f,%.2f,ang=%.1f,dist=%.2f) "
                 "bs_prev=%d bs_cur=%d bs_next=%d ptype=%d "
                 "pos=(%.2f,%.2f,%.2f) ppos=(%.2f,%.2f,%.2f) vel=(%.2f,%.2f,%.2f) velxz=%.2f tspd=%.1f "
                 "anim=(%d,%.3f,%.3f) cam=(%.2f,%.2f,%.2f) gs=%.5f\n",
                 stage_v, sub_v, cg, post, rec_x, rec_y, unsigned(rec_btn),
                 bits_f32(MEM_W(8, remap)), bits_f32(MEM_W(12, remap)),
                 bsx, bsy, bsang, bsd, bs_prev, bs_cur, bs_next, ptype,
                 px, py, pz, ppx, ppy, ppz, vx, vy, vz, std::sqrt(vx * vx + vz * vz), tspd,
                 anim_id, anim_timer, anim_dur, cx, cy, cz, gs);
}

// No-op capture ABI. Production frustum correction does not log.
extern "C" uint64_t tooie_get_monotonic_ns() { return 0; }
extern "C" uint32_t tooie_get_current_guest_frame() { return 0; }
extern "C" uint32_t tooie_get_current_capture_id() { return 0; }
extern "C" bool tooie_is_continuous_capture_active() { return false; }
extern "C" void tooie_ws_log_event(const char* /*fmt*/, ...) {}
extern "C" uint64_t tooie_ws_log_dl_rsp(uint32_t, uint32_t, uint32_t,
                                       uint32_t, uint32_t, uint32_t,
                                       uint64_t, uint64_t) {
    return 0;
}
extern "C" void tooie_ws_log_prim_emit(uint32_t, uint32_t, uint64_t,
                                       const char*, uint64_t, uint32_t,
                                       uint32_t, uint32_t, uint32_t, uint32_t) {}

// Production widescreen frustum correction (P08 gameplay maps + models).
// 800CAA24: horizontal-only aspect-aware AABB retest. k = target/source.
// 800CACEC: same H-expand only while a draw scope is open. Open scopes:
// func_8011447C (kind 2, preserved object-pop path) and func_800B97A4
// (kind 3, pure-draw allowlist in tooie_demo1_projectile.h). func_800E3E8C
// does not open a scope, so func_800DE498 and unclassified logic/FX keep
// the original v0. Disable with TOOIE_TEST_*_ASPECT_AWARE=0.
// Does not write expanded planes back into guest volume memory.
static bool tooie_ws_env_explicit_off(const char* name) {
    const char* val = std::getenv(name);
    return val != nullptr && val[0] == '0';
}

static bool tooie_map_vis_aspect_aware_enabled() {
    static const bool enabled = !tooie_ws_env_explicit_off("TOOIE_TEST_MAP_VIS_ASPECT_AWARE");
    return enabled;
}

static bool tooie_object_frustum_aspect_aware_enabled() {
    static const bool enabled = !tooie_ws_env_explicit_off("TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE");
    return enabled;
}

static thread_local int s_cacec_draw_consumer = 0;
// Diagnostic-only context of the innermost expansion scope (2 = func_8011447C, 3 = func_800B97A4) and the origin of its caller.
// Kind 1 (func_800E3E8C) is retired: that wrapper no longer opens a scope.
static thread_local int s_cacec_scope_kind = 0;
static thread_local int32_t s_cacec_scope_origin = 0;

struct TooieCacecDrawScope {
    int saved_kind;
    int32_t saved_origin;
    TooieCacecDrawScope(int kind, int32_t origin) : saved_kind(s_cacec_scope_kind), saved_origin(s_cacec_scope_origin) {
        ++s_cacec_draw_consumer;
        s_cacec_scope_kind = kind;
        s_cacec_scope_origin = origin;
    }
    ~TooieCacecDrawScope() {
        --s_cacec_draw_consumer;
        s_cacec_scope_kind = saved_kind;
        s_cacec_scope_origin = saved_origin;
    }
};

// TOOIE_DIAG_DEMO1_TARGET RNG/CACEC trace hooks (defined with the other d1t code below; no-ops while the flag is off).
extern "C" __attribute__((noinline, used)) void tooie_d1x_anchor() {}
static int32_t tooie_d1x_origin(const void* ret) {
    return static_cast<int32_t>(reinterpret_cast<intptr_t>(ret) - reinterpret_cast<intptr_t>(&tooie_d1x_anchor));
}
namespace banjo {
static void tooie_d1x_cacec_extra(uint8_t* rdram, uint32_t vp, int32_t origin, uint32_t radius_bits, float px, float py, float pz);
static void tooie_d1x_e3e8c(uint8_t* rdram, uint32_t pos_ptr, uint32_t radius_bits, int32_t origin, uint32_t result,
                            uint32_t extra_before);
static uint32_t tooie_d1x_extra_count();
}  // namespace banjo

static inline bool tooie_ws_guest_safe(uint32_t a, size_t size = 1) {
    const uint32_t p = a & 0x1FFFFFFFu;
    return (p + size <= 0x00800000u) && (a >= 0x80000000u && a < 0xC0000000u);
}

static inline float tooie_ws_read_f32(const uint8_t* rdram, uint32_t a) {
    if (!tooie_ws_guest_safe(a, 4)) {
        return 0.0f;
    }
    const uint32_t p = a & 0x00FFFFFFu;
    float val = 0.0f;
    std::memcpy(&val, rdram + p, sizeof(val));
    return val;
}

static float tooie_map_vis_target_aspect() {
    const float original = 4.0f / 3.0f;
    const ultramodern::renderer::GraphicsConfig graphics_config =
        ultramodern::renderer::get_graphics_config();
    int width = 0;
    int height = 0;
    recompui::get_window_size(width, height);
    return tooie_window::calculate_target_aspect_ratio(
        graphics_config.ar_option, width, height, original);
}

static float tooie_map_vis_k_for_volume(const uint8_t* rdram, uint32_t volume_ptr) {
    float source = tooie_ws_read_f32(rdram, volume_ptr + 0xE8);
    if (!(source > 0.0f) || !std::isfinite(source)) {
        source = 4.0f / 3.0f;
    }
    const float target = tooie_map_vis_target_aspect();
    if (!(source > 0.0f) || !std::isfinite(target)) {
        return 1.0f;
    }
    float k = target / source;
    if (!(k > 1.0f) || !std::isfinite(k)) {
        return 1.0f;
    }
    return k;
}

static void tooie_ws_expand_h_plane(float& nx, float& ny, float& nz, float& d,
                                    const float cam[3], const float fwd[3], float k) {
    const float nf = nx * fwd[0] + ny * fwd[1] + nz * fwd[2];
    nx = fwd[0] * nf + (nx - fwd[0] * nf) / k;
    ny = fwd[1] * nf + (ny - fwd[1] * nf) / k;
    nz = fwd[2] * nf + (nz - fwd[2] * nf) / k;
    d = -(cam[0] * nx + cam[1] * ny + cam[2] * nz);
}

static uint32_t tooie_map_vis_aspect_eval_float_aabb(
    const uint8_t* rdram, uint32_t volume_ptr, uint32_t min_ptr, uint32_t max_ptr, float k) {
    if (!tooie_ws_guest_safe(volume_ptr, 0x64) ||
        !tooie_ws_guest_safe(min_ptr, 12) ||
        !tooie_ws_guest_safe(max_ptr, 12)) {
        return 0xFFFFFFFFu;
    }

    const float minv[3] = {
        tooie_ws_read_f32(rdram, min_ptr + 0),
        tooie_ws_read_f32(rdram, min_ptr + 4),
        tooie_ws_read_f32(rdram, min_ptr + 8),
    };
    const float maxv[3] = {
        tooie_ws_read_f32(rdram, max_ptr + 0),
        tooie_ws_read_f32(rdram, max_ptr + 4),
        tooie_ws_read_f32(rdram, max_ptr + 8),
    };
    const float cam[3] = {
        tooie_ws_read_f32(rdram, volume_ptr + 0x0),
        tooie_ws_read_f32(rdram, volume_ptr + 0x4),
        tooie_ws_read_f32(rdram, volume_ptr + 0x8),
    };
    float fwd[3] = {
        tooie_ws_read_f32(rdram, volume_ptr + 0xC),
        tooie_ws_read_f32(rdram, volume_ptr + 0x10),
        tooie_ws_read_f32(rdram, volume_ptr + 0x14),
    };
    const float flen = std::sqrt(fwd[0] * fwd[0] + fwd[1] * fwd[1] + fwd[2] * fwd[2]);
    if (!(flen > 1.0e-6f) || !std::isfinite(flen)) {
        return 0xFFFFFFFFu;
    }
    fwd[0] /= flen;
    fwd[1] /= flen;
    fwd[2] /= flen;

    for (int pl = 0; pl < 4; ++pl) {
        float nx = tooie_ws_read_f32(rdram, volume_ptr + 0x24 + pl * 0x10);
        float ny = tooie_ws_read_f32(rdram, volume_ptr + 0x28 + pl * 0x10);
        float nz = tooie_ws_read_f32(rdram, volume_ptr + 0x2C + pl * 0x10);
        float d  = tooie_ws_read_f32(rdram, volume_ptr + 0x30 + pl * 0x10);
        if (pl < 2 && k > 1.0f) {
            tooie_ws_expand_h_plane(nx, ny, nz, d, cam, fwd, k);
        }
        const float px = (nx >= 0.0f) ? minv[0] : maxv[0];
        const float py = (ny >= 0.0f) ? minv[1] : maxv[1];
        const float pz = (nz >= 0.0f) ? minv[2] : maxv[2];
        if ((nx * px + ny * py + nz * pz + d) >= 0.0f) {
            return 0;
        }
    }
    return 1;
}

static uint32_t tooie_cacec_eval_sphere(
    const uint8_t* rdram, uint32_t volume_ptr,
    float x, float y, float z, float radius, float k) {
    if (!tooie_ws_guest_safe(volume_ptr, 0x64) || !std::isfinite(radius) || radius < 0.0f) {
        return 0xFFFFFFFFu;
    }
    const float cam[3] = {
        tooie_ws_read_f32(rdram, volume_ptr + 0x0),
        tooie_ws_read_f32(rdram, volume_ptr + 0x4),
        tooie_ws_read_f32(rdram, volume_ptr + 0x8),
    };
    float fwd[3] = {
        tooie_ws_read_f32(rdram, volume_ptr + 0xC),
        tooie_ws_read_f32(rdram, volume_ptr + 0x10),
        tooie_ws_read_f32(rdram, volume_ptr + 0x14),
    };
    const float flen = std::sqrt(fwd[0] * fwd[0] + fwd[1] * fwd[1] + fwd[2] * fwd[2]);
    if (!(flen > 1.0e-6f) || !std::isfinite(flen)) {
        return 0xFFFFFFFFu;
    }
    fwd[0] /= flen;
    fwd[1] /= flen;
    fwd[2] /= flen;
    const float rel[3] = {x - cam[0], y - cam[1], z - cam[2]};
    for (int pl = 0; pl < 4; ++pl) {
        float nx = tooie_ws_read_f32(rdram, volume_ptr + 0x24 + pl * 0x10);
        float ny = tooie_ws_read_f32(rdram, volume_ptr + 0x28 + pl * 0x10);
        float nz = tooie_ws_read_f32(rdram, volume_ptr + 0x2C + pl * 0x10);
        if (pl < 2 && k > 1.0f) {
            float d_unused = 0.0f;
            tooie_ws_expand_h_plane(nx, ny, nz, d_unused, cam, fwd, k);
        }
        const float dot = nx * rel[0] + ny * rel[1] + nz * rel[2];
        if (radius <= dot) {
            return 0;
        }
    }
    return 1;
}

static void tooie_wrap_func_800CAA24(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t volume_ptr = static_cast<uint32_t>(ctx->r4);
    const uint32_t min_ptr = static_cast<uint32_t>(ctx->r5);
    const uint32_t max_ptr = static_cast<uint32_t>(ctx->r6);
    func_800CAA24(rdram, ctx);
    if (!tooie_map_vis_aspect_aware_enabled()) {
        return;
    }
    const uint32_t orig = static_cast<uint32_t>(ctx->r2);
    const float k = tooie_map_vis_k_for_volume(rdram, volume_ptr);
    if (!(k > 1.0f) || orig != 0) {
        return;
    }
    const uint32_t aspect_vis =
        tooie_map_vis_aspect_eval_float_aabb(rdram, volume_ptr, min_ptr, max_ptr, k);
    if (aspect_vis == 1) {
        ctx->r2 = 1;
    }
}

static void tooie_wrap_func_800CACEC(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t vp = static_cast<uint32_t>(ctx->r4);
    const uint32_t pos_p = static_cast<uint32_t>(ctx->r5);
    const uint32_t r6_u = static_cast<uint32_t>(ctx->r6);
    float radius = 0.0f;
    std::memcpy(&radius, &r6_u, sizeof(radius));
    const float px = tooie_ws_read_f32(rdram, pos_p);
    const float py = tooie_ws_read_f32(rdram, pos_p + 4);
    const float pz = tooie_ws_read_f32(rdram, pos_p + 8);
    func_800CACEC(rdram, ctx);
    if (!tooie_object_frustum_aspect_aware_enabled()) {
        return;
    }
    const uint32_t orig_res = static_cast<uint32_t>(ctx->r2);
    if (orig_res != 0 || s_cacec_draw_consumer <= 0) {
        return;
    }
    const float k = tooie_ws_guest_safe(vp, 0xECu)
        ? tooie_map_vis_k_for_volume(rdram, vp) : 1.0f;
    if (!(k > 1.0f)) {
        return;
    }
    const uint32_t expanded = tooie_cacec_eval_sphere(rdram, vp, px, py, pz, radius, k);
    if (expanded == 1) {
        ctx->r2 = 1;
        banjo::tooie_d1x_cacec_extra(rdram, vp, tooie_d1x_origin(__builtin_return_address(0)), r6_u, px, py, pz);
    }
}

__attribute__((noinline)) static void tooie_wrap_func_800E3E8C(uint8_t* rdram, recomp_context* ctx) {
    const int32_t origin = tooie_d1x_origin(__builtin_return_address(0));
    const uint32_t pos_ptr = static_cast<uint32_t>(ctx->r4), radius_bits = static_cast<uint32_t>(ctx->r5);
    const uint32_t extra_before = banjo::tooie_d1x_extra_count();
    func_800E3E8C(rdram, ctx);
    banjo::tooie_d1x_e3e8c(rdram, pos_ptr, radius_bits, origin, static_cast<uint32_t>(ctx->r2), extra_before);
}

__attribute__((noinline)) static void tooie_wrap_func_8011447C(uint8_t* rdram, recomp_context* ctx) {
    TooieCacecDrawScope draw_scope(2, tooie_d1x_origin(__builtin_return_address(0)));
    func_8011447C(rdram, ctx);
}

// LOOKUP-only model draw (funcs_123.c). Scope opens only when the shared allowlist names this VRAM.
__attribute__((noinline)) static void tooie_wrap_func_800B97A4(uint8_t* rdram, recomp_context* ctx) {
    if (tooie_demo1_projectile::tooie_cacec_pure_draw_vram(tooie_demo1_projectile::kCacecPureDrawB97A4)) {
        TooieCacecDrawScope draw_scope(tooie_demo1_projectile::kCacecScopeKindB97A4,
                                       tooie_d1x_origin(__builtin_return_address(0)));
        func_800B97A4(rdram, ctx);
        return;
    }
    func_800B97A4(rdram, ctx);
}

namespace banjo {
// Demo7 write trace (TOOIE_DIAG_DEMO7_DRIFT=1 only; wrappers are NOT registered otherwise).
// Wraps original 0x8009B9B0 baphysics_set_target_horizontal_velocity (a0=player, a1=raw float
// bits stored to *(ps+0xC8)+0x3C) and 0x8009E5C8 bs_setState (a0=player, a1=state id; the
// function starts at 0x8009E5C8, 0x8009E5D8 is only its 4th instruction). Each wrapper calls
// the generated original exactly once with the untouched ctx. Rows only for player slot 0
// inside stage0/sub8 consumed glidx tooie_demo7_drift::kTraceLoGlidx..kTraceHiGlidx.
struct tooie_d7_snap {
    float tspd = 0, vx = 0, vy = 0, vz = 0;
    int32_t prev = -1, cur = -1, next = -1;
};
static uint32_t s_d7t_seq = 0;
static uint32_t s_d7t_rows = 0;
static uint32_t s_d7t_other = 0;

static float tooie_d7_f32(uint32_t u) {
    float f = 0.0f;
    std::memcpy(&f, &u, sizeof(f));
    return f;
}

static bool tooie_d7_is_guest(uint32_t a) {
    return (a >= 0x80000000u) && ((a + 0x80000000u) < 0x00800000u) && ((a & 3u) == 0u);
}

static tooie_d7_snap tooie_d7_take(uint8_t* rdram, uint32_t ps_addr) {
    tooie_d7_snap s;
    if (!tooie_d7_is_guest(ps_addr)) {
        return s;
    }
    const gpr ps = kseg0_gpr(ps_addr);
    const uint32_t phys = static_cast<uint32_t>(MEM_W(0xC8, ps));
    const uint32_t bs = static_cast<uint32_t>(MEM_W(0x120, ps));
    if (tooie_d7_is_guest(phys)) {
        const gpr p = kseg0_gpr(phys);
        s.tspd = tooie_d7_f32(static_cast<uint32_t>(MEM_W(0x3C, p)));
        s.vx = tooie_d7_f32(static_cast<uint32_t>(MEM_W(0x10, p)));
        s.vy = tooie_d7_f32(static_cast<uint32_t>(MEM_W(0x14, p)));
        s.vz = tooie_d7_f32(static_cast<uint32_t>(MEM_W(0x18, p)));
    }
    if (tooie_d7_is_guest(bs)) {
        const gpr b = kseg0_gpr(bs);
        s.prev = static_cast<int32_t>(MEM_W(tooie_demo7_drift::kBsPrevOff, b));
        s.cur = static_cast<int32_t>(MEM_W(tooie_demo7_drift::kBsCurOff, b));
        s.next = static_cast<int32_t>(MEM_W(tooie_demo7_drift::kBsNextOff, b));
    }
    return s;
}

// True when this call must be traced; counts player-slot-0 mismatches in the window.
static bool tooie_d7_gate(uint8_t* rdram, uint32_t a0, int* post_out) {
    const int stage = int(MEM_BU(0x0, kseg0_gpr(0x80135470u)));
    const int sub = int(MEM_BU(0x1, kseg0_gpr(0x80135470u)));
    const int post = static_cast<int32_t>(MEM_W(0x8, kseg0_gpr(0x8012D5B0u)));
    if (!tooie_demo7_drift::in_trace_window(stage, sub, post)) {
        return false;
    }
    if (a0 != static_cast<uint32_t>(MEM_W(0, kseg0_gpr(0x80135490u)))) {
        ++s_d7t_other;
        return false;
    }
    if (s_d7t_rows >= static_cast<uint32_t>(tooie_demo7_drift::kTraceMaxRows)) {
        return false;
    }
    ++s_d7t_rows;
    *post_out = post;
    return true;
}

static void tooie_d7_row(const char* what, const char* ph, uint32_t seq, int post, uint32_t ra,
                         uint32_t a0, uint32_t a1, const tooie_d7_snap& s) {
    std::fprintf(stderr,
                 "[demo7-trace] seq=%u %s %s post=%d glidx=%d tick=%llu ra=%08x a0=%08x a1=%08x(%.6f) "
                 "bs=%d/%d/%d tspd=%.4f vel=(%.4f,%.4f,%.4f) other=%u\n",
                 seq, what, ph, post, (post > 0) ? post - 1 : 0,
                 static_cast<unsigned long long>(s_d7_logic_tick), ra, a0, a1,
                 tooie_d7_f32(a1), s.prev, s.cur, s.next, s.tspd, s.vx, s.vy, s.vz, s_d7t_other);
}

static void tooie_d7_wrap_tspd_set(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t a1 = static_cast<uint32_t>(ctx->r5);
    int post = 0;
    const bool on = tooie_d7_gate(rdram, a0, &post);
    uint32_t seq = 0;
    tooie_d7_snap pre;
    if (on) {
        seq = ++s_d7t_seq;
        pre = tooie_d7_take(rdram, a0);
    }
    baphysics_set_target_horizontal_velocity(rdram, ctx);
    if (on) {
        const tooie_d7_snap post_s = tooie_d7_take(rdram, a0);
        tooie_d7_row("tspd_set", "pre ", seq, post, ra, a0, a1, pre);
        tooie_d7_row("tspd_set", "post", seq, post, ra, a0, a1, post_s);
    }
}

static void tooie_d7_wrap_bs_setstate(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t a1 = static_cast<uint32_t>(ctx->r5);
    int post = 0;
    const bool on = tooie_d7_gate(rdram, a0, &post);
    uint32_t seq = 0;
    if (on) {
        seq = ++s_d7t_seq;
        tooie_d7_row("bs_setState", "enter", seq, post, ra, a0, a1, tooie_d7_take(rdram, a0));
    }
    bs_setState(rdram, ctx);
    if (on) {
        tooie_d7_row("bs_setState", "leave", seq, post, ra, a0, a1, tooie_d7_take(rdram, a0));
    }
}

// USER_DEMO1 behavior parity (TOOIE_DIAG_DEMO1_PARITY=1, default OFF, isolated flag).
// Called at the ENTRY of func_800151BC (before the original runs, ctx untouched); one row per
// glrecord post for stage0/sub0/mode5, player slot 0 only, consumed glidx 0..420 (421 rows max).
// vi = entries of func_8001443C seen while the flag is on; seq = entries of 800151BC seen.
// elapsed_ns is host-monotonic and auxiliary only. Pure memory reads: no OSGetCount, no ctx use.
static bool s_d1_on = false;
static uint64_t s_d1_vi = 0;
static uint64_t s_d1_seq = 0;
static uint32_t s_d1_rows = 0;
static int32_t s_d1_last_post = -1;

static float tooie_d1_f(uint32_t u) {
    float f = 0.0f;
    std::memcpy(&f, &u, sizeof(f));
    return f;
}

static void tooie_demo1_parity_entry(uint8_t* rdram) {
    ++s_d1_seq;
    static bool s_hdr = false;
    static bool s_have_t0 = false;
    static std::chrono::steady_clock::time_point s_t0;
    const auto w = [&](uint32_t a) { return static_cast<uint32_t>(MEM_W(0, kseg0_gpr(a))); };
    const int mode = int(int16_t(MEM_H(0x2, kseg0_gpr(0x80127630u))));
    const int map = int(int16_t(MEM_H(0x10, kseg0_gpr(0x80127630u))));
    const int stage = int(MEM_BU(0x0, kseg0_gpr(0x80135470u)));
    const int sub = int(MEM_BU(0x1, kseg0_gpr(0x80135470u)));
    const int32_t post = static_cast<int32_t>(w(0x8012D5B8u));
    if (!tooie_demo1_parity::in_window(mode, stage, sub, post) || post == s_d1_last_post) {
        return;
    }
    const uint32_t ps_a = w(0x80135490u);
    if (!tooie_d7_is_guest(ps_a)) {
        return;
    }
    s_d1_last_post = post;
    if (!s_hdr) {
        s_hdr = true;
        std::fprintf(stderr, "[demo1-parity-header] %s\n", tooie_demo1_parity::kCsvHeader);
    }
    if (!s_have_t0) {
        s_t0 = std::chrono::steady_clock::now();
        s_have_t0 = true;
    }
    const unsigned long long elapsed = static_cast<unsigned long long>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - s_t0).count());
    const int consumed = post - 1;
    const uint32_t count = w(0x8012D5BCu);
    const uint32_t gs_b = w(0x8012C760u);
    // recorded raw 6 bytes at glrecord base + consumed*6 (last u16 meaning unknown, logged raw)
    unsigned r[6] = {0, 0, 0, 0, 0, 0};
    const uint32_t gl_base = w(0x8012D5B0u);
    if (tooie_d7_is_guest(gl_base)) {
        const gpr b = kseg0_gpr(gl_base + static_cast<uint32_t>(consumed) * 6u);
        for (int i = 0; i < 6; ++i) r[i] = unsigned(MEM_BU(i, b));
    }
    const uint32_t remap = 0x80079AF8u;
    float bx = 0, by = 0, bang = 0, bdist = 0;
    float yaw = 0, ideal = 0, phys_yaw = 0, accel = 0, mult = 0;
    uint32_t yaw_b = 0, tspd_b = 0, px_b = 0, py_b = 0, pz_b = 0, vx_b = 0, vz_b = 0;
    float px = 0, py = 0, pz = 0, ppx = 0, ppy = 0, ppz = 0, vx = 0, vy = 0, vz = 0, tspd = 0;
    int32_t ptype = -1, bs_prev = -1, bs_cur = -1, bs_next = -1, sel = -1, anim_id = -1;
    float anim_timer = 0, anim_dur = 0, anim_phase = 0, camx = 0, camy = 0, camz = 0, camctrl = 0;
    const gpr ps = kseg0_gpr(ps_a);
    const uint32_t pos_a = static_cast<uint32_t>(MEM_W(0xE4, ps));
    const uint32_t yaw_a = static_cast<uint32_t>(MEM_W(0xF8, ps));
    const uint32_t phys_a = static_cast<uint32_t>(MEM_W(0xC8, ps));
    const uint32_t bs_a = static_cast<uint32_t>(MEM_W(0x120, ps));
    const uint32_t st_a = static_cast<uint32_t>(MEM_W(0x128, ps));
    if (tooie_d7_is_guest(pos_a)) {
        const gpr p = kseg0_gpr(pos_a);
        px_b = static_cast<uint32_t>(MEM_W(0, p)); py_b = static_cast<uint32_t>(MEM_W(4, p));
        pz_b = static_cast<uint32_t>(MEM_W(8, p));
        px = tooie_d1_f(px_b); py = tooie_d1_f(py_b); pz = tooie_d1_f(pz_b);
    }
    if (tooie_d7_is_guest(yaw_a)) {
        const gpr p = kseg0_gpr(yaw_a);
        yaw_b = static_cast<uint32_t>(MEM_W(0, p));
        yaw = tooie_d1_f(yaw_b); ideal = tooie_d1_f(static_cast<uint32_t>(MEM_W(4, p)));
    }
    if (tooie_d7_is_guest(phys_a)) {
        const gpr p = kseg0_gpr(phys_a);
        ptype = static_cast<int32_t>(MEM_W(0, p));
        ppx = tooie_d1_f(static_cast<uint32_t>(MEM_W(0x04, p)));
        ppy = tooie_d1_f(static_cast<uint32_t>(MEM_W(0x08, p)));
        ppz = tooie_d1_f(static_cast<uint32_t>(MEM_W(0x0C, p)));
        vx_b = static_cast<uint32_t>(MEM_W(0x10, p));
        vx = tooie_d1_f(vx_b);
        vy = tooie_d1_f(static_cast<uint32_t>(MEM_W(0x14, p)));
        vz_b = static_cast<uint32_t>(MEM_W(0x18, p));
        vz = tooie_d1_f(vz_b);
        tspd_b = static_cast<uint32_t>(MEM_W(0x3C, p));
        tspd = tooie_d1_f(tspd_b);
        phys_yaw = tooie_d1_f(static_cast<uint32_t>(MEM_W(0x40, p)));
        accel = tooie_d1_f(static_cast<uint32_t>(MEM_W(0x44, p)));
        mult = tooie_d1_f(static_cast<uint32_t>(MEM_W(0x50, p)));
    }
    if (tooie_d7_is_guest(bs_a)) {
        const gpr b = kseg0_gpr(bs_a);
        bs_prev = static_cast<int32_t>(MEM_W(0, b)); bs_cur = static_cast<int32_t>(MEM_W(4, b));
        bs_next = static_cast<int32_t>(MEM_W(8, b));
    }
    if (tooie_d7_is_guest(st_a)) {
        const gpr s = kseg0_gpr(st_a);
        bx = tooie_d1_f(static_cast<uint32_t>(MEM_W(0x54, s)));
        by = tooie_d1_f(static_cast<uint32_t>(MEM_W(0x58, s)));
        bang = tooie_d1_f(static_cast<uint32_t>(MEM_W(0x5C, s)));
        bdist = tooie_d1_f(static_cast<uint32_t>(MEM_W(0x60, s)));
    }
    const uint32_t anim_p = static_cast<uint32_t>(MEM_W(0x04, ps));
    if (tooie_d7_is_guest(anim_p)) {
        const uint32_t ac_p = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(anim_p)));
        if (tooie_d7_is_guest(ac_p)) {
            const gpr ac = kseg0_gpr(ac_p);
            anim_dur = tooie_d1_f(static_cast<uint32_t>(MEM_W(0x0C, ac)));
            anim_timer = tooie_d1_f(static_cast<uint32_t>(MEM_W(0x2C, ac)));
            anim_id = int32_t(int16_t(MEM_H(0x34, ac)));
            anim_phase = (anim_dur > 0.0f) ? (anim_timer / anim_dur) : 0.0f;
        }
    }
    const uint32_t vp_a = w(0x8012D500u);
    if (tooie_d7_is_guest(vp_a)) {
        const gpr vp = kseg0_gpr(vp_a);
        camx = tooie_d1_f(static_cast<uint32_t>(MEM_W(0, vp)));
        camy = tooie_d1_f(static_cast<uint32_t>(MEM_W(4, vp)));
        camz = tooie_d1_f(static_cast<uint32_t>(MEM_W(8, vp)));
    }
    sel = static_cast<int32_t>(MEM_W(0x184, ps));
    if (sel >= 0 && sel < 8) {
        const uint32_t rec = w(0x80127708u + static_cast<uint32_t>(sel) * 4u);
        if (tooie_d7_is_guest(rec)) {
            camctrl = tooie_d1_f(static_cast<uint32_t>(MEM_W(0x1C, kseg0_gpr(rec))));
        }
    }
    ++s_d1_rows;
    std::fprintf(stderr, "[demo1-parity] ");
    std::fprintf(stderr, tooie_demo1_parity::kRowFmt,
                 "entry_800151BC", 0x800151BCu, static_cast<unsigned long long>(s_d1_seq), elapsed,
                 static_cast<unsigned long long>(s_d1_vi), gs_b, tooie_d1_f(gs_b),
                 mode, stage, sub, map, post, consumed, int(count),
                 r[0], r[1], r[2], r[3], r[4], r[5],
                 tooie_d1_f(static_cast<uint32_t>(MEM_W(8, kseg0_gpr(remap)))),
                 tooie_d1_f(static_cast<uint32_t>(MEM_W(12, kseg0_gpr(remap)))),
                 bx, by, bang, bdist, yaw, ideal, phys_yaw, yaw_b,
                 px, py, pz, px_b, py_b, pz_b, ppx, ppy, ppz, vx, vy, vz, vx_b, vz_b,
                 std::sqrt(vx * vx + vz * vz), tspd, tspd_b, accel, mult, ptype,
                 bs_prev, bs_cur, bs_next, anim_id, anim_timer, anim_dur, anim_phase,
                 camx, camy, camz, camctrl, sel);
}

// USER_DEMO1 projectile chain (TOOIE_DIAG_DEMO1_PROJECTILE=1, default OFF, isolated flag).
// Wrappers are registered ONLY when the flag is on (register_boot_lookup_stubs). Each wrapper reads
// ctx->r4..r7 (a0..a3), calls the generated original EXACTLY ONCE with the untouched ctx, then reads
// ctx->r2 (v0). No RA logged (recomp ctx->r31 is not a trustworthy caller). Pure memory reads.
// Coverage: all generated callers of these five functions go through LOOKUP_FUNC (no direct named
// calls; tests/demo1_projectile_coverage_test.sh); overlay-relative callback pointers are NOT covered.
// Liveness: objects are identified by ordinal; stable key = owner word *(obj+0) (sweep a0). A call with a
// new pointer but a known owner is an ALIAS (relocation candidate): tracked pointer is updated and an
// "alias" row is logged. Untracked sueggmove9/sweep38 calls are logged (obj=-1) up to kMaxUntracked.
static bool s_d1p_on = false;
static uint64_t s_d1p_tick = 0;
static uint64_t s_d1p_seq = 0;
static uint32_t s_d1p_rows = 0;
static uint32_t s_d1p_objs[tooie_demo1_projectile::kMaxObjs] = {};
static uint32_t s_d1p_owner[tooie_demo1_projectile::kMaxObjs] = {};
static int s_d1p_nobj = 0;
static uint32_t s_d1p_alias = 0, s_d1p_untracked = 0;
static bool s_d1p_started = false, s_d1p_finished = false, s_d1p_hdr = false;

struct tooie_d1p_ev {
    uint32_t w[12] = {};
    unsigned vmask = 0;
};

static bool tooie_d1p_gate(uint8_t* rdram, int* post_out) {
    const int mode = int(int16_t(MEM_H(0x2, kseg0_gpr(0x80127630u))));
    const int stage = int(MEM_BU(0x0, kseg0_gpr(0x80135470u)));
    const int sub = int(MEM_BU(0x1, kseg0_gpr(0x80135470u)));
    const int post = static_cast<int32_t>(MEM_W(0, kseg0_gpr(0x8012D5B8u)));
    *post_out = post;
    return tooie_demo1_projectile::in_window(mode, stage, sub, post);
}

static void tooie_d1p_row(uint8_t* rdram, const char* ev, const char* ph, int post, int obj, uint32_t a0,
                          uint32_t a1, uint32_t a2, uint32_t a3, uint32_t v0, tooie_d1p_ev e) {
    if (!s_d1p_hdr) {
        s_d1p_hdr = true;
        std::fprintf(stderr, "[demo1-proj-header] %s\n", tooie_demo1_projectile::kCsvHeader);
    }
    ++s_d1p_rows;
    if (std::strcmp(ph, "mark") != 0) {
        e.w[11] = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(0x8012C760u)));  // gs at this callback
    }
    std::fprintf(stderr, "[demo1-proj] ");
    std::fprintf(stderr, tooie_demo1_projectile::kRowFmt, ev, ph,
                 static_cast<unsigned long long>(s_d1p_tick), static_cast<unsigned long long>(++s_d1p_seq),
                 post, post - 1, obj, a0, a1, a2, a3, v0, e.vmask,
                 e.w[0], e.w[1], e.w[2], e.w[3], e.w[4], e.w[5], e.w[6], e.w[7], e.w[8], e.w[9], e.w[10], e.w[11],
                 tooie_d7_f32(e.w[0]), tooie_d7_f32(e.w[1]), tooie_d7_f32(e.w[2]), tooie_d7_f32(e.w[3]),
                 tooie_d7_f32(e.w[4]), tooie_d7_f32(e.w[5]), tooie_d7_f32(e.w[6]), tooie_d7_f32(e.w[7]),
                 tooie_d7_f32(e.w[8]), tooie_d7_f32(e.w[9]), tooie_d7_f32(e.w[10]), tooie_d7_f32(e.w[11]));
}

// 3 raw words at guest pointer p -> e.w[idx..idx+2]; sets vmask bit when p is a guest pointer.
static void tooie_d1p_vec(uint8_t* rdram, tooie_d1p_ev& e, int idx, uint32_t p, int bit) {
    if (!tooie_d7_is_guest(p)) return;
    const gpr g = kseg0_gpr(p);
    for (int i = 0; i < 3; ++i) e.w[idx + i] = static_cast<uint32_t>(MEM_W(4 * i, g));
    e.vmask |= 1u << bit;
}

static uint32_t tooie_d1p_owner_of(uint8_t* rdram, uint32_t obj) {
    return tooie_d7_is_guest(obj) ? static_cast<uint32_t>(MEM_W(0, kseg0_gpr(obj))) : 0u;
}

// Spawned-object layout: +4..0xC, +0x18..0x20, +0x38, +0x6C (raw; NOT an id), +0x70 raw, +0x72 byte,
// w10 = owner word *(obj+0).
static tooie_d1p_ev tooie_d1p_obj(uint8_t* rdram, uint32_t obj) {
    tooie_d1p_ev e;
    if (!tooie_d7_is_guest(obj)) return e;
    const gpr g = kseg0_gpr(obj);
    tooie_d1p_vec(rdram, e, 0, obj + 4, 0);
    tooie_d1p_vec(rdram, e, 3, obj + 0x18, 1);
    e.w[6] = static_cast<uint32_t>(MEM_W(0x38, g));
    e.w[7] = static_cast<uint32_t>(MEM_W(0x6C, g));
    e.w[8] = static_cast<uint32_t>(MEM_W(0x70, g));
    e.w[9] = static_cast<uint32_t>(MEM_BU(0x72, g));
    e.w[10] = static_cast<uint32_t>(MEM_W(0, g));
    e.vmask |= 4u;
    return e;
}

// Resolve (pointer|owner) to an ordinal; on alias updates the tracked pointer and logs an "alias" row.
static int tooie_d1p_resolve(uint8_t* rdram, uint32_t ptr, int post) {
    const uint32_t owner = tooie_d1p_owner_of(rdram, ptr);
    const auto r = tooie_demo1_projectile::resolve(s_d1p_objs, s_d1p_owner, s_d1p_nobj, ptr, owner);
    if (r.idx >= 0 && r.alias) {
        ++s_d1p_alias;
        tooie_d1p_ev e = tooie_d1p_obj(rdram, ptr);
        e.vmask |= 8u;
        tooie_d1p_row(rdram, "alias", "mark2", post, r.idx, ptr, s_d1p_objs[r.idx], ptr, 0, 0, e);
        s_d1p_objs[r.idx] = ptr;
    }
    return r.idx;
}

static void tooie_d1p_logic_entry(uint8_t* rdram) {
    ++s_d1p_tick;
    int post = 0;
    const bool in = tooie_d1p_gate(rdram, &post);
    tooie_d1p_ev e;
    if (in && !s_d1p_started) {
        s_d1p_started = true;
        tooie_d1p_row(rdram, "START", "mark", post, -1, 0, 0, 0, 0, 0, e);
    } else if (s_d1p_started && !s_d1p_finished && !in) {
        s_d1p_finished = true;
        e.w[0] = s_d1p_rows;
        e.w[1] = 0;  // unmatched pending: wrappers return exactly once, always 0 here
        e.w[2] = s_d1p_alias;
        e.w[3] = s_d1p_untracked;
        e.w[4] = 0;  // retry entries: not observable in the recomp wrapper
        tooie_d1p_row(rdram, "FINISH", "mark", post, -1, 0, 0, 0, 0, 0, e);
    }
    // Per-frame poll of tracked spawned objects (their update callbacks may be overlay-relative).
    if (in) {
        for (int i = 0; i < s_d1p_nobj && s_d1p_rows < tooie_demo1_projectile::kMaxRows; ++i) {
            tooie_d1p_row(rdram, "objpoll", "entry", post, i, s_d1p_objs[i], 0, 0, 0, 0,
                          tooie_d1p_obj(rdram, s_d1p_objs[i]));
        }
    }
}

static void tooie_d1p_wrap_baegg5(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t a0 = uint32_t(ctx->r4), a1 = uint32_t(ctx->r5), a2 = uint32_t(ctx->r6), a3 = uint32_t(ctx->r7);
    int post = 0;
    const bool on = tooie_d1p_gate(rdram, &post) && s_d1p_rows < tooie_demo1_projectile::kMaxRows &&
                    a0 == static_cast<uint32_t>(MEM_W(0, kseg0_gpr(0x80135490u)));
    auto snap = [&]() {
        tooie_d1p_ev e;
        if (tooie_d7_is_guest(a0)) {
            const gpr ps = kseg0_gpr(a0);
            const uint32_t P = static_cast<uint32_t>(MEM_W(0x70, ps));
            tooie_d1p_vec(rdram, e, 0, P, 0);
            tooie_d1p_vec(rdram, e, 3, P + 0xC, 1);
            tooie_d1p_vec(rdram, e, 6, P + 0x18, 2);
            if (tooie_d7_is_guest(P)) e.w[9] = static_cast<uint32_t>(MEM_BU(0x24, kseg0_gpr(P)));
            e.w[10] = static_cast<uint32_t>(MEM_W(0x184, ps));
        }
        return e;
    };
    if (on) tooie_d1p_row(rdram, "baeggsetup5", "enter", post, -1, a0, a1, a2, a3, 0, snap());
    _baeggsetup_entrypoint_5(rdram, ctx);
    if (on) tooie_d1p_row(rdram, "baeggsetup5", "leave", post, -1, a0, a1, a2, a3, uint32_t(ctx->r2), snap());
}

static void tooie_d1p_wrap_chegg2(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t a0 = uint32_t(ctx->r4), a1 = uint32_t(ctx->r5), a2 = uint32_t(ctx->r6), a3 = uint32_t(ctx->r7);
    int post = 0;
    const bool on = tooie_d1p_gate(rdram, &post) && s_d1p_rows < tooie_demo1_projectile::kMaxRows;
    if (on) {
        tooie_d1p_ev e;
        tooie_d1p_vec(rdram, e, 0, a1, 0);
        tooie_d1p_vec(rdram, e, 3, a2, 1);
        tooie_d1p_vec(rdram, e, 6, a3, 2);
        tooie_d1p_row(rdram, "cheggfire2", "enter", post, -1, a0, a1, a2, a3, 0, e);
    }
    _cheggfire_entrypoint_2(rdram, ctx);
    if (on) {
        const uint32_t v0 = uint32_t(ctx->r2);
        int idx = tooie_d1p_resolve(rdram, v0, post);
        if (idx < 0 && tooie_d7_is_guest(v0) && s_d1p_nobj < tooie_demo1_projectile::kMaxObjs) {
            idx = s_d1p_nobj;
            s_d1p_objs[s_d1p_nobj] = v0;
            s_d1p_owner[s_d1p_nobj] = tooie_d1p_owner_of(rdram, v0);
            ++s_d1p_nobj;
        }
        tooie_d1p_row(rdram, "cheggfire2", "leave", post, idx, a0, a1, a2, a3, v0, tooie_d1p_obj(rdram, v0));
    }
}

// sueggmove9/suegg9 (a0 = object). Tracked by pointer or owner (alias); untracked calls are logged with
// obj=-1 up to kMaxUntracked.
static void tooie_d1p_wrap_obj_update(uint8_t* rdram, recomp_context* ctx, const char* ev,
                                      void (*orig)(uint8_t*, recomp_context*)) {
    const uint32_t a0 = uint32_t(ctx->r4), a1 = uint32_t(ctx->r5), a2 = uint32_t(ctx->r6), a3 = uint32_t(ctx->r7);
    int post = 0;
    bool on = tooie_d1p_gate(rdram, &post) && s_d1p_rows < tooie_demo1_projectile::kMaxRows;
    int idx = -1;
    if (on) {
        idx = tooie_d1p_resolve(rdram, a0, post);
        if (idx < 0) {
            on = s_d1p_nobj > 0 && s_d1p_untracked < uint32_t(tooie_demo1_projectile::kMaxUntracked);
            if (on) ++s_d1p_untracked;
        }
    }
    if (on) tooie_d1p_row(rdram, ev, "enter", post, idx, a0, a1, a2, a3, 0, tooie_d1p_obj(rdram, a0));
    orig(rdram, ctx);
    if (on) tooie_d1p_row(rdram, ev, "leave", post, idx, a0, a1, a2, a3, uint32_t(ctx->r2), tooie_d1p_obj(rdram, a0));
}
static void tooie_d1p_wrap_suegg9(uint8_t* rdram, recomp_context* ctx) {
    tooie_d1p_wrap_obj_update(rdram, ctx, "suegg9", _suegg_entrypoint_9);
}
static void tooie_d1p_wrap_sueggmove9(uint8_t* rdram, recomp_context* ctx) {
    tooie_d1p_wrap_obj_update(rdram, ctx, "sueggmove9", _sueggmove_entrypoint_9);
}


static uint32_t tooie_d1p_w(uint8_t* rdram, uint32_t a) {
    return tooie_d7_is_guest(a) ? static_cast<uint32_t>(MEM_W(0, kseg0_gpr(a))) : 0u;
}
static uint32_t tooie_d1p_hu(uint8_t* rdram, uint32_t a) {
    return tooie_d7_is_guest(a & ~3u) ? static_cast<uint32_t>(MEM_HU(0, kseg0_gpr(a))) : 0u;  // halfword may sit at +2 of an aligned word
}
// USER_DEMO1 target producer prefix (TOOIE_DIAG_DEMO1_TARGET=1, default OFF, own flag + header/rows
// "[demo1-tgt-header]"/"[demo1-tgt]" with the same 37 columns). The marker (id halfwords 0x0671/0x0E3F) is
// recognised at func_80106790 calls (callback filter, no actor scan); its actor record (the "component"
// whose +4 vec is the bound center used by _subaddiecustomhits_entrypoint_5) is computed with the
// vector_at formula from memory only (no extra guest calls) and polled once per post at the 800151BC entry.
static bool s_d1t_on = false;
static uint64_t s_d1t_tick = 0, s_d1t_seq = 0;
static uint32_t s_d1t_rows = 0, s_d1t_marker = 0, s_d1t_first_post = 0;
static int s_d1t_last_post = -1;
static bool s_d1t_hdr = false, s_d1t_finished = false;
static tooie_demo1_projectile::TargetSelector s_d1t_sel;

static void tooie_d1t_row(uint8_t* rdram, const char* ev, const char* ph, int post, uint32_t a0, uint32_t a1,
                          uint32_t a2, uint32_t a3, uint32_t v0, tooie_d1p_ev e) {
    if (!s_d1t_hdr) {
        s_d1t_hdr = true;
        std::fprintf(stderr, "[demo1-tgt-header] %s\n", tooie_demo1_projectile::kCsvHeader);
    }
    ++s_d1t_rows;
    if (std::strcmp(ph, "mark") != 0) e.w[11] = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(0x8012C760u)));
    std::fprintf(stderr, "[demo1-tgt] ");
    std::fprintf(stderr, tooie_demo1_projectile::kRowFmt, ev, ph,
                 static_cast<unsigned long long>(s_d1t_tick), static_cast<unsigned long long>(++s_d1t_seq),
                 post, post - 1, -1, a0, a1, a2, a3, v0, e.vmask,
                 e.w[0], e.w[1], e.w[2], e.w[3], e.w[4], e.w[5], e.w[6], e.w[7], e.w[8], e.w[9], e.w[10], e.w[11],
                 tooie_d7_f32(e.w[0]), tooie_d7_f32(e.w[1]), tooie_d7_f32(e.w[2]), tooie_d7_f32(e.w[3]),
                 tooie_d7_f32(e.w[4]), tooie_d7_f32(e.w[5]), tooie_d7_f32(e.w[6]), tooie_d7_f32(e.w[7]),
                 tooie_d7_f32(e.w[8]), tooie_d7_f32(e.w[9]), tooie_d7_f32(e.w[10]), tooie_d7_f32(e.w[11]));
}

static bool tooie_d1t_gate(uint8_t* rdram, int* post_out) {
    const int mode = int(int16_t(MEM_H(0x2, kseg0_gpr(0x80127630u))));
    const int stage = int(MEM_BU(0x0, kseg0_gpr(0x80135470u)));
    const int sub = int(MEM_BU(0x1, kseg0_gpr(0x80135470u)));
    const int post = static_cast<int32_t>(MEM_W(0, kseg0_gpr(0x8012D5B8u)));
    *post_out = post;
    return tooie_demo1_projectile::in_tgt_window(mode, stage, sub, post);
}

// returns false if the vector header is unusable
static bool tooie_d1t_comp(uint8_t* rdram, uint32_t marker, uint32_t* vec_begin, uint32_t* elem, uint32_t* idx,
                           uint32_t* comp) {
    const uint32_t vec = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(0x80136EE0u)));
    if (!tooie_d7_is_guest(vec) || !tooie_d7_is_guest(marker & ~1u)) return false;
    *elem = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(vec)));
    *vec_begin = static_cast<uint32_t>(MEM_W(4, kseg0_gpr(vec)));
    *idx = tooie_d1p_hu(rdram, marker + 0x1A) >> 5;
    *comp = tooie_demo1_projectile::vector_at(*vec_begin, *elem, *idx);
    return *elem != 0 && tooie_d7_is_guest(*comp);
}

// called at the ENTRY of func_80106790 (a0 = actor); memory reads only. Selector: tooie_demo1_projectile::
// TargetSelector (id halfword 0x0671 + P6 bit0); first match binds, other distinct matches = ambiguity row.
static void tooie_d1t_note106790(uint8_t* rdram, uint32_t a0) {
    if (s_d1t_rows >= uint32_t(tooie_demo1_projectile::kTgtMaxRows) || !tooie_d7_is_guest(a0)) return;
    const uint32_t l14 = tooie_d1p_hu(rdram, a0 + 0x14), l18 = tooie_d1p_hu(rdram, a0 + 0x18);
    const int r = s_d1t_sel.feed(a0, l14, l18);
    if (r == 0) return;
    int post = 0;
    tooie_d1t_gate(rdram, &post);
    if (r == 1) {
        s_d1t_marker = a0;
        s_d1t_first_post = uint32_t(post);
    } else if (s_d1t_sel.ambiguous > 8) {
        return;
    }
    uint32_t vb = 0, el = 0, ix = 0, comp = 0;
    const bool ok = tooie_d1t_comp(rdram, a0, &vb, &el, &ix, &comp);
    tooie_d1p_ev e;
    e.w[0] = tooie_d1p_hu(rdram, a0 + 0x1A);
    e.w[1] = tooie_d1p_w(rdram, a0 + 0x28);
    e.w[2] = tooie_d1p_w(rdram, a0 + 0x10);
    e.w[3] = (l14 << 16) | l18;
    const uint32_t d = tooie_d1p_w(rdram, a0);
    e.w[4] = d;
    e.w[5] = tooie_d1p_w(rdram, d + 8);
    e.w[6] = ix;
    e.vmask = ok ? 1u : 0u;
    tooie_d1t_row(rdram, r == 1 ? "tgtfound" : "tgtambig", "mark2", post, a0, ix, comp, vb, 0, e);
}

// First-writer prefix (see tooie_demo1_projectile.h kUpd*): whole-record dump of the tracked actor around its
// per-frame update call. Memory reads only; the update itself runs unmodified between "pre" and "post".
static bool s_d1t_upd_pend = false;
static int s_d1t_upd_post = 0;
static uint32_t s_d1t_upd_rows = 0;
// counters (reported in tgtfinish w4..w8): direct getter calls seen / bound marker without a resolvable record /
// direct calls whose a0 equals the resolved record / "pre" dumps emitted / "postmiss" rows emitted
static uint32_t s_d1t_upd_direct = 0, s_d1t_upd_invalid = 0, s_d1t_upd_match = 0, s_d1t_upd_pre = 0, s_d1t_upd_miss = 0;
static thread_local tooie_demo1_projectile::UpdLoopScope s_d1t_loop_scope;

// ActorRecordAddress of the bound MarkerHandle, resolved from memory only (begin/stride re-read each call); 0 if unusable.
static uint32_t tooie_d1t_expected_record(uint8_t* rdram, uint32_t marker) {
    if (marker == 0 || !tooie_d7_is_guest(marker)) return 0;
    const uint32_t vec = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(0x80136EE0u)));
    if (!tooie_d7_is_guest(vec)) return 0;
    const uint32_t rec = tooie_demo1_projectile::upd_expected_record(
        tooie_d1p_hu(rdram, marker + 0x14), static_cast<uint32_t>(MEM_W(4, kseg0_gpr(vec))),
        static_cast<uint32_t>(MEM_W(0, kseg0_gpr(vec))), tooie_d1p_hu(rdram, marker + 0x1A) >> 5);
    return tooie_d7_is_guest(rec) ? rec : 0u;
}

static void tooie_d1t_upd_dump(uint8_t* rdram, const char* ph, int post, uint32_t record, uint32_t marker, uint32_t sp,
                               uint32_t v0) {
    const uint32_t rng = tooie_d1p_w(rdram, 0x8012C7F0u);
    for (int c = 0; c < tooie_demo1_projectile::kUpdChunks; ++c) {
        tooie_d1p_ev e;
        const int n = tooie_demo1_projectile::upd_chunk_words(c);
        for (int i = 0; i < n; ++i) {
            e.w[i] = tooie_d1p_w(rdram, record + 4u * uint32_t(c * tooie_demo1_projectile::kUpdChunkWords + i));
        }
        e.w[10] = marker;
        e.vmask = 1u;
        tooie_d1t_row(rdram, "updrec", ph, post, record, uint32_t(c), rng, sp, v0, e);
    }
}

// "post" resolves the record again from the stable MarkerHandle (the vector may have relocated since "pre"); if the marker no
// longer resolves, one "postmiss" row is emitted instead of dumping freed/stale memory.
static void tooie_d1t_upd_flush(uint8_t* rdram, uint32_t source) {
    if (!s_d1t_upd_pend) return;
    s_d1t_upd_pend = false;
    const uint32_t rec = tooie_d1t_expected_record(rdram, s_d1t_marker);
    if (rec != 0) {
        tooie_d1t_upd_dump(rdram, "post", s_d1t_upd_post, rec, s_d1t_marker, 0, source);
        return;
    }
    ++s_d1t_upd_miss;
    tooie_d1p_ev e;
    e.w[10] = s_d1t_marker;
    tooie_d1t_row(rdram, "updrec", "postmiss", s_d1t_upd_post, 0, 0, tooie_d1p_w(rdram, 0x8012C7F0u), 0, source, e);
}

// Call-local scope around the original func_800FF62C (its only generated caller is one LOOKUP_FUNC site). The generated
// body never assigns r31 before its func_80100368 calls, so the direct loop calls are identified by SP: entry SP - 0x58.
// Saved/restored (nesting-safe); ctx is not modified.
static void tooie_d1t_w_ff62c(uint8_t* rdram, recomp_context* ctx) {
    const tooie_demo1_projectile::UpdLoopScope saved = s_d1t_loop_scope;
    s_d1t_loop_scope = tooie_demo1_projectile::UpdLoopScope::enter(uint32_t(ctx->r29));
    func_800FF62C(rdram, ctx);
    s_d1t_loop_scope = saved;
}

// Wrapper of func_80100368 (a0 = ActorRecordAddress, NOT the MarkerHandle; all generated callers use LOOKUP_FUNC). Only DIRECT
// loop calls (scope + SP) flush the pending "post" dump or start a "pre" dump; getter calls nested inside the actor callback
// are ignored.
static void tooie_d1t_w_100368(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t record_arg = uint32_t(ctx->r4), sp = uint32_t(ctx->r29);
    if (s_d1t_loop_scope.direct(sp) && s_d1t_marker != 0) {
        ++s_d1t_upd_direct;
        tooie_d1t_upd_flush(rdram, 1u);
        const uint32_t expected = tooie_d1t_expected_record(rdram, s_d1t_marker);
        if (expected == 0) ++s_d1t_upd_invalid;
        else if (record_arg == expected) ++s_d1t_upd_match;
        int post = 0;
        if (tooie_d1t_gate(rdram, &post) &&
            tooie_demo1_projectile::upd_accept(s_d1t_loop_scope, sp, record_arg, expected, post, s_d1t_upd_rows)) {
            s_d1t_upd_rows += tooie_demo1_projectile::kUpdRowsPerCall;
            ++s_d1t_upd_pre;
            tooie_d1t_upd_dump(rdram, "pre", post, record_arg, s_d1t_marker, sp, tooie_d1p_w(rdram, record_arg + 0x10));
            s_d1t_upd_pend = true;
            s_d1t_upd_post = post;
        }
    }
    func_80100368(rdram, ctx);
}

// ---- RNG-consumer / CACEC extra-accept trace (kRng*; rows do not count towards the tgtrec row cap) ----
static uint32_t s_d1x_seq = 0, s_d1x_rng_rows = 0, s_d1x_cx_rows = 0, s_d1x_extra = 0;

static uint32_t tooie_d1x_extra_count() { return s_d1x_extra; }

static bool tooie_d1x_gate(uint8_t* rdram, int* post) {
    return s_d1t_on && tooie_d1t_gate(rdram, post) && tooie_demo1_projectile::in_rng_window(*post);
}

static void tooie_d1x_row(uint8_t* rdram, const char* ev, const char* ph, int post, uint32_t a0, uint32_t a1, uint32_t a2,
                          uint32_t a3, uint32_t v0, const tooie_d1p_ev& e) {
    tooie_d1t_row(rdram, ev, ph, post, a0, a1, a2, a3, v0, e);
    --s_d1t_rows;  // diagnostic rows must not starve tgtrec/tgtfinish
}

static void tooie_d1x_cacec_extra(uint8_t* rdram, uint32_t vp, int32_t origin, uint32_t radius_bits, float px, float py,
                                  float pz) {
    if (!s_d1t_on) return;
    ++s_d1x_extra;
    int post = 0;
    if (!tooie_d1x_gate(rdram, &post) || s_d1x_cx_rows >= uint32_t(tooie_demo1_projectile::kCacecMaxRows)) return;
    ++s_d1x_cx_rows;
    tooie_d1p_ev e;
    std::memcpy(&e.w[0], &px, 4);
    std::memcpy(&e.w[1], &py, 4);
    std::memcpy(&e.w[2], &pz, 4);
    e.w[3] = uint32_t(s_cacec_scope_origin);
    e.w[4] = ++s_d1x_seq;
    e.w[5] = tooie_d1p_w(rdram, 0x8012C7F0u);
    e.vmask = 1u;
    tooie_d1x_row(rdram, "cacecx", "extra", post, vp, uint32_t(origin), radius_bits, uint32_t(s_cacec_scope_kind), 1u, e);
}

static void tooie_d1x_e3e8c(uint8_t* rdram, uint32_t pos_ptr, uint32_t radius_bits, int32_t origin, uint32_t result,
                            uint32_t extra_before) {
    int post = 0;
    if (!tooie_d1x_gate(rdram, &post) || s_d1x_cx_rows >= uint32_t(tooie_demo1_projectile::kCacecMaxRows)) return;
    ++s_d1x_cx_rows;
    tooie_d1p_ev e;
    e.w[0] = ++s_d1x_seq;
    e.w[1] = s_d1x_extra - extra_before;
    e.vmask = 1u;
    tooie_d1x_row(rdram, "e3e8c", "enter", post, pos_ptr, radius_bits, uint32_t(origin), 0, result, e);
}

// func_800DC0C0 (float result, ctx->f0) and func_800DC090 (raw new state, v0): same state word D_8012C7F0.
__attribute__((noinline)) static void tooie_d1x_rng_common(uint8_t* rdram, recomp_context* ctx, bool is_float, const void* ret) {
    int post = 0;
    const bool on = tooie_d1x_gate(rdram, &post) && s_d1x_rng_rows < uint32_t(tooie_demo1_projectile::kRngMaxRows);
    const uint32_t before = on ? tooie_d1p_w(rdram, 0x8012C7F0u) : 0u;
    if (is_float) func_800DC0C0(rdram, ctx);
    else func_800DC090(rdram, ctx);
    if (!on) return;
    ++s_d1x_rng_rows;
    const uint32_t after = tooie_d1p_w(rdram, 0x8012C7F0u);
    tooie_d1p_ev e;
    e.w[0] = ++s_d1x_seq;
    e.w[1] = s_d1x_extra;
    e.w[2] = uint32_t(s_cacec_scope_origin);
    e.w[3] = (tooie_demo1_projectile::lcg_next(before) == after) ? 1u : 0u;
    e.vmask = 1u;
    tooie_d1x_row(rdram, "rngcall", is_float ? "dc0c0" : "dc090", post, before, after, uint32_t(tooie_d1x_origin(ret)),
                  uint32_t(s_cacec_scope_kind), is_float ? ctx->f0.u32l : uint32_t(ctx->r2), e);
}
__attribute__((noinline)) static void tooie_d1x_w_dc0c0(uint8_t* rdram, recomp_context* ctx) {
    tooie_d1x_rng_common(rdram, ctx, true, __builtin_return_address(0));
}
__attribute__((noinline)) static void tooie_d1x_w_dc090(uint8_t* rdram, recomp_context* ctx) {
    tooie_d1x_rng_common(rdram, ctx, false, __builtin_return_address(0));
}

static void tooie_d1t_logic_entry(uint8_t* rdram) {
    ++s_d1t_tick;
    tooie_d1t_upd_flush(rdram, 2u);
    int post = 0;
    const bool in = tooie_d1t_gate(rdram, &post);
    if (s_d1t_marker == 0) return;
    if (in && post != s_d1t_last_post && s_d1t_rows < uint32_t(tooie_demo1_projectile::kTgtMaxRows)) {
        s_d1t_last_post = post;
        uint32_t vb = 0, el = 0, ix = 0, comp = 0;
        const bool ok = tooie_d1t_comp(rdram, s_d1t_marker, &vb, &el, &ix, &comp);
        tooie_d1p_ev e;
        if (ok) {
            const gpr c = kseg0_gpr(comp);
            const int offs[11] = {0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C, 0x20, 0x28, 0x6C, 0x70};
            for (int i = 0; i < 11; ++i) e.w[i] = static_cast<uint32_t>(MEM_W(offs[i], c));
            e.vmask = 1u;
        }
        tooie_d1t_row(rdram, "tgtrec", "entry", post, comp, ix, vb, el, s_d1t_marker, e);
    } else if (!in && !s_d1t_finished && post > tooie_demo1_projectile::kTgtPostHi) {
        s_d1t_finished = true;
        tooie_d1p_ev e;
        e.w[0] = s_d1t_rows;
        e.w[1] = s_d1t_first_post;
        e.w[2] = uint32_t(s_d1t_sel.ambiguous);
        e.w[3] = uint32_t(s_d1t_sel.nhandles);
        e.w[4] = s_d1t_upd_direct;
        e.w[5] = s_d1t_upd_invalid;
        e.w[6] = s_d1t_upd_match;
        e.w[7] = s_d1t_upd_pre;
        e.w[8] = s_d1t_upd_miss;
        tooie_d1t_row(rdram, "tgtfinish", "mark", post, 0, 0, 0, 0, 0, e);
    }
}

// ---- first-branch trace of func_800CD820 (tracked object, posts kSweepTraceLo..Hi) ----
static int s_d1p_trace = 0;            // >0 while inside a traced sweep
static uint32_t s_d1p_trace_rows = 0;
static uint32_t s_d1p_ignore = 0;      // sweep a0 of the traced call
static uint32_t s_d1p_list_a0 = 0, s_d1p_list_start = 0;
enum { D1P_BE444 = 6, D1P_E9E88, D1P_E9EB4, D1P_106790, D1P_8A018, D1P_EEB40, D1P_EC3C4, D1P_CD6B4, D1P_CB854 };
static const char* const kD1pHelperName[] = {"", "", "", "", "", "", "s_BE444", "s_E9E88", "s_E9EB4", "s_106790",
                                             "s_8A018", "s_EEB40", "s_EC3C4", "s_CD6B4", "s_CB854"};


// Payload per helper kind (layout shared with the oracle hook p08_d1p_hp).
static tooie_d1p_ev tooie_d1p_hp(uint8_t* rdram, int kind, bool leave, uint32_t a0, uint32_t a1, uint32_t a2,
                                 uint32_t a3, uint32_t sp, uint32_t v0, uint32_t f0bits) {
    tooie_d1p_ev e;
    switch (kind) {
    case D1P_BE444:
        if (!leave) tooie_d1p_vec(rdram, e, 0, a0, 0);
        else {
            e.w[0] = v0;
            if (tooie_d7_is_guest(v0)) { for (int i = 0; i < 6; ++i) e.w[1 + i] = tooie_d1p_w(rdram, v0 + 4u * i); e.vmask |= 1; }
        }
        break;
    case D1P_E9E88:
    case D1P_E9EB4:
        if (!leave) tooie_d1p_vec(rdram, e, 0, a0, 0);
        else {
            e.w[0] = v0;
            if (kind == D1P_E9EB4) e.w[1] = (s_d1p_list_a0 == a0 && v0 >= s_d1p_list_start) ? (v0 - s_d1p_list_start) / 12u : 0xFFFFFFFFu;
        }
        break;
    case D1P_106790:
        if (!leave) {
            e.w[0] = tooie_d1p_w(rdram, a0 + 0x28);
            e.w[1] = tooie_d1p_hu(rdram, a0 + 0x14);
            e.w[2] = tooie_d1p_hu(rdram, a0 + 0x18);
            e.w[3] = (a0 == s_d1p_ignore) ? 1u : 0u;
        } else {
            e.w[0] = tooie_d1p_w(rdram, v0 + 0x64);
            e.w[1] = tooie_d1p_hu(rdram, v0 + 0x66);
        }
        break;
    case D1P_8A018:
        if (leave) { tooie_d1p_vec(rdram, e, 0, a1, 0); e.w[3] = f0bits; }
        break;
    case D1P_EEB40:
        if (!leave) { tooie_d1p_vec(rdram, e, 0, a0, 0); tooie_d1p_vec(rdram, e, 3, a1, 1); }
        else e.w[0] = f0bits;
        break;
    case D1P_EC3C4:
        if (leave) e.w[0] = tooie_d1p_w(rdram, v0 + 0x10);
        break;
    case D1P_CD6B4:
        if (!leave) { tooie_d1p_vec(rdram, e, 0, a1, 0); e.w[3] = tooie_d1p_w(rdram, sp + 0x10); }
        else { tooie_d1p_vec(rdram, e, 0, a3, 0); e.w[3] = f0bits; }
        break;
    default: break;
    }
    return e;
}

static void tooie_d1p_helper(uint8_t* rdram, recomp_context* ctx, int kind, void (*orig)(uint8_t*, recomp_context*)) {
    if (s_d1p_trace <= 0 || s_d1p_trace_rows >= uint32_t(tooie_demo1_projectile::kMaxTraceRows) ||
        s_d1p_rows >= uint32_t(tooie_demo1_projectile::kMaxRows)) {
        orig(rdram, ctx);
        return;
    }
    const uint32_t a0 = uint32_t(ctx->r4), a1 = uint32_t(ctx->r5), a2 = uint32_t(ctx->r6), a3 = uint32_t(ctx->r7);
    const uint32_t sp = uint32_t(ctx->r29);
    int post = 0;
    tooie_d1p_gate(rdram, &post);
    ++s_d1p_trace_rows;
    tooie_d1p_row(rdram, kD1pHelperName[kind], "enter", post, 0, a0, a1, a2, a3, 0,
                  tooie_d1p_hp(rdram, kind, false, a0, a1, a2, a3, sp, 0, 0));
    orig(rdram, ctx);
    const uint32_t v0 = uint32_t(ctx->r2), f0 = ctx->f0.u32l;
    ++s_d1p_trace_rows;
    tooie_d1p_row(rdram, kD1pHelperName[kind], "leave", post, 0, a0, a1, a2, a3, v0,
                  tooie_d1p_hp(rdram, kind, true, a0, a1, a2, a3, sp, v0, f0));
    if (kind == D1P_E9E88) {
        s_d1p_list_a0 = a0;
        s_d1p_list_start = v0;
    } else if (kind == D1P_E9EB4 && s_d1p_list_a0 == a0 && tooie_d7_is_guest(s_d1p_list_start) && v0 >= s_d1p_list_start) {
        uint32_t n = (v0 - s_d1p_list_start) / 12u;
        if (n > uint32_t(tooie_demo1_projectile::kMaxCandEntries)) n = tooie_demo1_projectile::kMaxCandEntries;
        for (uint32_t i = 0; i < n && s_d1p_trace_rows < uint32_t(tooie_demo1_projectile::kMaxTraceRows); ++i) {
            const uint32_t ent = s_d1p_list_start + 12u * i;
            const uint32_t actor = tooie_d1p_w(rdram, ent);
            tooie_d1p_ev e;
            e.w[0] = tooie_d1p_w(rdram, ent + 8);
            if (tooie_d7_is_guest(actor)) {
                e.w[1] = tooie_d1p_w(rdram, actor + 0x28);
                e.w[2] = tooie_d1p_hu(rdram, actor + 0x14);
                e.w[3] = tooie_d1p_hu(rdram, actor + 0x18);
                e.w[5] = tooie_d1p_w(rdram, actor + 4);
                e.w[6] = tooie_d1p_w(rdram, actor + 8);
                e.w[7] = tooie_d1p_w(rdram, actor + 12);
                e.vmask |= 1;
            }
            e.w[4] = (actor == s_d1p_ignore) ? 1u : 0u;
            ++s_d1p_trace_rows;
            tooie_d1p_row(rdram, "s_cand", "entry", post, 0, actor, ent, i, 0, 0, e);
        }
    }
}
#define D1P_HELPER_WRAP(NAME, KIND, FN) \
    static void tooie_d1p_w_##NAME(uint8_t* rdram, recomp_context* ctx) { tooie_d1p_helper(rdram, ctx, KIND, FN); }
D1P_HELPER_WRAP(be444, D1P_BE444, func_800BE444)
D1P_HELPER_WRAP(e9e88, D1P_E9E88, func_800E9E88)
D1P_HELPER_WRAP(e9eb4, D1P_E9EB4, func_800E9EB4)
static void tooie_d1p_w_c106790(uint8_t* rdram, recomp_context* ctx) {
    if (s_d1t_on) tooie_d1t_note106790(rdram, uint32_t(ctx->r4));
    tooie_d1p_helper(rdram, ctx, D1P_106790, func_80106790);
}
D1P_HELPER_WRAP(c8a018, D1P_8A018, _subaddiecustomhits_entrypoint_5)
D1P_HELPER_WRAP(eeb40, D1P_EEB40, func_800EEB40)
D1P_HELPER_WRAP(ec3c4, D1P_EC3C4, func_800EC3C4)
D1P_HELPER_WRAP(cd6b4, D1P_CD6B4, func_800CD6B4)
D1P_HELPER_WRAP(cb854, D1P_CB854, func_800CB854)

// func_800CD820: a0 = owner word (*(obj)), a1/a2 raw vec3 pointers, a3 = radius bits (suegg callers
// pass 0x42180000 and a2 = object+4). Logged when a3 == 38.0f bits AND (a0 is a tracked owner OR a2-4 is a
// tracked pointer); otherwise radius-38 calls are logged as untracked (capped). Hit normal/result beyond v0
// is NOT logged (out-parameters not ABI-proven). w6 = (v0 == player slot0 ps), w7 = a0 owner raw.
static void tooie_d1p_wrap_sweep(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t a0 = uint32_t(ctx->r4), a1 = uint32_t(ctx->r5), a2 = uint32_t(ctx->r6), a3 = uint32_t(ctx->r7);
    int post = 0;
    bool on = false;
    int idx = -1;
    if (a3 == tooie_demo1_projectile::kSweepRadiusBits && tooie_d1p_gate(rdram, &post) &&
        s_d1p_rows < tooie_demo1_projectile::kMaxRows) {
        for (int i = 0; i < s_d1p_nobj && idx < 0; ++i) {
            if (s_d1p_owner[i] == a0 || s_d1p_objs[i] == a2 - 4) idx = i;
        }
        on = idx >= 0;
        if (!on && s_d1p_nobj > 0 && s_d1p_untracked < uint32_t(tooie_demo1_projectile::kMaxUntracked)) {
            on = true;
            ++s_d1p_untracked;
        }
    }
    auto snap = [&](uint32_t v0) {
        tooie_d1p_ev e;
        tooie_d1p_vec(rdram, e, 0, a1, 0);
        tooie_d1p_vec(rdram, e, 3, a2, 1);
        e.w[6] = (v0 != 0 && v0 == static_cast<uint32_t>(MEM_W(0, kseg0_gpr(0x80135490u)))) ? 1u : 0u;
        e.w[7] = a0;
        if (v0 != 0 && tooie_d7_is_guest(v0)) {  // returned hit: raw identity words (w8=+0x10 sys word, w9=+0x28, w10=lhu14<<16|lhu18)
            e.w[8] = tooie_d1p_w(rdram, v0 + 0x10);
            e.w[9] = tooie_d1p_w(rdram, v0 + 0x28);
            e.w[10] = (tooie_d1p_hu(rdram, v0 + 0x14) << 16) | tooie_d1p_hu(rdram, v0 + 0x18);
        }
        return e;
    };
    if (on) tooie_d1p_row(rdram, "sweep800CD820", "enter", post, idx, a0, a1, a2, a3, 0, snap(0));
    const bool traced = on && idx >= 0 && tooie_demo1_projectile::in_sweep_trace(post);
    if (traced) {
        s_d1p_ignore = a0;
        ++s_d1p_trace;
    }
    func_800CD820(rdram, ctx);
    if (traced) --s_d1p_trace;
    if (on) tooie_d1p_row(rdram, "sweep800CD820", "leave", post, idx, a0, a1, a2, a3, uint32_t(ctx->r2), snap(uint32_t(ctx->r2)));
}

void register_boot_lookup_stubs() {
    // Prefer real ultramodern/libultra recomp hooks at Tooie VRAMs.
    for (const UltraLookupHook& hook : kUltraLookupHooks) {
        recomp::overlays::add_loaded_function(hook.vram, hook.func);
    }
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800136E4),
                                          tooie_wrap_degrees);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800138D0),
                                          tooie_cos_degrees);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80013970),
                                          tooie_sin_degrees);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800BCCC0),
                                          tooie_helper_800BCCC0);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800BCCD8),
                                          tooie_helper_800BCCD8);
    s_d1_on = tooie_diag::demo1_parity_enabled();
    s_d1p_on = tooie_diag::demo1_projectile_enabled();
    s_d1t_on = tooie_diag::demo1_target_enabled();
    if (s_d1t_on && !s_d1p_on) {
        recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80106790), tooie_d1p_w_c106790);
    }
    if (s_d1t_on) {
        recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800FF62C), tooie_d1t_w_ff62c);
        recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80100368), tooie_d1t_w_100368);
        recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800DC0C0), tooie_d1x_w_dc0c0);
        recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800DC090), tooie_d1x_w_dc090);
    }
    if (s_d1p_on) {
        recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80084A60), tooie_d1p_wrap_baegg5);
        recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800871F0), tooie_d1p_wrap_chegg2);
        recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8008A3C0), tooie_d1p_wrap_suegg9);
        recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8008A480), tooie_d1p_wrap_sueggmove9);
        recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800CD820), tooie_d1p_wrap_sweep);
        recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800BE444), tooie_d1p_w_be444);
        recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800E9E88), tooie_d1p_w_e9e88);
        recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800E9EB4), tooie_d1p_w_e9eb4);
        recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80106790), tooie_d1p_w_c106790);
        recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8008A018), tooie_d1p_w_c8a018);
        recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800EEB40), tooie_d1p_w_eeb40);
        recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800EC3C4), tooie_d1p_w_ec3c4);
        recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800CD6B4), tooie_d1p_w_cd6b4);
        recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800CB854), tooie_d1p_w_cb854);
    }
    // Boundary wrappers for live timing baseline measurement (TOOIE_DIAG_TIMING=1).
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8001443C),
        [](uint8_t* rdram, recomp_context* ctx) {
            if (s_d1_on) {
                ++s_d1_vi;
            }
            tooie_diag::note_guest_vi();
            func_8001443C(rdram, ctx);
        });
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800151BC),
        [](uint8_t* rdram, recomp_context* ctx) {
            if (s_d1_on) {
                tooie_demo1_parity_entry(rdram);
            }
            if (s_d1p_on) {
                tooie_d1p_logic_entry(rdram);
            }
            if (s_d1t_on) {
                tooie_d1t_logic_entry(rdram);
            }
            int32_t divisor = MEM_W(0, kseg0_gpr(0x80078488));
            tooie_diag::note_game_logic(divisor);
            tooie_diag::note_guest_logic_tick();
            tooie_rt_on_logic_tick(rdram);
            func_800151BC(rdram, ctx);
        });
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800A8E9C), tooie_wrap_800a8e9c);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80014148),
        [](uint8_t* rdram, recomp_context* ctx) {
            tooie_diag::note_display_list();
            func_80014148(rdram, ctx);
        });
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8008C894),
        [](uint8_t* rdram, recomp_context* ctx) {
            const gpr player = ctx != nullptr ? ctx->r4 : 0;
            tooie_diag::note_animation_update();
            baanim_update(rdram, ctx);
            if (tooie_diag::feather_anim_sample_time_enabled()) {
                tooie_diag::feather_anim_note_baanim_update(
                    rdram, static_cast<uint32_t>(player));
            }
        });
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80097C3C),
        [](uint8_t* rdram, recomp_context* ctx) {
            bainput_should_shoot_egg(rdram, ctx);
            if (tooie_diag::shot_enabled() && ctx->r2 != 0) {
                std::fprintf(stderr, "[shot-input] bainput_should_shoot_egg=1\n");
                std::fflush(stderr);
            }
        });
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800CA628),
                                          tooie_wrap_ca628);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8010FC38),
                                          tooie_wrap_8010fc38);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8010FE14),
                                          tooie_wrap_8010fe14);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800CA8B4),
                                          tooie_wrap_ca8b4);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800E1A58),
                                          tooie_wrap_e1a58);
    // Address-taken LOOKUP_FUNC calls bypass linker --wrap for these core2
    // helpers; bind the diagnostic wrappers into the runtime function map.
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800E0B8C),
                                          __wrap_func_800E0B8C);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800E0E94),
                                          __wrap_func_800E0E94);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800AE8A8),
                                          __wrap_func_800AE8A8);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800AE820),
                                          __wrap_func_800AE820);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800E2AA4),
                                          __wrap_func_800E2AA4);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800DE2A4),
                                          __wrap_func_800DE2A4);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800DE498),
                                          __wrap_func_800DE498);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800DCF48),
                                          __wrap_func_800DCF48);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800ADE2C),
                                          __wrap_func_800ADE2C);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800E2180),
                                          __wrap_func_800E2180);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8009C914),
                                          tooie_wrap_yaw_set_ideal);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8011607C), tooie_wrap_func_8011607C);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80113978), tooie_wrap_func_80113978);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80112524), tooie_wrap_func_80112524);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x801107F0), tooie_wrap_func_801107F0);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80110770), tooie_wrap_func_80110770);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x801108A0), tooie_wrap_func_801108A0);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80112A84), tooie_wrap_func_80112A84);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80112D08), tooie_wrap_func_80112D08);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8010FFB0), tooie_wrap_func_8010FFB0);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800EE7F8), tooie_wrap_func_800EE7F8);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800EFB24), tooie_wrap_func_800EFB24);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800EEF94), tooie_wrap_func_800EEF94);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800F1A88), tooie_wrap_func_800F1A88);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80115358), tooie_wrap_func_80115358);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80115750), tooie_wrap_func_80115750);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80112C3C), tooie_wrap_func_80112C3C);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800F1738), tooie_wrap_func_800F1738);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800F1DCC), tooie_wrap_func_800F1DCC);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x801125E8), tooie_wrap_func_801125E8);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800EEBF0), tooie_wrap_func_800EEBF0);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800EE780), tooie_wrap_func_800EE780);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800EFD24), tooie_wrap_func_800EFD24);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800EF214), tooie_wrap_func_800EF214);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80013B70), tooie_wrap_func_80013B70);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80013B7C), tooie_wrap_func_80013B7C);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800137AC), tooie_wrap_func_800137AC);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800137C4), tooie_wrap_func_800137C4);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800137F4), tooie_wrap_func_800137F4);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80013788), tooie_wrap_func_80013788);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80013818), tooie_wrap_func_80013818);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80111FD4), tooie_wrap_func_80111FD4);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x801138A0), tooie_wrap_func_801138A0);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x801138F4), tooie_wrap_func_801138F4);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80110F44), tooie_wrap_func_80110F44);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80115580), tooie_wrap_func_80115580);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80110A68), tooie_wrap_func_80110A68);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80110790), tooie_wrap_func_80110790);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80111310), tooie_wrap_func_80111310);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800A516C), tooie_wrap_func_800A516C);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8002DCA0), tooie_wrap_sqrtf);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800979D8), tooie_bainput_func_800979D8);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80097990), tooie_wrap_func_80097990);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80097978), tooie_wrap_func_80097978);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800BCE84), tooie_wrap_func_800BCE84);
    if (tooie_diag::demo7_drift_enabled()) {
        recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8009B9B0), tooie_d7_wrap_tspd_set);
        recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8009E5C8), tooie_d7_wrap_bs_setstate);
    }
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80098140), tooie_wrap_bainput_enable);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80098024), tooie_wrap_bainput_isEnabled);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8009811C), tooie_wrap_bainput_init);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80097B70), tooie_wrap_bainput_should_rotate_camera_left);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80097B94), tooie_wrap_bainput_should_rotate_camera_right);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800A4338), tooie_wrap_func_800A4338);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80110EFC), tooie_wrap_func_80110EFC);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80110E00), tooie_wrap_func_80110E00);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x801111D0), tooie_wrap_func_801111D0);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80112D90), tooie_wrap_func_80112D90);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80110588), tooie_wrap_func_80110588);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80116218), tooie_wrap_func_80116218);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80112E84), tooie_wrap_func_80112E84);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8011349C), tooie_wrap_func_8011349C);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800FAB50), tooie_wrap_func_800FAB50);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800C6A7C), tooie_wrap_func_800C6A7C);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8010FDEC), tooie_wrap_8010fdec);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80087920), tooie_wrap_plcamera13);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800EFA4C), tooie_wrap_800efa4c);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80110970), tooie_wrap_80110970);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8010FE34), tooie_wrap_8010fe34);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800A9318), tooie_wrap_800a9318);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800CAA24), tooie_wrap_func_800CAA24);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800CACEC), tooie_wrap_func_800CACEC);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800E3E8C), tooie_wrap_func_800E3E8C);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8011447C), tooie_wrap_func_8011447C);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(tooie_demo1_projectile::kCacecPureDrawB97A4), tooie_wrap_func_800B97A4);
    for (size_t i = 0; i < sizeof(kLookupGapAddrs) / sizeof(kLookupGapAddrs[0]); ++i) {
        recomp::overlays::add_loaded_function(kLookupGapAddrs[i], kLookupGapFuncs[i]);
    }
    std::fprintf(stderr,
                 "[tooie] registered %zu ultra LOOKUP hooks + 5 core helper hooks + %zu gap stubs\n",
                 sizeof(kUltraLookupHooks) / sizeof(kUltraLookupHooks[0]),
                 sizeof(kLookupGapAddrs) / sizeof(kLookupGapAddrs[0]));
}
} // namespace banjo
