// Strong overrides for boot-segment libultra that touch unmapped N64 MMIO
// (SI/PI at 0xA48xxxxx / 0xA46xxxxx). RecompiledFuncs symbols are weak under
// clang RECOMP_FUNC; these replace them at link time.
//
// Also registers LOOKUP targets that N64Recomp never emitted (unnamed helpers).

#include <cstdint>
#include <cstdio>

#include "recomp.h"
#include "librecomp/game.hpp"
#include "librecomp/addresses.hpp"
#include "librecomp/overlays.hpp"
#include "ultramodern/ultramodern.hpp"
#include "tooie_diag.h"

// MEM_* expects KSEG0 addresses sign-extended into gpr (same as recomp codegen).
static inline gpr kseg0_gpr(uint32_t addr) {
    return static_cast<gpr>(static_cast<int32_t>(addr));
}

// Declared in librecomp overlays.cpp
extern "C" recomp_func_t* get_function(int32_t addr);
extern "C" void load_overlays(uint32_t rom, int32_t ram_addr, uint32_t size);
extern "C" void unload_overlays(int32_t ram_addr, uint32_t size);
extern "C" void func_80013818(uint8_t* rdram, recomp_context* ctx);

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

// LOOKUP_FUNC targets with no recompiled symbol (N64Recomp gaps). No-op stubs
// keep boot/runtime from aborting for *unknown* helpers only.
// DO NOT put real libultra entry points here — that overwrites the func_map and
// deadlocks the scheduler (e.g. osDpGetStatus/osYieldThread were wrongly stubbed).
static void lookup_gap_stub(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    (void)ctx;
    ctx->r2 = 0;
}

// Truly unknown / not yet implemented helpers (not standard libultra).
static constexpr int32_t kLookupGapAddrs[] = {
    static_cast<int32_t>(0x80001998),
    static_cast<int32_t>(0x800136E4),
    static_cast<int32_t>(0x800138D0),
    static_cast<int32_t>(0x80013970),
    static_cast<int32_t>(0x80031178),
    static_cast<int32_t>(0x80036670),
    static_cast<int32_t>(0x80036728),
    static_cast<int32_t>(0x800979D8),
    static_cast<int32_t>(0x800BCCC0),
    static_cast<int32_t>(0x800BCCD8),
};

// Host libultra implementations (librecomp). Declared so the linker pulls them in
// and we can install them into the LOOKUP map at Tooie VRAMs.
extern "C" void osDpGetStatus_recomp(uint8_t* rdram, recomp_context* ctx);
extern "C" void osDpSetStatus_recomp(uint8_t* rdram, recomp_context* ctx);
extern "C" void osYieldThread_recomp(uint8_t* rdram, recomp_context* ctx);
extern "C" void __osPiGetAccess_recomp(uint8_t* rdram, recomp_context* ctx);
extern "C" void __osPiRelAccess_recomp(uint8_t* rdram, recomp_context* ctx);
extern "C" void osPfsInit_recomp(uint8_t* rdram, recomp_context* ctx);
extern "C" void osSendMesg_recomp(uint8_t* rdram, recomp_context* ctx);
extern "C" void osRecvMesg_recomp(uint8_t* rdram, recomp_context* ctx);

// Lightweight host stubs for SI/VI helpers without a librecomp body yet.
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

// Host-side 64-byte PIF-RAM mirror (SI DMA source/dest for CIC/controllers).
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
    { static_cast<int32_t>(0x8002F240), osDpGetStatus_recomp, "osDpGetStatus" },
    { static_cast<int32_t>(0x8002FE70), tooie_osYieldThread, "osYieldThread" },
    { static_cast<int32_t>(0x8002FF60), tooie_osViGetCurrentLine, "osViGetCurrentLine" },
    { static_cast<int32_t>(0x80030A10), osPfsInit_recomp, "osPfsInit" },
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

    // Pre-compute text_start for recomp registration. Caller/func_80082088 also set it.
    const gpr text_start = overlay_ptr + static_cast<gpr>(static_cast<int32_t>(text_off));
    MEM_W(0x34, overlay_ptr) = static_cast<int32_t>(text_start);

    if (text_size > 0 && text_off + text_size <= pkg_size) {
        load_overlays(pkg_rom + text_off, static_cast<int32_t>(text_start), text_size);
        std::fprintf(stderr,
                     "[tooie] ovl_load: idx=%d pkg=0x%x +0x%x -> ram=0x%x size=0x%x\n",
                     overlay_index, pkg_rom, text_off,
                     static_cast<uint32_t>(text_start), text_size);
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
    const uint32_t text_size = uint32_t(uint16_t(MEM_H(0x00, overlay_ptr))) * 0x10u;
    if (text_start != 0 && text_size != 0) {
        unload_overlays(text_start, text_size);
    }

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

    // Build a one-word view of the package header for overlay_text_offset.
    uint8_t hdr[16];
    for (int i = 0; i < 16; i++) {
        hdr[i] = static_cast<uint8_t>(MEM_BU(i, overlay));
    }
    const uint32_t text_off = overlay_text_offset(hdr);
    const int32_t num_entry = static_cast<int32_t>((hdr[8] << 8) | hdr[9]);
    const int32_t text_size = int32_t((uint16_t(hdr[0]) << 8) | hdr[1]) * 0x10;

    const gpr text_start =
        forced_text != 0 ? forced_text
                         : overlay + static_cast<gpr>(static_cast<int32_t>(text_off));

    // Relocate entrypoint table: package offsets → absolute VRAM.
    gpr ep = ADD32(overlay, 0x38);
    const gpr ep_end = ADD32(ep, S32(num_entry << 2));
    while (ep != ep_end) {
        const int32_t rel = MEM_W(0, ep);
        MEM_W(0, ep) = static_cast<int32_t>(ADD32(rel, text_start));
        ep = ADD32(ep, 4);
    }

    // Original does: fields.text_start += text_start (package-relative + base).
    // ovl_load already wrote the absolute address; keep that if present.
    const int32_t ts = MEM_W(0x34, overlay);
    if (ts == 0 || static_cast<uint32_t>(ts) < 0x80000000u) {
        MEM_W(0x34, overlay) = static_cast<int32_t>(text_start);
    } else if (static_cast<uint32_t>(ts) == static_cast<uint32_t>(text_off)) {
        MEM_W(0x34, overlay) = static_cast<int32_t>(text_start);
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
    ctx->r1 = S32(0x8004 << 16);
    ctx->f14.u32l = MEM_W(ctx->r1, 0x16D4);
    ctx->r1 = S32(0x43B4 << 16);
    ctx->f16.u32l = ctx->r1;
    func_80013818(rdram, ctx);
    ctx->f0.fl = ctx->f2.fl;
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

    const int32_t word0 = MEM_W(0, kseg0_gpr(0x8030B1F4u)); // was A030:-0x4E0C
    const int32_t word1 = MEM_W(0, kseg0_gpr(0x8030E1C0u)); // was A030:-0x1E40
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

namespace banjo {
void register_boot_lookup_stubs() {
    // Prefer real ultramodern/libultra recomp hooks at Tooie VRAMs.
    for (const UltraLookupHook& hook : kUltraLookupHooks) {
        recomp::overlays::add_loaded_function(hook.vram, hook.func);
    }
    for (int32_t addr : kLookupGapAddrs) {
        recomp::overlays::add_loaded_function(addr, lookup_gap_stub);
    }
    std::fprintf(stderr,
                 "[tooie] registered %zu ultra LOOKUP hooks + %zu gap stubs\n",
                 sizeof(kUltraLookupHooks) / sizeof(kUltraLookupHooks[0]),
                 sizeof(kLookupGapAddrs) / sizeof(kLookupGapAddrs[0]));
}
} // namespace banjo
