#include <cstdio>
#include <cstdint>
#include <chrono>
#include <algorithm>
#include <array>
#include <vector>

#include "recomp.h"
#include "librecomp/overlays.hpp"
#include "librecomp/game.hpp"
#include "librecomp/addresses.hpp"

#include "generated/overlay_dispatch.inc.h"
#include "tooie_diag.h"

// Core2 hosts the overlay_syscalls trampoline table.
// VRAM/ROM bases match section_16_core2 in recomp_overlays.inl.
static constexpr uint32_t kCore2Ram = 0x800815C0u;
static constexpr uint32_t kCore2Rom = 0x01E5AEB0u;
static constexpr int32_t kOverlayVram = static_cast<int32_t>(0x80800000u);
static constexpr uint32_t kOverlayWindowSize = 0x00200000u;

extern "C" void ovl_load(uint8_t* rdram, recomp_context* ctx);
extern "C" void ovl_unload(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_80082088(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_80108C90(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_80108474(uint8_t* rdram, recomp_context* ctx);

struct ActiveOverlayMapping {
    uint32_t overlay_index = 0;
    uint32_t rom_addr = 0;
    uint32_t code_size = 0;
    gpr package = 0;
    int32_t text_start = 0;
    uint32_t text_size = 0;
    bool dynamic_alive = false;
};

static thread_local std::vector<ActiveOverlayMapping> s_overlay_stack;
static thread_local ActiveOverlayMapping s_canonical_mapping;
static thread_local bool s_has_canonical_mapping = false;

struct ResidentOverlay {
    gpr package = 0;
    ActiveOverlayMapping mapping{};
};

static thread_local std::array<ResidentOverlay, kTooieOverlayCount + 1>
    s_resident_overlays{};

static void park_canonical_mapping() {
    // Keep mappings registered so intra-overlay LOOKUP_FUNC calls remain resolvable
    s_has_canonical_mapping = false;
}

static void activate_canonical_mapping(const ActiveOverlayMapping& mapping,
                                       bool restore_dynamic) {
    load_overlays(mapping.rom_addr, kOverlayVram, mapping.code_size);

    if (restore_dynamic && mapping.dynamic_alive && mapping.text_start != 0 &&
        mapping.text_size != 0) {
        load_overlays(mapping.rom_addr, mapping.text_start, mapping.code_size);
    }

    s_canonical_mapping = mapping;
    s_has_canonical_mapping = true;
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

    // Verbose first N dispatches always; TOOIE_DIAG keeps counters forever.
    if (s_dispatch_log_left > 0) {
        --s_dispatch_log_left;
        std::fprintf(stderr,
                     "[tooie] syscall dispatch ovl=%u ep=%u rom=0x%08x size=0x%x vram=0x%08x\n",
                     overlay_index, entry_index, entry.rom_addr, entry.size, vram);
        std::fflush(stderr);
    }
    tooie_diag::note_syscall_dispatch(overlay_index, entry_index);

    // Reproduce the original dynamic-overlay lifetime around the native call.
    // The MIPS ovl_entrypoint trampoline cannot be used directly after recomp
    // because its self-modified immediates are constants in generated C.  Its
    // load/relocate/unload protocol is still required, though: RELOC_* needs a
    // live package mapping and actors may retain package data when flag 0x04
    // marks an overlay resident.
    park_canonical_mapping();

    ActiveOverlayMapping mapping{};
    gpr overlay_ptr = 0;
    bool owns_package = false;
    ResidentOverlay& cached = s_resident_overlays[overlay_index];

    const auto active = std::find_if(
        s_overlay_stack.rbegin(), s_overlay_stack.rend(),
        [overlay_index](const ActiveOverlayMapping& candidate) {
            return candidate.overlay_index == overlay_index &&
                   candidate.package != 0 && candidate.dynamic_alive;
        });

    if (active != s_overlay_stack.rend()) {
        // The original relocation pass patches all syscall stubs belonging to
        // this package. Calls to another entrypoint while the package is active
        // therefore bypass ovl_load. Generated C keeps the syscall instruction,
        // so reproduce that intra-overlay fast path here.
        mapping = *active;
        overlay_ptr = mapping.package;
        activate_canonical_mapping(mapping, true);
    } else if (cached.package != 0) {
        // On N64, func_80082088 self-patches resident syscall stubs so later
        // calls jump straight into the already-relocated package. Generated C
        // cannot observe those instruction writes, therefore emulate that fast
        // path explicitly instead of allocating and resetting overlay BSS on
        // every call.
        overlay_ptr = cached.package;
        mapping = cached.mapping;
        activate_canonical_mapping(mapping, true);
    } else {
        owns_package = true;
        mapping.overlay_index = overlay_index;
        mapping.rom_addr = entry.rom_addr;
        mapping.code_size = entry.size;
        activate_canonical_mapping(mapping, false);

        recomp_context load_ctx = *ctx;
        load_ctx.r4 = static_cast<gpr>(overlay_index);
        load_ctx.r5 = 0;
        load_ctx.r6 = 0;
        ovl_load(rdram, &load_ctx);
        overlay_ptr = load_ctx.r2;
        if (overlay_ptr == 0) {
            std::fprintf(stderr,
                         "[tooie] recomp_syscall_handler: failed to load overlay %u ep=%u\n",
                         overlay_index, entry_index);
            std::fflush(stderr);
            return;
        }

        // These fields are normally written by the generated syscall handler
        // before func_80082088 patches the entrypoint table and text base.
        MEM_H(0x2C, overlay_ptr) = static_cast<int16_t>(overlay_index);
        MEM_H(0x2E, overlay_ptr) = static_cast<int16_t>(entry_index);

        recomp_context relocate_ctx = *ctx;
        relocate_ctx.r4 = overlay_ptr;
        relocate_ctx.r5 = 0;
        func_80082088(rdram, &relocate_ctx);

        // Save only after relocation so nested syscalls restore the correct
        // dynamic RELOC_* base.
        mapping.text_start = MEM_W(0x34, overlay_ptr);
        mapping.text_size = uint32_t(uint16_t(MEM_H(0x00, overlay_ptr))) * 0x10u;
        mapping.package = overlay_ptr;
        mapping.dynamic_alive = true;

        if ((static_cast<uint8_t>(MEM_BU(0x0F, overlay_ptr)) & 0x04u) != 0) {
            cached.package = overlay_ptr;
            cached.mapping = mapping;
        }
    }

    s_canonical_mapping = mapping;
    s_overlay_stack.push_back(mapping);

    // Hang suspects (gsworld→gsmaker→gldb): time long overlays when diag is on.
    const bool time_call = tooie_diag::enabled() &&
                           (overlay_index == 708u || overlay_index == 2u || overlay_index == 726u);
    const auto t0 = time_call ? std::chrono::steady_clock::now()
                              : std::chrono::steady_clock::time_point{};
    if (time_call) {
        std::fprintf(stderr, "[tooie-diag] enter ovl=%u ep=%u\n", overlay_index, entry_index);
        std::fflush(stderr);
    }

    // Opening attract path: dump mode machine on key overlays only.
    // D_80127630.unk2 @ +0x2, warp @ +0x12; D_80127760 attract kind @ +0x0;
    // D_80135470 attract stage @ +0x0, timer float @ +0x4.
    const bool log_opening = tooie_diag::enabled() &&
        (overlay_index == 731u || overlay_index == 419u || overlay_index == 441u ||
         overlay_index == 208u || overlay_index == 695u || overlay_index == 677u ||
         overlay_index == 639u || overlay_index == 778u);
    static uint32_t s_opening_log_left = 400;
    auto dump_opening = [&](const char* tag) {
        if (!log_opening || s_opening_log_left == 0) {
            return;
        }
        --s_opening_log_left;
        const gpr mode = static_cast<gpr>(static_cast<int32_t>(0x80127630u));
        const gpr attr = static_cast<gpr>(static_cast<int32_t>(0x80127760u));
        const gpr stg = static_cast<gpr>(static_cast<int32_t>(0x80135470u));
        std::fprintf(stderr,
                     "[tooie-opening] %s ovl=%u ep=%u "
                     "mode=%d warp=%u map=%d entr=%d "
                     "attr=%u/%u/%u/%u "
                     "stage=%u sub=%u t=%08x a0=%08x\n",
                     tag, overlay_index, entry_index,
                     int(MEM_H(0x2, mode)), uint32_t(MEM_BU(0x12, mode)),
                     int(MEM_H(0x10, mode)), int(MEM_B(0x13, mode)),
                     uint32_t(MEM_BU(0x0, attr)), uint32_t(MEM_BU(0x1, attr)),
                     uint32_t(MEM_BU(0x2, attr)), uint32_t(MEM_BU(0x3, attr)),
                     uint32_t(MEM_BU(0x0, stg)), uint32_t(MEM_BU(0x1, stg)),
                     static_cast<uint32_t>(MEM_W(0x4, stg)),
                     static_cast<uint32_t>(ctx->r4));
        std::fflush(stderr);
    };
    dump_opening("enter");

    func(rdram, ctx);

    dump_opening("leave");

    if (time_call) {
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - t0)
                            .count();
        std::fprintf(stderr, "[tooie-diag] leave ovl=%u ep=%u after %lld ms\n",
                     overlay_index, entry_index, static_cast<long long>(ms));
        std::fflush(stderr);
    }

    // This mirrors func_80081E30, the return trampoline installed by the game.
    // Resident overlays (package flag 0x04) intentionally stay mapped; ovl_unload
    // handles that rule and returns without freeing them.
    const bool resident = (static_cast<uint8_t>(MEM_BU(0x0F, overlay_ptr)) & 0x04u) != 0;
    if (owns_package) {
        if (cached.package == 0) {
            cached.package = overlay_ptr;
            cached.mapping = mapping;
        } else if (!resident) {
            recomp_context unload_ctx = *ctx;
            unload_ctx.r4 = overlay_ptr;
            unload_ctx.r5 = 1;
            ovl_unload(rdram, &unload_ctx);
        }
    }

    s_overlay_stack.back().dynamic_alive = !owns_package || resident;
    s_canonical_mapping = s_overlay_stack.back();
    s_overlay_stack.pop_back();

    if (!s_overlay_stack.empty()) {
        park_canonical_mapping();
        activate_canonical_mapping(s_overlay_stack.back(), true);
    }

    tooie_diag::note_syscall_return(overlay_index, entry_index);
    if (s_dispatch_log_left > 0) {
        std::fprintf(stderr, "[tooie] syscall return ovl=%u ep=%u\n", overlay_index, entry_index);
        std::fflush(stderr);
    }

    // Auto-spawn diagnostic on MAP_18B_CS_STARTUP_INTRO if not already active.
    // NOTE: func_80108C90 receives a MARKER ID, not an overlay index.
    // Marker 419 == chcollballcontrol (overlay 467) — NOT chintroticker
    // (chintroticker overlay index is 419, but its marker id is 569).
    // 419 is kept deliberately as the relocation regression case for P08;
    // do NOT change to 569 in this stage.
    static bool s_intro_ticker_spawned = false;
    if (overlay_index == 677u && entry_index == 4u && !s_intro_ticker_spawned) {
        const gpr mode = static_cast<gpr>(static_cast<int32_t>(0x80127630u));
        const int map_val = int(MEM_H(0x10, mode));
        if (map_val == 0x18B || ctx->r4 == 0x18B) {
            s_intro_ticker_spawned = true;
            std::fprintf(stderr, "[tooie-intro] spawning marker 419 (chcollballcontrol, NOT chintroticker) on MAP_18B...\n");
            std::fflush(stderr);

            // Write 0.0f pos into scratch area 0x8007D800
            const gpr scratch = static_cast<gpr>(static_cast<int32_t>(0x8007D800u));
            MEM_W(0x0, scratch) = 0;
            MEM_W(0x4, scratch) = 0;
            MEM_W(0x8, scratch) = 0;

            recomp_context spawn_ctx = *ctx;
            spawn_ctx.r4 = 419; // marker id (chcollballcontrol); NOT chintroticker (569)
            spawn_ctx.r5 = scratch;
            spawn_ctx.r6 = 0;
            spawn_ctx.r7 = 0;
            func_80108C90(rdram, &spawn_ctx);
            std::fprintf(stderr, "[tooie-intro] marker 419 (chcollballcontrol) spawn returned v0=0x%llx\n",
                         static_cast<unsigned long long>(spawn_ctx.r2));
            std::fflush(stderr);
        }
    }
}
