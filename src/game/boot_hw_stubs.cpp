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
#include "tooie_diag.h"
#include "generated/overlay_dispatch.inc.h"

namespace banjo {
void register_boot_lookup_stubs();
}

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
extern "C" void bainput_should_rotate_camera_right(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_800A4338(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_80110EFC(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_80110E00(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_801111D0(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_80112D90(uint8_t* rdram, recomp_context* ctx);
extern "C" void bs_setState(uint8_t* rdram, recomp_context* ctx);
extern "C" void anctrl_isStopped(uint8_t* rdram, recomp_context* ctx);
extern "C" void anctrl_getAnimTimer(uint8_t* rdram, recomp_context* ctx);
extern "C" void bastick_getZone(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_8008ADE4(uint8_t* rdram, recomp_context* ctx);
extern "C" void baanim_update(uint8_t* rdram, recomp_context* ctx);

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

// 0x800979D8 is a real static bainput helper. It has no public decomp symbol,
// but generated callers must observe its stateful contract.
static void tooie_bainput_func_800979D8(uint8_t* rdram, recomp_context* ctx) {
    const auto sign_extend_guest = [](int32_t value) -> gpr {
        return static_cast<gpr>(static_cast<int64_t>(value));
    };
    const gpr self = sign_extend_guest(static_cast<int32_t>(ctx->r4));
    const uint32_t button_id = static_cast<uint32_t>(ctx->r5);
    const uint32_t input_id = static_cast<uint32_t>(ctx->r6);
    

    // The original helper delegates the input-state query to bakey_held
    // (0x800919F4).  In particular, prev_state[id] == 0 is not the
    // not-held result: it is the condition that reaches the live
    // ButtonState.pressed query in func_800BCE84.
    ctx->r4 = self;
    ctx->r5 = static_cast<gpr>(button_id);
    bakey_held(rdram, ctx);
    const bool held = ctx->r2 != 0;

    // bainput_isEnabled returns the enabled mask, not a boolean.  The
    // original uses that exact value as its return (1 << input_id).
    ctx->r4 = self;
    ctx->r5 = static_cast<gpr>(input_id);
    bainput_isEnabled(rdram, ctx);
    const gpr enabled = ctx->r2;

    if (held) {
        if (enabled == 0) {
            ctx->r4 = self;
            ctx->r5 = static_cast<gpr>(input_id);
            ctx->r6 = 0;
            bainput_enable(rdram, ctx);
        }

        ctx->r4 = self;
        ctx->r5 = static_cast<gpr>(input_id);
        bainput_isEnabled(rdram, ctx);
        return;
    }

    ctx->r4 = self;
    ctx->r5 = static_cast<gpr>(input_id);
    ctx->r6 = 1;
    bainput_enable(rdram, ctx);
    ctx->r2 = 0;
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

static unsigned s_ud5_abc_trot[8];
static unsigned s_ud5_abc_pending[8];
static thread_local uint32_t s_ud5_eye_s0;
static thread_local unsigned s_ud5_fd4_ord;

static void tooie_ud5_read3(uint8_t* rdram, uint32_t addr, uint32_t* x, uint32_t* y, uint32_t* z) {
    *x = *y = *z = 0;
    if (tooie_is_guest(addr)) {
        *x = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(addr)));
        *y = static_cast<uint32_t>(MEM_W(4, kseg0_gpr(addr)));
        *z = static_cast<uint32_t>(MEM_W(8, kseg0_gpr(addr)));
    }
}

static int tooie_ud5_cam1c_window(uint8_t* rdram, int* glidx_out) {
    if (!tooie_diag::ud5_loco_enabled()) {
        return 0;
    }
    const int mode_v = int(MEM_H(0x2, kseg0_gpr(0x80127630u)));
    const int sub_v = int(MEM_BU(0x1, kseg0_gpr(0x80135470u)));
    const int32_t post_glidx = static_cast<int32_t>(MEM_W(0x8, kseg0_gpr(0x8012D5B0u)));
    const int glidx = (post_glidx > 0) ? int(post_glidx - 1) : 0;
    if (glidx_out) {
        *glidx_out = glidx;
    }
    return mode_v == 5 && sub_v == 5 && glidx >= 20 && glidx <= 28;
}

static uint32_t tooie_ud5_selected_cam(uint8_t* rdram, int32_t* sel_out) {
    const uint32_t ps = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(0x80135490u)));
    if (!tooie_is_guest(ps)) {
        if (sel_out) {
            *sel_out = -1;
        }
        return 0;
    }
    const int32_t sel = static_cast<int32_t>(MEM_W(0x184, kseg0_gpr(ps)));
    if (sel_out) {
        *sel_out = sel;
    }
    if (sel < 0 || sel >= 8) {
        return 0;
    }
    const uint32_t rec = static_cast<uint32_t>(MEM_W(sel * 4, kseg0_gpr(0x80127708u)));
    return tooie_is_guest(rec) ? rec : 0;
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
extern "C" void yaw_setIdeal(uint8_t* rdram, recomp_context* ctx);
extern "C" void bastick_getAngleRelativeToBanjo(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_800CAF00(uint8_t* rdram, recomp_context* ctx);
extern "C" void baphysics_set_target_yaw(uint8_t* rdram, recomp_context* ctx);
extern "C" void baphysics_set_horizontal_velocity(uint8_t* rdram, recomp_context* ctx);
extern "C" void baphysics_set_target_horizontal_velocity(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_8009D088(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_800EEB9C(uint8_t* rdram, recomp_context* ctx);

static void tooie_wrap_ca628(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win = tooie_ud4_cam_window(rdram, &glidx);
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t a1 = static_cast<uint32_t>(ctx->r5);
    const uint32_t a2 = static_cast<uint32_t>(ctx->r6);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    float src0 = 0, src1 = 0, src2 = 0, s20 = 0, s21 = 0, s22 = 0;
    if (win && tooie_is_guest(a1) && tooie_is_guest(a2)) {
        src0 = tooie_bits_f32(MEM_W(0, kseg0_gpr(a1)));
        src1 = tooie_bits_f32(MEM_W(4, kseg0_gpr(a1)));
        src2 = tooie_bits_f32(MEM_W(8, kseg0_gpr(a1)));
        s20 = tooie_bits_f32(MEM_W(0, kseg0_gpr(a2)));
        s21 = tooie_bits_f32(MEM_W(4, kseg0_gpr(a2)));
        s22 = tooie_bits_f32(MEM_W(8, kseg0_gpr(a2)));
        fprintf(stderr,
                "[ud4-ca628] enter seq=%u glidx=%d ra=%08x dst=%08x src=%08x src2=%08x "
                "eye=(%.6f,%.6f,%.6f) src2=(%.6f,%.6f,%.6f) src2_1_bits=%08x src2_2_bits=%08x",
                tooie_ud4_seq(), glidx, ra, a0, a1, a2, src0, src1, src2, s20, s21, s22,
                static_cast<uint32_t>(MEM_W(4, kseg0_gpr(a2))),
                static_cast<uint32_t>(MEM_W(8, kseg0_gpr(a2))));
        tooie_ud4_dump_player(rdram, stderr);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
    int ud5_glidx = -1;
    const int ud5 = tooie_ud5_cam1c_window(rdram, &ud5_glidx);
    int32_t sel = -1;
    uint32_t rec = 0;
    uint32_t old_1c = 0;
    uint32_t src2_0 = 0, src2_1 = 0, src2_2 = 0;
    static thread_local uint32_t s_ud5_ca628_ord = 0;
    uint32_t ca_ord = 0;
    if (ud5) {
        rec = tooie_ud5_selected_cam(rdram, &sel);
        if (tooie_is_guest(a0)) {
            old_1c = static_cast<uint32_t>(MEM_W(0x1C, kseg0_gpr(a0)));
        }
        if (tooie_is_guest(a2)) {
            src2_0 = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(a2)));
            src2_1 = static_cast<uint32_t>(MEM_W(4, kseg0_gpr(a2)));
            src2_2 = static_cast<uint32_t>(MEM_W(8, kseg0_gpr(a2)));
        }
        ca_ord = ++s_ud5_ca628_ord;
        std::fprintf(stderr,
                     "[ud5-cam1c] phase=ca628 enter glidx=%d ord=%u pc=800ca628 "
                     "store=800ca65c ra=%08x dst=%08x src=%08x src2=%08x sel=%d rec=%08x "
                     "selected=%d old_1c=%08x src2_0=%08x src2_1=%08x src2_2=%08x "
                     "source_iter=1 source_reg=f6\n",
                     ud5_glidx, ca_ord, ra, a0, a1, a2, sel, rec,
                     (a0 != 0 && a0 == rec) ? 1 : 0, old_1c, src2_0, src2_1, src2_2);
        std::fflush(stderr);
    }
    func_800CA628(rdram, ctx);
    if (win && tooie_is_guest(a0)) {
        const gpr dst = kseg0_gpr(a0);
        const uint32_t stored = static_cast<uint32_t>(MEM_W(0x1C, dst));
        fprintf(stderr,
                "[ud4-ca628] leave seq=%u glidx=%d dst=%08x stored_1c=%.6f bits=%08x "
                "eye_dst=(%.6f,%.6f,%.6f)\n",
                tooie_ud4_seq(), glidx, a0, tooie_bits_f32(int32_t(stored)), stored,
                tooie_bits_f32(MEM_W(0, dst)), tooie_bits_f32(MEM_W(4, dst)),
                tooie_bits_f32(MEM_W(8, dst)));
        fflush(stderr);
    }
    if (ud5 && tooie_is_guest(a0)) {
        const uint32_t new_1c = static_cast<uint32_t>(MEM_W(0x1C, kseg0_gpr(a0)));
        std::fprintf(stderr,
                     "[ud5-cam1c] phase=ca628 leave glidx=%d ord=%u pc=800ca628 "
                     "store=800ca65c dst=%08x selected=%d old_1c=%08x source_bits=%08x "
                     "new_1c=%08x dst18=%08x dst20=%08x\n",
                     ud5_glidx, ca_ord, a0, (a0 != 0 && a0 == rec) ? 1 : 0, old_1c, src2_1,
                     new_1c, static_cast<uint32_t>(MEM_W(0x18, kseg0_gpr(a0))),
                     static_cast<uint32_t>(MEM_W(0x20, kseg0_gpr(a0))));
        std::fflush(stderr);
    }
}

static void tooie_wrap_8010fc38(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    if (tooie_ud4_cam_window(rdram, &glidx)) {
        const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
        uint32_t rec = 0, p8 = 0;
        int16_t h14 = -1;
        if (tooie_is_guest(a0)) {
            rec = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(a0)));
            p8 = static_cast<uint32_t>(MEM_W(8, kseg0_gpr(a0)));
            h14 = static_cast<int16_t>(MEM_H(0x14, kseg0_gpr(a0)));
        }
        fprintf(stderr,
                "[ud4-fc38] enter seq=%u glidx=%d ra=%08x node=%08x rec=%08x p8=%08x h14=%d\n",
                tooie_ud4_seq(), glidx, static_cast<uint32_t>(ctx->r31), a0, rec, p8, int(h14));
        fflush(stderr);
    }
    {
        int ud5_glidx = -1;
        if (tooie_ud5_cam1c_window(rdram, &ud5_glidx)) {
            std::fprintf(stderr,
                         "[ud5-cam1c] phase=fc38 enter glidx=%d pc=8010fc38 ra=%08x a0=%08x\n",
                         ud5_glidx, static_cast<uint32_t>(ctx->r31),
                         static_cast<uint32_t>(ctx->r4));
            std::fflush(stderr);
        }
    }
    func_8010FC38(rdram, ctx);
}

static void tooie_wrap_8010fe14(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    if (tooie_ud4_cam_window(rdram, &glidx)) {
        fprintf(stderr, "[ud4-fe14] enter seq=%u glidx=%d ra=%08x a0=%08x\n",
                tooie_ud4_seq(), glidx, static_cast<uint32_t>(ctx->r31),
                static_cast<uint32_t>(ctx->r4));
        fflush(stderr);
    }
    func_8010FE14(rdram, ctx);
}

static void tooie_wrap_ca8b4(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win = tooie_ud4_cam_window(rdram, &glidx);
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
                "[ud4-ca8b4] enter seq=%u glidx=%d ord=%d first=%d ra=%08x "
                "a0=%08x a1=%08x a2=%08x a3=%08x f4=%08x f6=%08x",
                tooie_ud4_seq(), glidx, ord, first, ra, a0, a1, a2, a3,
                ctx->f4.u32l, ctx->f6.u32l);
        tooie_ud4_dump_ca8b4_reads(rdram, stderr, a0);
        tooie_ud4_dump_player(rdram, stderr);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
    int ud5_glidx = -1;
    const int ud5 = tooie_ud5_cam1c_window(rdram, &ud5_glidx);
    int32_t sel = -1;
    uint32_t rec = 0;
    static thread_local uint32_t s_ud5_ca8b4_ord = 0;
    uint32_t ud5_ord = 0;
    if (ud5) {
        rec = tooie_ud5_selected_cam(rdram, &sel);
        ud5_ord = ++s_ud5_ca8b4_ord;
        uint32_t e0 = 0, e1 = 0, e2 = 0, s0 = 0, s1 = 0, s2 = 0;
        if (tooie_is_guest(a0)) {
            e0 = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(a0)));
            e1 = static_cast<uint32_t>(MEM_W(4, kseg0_gpr(a0)));
            e2 = static_cast<uint32_t>(MEM_W(8, kseg0_gpr(a0)));
            s0 = static_cast<uint32_t>(MEM_W(0x18, kseg0_gpr(a0)));
            s1 = static_cast<uint32_t>(MEM_W(0x1C, kseg0_gpr(a0)));
            s2 = static_cast<uint32_t>(MEM_W(0x20, kseg0_gpr(a0)));
        }
        std::fprintf(stderr,
                     "[ud5-cam1c] phase=ca8b4 enter glidx=%d ord=%u pc=800ca8b4 ra=%08x "
                     "a0=%08x a1=%08x a2=%08x sel=%d rec=%08x selected=%d "
                     "eye_in=%08x,%08x,%08x src2_in=%08x,%08x,%08x\n",
                     ud5_glidx, ud5_ord, ra, a0, a1, a2, sel, rec,
                     (a0 != 0 && a0 == rec) ? 1 : 0, e0, e1, e2, s0, s1, s2);
        std::fflush(stderr);
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
                "[ud4-ca8b4] leave seq=%u glidx=%d ord=%d eye_out=%08x,%08x,%08x "
                "src2_out=%08x,%08x,%08x eye=(%.6f,%.6f,%.6f) src2=(%.6f,%.6f,%.6f) "
                "f4=%08x f6=%08x\n",
                tooie_ud4_seq(), glidx, ord, e0, e1, e2, s0, s1, s2,
                tooie_bits_f32(int32_t(e0)), tooie_bits_f32(int32_t(e1)),
                tooie_bits_f32(int32_t(e2)), tooie_bits_f32(int32_t(s0)),
                tooie_bits_f32(int32_t(s1)), tooie_bits_f32(int32_t(s2)),
                ctx->f4.u32l, ctx->f6.u32l);
        fflush(stderr);
    }
    if (ud5 && tooie_is_guest(a2)) {
        std::fprintf(stderr,
                     "[ud5-cam1c] phase=ca8b4 leave glidx=%d ord=%u pc=800ca8b4 "
                     "selected=%d src2_0=%08x src2_1=%08x src2_2=%08x\n",
                     ud5_glidx, ud5_ord, (a0 != 0 && a0 == rec) ? 1 : 0,
                     static_cast<uint32_t>(MEM_W(0, kseg0_gpr(a2))),
                     static_cast<uint32_t>(MEM_W(4, kseg0_gpr(a2))),
                     static_cast<uint32_t>(MEM_W(8, kseg0_gpr(a2))));
        std::fflush(stderr);
    }
}

static void tooie_wrap_e1a58(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win = tooie_ud4_cam_window(rdram, &glidx);
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t a1 = static_cast<uint32_t>(ctx->r5);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    if (win && tooie_is_guest(a0) && tooie_is_guest(a1)) {
        fprintf(stderr,
                "[ud4-e1a58] enter seq=%u glidx=%d ra=%08x a0=%08x a1=%08x "
                "v0=(%.6f,%.6f,%.6f) v1=(%.6f,%.6f,%.6f) v1_2_bits=%08x\n",
                tooie_ud4_seq(), glidx, ra, a0, a1,
                tooie_bits_f32(MEM_W(0, kseg0_gpr(a0))), tooie_bits_f32(MEM_W(4, kseg0_gpr(a0))),
                tooie_bits_f32(MEM_W(8, kseg0_gpr(a0))),
                tooie_bits_f32(MEM_W(0, kseg0_gpr(a1))), tooie_bits_f32(MEM_W(4, kseg0_gpr(a1))),
                tooie_bits_f32(MEM_W(8, kseg0_gpr(a1))),
                static_cast<uint32_t>(MEM_W(8, kseg0_gpr(a1))));
        fflush(stderr);
    }
    int ud5_glidx = -1;
    const int ud5 = tooie_ud5_cam1c_window(rdram, &ud5_glidx);
    static thread_local uint32_t s_ud5_e1a58_ord = 0;
    uint32_t e_ord = 0;
    uint32_t in_h = 0;
    if (ud5) {
        e_ord = ++s_ud5_e1a58_ord;
        if (tooie_is_guest(a1)) {
            in_h = static_cast<uint32_t>(MEM_W(4, kseg0_gpr(a1)));
        }
        std::fprintf(stderr,
                     "[ud5-cam1c] phase=e1a58 enter glidx=%d ord=%u pc=800e1a58 ra=%08x "
                     "a0=%08x a1=%08x heading_src=%08x a1_0=%08x a1_2=%08x\n",
                     ud5_glidx, e_ord, ra, a0, a1, in_h,
                     tooie_is_guest(a1) ? static_cast<uint32_t>(MEM_W(0, kseg0_gpr(a1))) : 0,
                     tooie_is_guest(a1) ? static_cast<uint32_t>(MEM_W(8, kseg0_gpr(a1))) : 0);
        std::fflush(stderr);
    }
    func_800E1A58(rdram, ctx);
    if (win && tooie_is_guest(a0) && tooie_is_guest(a1)) {
        fprintf(stderr,
                "[ud4-e1a58] leave seq=%u glidx=%d v0=(%.6f,%.6f,%.6f) v1=(%.6f,%.6f,%.6f) "
                "v1_2_bits=%08x\n",
                tooie_ud4_seq(), glidx,
                tooie_bits_f32(MEM_W(0, kseg0_gpr(a0))), tooie_bits_f32(MEM_W(4, kseg0_gpr(a0))),
                tooie_bits_f32(MEM_W(8, kseg0_gpr(a0))),
                tooie_bits_f32(MEM_W(0, kseg0_gpr(a1))), tooie_bits_f32(MEM_W(4, kseg0_gpr(a1))),
                tooie_bits_f32(MEM_W(8, kseg0_gpr(a1))),
                static_cast<uint32_t>(MEM_W(8, kseg0_gpr(a1))));
        fflush(stderr);
    }
    if (ud5 && tooie_is_guest(a1)) {
        const uint32_t out_h = static_cast<uint32_t>(MEM_W(4, kseg0_gpr(a1)));
        std::fprintf(stderr,
                     "[ud5-cam1c] phase=e1a58 leave glidx=%d ord=%u pc=800e1a58 "
                     "heading_in=%08x heading_out=%08x a1_0=%08x a1_2=%08x\n",
                     ud5_glidx, e_ord, in_h, out_h,
                     static_cast<uint32_t>(MEM_W(0, kseg0_gpr(a1))),
                     static_cast<uint32_t>(MEM_W(8, kseg0_gpr(a1))));
        std::fflush(stderr);
    }
}

static int tooie_ud5_dir_window_early(uint8_t* rdram, int* glidx_out) {
    if (!tooie_diag::ud5_loco_enabled()) {
        return 0;
    }
    const int mode_v = int(MEM_H(0x2, kseg0_gpr(0x80127630u)));
    const int sub_v = int(MEM_BU(0x1, kseg0_gpr(0x80135470u)));
    const int32_t post_glidx = static_cast<int32_t>(MEM_W(0x8, kseg0_gpr(0x8012D5B0u)));
    const int glidx = (post_glidx > 0) ? int(post_glidx - 1) : 0;
    if (glidx_out) {
        *glidx_out = glidx;
    }
    return mode_v == 5 && sub_v == 5 && glidx >= 20 && glidx <= 50;
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
    int ud5_glidx = -1;
    const int ud5 = tooie_ud5_dir_window_early(rdram, &ud5_glidx);
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t src_bits = static_cast<uint32_t>(ctx->r5);
    uint32_t old_ideal = 0;
    uint32_t yawp = 0;
    if (ud5 && tooie_is_guest(a0)) {
        yawp = static_cast<uint32_t>(MEM_W(0xF8, kseg0_gpr(a0)));
        if (tooie_is_guest(yawp)) {
            old_ideal = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(yawp)));
        }
        static thread_local uint32_t s_yawideal_ord = 0;
        std::fprintf(stderr,
                     "[ud5-dir] phase=yaw_setIdeal enter glidx=%d ord=%u pc=8009c914 "
                     "store=8009c930 ra=%08x a0=%08x src_bits=%08x old_ideal=%08x\n",
                     ud5_glidx, ++s_yawideal_ord, static_cast<uint32_t>(ctx->r31), a0,
                     src_bits, old_ideal);
        std::fflush(stderr);
    }
    yaw_setIdeal(rdram, ctx);
    if (ud5 && tooie_is_guest(yawp)) {
        const uint32_t new_ideal = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(yawp)));
        std::fprintf(stderr,
                     "[ud5-dir] phase=yaw_setIdeal leave glidx=%d pc=8009c914 "
                     "store=8009c930 src_bits=%08x old_ideal=%08x new_ideal=%08x\n",
                     ud5_glidx, src_bits, old_ideal, new_ideal);
        std::fflush(stderr);
    }
}

static thread_local int s_ud5_in_ep1 = 0;
static thread_local int s_ud5_ep1_glidx = -1;

static int tooie_ud5_glidx(uint8_t* rdram) {
    const int32_t post_glidx = static_cast<int32_t>(MEM_W(0x8, kseg0_gpr(0x8012D5B0u)));
    return (post_glidx > 0) ? int(post_glidx - 1) : 0;
}

static int tooie_ud5_scene_ok(uint8_t* rdram) {
    if (!tooie_diag::ud5_loco_enabled()) {
        return 0;
    }
    const int mode_v = int(MEM_H(0x2, kseg0_gpr(0x80127630u)));
    const int sub_v = int(MEM_BU(0x1, kseg0_gpr(0x80135470u)));
    return mode_v == 5 && sub_v == 5;
}

static void tooie_ud5_dump_anctrl(uint8_t* rdram, const char* phase, int glidx,
                                 uint32_t extra0, uint32_t extra1) {
    const uint32_t ps_addr = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(0x80135490u)));
    int32_t bs_prev = -1, bs_cur = -1, bs_next = -1;
    uint32_t anctrl = 0;
    uint32_t timer_bits = 0, dur_bits = 0, id_word = 0, flag_word = 0, rate_bits = 0;
    uint8_t playback = 0, flags = 0;
    int16_t anim_id = -1;
    int32_t zone = -1;
    uint32_t flag160 = 0;
    uint32_t dist_bits = 0, stick_x_bits = 0, stick_y_bits = 0;
    if (tooie_is_guest(ps_addr)) {
        const gpr ps = kseg0_gpr(ps_addr);
        const uint32_t bsp = static_cast<uint32_t>(MEM_W(0x120, ps));
        if (tooie_is_guest(bsp)) {
            const gpr p = kseg0_gpr(bsp);
            bs_prev = static_cast<int32_t>(MEM_W(0x0, p));
            bs_cur = static_cast<int32_t>(MEM_W(0x4, p));
            bs_next = static_cast<int32_t>(MEM_W(0x8, p));
        }
        flag160 = static_cast<uint32_t>(MEM_W(0x160, ps));
        const uint32_t stick = static_cast<uint32_t>(MEM_W(0x128, ps));
        if (tooie_is_guest(stick)) {
            const gpr st = kseg0_gpr(stick);
            zone = static_cast<int32_t>(MEM_W(0x3C, st));
            stick_x_bits = static_cast<uint32_t>(MEM_W(0x54, st));
            stick_y_bits = static_cast<uint32_t>(MEM_W(0x58, st));
            dist_bits = static_cast<uint32_t>(MEM_W(0x60, st));
        }
        const uint32_t anim_p = static_cast<uint32_t>(MEM_W(0x04, ps));
        if (tooie_is_guest(anim_p)) {
            anctrl = static_cast<uint32_t>(MEM_W(0x00, kseg0_gpr(anim_p)));
            if (tooie_is_guest(anctrl)) {
                const gpr ac = kseg0_gpr(anctrl);
                dur_bits = static_cast<uint32_t>(MEM_W(0x0C, ac));
                rate_bits = static_cast<uint32_t>(MEM_W(0x10, ac));
                flag_word = static_cast<uint32_t>(MEM_W(0x18, ac));
                timer_bits = static_cast<uint32_t>(MEM_W(0x2C, ac));
                id_word = static_cast<uint32_t>(MEM_W(0x34, ac));
                playback = static_cast<uint8_t>(MEM_BU(0x1A, ac));
                flags = static_cast<uint8_t>(MEM_BU(0x1B, ac));
                anim_id = static_cast<int16_t>(MEM_H(0x34, ac));
            }
        }
    }
    const float timer = tooie_bits_f32(static_cast<int32_t>(timer_bits));
    const int isstopped = (playback == 3) ? 1 : 0;
    const int gt_half = (timer > 0.5f) ? 1 : 0;
    std::fprintf(stderr,
                 "[ud5-gate] phase=%s glidx=%d bs_prev=%d bs_cur=%d bs_next=%d "
                 "anim_id=%d anctrl=%08x timer_bits=%08x dur_bits=%08x rate_bits=%08x "
                 "playback=%u flags=0x%02x flag_word=%08x id_word=%08x "
                 "isstopped=%d gt_half=%d zone=%d flag160=%08x "
                 "stick_x_bits=%08x stick_y_bits=%08x dist_bits=%08x extra0=%08x extra1=%08x\n",
                 phase, glidx, bs_prev, bs_cur, bs_next, int(anim_id), anctrl,
                 timer_bits, dur_bits, rate_bits, unsigned(playback), unsigned(flags),
                 flag_word, id_word, isstopped, gt_half, zone, flag160,
                 stick_x_bits, stick_y_bits, dist_bits, extra0, extra1);
    std::fflush(stderr);
}

extern "C" void tooie_ud5_overlay_gate(uint8_t* rdram, recomp_context* ctx, uint32_t ovl,
                                      uint32_t ep, int leaving) {
    if (!tooie_ud5_scene_ok(rdram) || ovl != 29u) {
        return;
    }
    const int glidx = tooie_ud5_glidx(rdram);
    if (ep == 1u) {
        s_ud5_in_ep1 = leaving ? 0 : 1;
        s_ud5_ep1_glidx = glidx;
    }
    char phase[32];
    std::snprintf(phase, sizeof(phase), "ovl29-ep%u-%s", ep, leaving ? "leave" : "enter");
    tooie_ud5_dump_anctrl(rdram, phase, glidx,
                         static_cast<uint32_t>(ctx->r4),
                         static_cast<uint32_t>(ctx->r5));
    std::fprintf(stderr, "[ud5-gate] phase=order glidx=%d event=%s\n", glidx, phase);
    std::fflush(stderr);
}

static void tooie_wrap_anctrl_isStopped(uint8_t* rdram, recomp_context* ctx) {
    const int log = s_ud5_in_ep1 && tooie_ud5_scene_ok(rdram);
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    uint8_t playback = 0;
    if (log && tooie_is_guest(a0)) {
        playback = static_cast<uint8_t>(MEM_BU(0x1A, kseg0_gpr(a0)));
    }
    anctrl_isStopped(rdram, ctx);
    if (log) {
        std::fprintf(stderr,
                     "[ud5-gate] phase=isstopped glidx=%d a0=%08x playback=%u v0=%d ra=%08x\n",
                     s_ud5_ep1_glidx, a0, unsigned(playback), int(ctx->r2),
                     static_cast<uint32_t>(ctx->r31));
        std::fflush(stderr);
    }
}

static void tooie_wrap_anctrl_getAnimTimer(uint8_t* rdram, recomp_context* ctx) {
    const int log = s_ud5_in_ep1 && tooie_ud5_scene_ok(rdram);
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    anctrl_getAnimTimer(rdram, ctx);
    if (log) {
        const uint32_t bits = ctx->f0.u32l;
        const float t = tooie_bits_f32(static_cast<int32_t>(bits));
        std::fprintf(stderr,
                     "[ud5-gate] phase=timer glidx=%d a0=%08x f0_bits=%08x gt_half=%d ra=%08x\n",
                     s_ud5_ep1_glidx, a0, bits, (t > 0.5f) ? 1 : 0,
                     static_cast<uint32_t>(ctx->r31));
        std::fflush(stderr);
    }
}

static void tooie_wrap_bastick_getZone(uint8_t* rdram, recomp_context* ctx) {
    const int log = s_ud5_in_ep1 && tooie_ud5_scene_ok(rdram);
    bastick_getZone(rdram, ctx);
    if (log) {
        std::fprintf(stderr,
                     "[ud5-gate] phase=arbiter-zone glidx=%d v0=%d ra=%08x\n",
                     s_ud5_ep1_glidx, int(ctx->r2), static_cast<uint32_t>(ctx->r31));
        std::fflush(stderr);
    }
}

static int tooie_ud5_is_player_anctrl(uint8_t* rdram, uint32_t a0) {
    const uint32_t ps = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(0x80135490u)));
    if (!tooie_is_guest(ps) || !tooie_is_guest(a0)) {
        return 0;
    }
    const uint32_t anim = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(ps)));
    if (!tooie_is_guest(anim)) {
        return 0;
    }
    const uint32_t anctrl = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(anim)));
    return a0 == anctrl;
}

static void tooie_wrap_func_8008ADE4(uint8_t* rdram, recomp_context* ctx) {
    if (tooie_ud5_scene_ok(rdram) &&
        tooie_ud5_is_player_anctrl(rdram, static_cast<uint32_t>(ctx->r4))) {
        const int glidx = tooie_ud5_glidx(rdram);
        tooie_ud5_dump_anctrl(rdram, "anctrl-update-enter", glidx,
                             static_cast<uint32_t>(ctx->r4), 0);
        std::fprintf(stderr, "[ud5-gate] phase=order glidx=%d event=anctrl-update\n", glidx);
        std::fflush(stderr);
        func_8008ADE4(rdram, ctx);
        tooie_ud5_dump_anctrl(rdram, "anctrl-update-leave", glidx,
                             static_cast<uint32_t>(ctx->r4), 0);
        return;
    }
    func_8008ADE4(rdram, ctx);
}

static void tooie_wrap_bs_setState(uint8_t* rdram, recomp_context* ctx) {
    if (!tooie_diag::ud5_loco_enabled()) {
        bs_setState(rdram, ctx);
        return;
    }
    const int mode_v = int(MEM_H(0x2, kseg0_gpr(0x80127630u)));
    const int sub_v = int(MEM_BU(0x1, kseg0_gpr(0x80135470u)));
    const int32_t post_glidx = static_cast<int32_t>(MEM_W(0x8, kseg0_gpr(0x8012D5B0u)));
    const int32_t glidx = (post_glidx > 0) ? (post_glidx - 1) : 0;
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const int32_t next_state = static_cast<int32_t>(ctx->r5);
    int32_t prev = -1, cur = -1, nxt = -1;
    if (tooie_is_guest(a0)) {
        const uint32_t bsp = static_cast<uint32_t>(MEM_W(0x120, kseg0_gpr(a0)));
        if (tooie_is_guest(bsp)) {
            const gpr p = kseg0_gpr(bsp);
            prev = static_cast<int32_t>(MEM_W(0x0, p));
            cur = static_cast<int32_t>(MEM_W(0x4, p));
            nxt = static_cast<int32_t>(MEM_W(0x8, p));
        }
    }
    if (mode_v == 5 && sub_v == 5) {
        std::fprintf(stderr,
                     "[ud5-setstate] enter glidx=%d ra=%08x a0=%08x next=%d "
                     "before prev=%d cur=%d nextf=%d\n",
                     glidx, static_cast<uint32_t>(ctx->r31), a0, next_state,
                     prev, cur, nxt);
        std::fflush(stderr);
        tooie_ud5_dump_anctrl(rdram, "setstate-enter", glidx, a0,
                             static_cast<uint32_t>(next_state));
        std::fprintf(stderr,
                     "[ud5-gate] phase=setstate glidx=%d next=%d ra=%08x in_ep1=%d\n",
                     glidx, next_state, static_cast<uint32_t>(ctx->r31), s_ud5_in_ep1);
        std::fflush(stderr);
    }
    if (tooie_diag::ud5_loco_enabled() && mode_v == 5 && next_state == 22 &&
        (sub_v == 1 || sub_v == 5) && sub_v >= 0 && sub_v < 8) {
        const unsigned tord = ++s_ud5_abc_trot[sub_v];
        s_ud5_abc_pending[sub_v] = 1;
        std::fprintf(stderr,
                     "[ud5-abc] event=state22 sub=%d trot_ord=%u glidx=%d map=%d\n",
                     sub_v, tord, glidx, int(MEM_H(0x10, kseg0_gpr(0x80127630u))));
        std::fflush(stderr);
    }
    bs_setState(rdram, ctx);
    if (mode_v == 5 && sub_v == 5 && tooie_is_guest(a0)) {
        const uint32_t bsp = static_cast<uint32_t>(MEM_W(0x120, kseg0_gpr(a0)));
        if (tooie_is_guest(bsp)) {
            const gpr p = kseg0_gpr(bsp);
            prev = static_cast<int32_t>(MEM_W(0x0, p));
            cur = static_cast<int32_t>(MEM_W(0x4, p));
            nxt = static_cast<int32_t>(MEM_W(0x8, p));
        }
        std::fprintf(stderr,
                     "[ud5-setstate] leave glidx=%d ra=%08x next_arg=%d "
                     "after prev=%d cur=%d nextf=%d\n",
                     glidx, static_cast<uint32_t>(ctx->r31), next_state,
                     prev, cur, nxt);
        std::fflush(stderr);
    }
}

/* USER_DEMO_5 diagnostic only: capture the exact target-horizontal-speed
 * writer so RT64/AN can be joined by semantic call event, not CSV glidx. */
static void tooie_wrap_baphysics_set_target_horizontal_velocity(
    uint8_t* rdram, recomp_context* ctx) {
    if (!tooie_ud5_scene_ok(rdram)) {
        baphysics_set_target_horizontal_velocity(rdram, ctx);
        return;
    }
    static thread_local uint32_t call_ord = 0;
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t source_bits = static_cast<uint32_t>(ctx->r5);
    uint32_t phys = 0;
    uint32_t old_bits = 0;
    int32_t bs_prev = -1, bs_cur = -1, bs_next = -1;
    if (tooie_is_guest(a0)) {
        phys = static_cast<uint32_t>(MEM_W(0xC8, kseg0_gpr(a0)));
        const uint32_t bsp = static_cast<uint32_t>(MEM_W(0x120, kseg0_gpr(a0)));
        if (tooie_is_guest(phys)) {
            old_bits = static_cast<uint32_t>(MEM_W(0x3C, kseg0_gpr(phys)));
        }
        if (tooie_is_guest(bsp)) {
            const gpr p = kseg0_gpr(bsp);
            bs_prev = static_cast<int32_t>(MEM_W(0x0, p));
            bs_cur = static_cast<int32_t>(MEM_W(0x4, p));
            bs_next = static_cast<int32_t>(MEM_W(0x8, p));
        }
    }
    const int glidx = tooie_ud5_glidx(rdram);
    const uint32_t guest_tick = static_cast<uint32_t>(tooie_diag::guest_logic_tick());
    const uint32_t ordinal = ++call_ord;
    std::fprintf(stderr,
                 "[ud5-tspd] enter glidx=%d logic=%u ord=%u pc=8009b9b0 "
                 "store=8009b9bc ra=%08x a0=%08x phys=%08x "
                 "state=%d>%d next=%d old_bits=%08x source_bits=%08x\n",
                 glidx, guest_tick, ordinal, static_cast<uint32_t>(ctx->r31),
                 a0, phys, bs_prev, bs_cur, bs_next, old_bits, source_bits);
    std::fflush(stderr);
    baphysics_set_target_horizontal_velocity(rdram, ctx);
    uint32_t new_bits = 0;
    if (tooie_is_guest(phys)) {
        new_bits = static_cast<uint32_t>(MEM_W(0x3C, kseg0_gpr(phys)));
    }
    std::fprintf(stderr,
                 "[ud5-tspd] leave glidx=%d logic=%u ord=%u pc=8009b9b0 "
                 "store=8009b9bc phys=%08x old_bits=%08x new_bits=%08x "
                 "source_bits=%08x\n",
                 glidx, guest_tick, ordinal, phys, old_bits, new_bits, source_bits);
    std::fflush(stderr);
}

static int tooie_ud5_dir_window(uint8_t* rdram, int* glidx_out) {
    if (!tooie_ud5_scene_ok(rdram)) {
        return 0;
    }
    const int glidx = tooie_ud5_glidx(rdram);
    if (glidx_out) {
        *glidx_out = glidx;
    }
    return glidx >= 20 && glidx <= 50;
}

static uint32_t tooie_ud5_camctrl_bits(uint8_t* rdram, uint32_t player) {
    if (!tooie_is_guest(player)) {
        return 0;
    }
    const int32_t sel = static_cast<int32_t>(MEM_W(0x184, kseg0_gpr(player)));
    if (sel < 0 || sel >= 8) {
        return 0;
    }
    const uint32_t rec = static_cast<uint32_t>(MEM_W(sel * 4, kseg0_gpr(0x80127708u)));
    if (!tooie_is_guest(rec)) {
        return 0;
    }
    return static_cast<uint32_t>(MEM_W(0x1C, kseg0_gpr(rec)));
}

static void tooie_ud5_stick_bits(uint8_t* rdram, uint32_t player, uint32_t* ang,
                                 uint32_t* x, uint32_t* y, uint32_t* dist, int* zone) {
    *ang = *x = *y = *dist = 0;
    *zone = -1;
    if (!tooie_is_guest(player)) {
        return;
    }
    const uint32_t stick = static_cast<uint32_t>(MEM_W(0x128, kseg0_gpr(player)));
    if (!tooie_is_guest(stick)) {
        return;
    }
    const gpr s = kseg0_gpr(stick);
    *x = static_cast<uint32_t>(MEM_W(0x54, s));
    *y = static_cast<uint32_t>(MEM_W(0x58, s));
    *ang = static_cast<uint32_t>(MEM_W(0x5C, s));
    *dist = static_cast<uint32_t>(MEM_W(0x60, s));
    *zone = static_cast<int32_t>(MEM_W(0x3C, s));
}

static void tooie_wrap_bastick_get_angle_relative(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    if (!tooie_ud5_dir_window(rdram, &glidx)) {
        bastick_getAngleRelativeToBanjo(rdram, ctx);
        return;
    }
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    uint32_t ang = 0, sx = 0, sy = 0, dist = 0;
    int zone = -1;
    tooie_ud5_stick_bits(rdram, a0, &ang, &sx, &sy, &dist, &zone);
    const uint32_t cam_bits = tooie_ud5_camctrl_bits(rdram, a0);
    static thread_local uint32_t s_rel_ord = 0;
    const uint32_t ord = ++s_rel_ord;
    std::fprintf(stderr,
                 "[ud5-dir] phase=rel_angle enter glidx=%d ord=%u pc=8009eeb8 ra=%08x "
                 "a0=%08x camctrl_bits=%08x stick_ang=%08x stick_xy=%08x,%08x dist=%08x zone=%d\n",
                 glidx, ord, static_cast<uint32_t>(ctx->r31), a0, cam_bits, ang, sx, sy,
                 dist, zone);
    std::fflush(stderr);
    bastick_getAngleRelativeToBanjo(rdram, ctx);
    std::fprintf(stderr,
                 "[ud5-dir] phase=rel_angle leave glidx=%d ord=%u pc=8009eeb8 "
                 "f0_bits=%08x camctrl_bits=%08x stick_ang=%08x\n",
                 glidx, ord, ctx->f0.u32l, cam_bits, ang);
    std::fflush(stderr);
}

static void tooie_wrap_func_800CAF00(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const int dir = tooie_ud5_dir_window(rdram, &glidx);
    const int mode_v = int(MEM_H(0x2, kseg0_gpr(0x80127630u)));
    const int sub_v = int(MEM_BU(0x1, kseg0_gpr(0x80135470u)));
    const int32_t post_glidx = static_cast<int32_t>(MEM_W(0x8, kseg0_gpr(0x8012D5B0u)));
    const int abc_glidx = (post_glidx > 0) ? int(post_glidx - 1) : 0;
    const int abc = tooie_diag::ud5_loco_enabled() && mode_v == 5 &&
                    (sub_v == 1 || sub_v == 5);
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t stick_bits = static_cast<uint32_t>(ctx->r5);
    uint32_t cam_field = 0;
    if ((dir || abc) && tooie_is_guest(a0)) {
        cam_field = static_cast<uint32_t>(MEM_W(0x1C, kseg0_gpr(a0)));
    }
    static thread_local uint32_t s_caf_ord = 0;
    uint32_t ord = 0;
    if (dir) {
        ord = ++s_caf_ord;
        std::fprintf(stderr,
                     "[ud5-dir] phase=caf00 enter glidx=%d ord=%u pc=800caf00 ra=%08x "
                     "cam_obj=%08x cam_plus_1c=%08x stick_ang=%08x\n",
                     glidx, ord, static_cast<uint32_t>(ctx->r31), a0, cam_field, stick_bits);
        std::fflush(stderr);
    }
    func_800CAF00(rdram, ctx);
    if (dir) {
        std::fprintf(stderr,
                     "[ud5-dir] phase=caf00 leave glidx=%d ord=%u pc=800caf00 "
                     "cam_plus_1c=%08x stick_ang=%08x f0_bits=%08x\n",
                     glidx, ord, cam_field, stick_bits, ctx->f0.u32l);
        std::fflush(stderr);
    }
    if (abc && sub_v >= 0 && sub_v < 8 && s_ud5_abc_pending[sub_v]) {
        s_ud5_abc_pending[sub_v] = 0;
        std::fprintf(stderr,
                     "[ud5-abc] event=caf00 sub=%d trot_ord=%u glidx=%d "
                     "cam_plus_1c=%08x stick_ang=%08x f0_bits=%08x\n",
                     sub_v, s_ud5_abc_trot[sub_v], abc_glidx, cam_field,
                     stick_bits, ctx->f0.u32l);
        std::fflush(stderr);
    }
}

static void tooie_wrap_baphysics_set_target_yaw(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    if (!tooie_ud5_dir_window(rdram, &glidx)) {
        baphysics_set_target_yaw(rdram, ctx);
        return;
    }
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t src_bits = static_cast<uint32_t>(ctx->r5);
    uint32_t phys = 0, old_bits = 0;
    if (tooie_is_guest(a0)) {
        phys = static_cast<uint32_t>(MEM_W(0xC8, kseg0_gpr(a0)));
        if (tooie_is_guest(phys)) {
            old_bits = static_cast<uint32_t>(MEM_W(0x40, kseg0_gpr(phys)));
        }
    }
    static thread_local uint32_t s_tyaw_ord = 0;
    const uint32_t ord = ++s_tyaw_ord;
    std::fprintf(stderr,
                 "[ud5-dir] phase=set_target_yaw enter glidx=%d ord=%u pc=8009b9c0 "
                 "store=8009b9dc ra=%08x a0=%08x phys=%08x src_bits=%08x old_bits=%08x\n",
                 glidx, ord, static_cast<uint32_t>(ctx->r31), a0, phys, src_bits, old_bits);
    std::fflush(stderr);
    baphysics_set_target_yaw(rdram, ctx);
    uint32_t new_bits = 0;
    if (tooie_is_guest(phys)) {
        new_bits = static_cast<uint32_t>(MEM_W(0x40, kseg0_gpr(phys)));
    }
    std::fprintf(stderr,
                 "[ud5-dir] phase=set_target_yaw leave glidx=%d ord=%u pc=8009b9c0 "
                 "src_bits=%08x old_bits=%08x new_bits=%08x\n",
                 glidx, ord, src_bits, old_bits, new_bits);
    std::fflush(stderr);
}

static void tooie_wrap_baphysics_set_horizontal_velocity(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    if (!tooie_ud5_dir_window(rdram, &glidx)) {
        baphysics_set_horizontal_velocity(rdram, ctx);
        return;
    }
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t yaw_bits = static_cast<uint32_t>(ctx->r5);
    const uint32_t spd_bits = static_cast<uint32_t>(ctx->r6);
    uint32_t phys = 0, vx = 0, vz = 0;
    if (tooie_is_guest(a0)) {
        phys = static_cast<uint32_t>(MEM_W(0xC8, kseg0_gpr(a0)));
        if (tooie_is_guest(phys)) {
            vx = static_cast<uint32_t>(MEM_W(0x10, kseg0_gpr(phys)));
            vz = static_cast<uint32_t>(MEM_W(0x18, kseg0_gpr(phys)));
        }
    }
    static thread_local uint32_t s_hvel_ord = 0;
    const uint32_t ord = ++s_hvel_ord;
    std::fprintf(stderr,
                 "[ud5-dir] phase=set_hvel enter glidx=%d ord=%u pc=8009ba68 ra=%08x "
                 "yaw_bits=%08x spd_bits=%08x old_vx=%08x old_vz=%08x\n",
                 glidx, ord, static_cast<uint32_t>(ctx->r31), yaw_bits, spd_bits, vx, vz);
    std::fflush(stderr);
    baphysics_set_horizontal_velocity(rdram, ctx);
    uint32_t nvx = 0, nvz = 0;
    if (tooie_is_guest(phys)) {
        nvx = static_cast<uint32_t>(MEM_W(0x10, kseg0_gpr(phys)));
        nvz = static_cast<uint32_t>(MEM_W(0x18, kseg0_gpr(phys)));
    }
    std::fprintf(stderr,
                 "[ud5-dir] phase=set_hvel leave glidx=%d ord=%u pc=8009ba68 "
                 "yaw_bits=%08x spd_bits=%08x new_vx=%08x new_vz=%08x\n",
                 glidx, ord, yaw_bits, spd_bits, nvx, nvz);
    std::fflush(stderr);
}

static void tooie_wrap_func_8009D088(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    if (!tooie_ud5_dir_window(rdram, &glidx)) {
        func_8009D088(rdram, ctx);
        return;
    }
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    int ytype = -1;
    uint32_t ang = 0, sx = 0, sy = 0, dist = 0;
    int zone = -1;
    int32_t bs_cur = -1;
    if (tooie_is_guest(a0)) {
        const uint32_t ctrl = static_cast<uint32_t>(MEM_W(0xFC, kseg0_gpr(a0)));
        if (tooie_is_guest(ctrl)) {
            ytype = static_cast<int32_t>(MEM_W(0x0, kseg0_gpr(ctrl)));
        }
        const uint32_t bsp = static_cast<uint32_t>(MEM_W(0x120, kseg0_gpr(a0)));
        if (tooie_is_guest(bsp)) {
            bs_cur = static_cast<int32_t>(MEM_W(0x4, kseg0_gpr(bsp)));
        }
        tooie_ud5_stick_bits(rdram, a0, &ang, &sx, &sy, &dist, &zone);
    }
    static thread_local uint32_t s_d088_ord = 0;
    const uint32_t ord = ++s_d088_ord;
    std::fprintf(stderr,
                 "[ud5-dir] phase=yaw_ctrl enter glidx=%d ord=%u pc=8009d088 ra=%08x "
                 "ytype=%d bs_cur=%d camctrl_bits=%08x stick_ang=%08x dist=%08x zone=%d\n",
                 glidx, ord, static_cast<uint32_t>(ctx->r31), ytype, bs_cur,
                 tooie_ud5_camctrl_bits(rdram, a0), ang, dist, zone);
    std::fflush(stderr);
    func_8009D088(rdram, ctx);
    std::fprintf(stderr, "[ud5-dir] phase=yaw_ctrl leave glidx=%d ord=%u pc=8009d088\n",
                 glidx, ord);
    std::fflush(stderr);
}

static void tooie_wrap_func_800EEB9C(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    if (!tooie_ud5_dir_window(rdram, &glidx)) {
        func_800EEB9C(rdram, ctx);
        return;
    }
    const uint32_t dst = static_cast<uint32_t>(ctx->r4);
    const uint32_t yaw_bits = static_cast<uint32_t>(ctx->r5);
    const uint32_t spd_bits = static_cast<uint32_t>(ctx->r6);
    static thread_local uint32_t s_eeb_ord = 0;
    const uint32_t ord = ++s_eeb_ord;
    std::fprintf(stderr,
                 "[ud5-dir] phase=sincos_vel enter glidx=%d ord=%u pc=800eeb9c ra=%08x "
                 "dst=%08x yaw_bits=%08x spd_bits=%08x\n",
                 glidx, ord, static_cast<uint32_t>(ctx->r31), dst, yaw_bits, spd_bits);
    std::fflush(stderr);
    func_800EEB9C(rdram, ctx);
    uint32_t vx = 0, vy = 0, vz = 0;
    if (tooie_is_guest(dst)) {
        vx = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(dst)));
        vy = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(dst)));
        vz = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(dst)));
    }
    std::fprintf(stderr,
                 "[ud5-dir] phase=sincos_vel leave glidx=%d ord=%u pc=800eeb9c "
                 "yaw_bits=%08x spd_bits=%08x vx=%08x vy=%08x vz=%08x\n",
                 glidx, ord, yaw_bits, spd_bits, vx, vy, vz);
    std::fflush(stderr);
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
    int ud5_glidx = -1;
    const int ud5 = tooie_ud5_cam1c_window(rdram, &ud5_glidx);
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
    static thread_local unsigned s_ud5_set74_ord = 0;
    if (ud5) {
        uint32_t old_74 = 0, old_78 = 0, old_7c = 0;
        uint32_t s0_v = 0, s1_v = 0, s2_v = 0;
        if (tooie_is_guest(node)) {
            old_74 = static_cast<uint32_t>(MEM_W(0x74, kseg0_gpr(node)));
            old_78 = static_cast<uint32_t>(MEM_W(0x78, kseg0_gpr(node)));
            old_7c = static_cast<uint32_t>(MEM_W(0x7C, kseg0_gpr(node)));
        }
        if (tooie_is_guest(src)) {
            s0_v = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(src)));
            s1_v = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(src)));
            s2_v = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(src)));
        }
        const unsigned uord = ++s_ud5_set74_ord;
        fprintf(stderr,
                "[ud5-set74] phase=set74 enter glidx=%d ord=%u pc=80110770 ra=%08x "
                "s0=%08x src=%08x old_74=(%08x,%08x,%08x / %.4f,%.4f,%.4f) src=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                ud5_glidx, uord, ra, node, src,
                old_74, old_78, old_7c, tooie_bits_f32(old_74), tooie_bits_f32(old_78), tooie_bits_f32(old_7c),
                s0_v, s1_v, s2_v, tooie_bits_f32(s0_v), tooie_bits_f32(s1_v), tooie_bits_f32(s2_v));
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
    if (ud5) {
        uint32_t new_74 = 0, new_78 = 0, new_7c = 0;
        if (tooie_is_guest(node)) {
            new_74 = static_cast<uint32_t>(MEM_W(0x74, kseg0_gpr(node)));
            new_78 = static_cast<uint32_t>(MEM_W(0x78, kseg0_gpr(node)));
            new_7c = static_cast<uint32_t>(MEM_W(0x7C, kseg0_gpr(node)));
        }
        fprintf(stderr,
                "[ud5-set74] phase=set74 leave glidx=%d ord=%u pc=80110770 "
                "s0=%08x new_74=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                ud5_glidx, s_ud5_set74_ord, node,
                new_74, new_78, new_7c, tooie_bits_f32(new_74), tooie_bits_f32(new_78), tooie_bits_f32(new_7c));
        fflush(stderr);
    }
}

static void tooie_wrap_func_80110790(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win = tooie_ud4_writer_window(rdram, &glidx) || (s_ud4_np_active != 0);
    int ud5_glidx = -1;
    const int ud5 = tooie_ud5_cam1c_window(rdram, &ud5_glidx);
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
    static thread_local unsigned s_ud5_set80_ord = 0;
    int32_t sel = -1;
    uint32_t rec = 0;
    uint32_t target_mode2 = 0;
    if (ud5) {
        rec = tooie_ud5_selected_cam(rdram, &sel);
        if (tooie_is_guest(rec)) {
            target_mode2 = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(rec)));
        }
        uint32_t old_80 = 0, old_84 = 0, old_88 = 0;
        uint32_t src_0 = 0, src_1 = 0, src_2 = 0;
        if (tooie_is_guest(node)) {
            old_80 = static_cast<uint32_t>(MEM_W(0x80, kseg0_gpr(node)));
            old_84 = static_cast<uint32_t>(MEM_W(0x84, kseg0_gpr(node)));
            old_88 = static_cast<uint32_t>(MEM_W(0x88, kseg0_gpr(node)));
        }
        if (tooie_is_guest(src)) {
            src_0 = static_cast<uint32_t>(MEM_W(0x00, kseg0_gpr(src)));
            src_1 = static_cast<uint32_t>(MEM_W(0x04, kseg0_gpr(src)));
            src_2 = static_cast<uint32_t>(MEM_W(0x08, kseg0_gpr(src)));
        }
        const unsigned uord = ++s_ud5_set80_ord;
        fprintf(stderr,
                "[ud5-set80] phase=set80 enter glidx=%d ord=%u pc=80110790 store=800ee804 ra=%08x "
                "node=%08x src=%08x sel=%d rec=%08x target_mode2=%08x selected=%d "
                "old_80=%08x old_84=%08x old_88=%08x src_0=%08x src_1=%08x src_2=%08x\n",
                ud5_glidx, uord, ra, node, src, sel, rec, target_mode2,
                (node != 0 && node == target_mode2) ? 1 : 0,
                old_80, old_84, old_88, src_0, src_1, src_2);
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
    if (ud5) {
        uint32_t new_80 = 0, new_84 = 0, new_88 = 0;
        if (tooie_is_guest(node)) {
            new_80 = static_cast<uint32_t>(MEM_W(0x80, kseg0_gpr(node)));
            new_84 = static_cast<uint32_t>(MEM_W(0x84, kseg0_gpr(node)));
            new_88 = static_cast<uint32_t>(MEM_W(0x88, kseg0_gpr(node)));
        }
        fprintf(stderr,
                "[ud5-set80] phase=set80 leave glidx=%d ord=%u pc=80110790 store=800ee804 "
                "node=%08x selected=%d new_80=%08x new_84=%08x new_88=%08x\n",
                ud5_glidx, s_ud5_set80_ord, node,
                (node != 0 && node == target_mode2) ? 1 : 0,
                new_80, new_84, new_88);
        fflush(stderr);
    }
}

static void tooie_wrap_func_800EE7F8(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool writer = (s_ud4_in_set74 || s_ud4_in_set80) &&
                        (tooie_ud4_writer_window(rdram, &glidx) || (s_ud4_np_active != 0));
    int ud5_glidx = -1;
    const int ud5 = tooie_ud5_cam1c_window(rdram, &ud5_glidx);
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
    if (ud5) {
        int32_t sel = -1;
        uint32_t rec = tooie_ud5_selected_cam(rdram, &sel);
        uint32_t target_mode2 = 0;
        if (tooie_is_guest(rec)) {
            target_mode2 = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(rec)));
        }
        const int is_8c = (s_ud5_eye_s0 != 0 && (dst == s_ud5_eye_s0 + 0x8Cu || src == s_ud5_eye_s0 + 0x8Cu)) ||
                          (target_mode2 != 0 && (dst == target_mode2 + 0x8Cu || src == target_mode2 + 0x8Cu));
        uint32_t d0 = 0, d1 = 0, d2 = 0, s0_v = 0, s1_v = 0, s2_v = 0;
        if (is_8c || (target_mode2 != 0 && (dst == target_mode2 + 0x80u || src == target_mode2 + 0x80u))) {
            if (tooie_is_guest(dst)) {
                d0 = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(dst)));
                d1 = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(dst)));
                d2 = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(dst)));
            }
            if (tooie_is_guest(src)) {
                s0_v = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(src)));
                s1_v = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(src)));
                s2_v = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(src)));
            }
        }
        if (target_mode2 != 0 && (dst == target_mode2 + 0x80u || src == target_mode2 + 0x80u)) {
            fprintf(stderr,
                    "[ud5-ee7f8] enter glidx=%d ra=%08x dst=%08x src=%08x is_write=%d is_read=%d "
                    "dst_pre=%08x,%08x,%08x src=%08x,%08x,%08x\n",
                    ud5_glidx, static_cast<uint32_t>(ctx->r31), dst, src,
                    (dst == target_mode2 + 0x80u) ? 1 : 0,
                    (src == target_mode2 + 0x80u) ? 1 : 0,
                    d0, d1, d2, s0_v, s1_v, s2_v);
            fflush(stderr);
        }
        if (is_8c) {
            fprintf(stderr,
                    "[ud5-eye] phase=s0_8c_write enter glidx=%d pc=800ee7f8 ra=%08x "
                    "dst=%08x src=%08x is_write=%d old=(%08x,%08x,%08x / %.4f,%.4f,%.4f) "
                    "srcv=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                    ud5_glidx, static_cast<uint32_t>(ctx->r31), dst, src,
                    (dst == s_ud5_eye_s0 + 0x8Cu || dst == target_mode2 + 0x8Cu) ? 1 : 0,
                    d0, d1, d2, tooie_bits_f32(d0), tooie_bits_f32(d1), tooie_bits_f32(d2),
                    s0_v, s1_v, s2_v, tooie_bits_f32(s0_v), tooie_bits_f32(s1_v), tooie_bits_f32(s2_v));
            fflush(stderr);
        }
        if (ud5_glidx >= 22 && ud5_glidx <= 25) {
            uint32_t od0 = 0, od1 = 0, od2 = 0, s0b = 0, s1b = 0, s2b = 0;
            tooie_ud5_read3(rdram, dst, &od0, &od1, &od2);
            tooie_ud5_read3(rdram, src, &s0b, &s1b, &s2b);
            fprintf(stderr,
                    "[ud5-ee7f8] enter glidx=%d ra=%08x dst=%08x src=%08x "
                    "old=(%08x,%08x,%08x / %.4f,%.4f,%.4f) "
                    "srcv=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                    ud5_glidx, static_cast<uint32_t>(ctx->r31), dst, src,
                    od0, od1, od2, tooie_bits_f32(od0), tooie_bits_f32(od1), tooie_bits_f32(od2),
                    s0b, s1b, s2b, tooie_bits_f32(s0b), tooie_bits_f32(s1b), tooie_bits_f32(s2b));
            fflush(stderr);
        }
    }
    tooie_ud4_np_call("800EE7F8", 0x800EE7F8u, rdram, ctx, 1);
    func_800EE7F8(rdram, ctx);
    tooie_ud4_np_call("800EE7F8", 0x800EE7F8u, rdram, ctx, 0);
    if (ud5 && tooie_is_guest(dst)) {
        const uint32_t n0 = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(dst)));
        const uint32_t n1 = static_cast<uint32_t>(MEM_W(4, kseg0_gpr(dst)));
        const uint32_t n2 = static_cast<uint32_t>(MEM_W(8, kseg0_gpr(dst)));
        const int wrote_8c = (s_ud5_eye_s0 != 0 && dst == s_ud5_eye_s0 + 0x8Cu);
        if (wrote_8c) {
            fprintf(stderr,
                    "[ud5-eye] phase=s0_8c_write leave glidx=%d pc=800ee7f8 ra=%08x "
                    "new=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                    ud5_glidx, static_cast<uint32_t>(ctx->r31),
                    n0, n1, n2, tooie_bits_f32(n0), tooie_bits_f32(n1), tooie_bits_f32(n2));
            fflush(stderr);
        }
        if (ud5_glidx >= 22 && ud5_glidx <= 25) {
            fprintf(stderr,
                    "[ud5-ee7f8] leave glidx=%d ra=%08x dst=%08x "
                    "out=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                    ud5_glidx, static_cast<uint32_t>(ctx->r31), dst,
                    n0, n1, n2, tooie_bits_f32(n0), tooie_bits_f32(n1), tooie_bits_f32(n2));
            fflush(stderr);
        }
    }
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
    const bool win = tooie_ud4_writer_window(rdram, &glidx);
    const uint32_t node = static_cast<uint32_t>(ctx->r4);
    if (win) {
        fprintf(stderr,
                "[ud4-p60-write] enter seq=%u glidx=%d pc=80112D08 ra=%08x"
                " node=%08x a1=%08x a2=%08x a3=%08x",
                tooie_ud4_seq(), glidx, static_cast<uint32_t>(ctx->r31), node,
                static_cast<uint32_t>(ctx->r5), static_cast<uint32_t>(ctx->r6),
                static_cast<uint32_t>(ctx->r7));
        tooie_ud4_dump_p60_object(rdram, stderr, node);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
    func_80112D08(rdram, ctx);
    if (win) {
        fprintf(stderr,
                "[ud4-p60-write] leave seq=%u glidx=%d pc=80112D08 node=%08x",
                tooie_ud4_seq(), glidx, node);
        tooie_ud4_dump_p60_object(rdram, stderr, node);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
}

static void tooie_wrap_func_80112A84(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win = tooie_ud4_writer_window(rdram, &glidx);
    const uint32_t node = static_cast<uint32_t>(ctx->r4);
    if (win) {
        fprintf(stderr,
                "[ud4-p60-writer] enter seq=%u glidx=%d pc=80112A84 ra=%08x node=%08x",
                tooie_ud4_seq(), glidx, static_cast<uint32_t>(ctx->r31), node);
        tooie_ud4_dump_p60_object(rdram, stderr, node);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
    func_80112A84(rdram, ctx);
    if (win) {
        fprintf(stderr,
                "[ud4-p60-writer] leave seq=%u glidx=%d pc=80112A84 node=%08x",
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
    const bool win = tooie_ud4_writer_window(rdram, &glidx);
    const uint32_t self = static_cast<uint32_t>(ctx->r4);
    if (win) {
        fprintf(stderr, "[ud4-cam-chain] enter 80097B94 (rotate_camera_right) glidx=%d self=%08x\n",
                glidx, self);
        fflush(stderr);
    }
    bainput_should_rotate_camera_right(rdram, ctx);
    if (win) {
        fprintf(stderr, "[ud4-cam-chain] leave 80097B94 (rotate_camera_right) glidx=%d v0=%08x\n",
                glidx, static_cast<uint32_t>(ctx->r2));
        fflush(stderr);
    }
}

static void tooie_wrap_func_800A4338(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win = tooie_ud4_writer_window(rdram, &glidx);
    if (win) {
        fprintf(stderr, "[ud4-cam-chain] enter 800A4338 glidx=%d a0=%08x a1=%08x a2=%08x\n",
                glidx, static_cast<uint32_t>(ctx->r4), static_cast<uint32_t>(ctx->r5), static_cast<uint32_t>(ctx->r6));
        fflush(stderr);
    }
    func_800A4338(rdram, ctx);
}

static void tooie_wrap_func_80110EFC(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win = tooie_ud4_writer_window(rdram, &glidx);
    if (win) {
        fprintf(stderr, "[ud4-cam-chain] enter 80110EFC glidx=%d node=%08x a1=%08x\n",
                glidx, static_cast<uint32_t>(ctx->r4), static_cast<uint32_t>(ctx->r5));
        fflush(stderr);
    }
    func_80110EFC(rdram, ctx);
}

static void tooie_wrap_func_80110E00(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win = tooie_ud4_writer_window(rdram, &glidx);
    if (win) {
        fprintf(stderr, "[ud4-cam-chain] enter 80110E00 glidx=%d node=%08x a1=%08x\n",
                glidx, static_cast<uint32_t>(ctx->r4), static_cast<uint32_t>(ctx->r5));
        fflush(stderr);
    }
    func_80110E00(rdram, ctx);
}

static void tooie_wrap_func_801111D0(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win = tooie_ud4_writer_window(rdram, &glidx);
    if (win) {
        float f12 = ctx->f12.fl;
        fprintf(stderr, "[ud4-cam-chain] enter 801111D0 glidx=%d node=%08x a1=%.4f (bits=%08x)\n",
                glidx, static_cast<uint32_t>(ctx->r4), f12, ctx->f12.u32l);
        fflush(stderr);
    }
    func_801111D0(rdram, ctx);
}

static void tooie_wrap_func_80112D90(uint8_t* rdram, recomp_context* ctx) {
    int glidx = -1;
    const bool win = tooie_ud4_writer_window(rdram, &glidx);
    const uint32_t node = static_cast<uint32_t>(ctx->r4);
    const uint32_t a3 = static_cast<uint32_t>(ctx->r7);
    if (win) {
        fprintf(stderr,
                "[ud4-p60-writer-2d90] enter seq=%u glidx=%d pc=80112D90 ra=%08x node=%08x a1=%.4f a2=%.4f a3=%u",
                tooie_ud4_seq(), glidx, static_cast<uint32_t>(ctx->r31), node,
                ctx->f12.fl, ctx->f14.fl, a3);
        tooie_ud4_dump_p60_object(rdram, stderr, node);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
    func_80112D90(rdram, ctx);
    if (win) {
        fprintf(stderr,
                "[ud4-p60-writer-2d90] leave seq=%u glidx=%d pc=80112D90 node=%08x",
                tooie_ud4_seq(), glidx, node);
        tooie_ud4_dump_p60_object(rdram, stderr, node);
        fprintf(stderr, "\n");
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

#define TOOIE_UD4_WRAP(sym, addr, tag)                                                    \
    static void tooie_wrap_##sym(uint8_t* rdram, recomp_context* ctx) {                    \
        tooie_ud4_np_call(tag, addr, rdram, ctx, 1);                                      \
        sym(rdram, ctx);                                                                  \
        tooie_ud4_np_call(tag, addr, rdram, ctx, 0);                                      \
    }

TOOIE_UD4_WRAP(func_8011607C, 0x8011607Cu, "8011607C")
TOOIE_UD4_WRAP(func_80113978, 0x80113978u, "80113978")

extern "C" void func_80111FD4(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_801107F0(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_8010FFD0(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_80111F70(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_80111DB0(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_80111E88(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_800EF174(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_8011106C(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_800EF04C(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_800EEF94(uint8_t* rdram, recomp_context* ctx);

static void tooie_wrap_func_80111FD4(uint8_t* rdram, recomp_context* ctx) {
    int ud5_glidx = -1;
    const int ud5 = tooie_ud5_cam1c_window(rdram, &ud5_glidx);
    const uint32_t s0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t arg = static_cast<uint32_t>(ctx->r5);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    unsigned ord = 0;
    uint32_t e0 = 0, e1 = 0, e2 = 0, t0 = 0, t1 = 0, t2 = 0, p34 = 0, s78 = 0;
    if (ud5) {
        s_ud5_eye_s0 = s0;
        ord = ++s_ud5_fd4_ord;
        tooie_ud5_read3(rdram, s0 + 0x74u, &e0, &e1, &e2);
        tooie_ud5_read3(rdram, s0 + 0x8Cu, &t0, &t1, &t2);
        if (tooie_is_guest(s0)) {
            p34 = static_cast<uint32_t>(MEM_W(0x34, kseg0_gpr(s0)));
            if (tooie_is_guest(p34)) {
                s78 = static_cast<uint32_t>(MEM_W(0x78, kseg0_gpr(p34)));
            }
        }
        uint32_t a0 = 0, a1b = 0, a2b = 0;
        tooie_ud5_read3(rdram, arg, &a0, &a1b, &a2b);
        std::fprintf(stderr,
                     "[ud5-eye] phase=P0 enter glidx=%d ord=%u pc=80111fd4 ra=%08x s0=%08x arg=%08x "
                     "p34=%08x p34_78=%08x eye74=(%08x,%08x,%08x / %.4f,%.4f,%.4f) "
                     "s08c=(%08x,%08x,%08x / %.4f,%.4f,%.4f) "
                     "argv=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                     ud5_glidx, ord, ra, s0, arg, p34, s78,
                     e0, e1, e2, tooie_bits_f32(e0), tooie_bits_f32(e1), tooie_bits_f32(e2),
                     t0, t1, t2, tooie_bits_f32(t0), tooie_bits_f32(t1), tooie_bits_f32(t2),
                     a0, a1b, a2b, tooie_bits_f32(a0), tooie_bits_f32(a1b), tooie_bits_f32(a2b));
        std::fflush(stderr);
    }
    tooie_ud4_np_call("80111FD4", 0x80111FD4u, rdram, ctx, 1);
    func_80111FD4(rdram, ctx);
    tooie_ud4_np_call("80111FD4", 0x80111FD4u, rdram, ctx, 0);
    if (ud5) {
        tooie_ud5_read3(rdram, s0 + 0x74u, &e0, &e1, &e2);
        tooie_ud5_read3(rdram, s0 + 0x8Cu, &t0, &t1, &t2);
        std::fprintf(stderr,
                     "[ud5-eye] phase=P0 leave glidx=%d ord=%u v0=%08x "
                     "eye74=(%08x,%08x,%08x / %.4f,%.4f,%.4f) "
                     "s08c=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                     ud5_glidx, ord, static_cast<uint32_t>(ctx->r2),
                     e0, e1, e2, tooie_bits_f32(e0), tooie_bits_f32(e1), tooie_bits_f32(e2),
                     t0, t1, t2, tooie_bits_f32(t0), tooie_bits_f32(t1), tooie_bits_f32(t2));
        std::fflush(stderr);
    }
}

static void tooie_wrap_func_801107F0(uint8_t* rdram, recomp_context* ctx) {
    int ud5_glidx = -1;
    const int ud5 = tooie_ud5_cam1c_window(rdram, &ud5_glidx);
    const uint32_t s0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t dst = static_cast<uint32_t>(ctx->r5);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    tooie_ud4_np_call("801107F0", 0x801107F0u, rdram, ctx, 1);
    func_801107F0(rdram, ctx);
    tooie_ud4_np_call("801107F0", 0x801107F0u, rdram, ctx, 0);
    if (ud5) {
        uint32_t x = 0, y = 0, z = 0;
        tooie_ud5_read3(rdram, dst, &x, &y, &z);
        std::fprintf(stderr,
                     "[ud5-eye] phase=P1 after_107F0 glidx=%d pc=801107f0 ra=%08x s0=%08x dst=%08x "
                     "work=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                     ud5_glidx, ra, s0, dst,
                     x, y, z, tooie_bits_f32(x), tooie_bits_f32(y), tooie_bits_f32(z));
        std::fflush(stderr);
    }
}

static void tooie_wrap_func_8010FFD0(uint8_t* rdram, recomp_context* ctx) {
    int ud5_glidx = -1;
    const int ud5 = tooie_ud5_cam1c_window(rdram, &ud5_glidx);
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    func_8010FFD0(rdram, ctx);
    if (ud5 && ra == 0x80112004u) {
        std::fprintf(stderr,
                     "[ud5-eye] phase=BR_0FFD0 glidx=%d pc=8010ffd0 ra=%08x a0=%08x v0=%08x skip_trans_path=%d\n",
                     ud5_glidx, ra, a0, static_cast<uint32_t>(ctx->r2),
                     (ctx->r2 != 0) ? 1 : 0);
        std::fflush(stderr);
    }
}

static void tooie_wrap_func_80111F70(uint8_t* rdram, recomp_context* ctx) {
    int ud5_glidx = -1;
    const int ud5 = tooie_ud5_cam1c_window(rdram, &ud5_glidx);
    const uint32_t s0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t arg = static_cast<uint32_t>(ctx->r5);
    const uint32_t work = static_cast<uint32_t>(ctx->r6);
    const uint32_t a3 = static_cast<uint32_t>(ctx->r7);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    uint32_t w0 = 0, w1 = 0, w2 = 0, sc = 0;
    if (ud5) {
        tooie_ud5_read3(rdram, work, &w0, &w1, &w2);
        if (tooie_is_guest(a3)) {
            sc = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(a3)));
        }
        uint32_t a0 = 0, a1b = 0, a2b = 0;
        tooie_ud5_read3(rdram, arg, &a0, &a1b, &a2b);
        std::fprintf(stderr,
                     "[ud5-eye] phase=P2 before_11F70 glidx=%d pc=80111f70 ra=%08x s0=%08x arg=%08x "
                     "work=%08x a3=%08x a3_0=%08x workv=(%08x,%08x,%08x / %.4f,%.4f,%.4f) "
                     "argv=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                     ud5_glidx, ra, s0, arg, work, a3, sc,
                     w0, w1, w2, tooie_bits_f32(w0), tooie_bits_f32(w1), tooie_bits_f32(w2),
                     a0, a1b, a2b, tooie_bits_f32(a0), tooie_bits_f32(a1b), tooie_bits_f32(a2b));
        std::fflush(stderr);
    }
    func_80111F70(rdram, ctx);
    if (ud5) {
        tooie_ud5_read3(rdram, work, &w0, &w1, &w2);
        if (tooie_is_guest(a3)) {
            sc = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(a3)));
        }
        std::fprintf(stderr,
                     "[ud5-eye] phase=P3 after_11F70 glidx=%d pc=80111f70 a3_0=%08x "
                     "workv=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                     ud5_glidx, sc,
                     w0, w1, w2, tooie_bits_f32(w0), tooie_bits_f32(w1), tooie_bits_f32(w2));
        std::fflush(stderr);
    }
}

static void tooie_wrap_func_80111DB0(uint8_t* rdram, recomp_context* ctx) {
    int ud5_glidx = -1;
    const int ud5 = tooie_ud5_cam1c_window(rdram, &ud5_glidx);
    const uint32_t s0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t arg = static_cast<uint32_t>(ctx->r5);
    const uint32_t work = static_cast<uint32_t>(ctx->r6);
    const uint32_t a3 = static_cast<uint32_t>(ctx->r7);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    uint32_t w0 = 0, w1 = 0, w2 = 0, sc = 0, thr = 0;
    if (ud5) {
        tooie_ud5_read3(rdram, work, &w0, &w1, &w2);
        if (tooie_is_guest(a3)) {
            sc = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(a3)));
        }
        thr = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(0x8012655Cu)));
        uint32_t a0 = 0, a1b = 0, a2b = 0;
        tooie_ud5_read3(rdram, arg, &a0, &a1b, &a2b);
        std::fprintf(stderr,
                     "[ud5-eye] phase=P4 before_11DB0 glidx=%d pc=80111db0 ra=%08x s0=%08x arg=%08x "
                     "work=%08x a3=%08x a3_0=%08x D_8012655C=%08x "
                     "workv=(%08x,%08x,%08x / %.4f,%.4f,%.4f) "
                     "argv=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                     ud5_glidx, ra, s0, arg, work, a3, sc, thr,
                     w0, w1, w2, tooie_bits_f32(w0), tooie_bits_f32(w1), tooie_bits_f32(w2),
                     a0, a1b, a2b, tooie_bits_f32(a0), tooie_bits_f32(a1b), tooie_bits_f32(a2b));
        std::fflush(stderr);
    }
    func_80111DB0(rdram, ctx);
    if (ud5) {
        tooie_ud5_read3(rdram, work, &w0, &w1, &w2);
        if (tooie_is_guest(a3)) {
            sc = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(a3)));
        }
        std::fprintf(stderr,
                     "[ud5-eye] phase=P5 after_11DB0 glidx=%d a3_0=%08x "
                     "workv=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                     ud5_glidx, sc,
                     w0, w1, w2, tooie_bits_f32(w0), tooie_bits_f32(w1), tooie_bits_f32(w2));
        std::fflush(stderr);
    }
}

static void tooie_wrap_func_800EEF94(uint8_t* rdram, recomp_context* ctx) {
    int ud5_glidx = -1;
    const int ud5 = tooie_ud5_cam1c_window(rdram, &ud5_glidx);
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    uint32_t v0 = 0, v1 = 0, v2 = 0;
    if (ud5 && ra == 0x80111DD4u) {
        tooie_ud5_read3(rdram, a0, &v0, &v1, &v2);
    }
    tooie_ud4_np_call("800EEF94", 0x800EEF94u, rdram, ctx, 1);
    func_800EEF94(rdram, ctx);
    tooie_ud4_np_call("800EEF94", 0x800EEF94u, rdram, ctx, 0);
    if (ud5 && ra == 0x80111DD4u) {
        const uint32_t f0 = ctx->f0.u32l;
        const uint32_t thr = static_cast<uint32_t>(MEM_W(0, kseg0_gpr(0x8012655Cu)));
        const int take_ef174 = tooie_bits_f32(thr) < tooie_bits_f32(f0) ? 1 : 0;
        std::fprintf(stderr,
                     "[ud5-eye] phase=BR_EEF94 glidx=%d pc=800eef94 ra=%08x a0=%08x "
                     "vec=(%08x,%08x,%08x / %.4f,%.4f,%.4f) f0=%08x/%.4f D_8012655C=%08x/%.4f "
                     "c_lt_thr_f0=%d\n",
                     ud5_glidx, ra, a0, v0, v1, v2,
                     tooie_bits_f32(v0), tooie_bits_f32(v1), tooie_bits_f32(v2),
                     f0, tooie_bits_f32(f0), thr, tooie_bits_f32(thr), take_ef174);
        std::fflush(stderr);
    }
}

static void tooie_wrap_func_800EF174(uint8_t* rdram, recomp_context* ctx) {
    int ud5_glidx = -1;
    const int ud5 = tooie_ud5_cam1c_window(rdram, &ud5_glidx);
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t a1 = static_cast<uint32_t>(ctx->r5);
    const uint32_t a2 = static_cast<uint32_t>(ctx->r6);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    uint32_t e0 = 0, e1 = 0, e2 = 0, d0 = 0, d1 = 0, d2 = 0;
    if (ud5) {
        tooie_ud5_read3(rdram, a0, &e0, &e1, &e2);
        tooie_ud5_read3(rdram, a1, &d0, &d1, &d2);
        std::fprintf(stderr,
                     "[ud5-eye] phase=P6 before_EF174 glidx=%d pc=800ef174 ra=%08x a0=%08x a1=%08x "
                     "scale=%08x/%.4f eye=(%08x,%08x,%08x / %.4f,%.4f,%.4f) "
                     "dir=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                     ud5_glidx, ra, a0, a1, a2, tooie_bits_f32(a2),
                     e0, e1, e2, tooie_bits_f32(e0), tooie_bits_f32(e1), tooie_bits_f32(e2),
                     d0, d1, d2, tooie_bits_f32(d0), tooie_bits_f32(d1), tooie_bits_f32(d2));
        std::fflush(stderr);
    }
    func_800EF174(rdram, ctx);
    if (ud5) {
        tooie_ud5_read3(rdram, a0, &e0, &e1, &e2);
        std::fprintf(stderr,
                     "[ud5-eye] phase=P7 after_EF174 glidx=%d eye=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                     ud5_glidx, e0, e1, e2, tooie_bits_f32(e0), tooie_bits_f32(e1), tooie_bits_f32(e2));
        std::fflush(stderr);
    }
}

static void tooie_wrap_func_80111E88(uint8_t* rdram, recomp_context* ctx) {
    int ud5_glidx = -1;
    const int ud5 = tooie_ud5_cam1c_window(rdram, &ud5_glidx);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    if (ud5 && ra == 0x80111FC4u) {
        std::fprintf(stderr,
                     "[ud5-eye] phase=P3b before_11E88 glidx=%d pc=80111e88 ra=%08x "
                     "a0=%08x a1=%08x a2=%08x a3=%08x f12=%08x\n",
                     ud5_glidx, ra,
                     static_cast<uint32_t>(ctx->r4), static_cast<uint32_t>(ctx->r5),
                     static_cast<uint32_t>(ctx->r6), static_cast<uint32_t>(ctx->r7),
                     ctx->f12.u32l);
        std::fflush(stderr);
    }
    func_80111E88(rdram, ctx);
}

static void tooie_wrap_func_8011106C(uint8_t* rdram, recomp_context* ctx) {
    int ud5_glidx = -1;
    const int ud5 = tooie_ud5_cam1c_window(rdram, &ud5_glidx);
    const uint32_t s0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t dst = static_cast<uint32_t>(ctx->r5);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    uint32_t t0 = 0, t1 = 0, t2 = 0;
    if (ud5) {
        tooie_ud5_read3(rdram, s0 + 0x8Cu, &t0, &t1, &t2);
        std::fprintf(stderr,
                     "[ud5-eye] phase=P8 before_1106C glidx=%d pc=8011106c ra=%08x s0=%08x dst=%08x "
                     "s08c=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                     ud5_glidx, ra, s0, dst,
                     t0, t1, t2, tooie_bits_f32(t0), tooie_bits_f32(t1), tooie_bits_f32(t2));
        std::fflush(stderr);
    }
    func_8011106C(rdram, ctx);
    if (ud5) {
        uint32_t d0 = 0, d1 = 0, d2 = 0;
        tooie_ud5_read3(rdram, dst, &d0, &d1, &d2);
        std::fprintf(stderr,
                     "[ud5-eye] phase=P9 after_1106C glidx=%d dst=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                     ud5_glidx, d0, d1, d2, tooie_bits_f32(d0), tooie_bits_f32(d1), tooie_bits_f32(d2));
        std::fflush(stderr);
    }
}

static void tooie_wrap_func_800EF04C(uint8_t* rdram, recomp_context* ctx) {
    int ud5_glidx = -1;
    const int ud5 = tooie_ud5_cam1c_window(rdram, &ud5_glidx);
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t a1 = static_cast<uint32_t>(ctx->r5);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    uint32_t e0 = 0, e1 = 0, e2 = 0, t0 = 0, t1 = 0, t2 = 0;
    if (ud5) {
        tooie_ud5_read3(rdram, a0, &e0, &e1, &e2);
        tooie_ud5_read3(rdram, a1, &t0, &t1, &t2);
        std::fprintf(stderr,
                     "[ud5-eye] phase=P10 before_EF04C glidx=%d pc=800ef04c ra=%08x "
                     "work=(%08x,%08x,%08x / %.4f,%.4f,%.4f) "
                     "add=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                     ud5_glidx, ra,
                     e0, e1, e2, tooie_bits_f32(e0), tooie_bits_f32(e1), tooie_bits_f32(e2),
                     t0, t1, t2, tooie_bits_f32(t0), tooie_bits_f32(t1), tooie_bits_f32(t2));
        std::fflush(stderr);
    }
    func_800EF04C(rdram, ctx);
    if (ud5) {
        tooie_ud5_read3(rdram, a0, &e0, &e1, &e2);
        std::fprintf(stderr,
                     "[ud5-eye] phase=P11 after_EF04C glidx=%d work=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                     ud5_glidx, e0, e1, e2, tooie_bits_f32(e0), tooie_bits_f32(e1), tooie_bits_f32(e2));
        std::fflush(stderr);
    }
}

static void tooie_wrap_func_80112524(uint8_t* rdram, recomp_context* ctx) {
    int ud5_glidx = -1;
    const int ud5 = tooie_ud5_cam1c_window(rdram, &ud5_glidx);
    const uint32_t s0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t dst = static_cast<uint32_t>(ctx->r5);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    static thread_local unsigned s_ud5_sp6c_prod_ord = 0;
    if (ud5) {
        uint32_t p30 = 0, p30_4 = 0, p30_8 = 0, p30_c = 0;
        if (tooie_is_guest(s0)) {
            p30 = static_cast<uint32_t>(MEM_W(0x30, kseg0_gpr(s0)));
            if (tooie_is_guest(p30)) {
                p30_4 = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(p30)));
                p30_8 = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(p30)));
                p30_c = static_cast<uint32_t>(MEM_W(0xC, kseg0_gpr(p30)));
            }
        }
        const unsigned uord = ++s_ud5_sp6c_prod_ord;
        fprintf(stderr,
                "[ud5-sp6c-prod] phase=sp6c_prod enter glidx=%d ord=%u pc=80112524 ra=%08x "
                "s0=%08x dst=%08x p30=%08x src4=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                ud5_glidx, uord, ra, s0, dst, p30,
                p30_4, p30_8, p30_c,
                tooie_bits_f32(p30_4), tooie_bits_f32(p30_8), tooie_bits_f32(p30_c));
        fflush(stderr);
    }
    tooie_ud4_np_call("80112524", 0x80112524u, rdram, ctx, 1);
    func_80112524(rdram, ctx);
    tooie_ud4_np_call("80112524", 0x80112524u, rdram, ctx, 0);
    if (ud5) {
        uint32_t d0 = 0, d1 = 0, d2 = 0;
        if (tooie_is_guest(dst)) {
            d0 = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(dst)));
            d1 = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(dst)));
            d2 = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(dst)));
        }
        fprintf(stderr,
                "[ud5-sp6c-prod] phase=sp6c_prod leave glidx=%d ord=%u pc=80112524 "
                "dst=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                ud5_glidx, s_ud5_sp6c_prod_ord,
                d0, d1, d2,
                tooie_bits_f32(d0), tooie_bits_f32(d1), tooie_bits_f32(d2));
        fflush(stderr);
    }
}

extern "C" void func_800EFB24(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_800EEBF0(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_800EE780(uint8_t* rdram, recomp_context* ctx);

static void tooie_wrap_func_800EFB24(uint8_t* rdram, recomp_context* ctx) {
    int ud5_glidx = -1;
    const int ud5 = tooie_ud5_cam1c_window(rdram, &ud5_glidx);
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t a1 = static_cast<uint32_t>(ctx->r5);
    const uint32_t a2 = static_cast<uint32_t>(ctx->r6);
    const uint32_t a3 = static_cast<uint32_t>(ctx->r7);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    static thread_local unsigned s_ud5_efb24_ord = 0;
    uint32_t a1x = 0, a1y = 0, a1z = 0, a2x = 0, a2y = 0, a2z = 0;
    unsigned uord = 0;
    if (ud5 && ud5_glidx >= 22 && ud5_glidx <= 25) {
        uord = ++s_ud5_efb24_ord;
        tooie_ud5_read3(rdram, a1, &a1x, &a1y, &a1z);
        tooie_ud5_read3(rdram, a2, &a2x, &a2y, &a2z);
        fprintf(stderr,
                "[ud5-efb24] enter glidx=%d ord=%u pc=800efb24 ra=%08x "
                "a0=%08x a1=%08x a2=%08x a3=%08x "
                "a1v=(%08x,%08x,%08x / %.4f,%.4f,%.4f) "
                "a2v=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                ud5_glidx, uord, ra, a0, a1, a2, a3,
                a1x, a1y, a1z, tooie_bits_f32(a1x), tooie_bits_f32(a1y), tooie_bits_f32(a1z),
                a2x, a2y, a2z, tooie_bits_f32(a2x), tooie_bits_f32(a2y), tooie_bits_f32(a2z));
        fflush(stderr);
    }
    tooie_ud4_np_call("800EFB24", 0x800EFB24u, rdram, ctx, 1);
    func_800EFB24(rdram, ctx);
    tooie_ud4_np_call("800EFB24", 0x800EFB24u, rdram, ctx, 0);
    if (ud5 && ud5_glidx >= 22 && ud5_glidx <= 25) {
        uint32_t dx = 0, dy = 0, dz = 0;
        tooie_ud5_read3(rdram, a0, &dx, &dy, &dz);
        fprintf(stderr,
                "[ud5-efb24] leave glidx=%d ord=%u ra=%08x dest=%08x "
                "delta=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                ud5_glidx, uord, ra, a0,
                dx, dy, dz, tooie_bits_f32(dx), tooie_bits_f32(dy), tooie_bits_f32(dz));
        fflush(stderr);
    }
}

static void tooie_wrap_func_800EEBF0(uint8_t* rdram, recomp_context* ctx) {
    int ud5_glidx = -1;
    const int ud5 = tooie_ud5_cam1c_window(rdram, &ud5_glidx);
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t a1 = static_cast<uint32_t>(ctx->r5);
    const uint32_t a2 = static_cast<uint32_t>(ctx->r6);
    const uint32_t a3 = static_cast<uint32_t>(ctx->r7);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    static thread_local unsigned s_ud5_eebf0_ord = 0;
    unsigned uord = 0;
    if (ud5 && ud5_glidx >= 22 && ud5_glidx <= 25) {
        uint32_t ox = 0, oy = 0, oz = 0, sx = 0, sy = 0, sz = 0;
        uord = ++s_ud5_eebf0_ord;
        tooie_ud5_read3(rdram, a0, &ox, &oy, &oz);
        tooie_ud5_read3(rdram, a1, &sx, &sy, &sz);
        fprintf(stderr,
                "[ud5-eebf0] enter glidx=%d ord=%u pc=800eebf0 ra=%08x "
                "a0=%08x a1=%08x a2=%08x/%.4f a3=%08x "
                "old=(%08x,%08x,%08x / %.4f,%.4f,%.4f) "
                "src_dir=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                ud5_glidx, uord, ra, a0, a1, a2, tooie_bits_f32(a2), a3,
                ox, oy, oz, tooie_bits_f32(ox), tooie_bits_f32(oy), tooie_bits_f32(oz),
                sx, sy, sz, tooie_bits_f32(sx), tooie_bits_f32(sy), tooie_bits_f32(sz));
        fflush(stderr);
    }
    tooie_ud4_np_call("800EEBF0", 0x800EEBF0u, rdram, ctx, 1);
    func_800EEBF0(rdram, ctx);
    tooie_ud4_np_call("800EEBF0", 0x800EEBF0u, rdram, ctx, 0);
    if (ud5 && ud5_glidx >= 22 && ud5_glidx <= 25) {
        uint32_t nx = 0, ny = 0, nz = 0;
        tooie_ud5_read3(rdram, a0, &nx, &ny, &nz);
        fprintf(stderr,
                "[ud5-eebf0] leave glidx=%d ord=%u dest=%08x "
                "out=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                ud5_glidx, uord, a0,
                nx, ny, nz, tooie_bits_f32(nx), tooie_bits_f32(ny), tooie_bits_f32(nz));
        fflush(stderr);
    }
}

static void tooie_wrap_func_800EE780(uint8_t* rdram, recomp_context* ctx) {
    int ud5_glidx = -1;
    const int ud5 = tooie_ud5_cam1c_window(rdram, &ud5_glidx);
    const uint32_t a0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t a1 = static_cast<uint32_t>(ctx->r5);
    const uint32_t a2 = static_cast<uint32_t>(ctx->r6);
    const uint32_t a3 = static_cast<uint32_t>(ctx->r7);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    static thread_local unsigned s_ud5_ee780_ord = 0;
    unsigned uord = 0;
    if (ud5) {
        uint32_t oldx = 0, oldy = 0, oldz = 0, s1x = 0, s1y = 0, s1z = 0, s2x = 0, s2y = 0, s2z = 0;
        uord = ++s_ud5_ee780_ord;
        tooie_ud5_read3(rdram, a0, &oldx, &oldy, &oldz);
        tooie_ud5_read3(rdram, a1, &s1x, &s1y, &s1z);
        tooie_ud5_read3(rdram, a2, &s2x, &s2y, &s2z);
        fprintf(stderr,
                "[ud5-ee780] enter glidx=%d ord=%u pc=800ee780 ra=%08x "
                "a0=%08x a1=%08x a2=%08x a3=%08x "
                "old=(%08x,%08x,%08x / %.4f,%.4f,%.4f) "
                "src_a1=(%08x,%08x,%08x / %.4f,%.4f,%.4f) "
                "src_a2=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                ud5_glidx, uord, ra, a0, a1, a2, a3,
                oldx, oldy, oldz, tooie_bits_f32(oldx), tooie_bits_f32(oldy), tooie_bits_f32(oldz),
                s1x, s1y, s1z, tooie_bits_f32(s1x), tooie_bits_f32(s1y), tooie_bits_f32(s1z),
                s2x, s2y, s2z, tooie_bits_f32(s2x), tooie_bits_f32(s2y), tooie_bits_f32(s2z));
        fflush(stderr);
    }
    tooie_ud4_np_call("800EE780", 0x800EE780u, rdram, ctx, 1);
    func_800EE780(rdram, ctx);
    tooie_ud4_np_call("800EE780", 0x800EE780u, rdram, ctx, 0);
    if (ud5) {
        uint32_t nx = 0, ny = 0, nz = 0;
        tooie_ud5_read3(rdram, a0, &nx, &ny, &nz);
        fprintf(stderr,
                "[ud5-ee780] leave glidx=%d ord=%u dest=%08x "
                "out=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                ud5_glidx, uord, a0,
                nx, ny, nz, tooie_bits_f32(nx), tooie_bits_f32(ny), tooie_bits_f32(nz));
        fflush(stderr);
    }
}

TOOIE_UD4_WRAP(func_800F1A88, 0x800F1A88u, "800F1A88")
TOOIE_UD4_WRAP(func_80115358, 0x80115358u, "80115358")
TOOIE_UD4_WRAP(func_80115750, 0x80115750u, "80115750")
TOOIE_UD4_WRAP(func_80112C3C, 0x80112C3Cu, "80112C3C")
TOOIE_UD4_WRAP(func_800F1738, 0x800F1738u, "800F1738")
TOOIE_UD4_WRAP(func_800F1DCC, 0x800F1DCCu, "800F1DCC")
TOOIE_UD4_WRAP(func_801125E8, 0x801125E8u, "801125E8")
TOOIE_UD4_WRAP(func_800EFD24, 0x800EFD24u, "800EFD24")
TOOIE_UD4_WRAP(func_800EF214, 0x800EF214u, "800EF214")
TOOIE_UD4_WRAP(func_80013B70, 0x80013B70u, "80013B70")
TOOIE_UD4_WRAP(func_80013B7C, 0x80013B7Cu, "80013B7C")
TOOIE_UD4_WRAP(func_800137AC, 0x800137ACu, "800137AC")
TOOIE_UD4_WRAP(func_800137C4, 0x800137C4u, "800137C4")
TOOIE_UD4_WRAP(func_800137F4, 0x800137F4u, "800137F4")
TOOIE_UD4_WRAP(func_80013788, 0x80013788u, "80013788")
TOOIE_UD4_WRAP(func_80013818, 0x80013818u, "80013818")
TOOIE_UD4_WRAP(func_801138A0, 0x801138A0u, "801138A0")
TOOIE_UD4_WRAP(func_801138F4, 0x801138F4u, "801138F4")
TOOIE_UD4_WRAP(func_80110F44, 0x80110F44u, "80110F44")
TOOIE_UD4_WRAP(func_80115580, 0x80115580u, "80115580")

static void tooie_wrap_func_800F18FC(uint8_t* rdram, recomp_context* ctx) {
    int ud5_glidx = -1;
    const int ud5 = tooie_ud5_cam1c_window(rdram, &ud5_glidx);
    const uint32_t from = static_cast<uint32_t>(ctx->r4);
    const uint32_t to = static_cast<uint32_t>(ctx->r5);
    const uint32_t out = static_cast<uint32_t>(ctx->r6);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    static thread_local unsigned s_ud5_f18fc_ord = 0;
    if (ud5) {
        uint32_t f0 = 0, f1 = 0, f2 = 0, t0 = 0, t1 = 0, t2 = 0;
        if (tooie_is_guest(from)) {
            f0 = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(from)));
            f1 = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(from)));
            f2 = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(from)));
        }
        if (tooie_is_guest(to)) {
            t0 = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(to)));
            t1 = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(to)));
            t2 = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(to)));
        }
        const unsigned uord = ++s_ud5_f18fc_ord;
        fprintf(stderr,
                "[ud5-f18fc] phase=f18fc enter glidx=%d ord=%u pc=800f18fc ra=%08x "
                "from=%08x to=%08x out=%08x from_v=(%08x,%08x,%08x / %.4f,%.4f,%.4f) to_v=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                ud5_glidx, uord, ra, from, to, out,
                f0, f1, f2, tooie_bits_f32(f0), tooie_bits_f32(f1), tooie_bits_f32(f2),
                t0, t1, t2, tooie_bits_f32(t0), tooie_bits_f32(t1), tooie_bits_f32(t2));
        fflush(stderr);
    }
    func_800F18FC(rdram, ctx);
    if (ud5) {
        uint32_t o0 = 0, o1 = 0, o2 = 0;
        if (tooie_is_guest(out)) {
            o0 = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(out)));
            o1 = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(out)));
            o2 = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(out)));
        }
        fprintf(stderr,
                "[ud5-f18fc] phase=f18fc leave glidx=%d ord=%u pc=800f18fc "
                "out=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                ud5_glidx, s_ud5_f18fc_ord,
                o0, o1, o2, tooie_bits_f32(o0), tooie_bits_f32(o1), tooie_bits_f32(o2));
        fflush(stderr);
    }
}

static void tooie_wrap_func_800A516C(uint8_t* rdram, recomp_context* ctx) {
    int ud5_glidx = -1;
    const int ud5 = tooie_ud5_cam1c_window(rdram, &ud5_glidx);
    const uint32_t dst = static_cast<uint32_t>(ctx->r4);
    const uint32_t target = static_cast<uint32_t>(ctx->r5);
    const uint32_t eye = static_cast<uint32_t>(ctx->r6);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    static thread_local unsigned s_ud5_a516c_ord = 0;
    if (ud5) {
        uint32_t t0 = 0, t1 = 0, t2 = 0, e0 = 0, e1 = 0, e2 = 0;
        if (tooie_is_guest(target)) {
            t0 = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(target)));
            t1 = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(target)));
            t2 = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(target)));
        }
        if (tooie_is_guest(eye)) {
            e0 = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(eye)));
            e1 = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(eye)));
            e2 = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(eye)));
        }
        const unsigned uord = ++s_ud5_a516c_ord;
        fprintf(stderr,
                "[ud5-a516c] phase=a516c enter glidx=%d ord=%u pc=800a516c ra=%08x "
                "dst=%08x target=%08x eye=%08x target_v=(%08x,%08x,%08x / %.4f,%.4f,%.4f) eye_v=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                ud5_glidx, uord, ra, dst, target, eye,
                t0, t1, t2, tooie_bits_f32(t0), tooie_bits_f32(t1), tooie_bits_f32(t2),
                e0, e1, e2, tooie_bits_f32(e0), tooie_bits_f32(e1), tooie_bits_f32(e2));
        fflush(stderr);
    }
    tooie_ud4_np_call("800A516C", 0x800A516Cu, rdram, ctx, 1);
    func_800A516C(rdram, ctx);
    tooie_ud4_np_call("800A516C", 0x800A516Cu, rdram, ctx, 0);
    if (ud5) {
        uint32_t o0 = 0, o1 = 0, o2 = 0;
        if (tooie_is_guest(dst)) {
            o0 = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(dst)));
            o1 = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(dst)));
            o2 = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(dst)));
        }
        fprintf(stderr,
                "[ud5-a516c] phase=a516c leave glidx=%d ord=%u pc=800a516c "
                "out=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                ud5_glidx, s_ud5_a516c_ord,
                o0, o1, o2, tooie_bits_f32(o0), tooie_bits_f32(o1), tooie_bits_f32(o2));
        fflush(stderr);
    }
}

static void tooie_wrap_func_80110A68(uint8_t* rdram, recomp_context* ctx) {
    int ud5_glidx = -1;
    const int ud5 = tooie_ud5_cam1c_window(rdram, &ud5_glidx);
    const uint32_t s0 = static_cast<uint32_t>(ctx->r4);
    const uint32_t dst_sp60 = static_cast<uint32_t>(ctx->r5);
    const uint32_t in_sp6c = static_cast<uint32_t>(ctx->r6);
    const uint32_t ra = static_cast<uint32_t>(ctx->r31);
    static thread_local unsigned s_ud5_a68_ord = 0;
    if (ud5) {
        uint32_t sp6c_0 = 0, sp6c_1 = 0, sp6c_2 = 0;
        uint32_t s0_74 = 0, s0_78 = 0, s0_7c = 0;
        uint32_t p30 = 0;
        if (tooie_is_guest(in_sp6c)) {
            sp6c_0 = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(in_sp6c)));
            sp6c_1 = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(in_sp6c)));
            sp6c_2 = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(in_sp6c)));
        }
        if (tooie_is_guest(s0)) {
            s0_74 = static_cast<uint32_t>(MEM_W(0x74, kseg0_gpr(s0)));
            s0_78 = static_cast<uint32_t>(MEM_W(0x78, kseg0_gpr(s0)));
            s0_7c = static_cast<uint32_t>(MEM_W(0x7C, kseg0_gpr(s0)));
            p30 = static_cast<uint32_t>(MEM_W(0x30, kseg0_gpr(s0)));
        }
        const unsigned uord = ++s_ud5_a68_ord;
        fprintf(stderr,
                "[ud5-a68] phase=a68 enter glidx=%d ord=%u pc=80110a68 ra=%08x "
                "s0=%08x dst_sp60=%08x in_sp6c=%08x sp6c=(%08x,%08x,%08x / %.4f,%.4f,%.4f) s0_74=(%08x,%08x,%08x / %.4f,%.4f,%.4f) p30=%08x\n",
                ud5_glidx, uord, ra, s0, dst_sp60, in_sp6c,
                sp6c_0, sp6c_1, sp6c_2, tooie_bits_f32(sp6c_0), tooie_bits_f32(sp6c_1), tooie_bits_f32(sp6c_2),
                s0_74, s0_78, s0_7c, tooie_bits_f32(s0_74), tooie_bits_f32(s0_78), tooie_bits_f32(s0_7c),
                p30);
        fflush(stderr);
    }
    tooie_ud4_np_call("80110A68", 0x80110A68u, rdram, ctx, 1);
    func_80110A68(rdram, ctx);
    tooie_ud4_np_call("80110A68", 0x80110A68u, rdram, ctx, 0);
    if (ud5) {
        uint32_t out_80 = 0, out_84 = 0, out_88 = 0;
        if (tooie_is_guest(dst_sp60)) {
            out_80 = static_cast<uint32_t>(MEM_W(0x0, kseg0_gpr(dst_sp60)));
            out_84 = static_cast<uint32_t>(MEM_W(0x4, kseg0_gpr(dst_sp60)));
            out_88 = static_cast<uint32_t>(MEM_W(0x8, kseg0_gpr(dst_sp60)));
        }
        fprintf(stderr,
                "[ud5-a68] phase=a68 leave glidx=%d ord=%u pc=80110a68 "
                "out_sp60=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                ud5_glidx, s_ud5_a68_ord,
                out_80, out_84, out_88,
                tooie_bits_f32(out_80), tooie_bits_f32(out_84), tooie_bits_f32(out_88));
        fflush(stderr);
    }
}

TOOIE_UD4_WRAP(func_80111310, 0x80111310u, "80111310")

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
static bool tooie_ud5_scene_tick(uint8_t* rdram, int* stage_out, int* sub_out,
                                 int* map_out, int* mode_out) {
    const int mode_v = int(MEM_H(0x2, kseg0_gpr(0x80127630u)));
    const int map_v = int(MEM_H(0x10, kseg0_gpr(0x80127630u)));
    const int stage_v = int(MEM_BU(0x0, kseg0_gpr(0x80135470u)));
    const int sub_v = int(MEM_BU(0x1, kseg0_gpr(0x80135470u)));
    if (stage_out) {
        *stage_out = stage_v;
    }
    if (sub_out) {
        *sub_out = sub_v;
    }
    if (map_out) {
        *map_out = map_v;
    }
    if (mode_out) {
        *mode_out = mode_v;
    }
    if (!tooie_diag::ud5_loco_enabled()) {
        return true;
    }
    static int s_last_stage = -1;
    static int s_last_sub = -1;
    static int s_last_map = -1;
    static int s_last_mode = -1;
    if (stage_v != s_last_stage || sub_v != s_last_sub || map_v != s_last_map ||
        mode_v != s_last_mode) {
        std::fprintf(stderr,
                     "[ud5-scene] stage=%d sub=%d map=%d mode=%d\n",
                     stage_v, sub_v, map_v, mode_v);
        std::fflush(stderr);
        s_last_stage = stage_v;
        s_last_sub = sub_v;
        s_last_map = map_v;
        s_last_mode = mode_v;
    }
    return mode_v == 5 && stage_v == 0 && sub_v == 5;
}

extern "C" void tooie_demo_sync_tick(uint8_t* rdram, uint64_t tick) {
    if (!tooie_diag::demo_sync_enabled()) {
        return;
    }
    int stage_early = 0, sub_early = 0, map_early = 0, mode_early = 0;
    if (!tooie_ud5_scene_tick(rdram, &stage_early, &sub_early, &map_early,
                              &mode_early)) {
        return;
    }
    const int mode_v = mode_early;
    const int map_v = map_early;
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
    int32_t bs_prev = -1, bs_cur = -1, bs_next = -1;
    int32_t phys_type = -1;

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
        phys_type = static_cast<int32_t>(MEM_W(0x00, ph));
    }
    if (is_guest(ba_stick_addr)) {
        const gpr st = kseg0_gpr(ba_stick_addr);
        bstick_x = bits_f32(MEM_W(0x54, st));
        bstick_y = bits_f32(MEM_W(0x58, st));
        bstick_ang = bits_f32(MEM_W(0x5C, st));
        bstick_dist = bits_f32(MEM_W(0x60, st));
    }
    if (is_guest(bs_addr)) {
        const gpr bsp = kseg0_gpr(bs_addr);
        bs_prev = static_cast<int32_t>(MEM_W(0x0, bsp));
        bs_cur = static_cast<int32_t>(MEM_W(0x4, bsp));
        bs_next = static_cast<int32_t>(MEM_W(0x8, bsp));
        bs_id = bs_prev;
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
                 "bs=%d bs_prev=%d bs_cur=%d bs_next=%d ptype=%d "
                 "pos=(%.2f,%.2f,%.2f) ppos=(%.2f,%.2f,%.2f) "
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
                 bs_id, bs_prev, bs_cur, bs_next, phys_type, px, py, pz, ppx, ppy, ppz,
                 vx, vy, vz, velxz, tspd,
                 yaw, yaw_ideal, phys_yaw, mult, accel,
                 cx, cy, cz, cam_yaw, camctrl, cam_mode, cam_sel,
                 anim_id, anim_timer, anim_dur, int(anim_type), unsigned(anim_flags), anim_phase,
                 egg_slot, egg_enum, stage_v, sub_v,
                 dig_x, dig_y, dig_z, act2_mid, act2_sys, act2_x, act2_y, act2_z,
                 gamespeed, gfcnt, gl_count, map_v);
    std::fflush(stderr);

    if (tooie_diag::ud5_loco_enabled()) {
        static FILE* s_ud5_csv = nullptr;
        static bool s_ud5_csv_tried = false;
        if (!s_ud5_csv_tried) {
            s_ud5_csv_tried = true;
            const char* path = std::getenv("TOOIE_UD5_CSV");
            if (path == nullptr || path[0] == '\0') {
                path = "/tmp/p08-rt64-user_demo5.csv";
            }
            s_ud5_csv = std::fopen(path, "w");
            if (s_ud5_csv) {
                std::fprintf(s_ud5_csv,
                             "glidx,post,tick,stage,sub,map,pad_x,pad_y,pad_btn,"
                             "bs_prev,bs_cur,bs_next,ptype,px,py,pz,ppx,ppy,ppz,"
                             "vx,vy,vz,velxz,tspd,accel,mult,yaw,ideal,phys_yaw,"
                             "camctrl,cam_yaw,cammode,sel,anim_id,anim_timer,"
                             "anim_dur,anim_phase,anim_type,anim_flags,"
                             "bstick_x,bstick_y,bstick_ang,bstick_dist\n");
            }
        }
        if (s_ud5_csv) {
            std::fprintf(s_ud5_csv,
                         "%d,%d,%llu,%d,%d,%d,%d,%d,0x%04x,"
                         "%d,%d,%d,%d,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,"
                         "%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,"
                         "%.6f,%.6f,%d,%d,%d,%.6f,%.6f,%.6f,%d,0x%02x,"
                         "%.6f,%.6f,%.6f,%.6f\n",
                         consumed_glidx, post_glidx,
                         static_cast<unsigned long long>(tick), stage_v, sub_v, map_v,
                         (int)rec_x, (int)rec_y, (unsigned)rec_btn,
                         bs_prev, bs_cur, bs_next, phys_type, px, py, pz, ppx, ppy, ppz,
                         vx, vy, vz, velxz, tspd, accel, mult, yaw, yaw_ideal, phys_yaw,
                         camctrl, cam_yaw, cam_mode, cam_sel, (int)anim_id, anim_timer,
                         anim_dur, anim_phase, int(anim_type), unsigned(anim_flags),
                         bstick_x, bstick_y, bstick_ang, bstick_dist);
            std::fflush(s_ud5_csv);
        }
        static int32_t s_last_bs_cur = 0x7fffffff;
        if (bs_cur != s_last_bs_cur) {
            std::fprintf(stderr,
                         "[ud5-bs] glidx=%d prev=%d cur=%d next=%d ptype=%d "
                         "anim=%d tspd=%.3f accel=%.3f velxz=%.3f yaw=%.3f "
                         "ideal=%.3f pad=(%d,%d,%04x)\n",
                         consumed_glidx, bs_prev, bs_cur, bs_next, phys_type,
                         (int)anim_id, tspd, accel, velxz, yaw, yaw_ideal,
                         (int)rec_x, (int)rec_y, (unsigned)rec_btn);
            std::fflush(stderr);
            s_last_bs_cur = bs_cur;
        }
    }
}

namespace banjo {
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
    // Boundary wrappers for live timing baseline measurement (TOOIE_DIAG_TIMING=1).
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8001443C),
        [](uint8_t* rdram, recomp_context* ctx) {
            tooie_diag::note_guest_vi();
            func_8001443C(rdram, ctx);
        });
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800151BC),
        [](uint8_t* rdram, recomp_context* ctx) {
            int32_t divisor = MEM_W(0, kseg0_gpr(0x80078488));
            tooie_diag::note_game_logic(divisor);
            tooie_diag::note_guest_logic_tick();
            func_800151BC(rdram, ctx);
        });
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80014148),
        [](uint8_t* rdram, recomp_context* ctx) {
            tooie_diag::note_display_list();
            func_80014148(rdram, ctx);
        });
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8008C894),
        [](uint8_t* rdram, recomp_context* ctx) {
            tooie_diag::note_animation_update();
            if (tooie_ud5_scene_ok(rdram) &&
                tooie_ud5_is_player_anctrl(rdram, static_cast<uint32_t>(ctx->r4))) {
                const int glidx = tooie_ud5_glidx(rdram);
                tooie_ud5_dump_anctrl(rdram, "baanim-update-enter", glidx,
                                     static_cast<uint32_t>(ctx->r4), 0);
                std::fprintf(stderr, "[ud5-gate] phase=order glidx=%d event=baanim-update\n", glidx);
                std::fflush(stderr);
                baanim_update(rdram, ctx);
                tooie_ud5_dump_anctrl(rdram, "baanim-update-leave", glidx,
                                     static_cast<uint32_t>(ctx->r4), 0);
                return;
            }
            baanim_update(rdram, ctx);
        });
    recomp::overlays::add_loaded_function(
        static_cast<int32_t>(0x8009B9B0),
        tooie_wrap_baphysics_set_target_horizontal_velocity);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8009EEB8),
                                          tooie_wrap_bastick_get_angle_relative);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800CAF00),
                                          tooie_wrap_func_800CAF00);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8009B9C0),
                                          tooie_wrap_baphysics_set_target_yaw);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8009BA68),
                                          tooie_wrap_baphysics_set_horizontal_velocity);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8009D088),
                                          tooie_wrap_func_8009D088);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800EEB9C),
                                          tooie_wrap_func_800EEB9C);
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
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8010FFD0), tooie_wrap_func_8010FFD0);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80111F70), tooie_wrap_func_80111F70);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80111DB0), tooie_wrap_func_80111DB0);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80111E88), tooie_wrap_func_80111E88);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800EF174), tooie_wrap_func_800EF174);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8011106C), tooie_wrap_func_8011106C);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800EF04C), tooie_wrap_func_800EF04C);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x801138A0), tooie_wrap_func_801138A0);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x801138F4), tooie_wrap_func_801138F4);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80110F44), tooie_wrap_func_80110F44);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80115580), tooie_wrap_func_80115580);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80110A68), tooie_wrap_func_80110A68);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80110790), tooie_wrap_func_80110790);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80111310), tooie_wrap_func_80111310);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800F18FC), tooie_wrap_func_800F18FC);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800A516C), tooie_wrap_func_800A516C);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8002DCA0), tooie_wrap_sqrtf);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800979D8), tooie_bainput_func_800979D8);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8009E5C8), tooie_wrap_bs_setState);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8008B324), tooie_wrap_anctrl_isStopped);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8008B2B4), tooie_wrap_anctrl_getAnimTimer);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8009EF10), tooie_wrap_bastick_getZone);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x8008ADE4), tooie_wrap_func_8008ADE4);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80097B94), tooie_wrap_bainput_should_rotate_camera_right);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x800A4338), tooie_wrap_func_800A4338);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80110EFC), tooie_wrap_func_80110EFC);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80110E00), tooie_wrap_func_80110E00);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x801111D0), tooie_wrap_func_801111D0);
    recomp::overlays::add_loaded_function(static_cast<int32_t>(0x80112D90), tooie_wrap_func_80112D90);
    for (size_t i = 0; i < sizeof(kLookupGapAddrs) / sizeof(kLookupGapAddrs[0]); ++i) {
        recomp::overlays::add_loaded_function(kLookupGapAddrs[i], kLookupGapFuncs[i]);
    }
    std::fprintf(stderr,
                 "[tooie] registered %zu ultra LOOKUP hooks + 5 core helper hooks + %zu gap stubs\n",
                 sizeof(kUltraLookupHooks) / sizeof(kUltraLookupHooks[0]),
                 sizeof(kLookupGapAddrs) / sizeof(kLookupGapAddrs[0]));
}
} // namespace banjo
