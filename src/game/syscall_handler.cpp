#include <cstdio>
#include <cstdint>

#include "recomp.h"
#include "librecomp/overlays.hpp"
#include "librecomp/game.hpp"
#include "librecomp/addresses.hpp"

#include "generated/overlay_dispatch.inc.h"

// Core2 hosts the overlay_syscalls trampoline table.
// VRAM/ROM bases match section_16_core2 in recomp_overlays.inl.
static constexpr uint32_t kCore2Ram = 0x800815C0u;
static constexpr uint32_t kCore2Rom = 0x01E5AEB0u;
static constexpr uint32_t kOverlayVram = 0x80800000u;

// MEM_* needs KSEG0 sign-extended into 64-bit gpr.
static inline gpr kseg0_gpr(uint32_t addr) {
    return static_cast<gpr>(static_cast<int32_t>(addr));
}

// MIPS: SPECIAL/SYSCALL (op=0, funct=0x0C), code in bits [25:6].
// Following word is addi $t0, $zero, imm or xori $t0, $zero, imm (t0 = entry_index * 4).
static bool decode_syscall_stub(uint32_t word0, uint32_t word1,
                                uint32_t* overlay_index_out, uint32_t* entry_index_out) {
    const uint32_t op0 = (word0 >> 26) & 0x3Fu;
    const uint32_t funct = word0 & 0x3Fu;
    if (op0 != 0u || funct != 0x0Cu) {
        return false;
    }
    const uint32_t syscall_code = (word0 >> 6) & 0xFFFFFu;

    const uint32_t op1 = (word1 >> 26) & 0x3Fu;
    const uint32_t rs = (word1 >> 21) & 0x1Fu;
    const uint32_t rt = (word1 >> 16) & 0x1Fu;
    const uint32_t imm = word1 & 0xFFFFu;
    // addi (8) or xori (0xE), rs=$zero, rt=$t0
    if ((op1 != 8u && op1 != 0x0Eu) || rs != 0u || rt != 8u) {
        return false;
    }
    if ((imm & 3u) != 0u) {
        return false;
    }

    *overlay_index_out = syscall_code; // ROM: code == overlay index (gsmaker=2, …)
    *entry_index_out = imm >> 2;
    return true;
}

static bool read_core2_words(uint32_t vram, uint32_t* word0, uint32_t* word1) {
    if (vram < kCore2Ram) {
        return false;
    }
    const uint32_t rom_off = kCore2Rom + (vram - kCore2Ram);
    const auto rom = recomp::get_rom();
    if (rom_off + 8u > rom.size()) {
        return false;
    }
    // ROM is big-endian instruction stream.
    *word0 = (uint32_t(rom[rom_off + 0]) << 24) | (uint32_t(rom[rom_off + 1]) << 16) |
             (uint32_t(rom[rom_off + 2]) << 8) | uint32_t(rom[rom_off + 3]);
    *word1 = (uint32_t(rom[rom_off + 4]) << 24) | (uint32_t(rom[rom_off + 5]) << 16) |
             (uint32_t(rom[rom_off + 6]) << 8) | uint32_t(rom[rom_off + 7]);
    return true;
}

// Exported for unit tests (decode only).
extern "C" bool tooie_decode_syscall_stub(uint32_t word0, uint32_t word1,
                                          uint32_t* overlay_index_out, uint32_t* entry_index_out) {
    return decode_syscall_stub(word0, word1, overlay_index_out, entry_index_out);
}

extern "C" void recomp_syscall_handler(uint8_t* rdram, recomp_context* ctx, int32_t instruction_vram) {
    const uint32_t vram = static_cast<uint32_t>(instruction_vram);

    uint32_t word0 = 0, word1 = 0;
    if (!read_core2_words(vram, &word0, &word1)) {
        std::fprintf(stderr,
                     "[tooie] recomp_syscall_handler: cannot read stub at vram=0x%08x\n",
                     vram);
        return;
    }

    uint32_t overlay_index = 0;
    uint32_t entry_index = 0;
    if (!decode_syscall_stub(word0, word1, &overlay_index, &entry_index)) {
        std::fprintf(stderr,
                     "[tooie] recomp_syscall_handler: bad stub vram=0x%08x words=%08x %08x\n",
                     vram, word0, word1);
        return;
    }

    // Reflect t0 as the original addi/xori would have (dead after early return in generated C).
    ctx->r8 = static_cast<gpr>(entry_index * 4u);

    if (overlay_index == 0u || overlay_index > kTooieOverlayCount) {
        std::fprintf(stderr,
                     "[tooie] recomp_syscall_handler: overlay index %u out of range (vram=0x%08x)\n",
                     overlay_index, vram);
        return;
    }

    const TooieOverlayDispatchEntry& entry = g_tooie_overlay_dispatch[overlay_index];
    if (entry.entrypoints == nullptr || entry.size == 0u || entry.num_entrypoints == 0u) {
        std::fprintf(stderr,
                     "[tooie] recomp_syscall_handler: empty/unknown overlay %u (vram=0x%08x)\n",
                     overlay_index, vram);
        return;
    }
    if (entry_index >= entry.num_entrypoints) {
        std::fprintf(stderr,
                     "[tooie] recomp_syscall_handler: entry %u >= %u for overlay %u (vram=0x%08x)\n",
                     entry_index, entry.num_entrypoints, overlay_index, vram);
        return;
    }

    recomp_func_t* func = entry.entrypoints[entry_index];
    if (func == nullptr) {
        std::fprintf(stderr,
                     "[tooie] recomp_syscall_handler: null entrypoint ovl=%u ep=%u\n",
                     overlay_index, entry_index);
        std::fflush(stderr);
        return;
    }

    static unsigned s_dispatch_log_left = 128;
    static uint32_t s_loaded_ovl = 0;

    if (s_dispatch_log_left > 0) {
        --s_dispatch_log_left;
        std::fprintf(stderr,
                     "[tooie] syscall dispatch ovl=%u ep=%u rom=0x%08x size=0x%x vram=0x%08x\n",
                     overlay_index, entry_index, entry.rom_addr, entry.size, vram);
        std::fflush(stderr);
    }

    // Shared overlay bank at link VRAM (single slot @ 0x80800000). Nested syscalls
    // can switch the bank mid-call; remember the previous overlay and restore it
    // after the callee returns so LOOKUPs in the outer overlay keep working.
    const uint32_t previous_ovl = s_loaded_ovl;
    auto load_overlay_bank = [&](uint32_t ovl_index) {
        if (ovl_index == 0 || ovl_index > kTooieOverlayCount) {
            return;
        }
        const TooieOverlayDispatchEntry& e = g_tooie_overlay_dispatch[ovl_index];
        if (e.entrypoints == nullptr || e.size == 0u) {
            return;
        }
        unload_overlays(static_cast<int32_t>(kOverlayVram), 0x200000u);
        recomp::do_rom_read(rdram, kseg0_gpr(kOverlayVram),
                            recomp::rom_base + e.rom_addr, e.size);
        load_overlays(e.rom_addr, static_cast<int32_t>(kOverlayVram), e.size);
        s_loaded_ovl = ovl_index;
    };

    if (s_loaded_ovl != overlay_index) {
        load_overlay_bank(overlay_index);
    }

    func(rdram, ctx);

    if (previous_ovl != 0u && previous_ovl != overlay_index) {
        load_overlay_bank(previous_ovl);
    }

    if (s_dispatch_log_left > 0) {
        std::fprintf(stderr, "[tooie] syscall return ovl=%u ep=%u\n", overlay_index, entry_index);
        std::fflush(stderr);
    }
}
