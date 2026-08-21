#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <algorithm>
#include <array>
#include <string>
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
extern "C" void tooie_demo_path_tick(uint8_t* rdram, uint64_t tick);
extern "C" void tooie_demo_sync_tick(uint8_t* rdram, uint64_t tick);
extern "C" void tooie_ud4_ncbapivot_ep3(uint8_t* rdram, recomp_context* ctx);
extern "C" void tooie_shot_tick(uint8_t* rdram, uint64_t tick);
extern "C" void tooie_shot_note_overlay(uint8_t* rdram, void* ctx, uint32_t ovl, uint32_t ep, int leaving);
extern "C" void baflameline_entrypoint_5(uint8_t* rdram, recomp_context* ctx);

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

// Invalidates any resident-overlay cache entry whose package points at the
// given (now freed) heap block. The game unloads non-resident overlays after
// use; without this, a later syscall dispatch would reuse the stale cached
// mapping whose heap region may have been reallocated to another overlay.
extern "C" void tooie_invalidate_resident_overlay(gpr overlay_ptr) {
    if (overlay_ptr == 0) {
        return;
    }
    for (auto& entry : s_resident_overlays) {
        if (entry.package == overlay_ptr) {
            entry.package = 0;
            entry.mapping = ActiveOverlayMapping{};
        }
    }
}

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

    // Compact attract-cycle markers (T1/T2/T3). Always-on; change-only plus
    // bsrest entrypoints that jal 0x808003FC (unnamed local vs baflameline).
    {
        const gpr mode = static_cast<gpr>(static_cast<int32_t>(0x80127630u));
        const gpr attr = static_cast<gpr>(static_cast<int32_t>(0x80127760u));
        const gpr stg = static_cast<gpr>(static_cast<int32_t>(0x80135470u));
        const int mode_v = int(MEM_H(0x2, mode));
        const int map_v = int(MEM_H(0x10, mode));
        const int entr_v = int(MEM_B(0x13, mode));
        const uint32_t warp_v = uint32_t(MEM_BU(0x12, mode));
        const uint32_t stage_v = uint32_t(MEM_BU(0x0, stg));
        const uint32_t sub_v = uint32_t(MEM_BU(0x1, stg));
        const uint32_t attr0 = uint32_t(MEM_BU(0x0, attr));
        static int s_xm = 0x7fffffff;
        static int s_xp = 0x7fffffff;
        static uint32_t s_xw = 0xffffffffu;
        static uint32_t s_xs = 0xffffffffu;
        static uint32_t s_xu = 0xffffffffu;
        const bool changed = (mode_v != s_xm) || (map_v != s_xp) || (warp_v != s_xw) ||
                             (stage_v != s_xs) || (sub_v != s_xu);
        const bool rest_jal =
            (overlay_index == 84u) &&
            (entry_index == 3u || entry_index == 7u || entry_index == 11u);
        if (changed || rest_jal) {
            const char* tag = "other";
            if (map_v == 335) {
                tag = "MAP_335_WOODED_HOLLOW";
            } else if (map_v == 278) {
                tag = "MAP_278_TDL_INSIDE_MOUNTAIN";
            } else if (map_v == 314) {
                tag = "MAP_314_CCL_CENTRAL_CAVERN";
            } else if (map_v == 395) {
                tag = "MAP_395_CS_STARTUP_INTRO";
            } else if (map_v == 235) {
                tag = "MAP_235_WW_HAUNTED_CAVERN";
            } else if (map_v == 296) {
                tag = "MAP_296";
            } else if (map_v == 185) {
                tag = "MAP_185_MT";
            } else if (map_v == 282) {
                tag = "MAP_282_TDL_STOMPING_PLAINS";
            } else if (map_v == 264) {
                tag = "MAP_264";
            } else if (map_v == 230) {
                tag = "MAP_230";
            } else if (map_v == 244) {
                tag = "MAP_244_JRL";
            } else if (map_v == 276) {
                tag = "MAP_276";
            } else if (map_v == 423) {
                tag = "MAP_423";
            } else if (mode_v == 3 && map_v == 0) {
                tag = "T_INTRO";
            }
            static int s_user_demo = 0;
            static int s_intro_occ = 0;
            static bool s_in_intro_block = true;
            static uint32_t s_last_demo_key = 0xffffffffu;
            const bool is_title_intro = (mode_v == 3 && map_v == 0);
            const bool is_cs_intro = (map_v == 395);
            const bool is_intro = is_title_intro || is_cs_intro;
            const bool is_gameplay = (mode_v == 5 && map_v != 0 && map_v != 395);
            if (is_intro) {
                if (!s_in_intro_block) {
                    ++s_intro_occ;
                    s_in_intro_block = true;
                }
            } else if (is_gameplay) {
                const uint32_t demo_key = (stage_v << 16) | (sub_v << 8) | uint32_t(map_v & 0xff);
                if (demo_key != s_last_demo_key) {
                    ++s_user_demo;
                    s_last_demo_key = demo_key;
                    s_in_intro_block = false;
                }
            }
            recomp_func_t* looked = rest_jal ? get_function(static_cast<int32_t>(0x808003FC)) : nullptr;
            const bool is_flameline =
                rest_jal && (looked == reinterpret_cast<recomp_func_t*>(baflameline_entrypoint_5));
            std::fprintf(stderr,
                         "[tooie-xition] %s ovl=%u ep=%u mode=%d map=%d entr=%d warp=%u "
                         "stage=%u sub=%u attr=%u/%u/%u/%u a0=%08x a1=%08x "
                         "lookup808003FC=%p flameline=%d user_demo=%d intro_occ=%d "
                         "gameplay=%d\n",
                         tag, overlay_index, entry_index, mode_v, map_v, entr_v, warp_v,
                         stage_v, sub_v,
                         attr0, uint32_t(MEM_BU(0x1, attr)),
                         uint32_t(MEM_BU(0x2, attr)), uint32_t(MEM_BU(0x3, attr)),
                         static_cast<uint32_t>(ctx->r4), static_cast<uint32_t>(ctx->r5),
                         reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(looked)),
                         int(is_flameline), s_user_demo, s_intro_occ, int(is_gameplay));
            std::fflush(stderr);
            s_xm = mode_v;
            s_xp = map_v;
            s_xw = warp_v;
            s_xs = stage_v;
            s_xu = sub_v;
        }

        if (tooie_diag::attract_enabled() && overlay_index == 731u) {
            static bool s_dumped_table = false;
            if (!s_dumped_table) {
                s_dumped_table = true;
                std::fprintf(stderr, "[attract-table] D_808003D0 6-byte records:\n");
                for (int i = 0; i < 32; ++i) {
                    const uint32_t rec = 0x808003D0u + uint32_t(i) * 6u;
                    const gpr rp = static_cast<gpr>(static_cast<int32_t>(rec));
                    const int map_id = int(MEM_H(0, rp));
                    const int entrance = int(MEM_BU(2, rp));
                    const int flags = int(MEM_BU(3, rp));
                    const int extra = int(MEM_HU(4, rp));
                    std::fprintf(stderr,
                                 "[attract-table] i=%02d map=%d (0x%04x) entr=%d flags=%02x extra=%04x\n",
                                 i, map_id, map_id & 0xffff, entrance, flags, extra);
                }
                std::fprintf(stderr, "[attract-table] D_80800490 12-byte stage records:\n");
                for (int i = 0; i < 2; ++i) {
                    const uint32_t rec = 0x80800490u + uint32_t(i) * 12u;
                    const gpr rp = static_cast<gpr>(static_cast<int32_t>(rec));
                    std::fprintf(stderr,
                                 "[attract-table] stage=%d h0=%04x b2=%02x ptr=%08x flags=%02x\n",
                                 i, int(MEM_HU(0, rp)), int(MEM_BU(2, rp)),
                                 uint32_t(MEM_W(4, rp)), int(MEM_BU(8, rp)));
                }
                std::fflush(stderr);
            }
        }

        if (tooie_diag::attract_enabled() &&
            overlay_index >= 774u && overlay_index <= 791u) {
            static uint32_t s_last_cam_ovl = 0xffffffffu;
            static uint32_t s_last_cam_ep = 0xffffffffu;
            if (overlay_index != s_last_cam_ovl || entry_index != s_last_cam_ep) {
                const char* cam_name = "ncba?";
                switch (overlay_index) {
                case 774: cam_name = "ncbafixpos"; break;
                case 775: cam_name = "ncbadolly"; break;
                case 776: cam_name = "ncbaspline"; break;
                case 777: cam_name = "ncba1p"; break;
                case 780: cam_name = "ncbawaypoint"; break;
                case 781: cam_name = "ncbapivot"; break;
                case 782: cam_name = "ncbatarget"; break;
                case 789: cam_name = "ncbasub"; break;
                default: break;
                }
                std::fprintf(stderr,
                             "[cam-ovl] enter ovl=%u ep=%u name=%s mode=%d map=%d\n",
                             overlay_index, entry_index, cam_name,
                             int(MEM_H(0x2, static_cast<gpr>(static_cast<int32_t>(0x80127630u)))),
                             int(MEM_H(0x10, static_cast<gpr>(static_cast<int32_t>(0x80127630u)))));
                std::fflush(stderr);
                s_last_cam_ovl = overlay_index;
                s_last_cam_ep = entry_index;
            }
            if (overlay_index == 781u && entry_index == 3u) {
                tooie_ud4_ncbapivot_ep3(rdram, ctx);
            }
        }
    }

    if (tooie_diag::frontend_enabled()) {
        const gpr mode_gpr = static_cast<gpr>(static_cast<int32_t>(0x80127630u));
        const int mode_val = int(MEM_H(0x2, mode_gpr));
        const int map_val = int(MEM_H(0x10, mode_gpr));
        const uint32_t warp_val = uint32_t(MEM_BU(0x12, mode_gpr));
        tooie_diag::note_frontend_state(overlay_index, entry_index, mode_val, map_val, warp_val);
    }

    // TOOIE_DIAG_MOVEMENT: independent of TOOIE_DIAG. Dumps the opening
    // probe fields plus player-0 simulation pose so visible-demo movement
    // can be classified without the older overlay-timed diagnostic path.
    auto dump_movement = [&](const char* tag) {
        if (!tooie_diag::movement_enabled() && !tooie_diag::input_enabled() &&
            !tooie_diag::forced_facing_enabled()) {
            return;
        }
        static auto s_last = std::chrono::steady_clock::time_point{};
        static int s_last_mode = 0x7fffffff;
        static int s_last_map = 0x7fffffff;
        const auto now = std::chrono::steady_clock::now();
        const gpr mode = static_cast<gpr>(static_cast<int32_t>(0x80127630u));
        const gpr attr = static_cast<gpr>(static_cast<int32_t>(0x80127760u));
        const gpr stg = static_cast<gpr>(static_cast<int32_t>(0x80135470u));
        const int mode_v = int(MEM_H(0x2, mode));
        const int map_v = int(MEM_H(0x10, mode));
        const bool changed = (mode_v != s_last_mode) || (map_v != s_last_map);
        const bool demo_clip = tooie_diag::demo_path_enabled() && (mode_v == 5);
        const bool facing_clip =
            tooie_diag::forced_facing_enabled() && (mode_v == 3 || mode_v == 5);
        const int due_ms = (demo_clip || facing_clip) ? 50 : 200;
        const bool due = (s_last.time_since_epoch().count() == 0) ||
                         (now - s_last >= std::chrono::milliseconds(due_ms));
        // Always sample gsattract (731) and player overlays more densely.
        const bool hot = (overlay_index == 731u || overlay_index == 419u ||
                          demo_clip || facing_clip);
        if (!changed && !due && !hot) {
            return;
        }
        if (!changed && hot &&
            (now - s_last < std::chrono::milliseconds((demo_clip || facing_clip) ? 50 : 80))) {
            return;
        }
        s_last = now;
        s_last_mode = mode_v;
        s_last_map = map_v;

        auto bits_f32 = [](uint32_t bits) {
            float f = 0.0f;
            std::memcpy(&f, &bits, sizeof(f));
            return f;
        };
        auto guest_ok = [](uint32_t addr) {
            return ((addr & 0x1FFFFFFFu) < 0x00800000u) && ((addr & 3u) == 0u) &&
                   (addr >= 0x80000000u);
        };

        const uint32_t warp = uint32_t(MEM_BU(0x12, mode));
        const int entr = int(MEM_B(0x13, mode));
        const uint32_t a0 = uint32_t(MEM_BU(0x0, attr));
        const uint32_t a1 = uint32_t(MEM_BU(0x1, attr));
        const uint32_t a2 = uint32_t(MEM_BU(0x2, attr));
        const uint32_t a3 = uint32_t(MEM_BU(0x3, attr));
        const uint32_t stage = uint32_t(MEM_BU(0x0, stg));
        const uint32_t sub = uint32_t(MEM_BU(0x1, stg));
        const uint32_t timer_bits = uint32_t(MEM_W(0x4, stg));

        float px = 0, py = 0, pz = 0;
        float yaw = 0, yaw_ideal = 0;
        float sx = 0, sy = 0, sang = 0, sdist = 0;
        uint32_t ps_addr = 0, pos_addr = 0, yaw_addr = 0, stick_addr = 0;
        const gpr plist = static_cast<gpr>(static_cast<int32_t>(0x80135490u));
        ps_addr = uint32_t(MEM_W(0, plist));
        if (guest_ok(ps_addr)) {
            const gpr ps = static_cast<gpr>(static_cast<int32_t>(ps_addr));
            pos_addr = uint32_t(MEM_W(0xE4, ps));
            yaw_addr = uint32_t(MEM_W(0xF8, ps));
            stick_addr = uint32_t(MEM_W(0x128, ps));
            if (guest_ok(pos_addr)) {
                const gpr pos = static_cast<gpr>(static_cast<int32_t>(pos_addr));
                px = bits_f32(uint32_t(MEM_W(0x0, pos)));
                py = bits_f32(uint32_t(MEM_W(0x4, pos)));
                pz = bits_f32(uint32_t(MEM_W(0x8, pos)));
            }
            if (guest_ok(yaw_addr)) {
                const gpr yw = static_cast<gpr>(static_cast<int32_t>(yaw_addr));
                yaw = bits_f32(uint32_t(MEM_W(0x0, yw)));
                yaw_ideal = bits_f32(uint32_t(MEM_W(0x4, yw)));
            }
            if (guest_ok(stick_addr)) {
                const gpr st = static_cast<gpr>(static_cast<int32_t>(stick_addr));
                sx = bits_f32(uint32_t(MEM_W(0x54, st)));
                sy = bits_f32(uint32_t(MEM_W(0x58, st)));
                sang = bits_f32(uint32_t(MEM_W(0x5C, st)));
                sdist = bits_f32(uint32_t(MEM_W(0x60, st)));
            }
        }

        std::fprintf(stderr,
                     "[tooie-move] %s ovl=%u ep=%u gfx=%llu "
                     "mode=%d warp=%u map=%d entr=%d "
                     "attr=%u/%u/%u/%u stage=%u sub=%u t=%g "
                     "ps=%08x pos=%08x xyz=%.3f,%.3f,%.3f "
                     "yaw=%.2f ideal=%.2f stick=%.3f,%.3f ang=%.2f dist=%.3f\n",
                     tag, overlay_index, entry_index,
                     static_cast<unsigned long long>(tooie_diag::gfx_ticks()),
                     mode_v, warp, map_v, entr,
                     a0, a1, a2, a3, stage, sub, bits_f32(timer_bits),
                     ps_addr, pos_addr, px, py, pz,
                     yaw, yaw_ideal, sx, sy, sang, sdist);
        if (tooie_diag::demo_path_enabled() && mode_v == 5) {
            const gpr pad = static_cast<gpr>(static_cast<int32_t>(0x80079B40u));
            const int8_t psx = int8_t(MEM_B(2, pad));
            const int8_t psy = int8_t(MEM_B(3, pad));
            const uint16_t pbuttons = uint16_t(MEM_HU(0, pad));
            const gpr remap = static_cast<gpr>(static_cast<int32_t>(0x80079AF8u));
            const float rx = bits_f32(uint32_t(MEM_W(8, remap)));
            const float ry = bits_f32(uint32_t(MEM_W(12, remap)));
            float vx = 0, vy = 0, vz = 0, tgt_spd = 0;
            float gx = 0, gy = 0, gz = 0;
            uint32_t phys_addr = 0;
            uint32_t bs_addr = 0;
            int32_t bs_id = -1;
            static float s_px = 0, s_pz = 0;
            static bool s_have_prev = false;
            float dx = 0, dz = 0, dist = 0, head = 0;
            if (guest_ok(ps_addr)) {
                const gpr ps = static_cast<gpr>(static_cast<int32_t>(ps_addr));
                phys_addr = uint32_t(MEM_W(0xC8, ps));
                bs_addr = uint32_t(MEM_W(0x120, ps));
                if (guest_ok(phys_addr)) {
                    const gpr ph = static_cast<gpr>(static_cast<int32_t>(phys_addr));
                    vx = bits_f32(uint32_t(MEM_W(0x10, ph)));
                    vy = bits_f32(uint32_t(MEM_W(0x14, ph)));
                    vz = bits_f32(uint32_t(MEM_W(0x18, ph)));
                    tgt_spd = bits_f32(uint32_t(MEM_W(0x3C, ph)));
                    gx = bits_f32(uint32_t(MEM_W(0x64, ph)));
                    gy = bits_f32(uint32_t(MEM_W(0x68, ph)));
                    gz = bits_f32(uint32_t(MEM_W(0x6C, ph)));
                }
                if (guest_ok(bs_addr)) {
                    bs_id = int32_t(MEM_W(0, static_cast<gpr>(static_cast<int32_t>(bs_addr))));
                }
            }
            if (s_have_prev) {
                dx = px - s_px;
                dz = pz - s_pz;
                dist = std::sqrt(dx * dx + dz * dz);
                if (dist > 1.0e-4f) {
                    head = std::atan2(dx, dz) * (180.0f / 3.14159265f);
                    if (head < 0.0f) {
                        head += 360.0f;
                    }
                }
            }
            s_px = px;
            s_pz = pz;
            s_have_prev = true;
            std::fprintf(stderr,
                         "[tooie-demopath] sample gfx=%llu map=%d "
                         "xyz=%.3f,%.3f,%.3f dlt=%.3f,%.3f dist=%.3f head=%.2f "
                         "yaw=%.2f ideal=%.2f vel=%.3f,%.3f,%.3f tspd=%.3f "
                         "goto=%.3f,%.3f,%.3f pad=(%d,%d) btn=%04x remap=(%.3f,%.3f) "
                         "stick=%.3f,%.3f ang=%.2f distick=%.3f bs=%d phys=%08x\n",
                         static_cast<unsigned long long>(tooie_diag::gfx_ticks()),
                         map_v, px, py, pz, dx, dz, dist, head,
                         yaw, yaw_ideal, vx, vy, vz, tgt_spd,
                         gx, gy, gz, int(psx), int(psy), unsigned(pbuttons),
                         rx, ry, sx, sy, sang, sdist, bs_id, phys_addr);
        }
        if (tooie_diag::input_enabled()) {
            float hx = 0.0f, hy = 0.0f;
            tooie_diag::last_host_input(&hx, &hy);
            const gpr pad = static_cast<gpr>(static_cast<int32_t>(0x80079B40u));
            const int8_t psx = int8_t(MEM_B(2, pad));
            const int8_t psy = int8_t(MEM_B(3, pad));
            const uint16_t pbuttons = uint16_t(MEM_HU(0, pad));
            const uint32_t scale = uint32_t(MEM_W(0, static_cast<gpr>(static_cast<int32_t>(0x8007DB54u))));
            const gpr remap = static_cast<gpr>(static_cast<int32_t>(0x80079AF8u));
            const float rx = bits_f32(uint32_t(MEM_W(8, remap)));
            const float ry = bits_f32(uint32_t(MEM_W(12, remap)));
            // Camera yaw from D_80127708[camIdx] + 0x1C
            float cam_yaw = -999.0f;
            uint32_t cam_ptr = 0;
            uint32_t cam_idx = 0;
            if (guest_ok(ps_addr)) {
                const gpr ps = static_cast<gpr>(static_cast<int32_t>(ps_addr));
                cam_idx = uint32_t(MEM_W(0x184, ps));
                const gpr cam_tab = static_cast<gpr>(static_cast<int32_t>(0x80127708u));
                cam_ptr = uint32_t(MEM_W(cam_idx * 4, cam_tab));
            }
            if (guest_ok(cam_ptr)) {
                const gpr cp = static_cast<gpr>(static_cast<int32_t>(cam_ptr));
                cam_yaw = bits_f32(uint32_t(MEM_W(0x1C, cp)));
            }
            std::fprintf(stderr,
                         "[tooie-input] %s host=(%.3f,%.3f) pad=(%d,%d) "
                         "buttons=%04x scale=%08x remap=(%.3f,%.3f) "
                         "guest=(%.3f,%.3f) dist=%.3f "
                         "stick_angle=%.2f cam_yaw=%.2f desired_yaw=%.2f "
                         "cam_ptr=%08x cam_idx=%u "
                         "pos=(%.3f,%.3f,%.3f)\n",
                         tag, hx, hy, int(psx), int(psy), unsigned(pbuttons),
                         scale, rx, ry, sx, sy, sdist,
                         sang, cam_yaw, yaw_ideal,
                         cam_ptr, cam_idx,
                         px, py, pz);
            // Direction pipeline trace line — one per call, no throttle.
            if (tooie_diag::direction_enabled()) {
                std::fprintf(stderr,
                             "[tooie-dir] %s stick=(%.3f,%.3f) angle=%.2f "
                             "cam_yaw=%.2f desired_yaw=%.2f actual_yaw=%.2f "
                             "pos=(%.3f,%.3f,%.3f)\n",
                             tag, sx, sy, sang,
                             cam_yaw, yaw_ideal, yaw,
                             px, py, pz);
            }
        }

        // Dump actors vector D_80136EE0
        const gpr actors_vec_sym = static_cast<gpr>(static_cast<int32_t>(0x80136EE0u));
        const uint32_t vec_addr = uint32_t(MEM_W(0, actors_vec_sym));
        if (guest_ok(vec_addr)) {
            const gpr vec = static_cast<gpr>(static_cast<int32_t>(vec_addr));
            const uint32_t elem_size = uint32_t(MEM_W(0x00, vec));
            const uint32_t begin = uint32_t(MEM_W(0x04, vec));
            const uint32_t end = uint32_t(MEM_W(0x08, vec));
            if (elem_size == 0x9C && guest_ok(begin) && guest_ok(end) && end >= begin) {
                const uint32_t count = (end - begin) / elem_size;
                struct FacingPrev {
                    uint32_t addr;
                    float x, z, yaw;
                    uint64_t last;
                    bool have;
                };
                static FacingPrev s_face[32]{};
                const bool dump_all = tooie_diag::movement_enabled();
                const bool dump_face = tooie_diag::forced_facing_enabled();
                auto wrap360 = [](float a) {
                    a = std::fmod(a, 360.0f);
                    if (a < 0.0f) {
                        a += 360.0f;
                    }
                    return a;
                };
                auto shortest = [&](float a, float b) {
                    float d = wrap360(b) - wrap360(a);
                    if (d > 180.0f) {
                        d -= 360.0f;
                    }
                    if (d < -180.0f) {
                        d += 360.0f;
                    }
                    return d;
                };
                for (uint32_t i = 0; i < count && i < 40; ++i) {
                    const uint32_t act_addr = begin + i * 0x9C;
                    const gpr act = static_cast<gpr>(static_cast<int32_t>(act_addr));
                    const float ax = bits_f32(uint32_t(MEM_W(0x04, act)));
                    const float ay = bits_f32(uint32_t(MEM_W(0x08, act)));
                    const float az = bits_f32(uint32_t(MEM_W(0x0C, act)));
                    const float rx = bits_f32(uint32_t(MEM_W(0x44, act)));
                    const float ry = bits_f32(uint32_t(MEM_W(0x48, act)));
                    const float rz = bits_f32(uint32_t(MEM_W(0x4C, act)));
                    const float sc = bits_f32(uint32_t(MEM_W(0x38, act)));
                    const float unk54 = bits_f32(uint32_t(MEM_W(0x54, act)));
                    const uint32_t sys_entry = uint32_t(MEM_W(0x10, act));
                    const uint32_t unk14 = uint32_t(MEM_W(0x14, act));
                    const uint32_t d0 = uint32_t(MEM_W(0x18, act));
                    const uint32_t d1 = uint32_t(MEM_W(0x1C, act));
                    const uint32_t unk74 = uint32_t(MEM_W(0x74, act));
                    if (dump_all) {
                        std::fprintf(stderr,
                                     "[tooie-actor] i=%u act=%08x pos=%.2f,%.2f,%.2f rot=%.2f,%.2f,%.2f scale=%.3f sys=%08x u14=%08x d0=%08x d1=%08x u74=%08x\n",
                                     i, act_addr, ax, ay, az, rx, ry, rz, sc, sys_entry, unk14, d0, d1, unk74);
                    }
                    if (dump_face) {
                        // Visible scripted characters. Skip blob shadows and
                        // Banjo's Actor.rotation.y=0 player slot.
                        const bool intro = (sys_entry == 0x80086C98u);
                        const bool digger_g = (sys_entry == 0x80085C98u);
                        const bool digger_f = (sys_entry == 0x80085CA8u);
                        const bool skip = (sys_entry == 0x80085768u);
                        FacingPrev* prev = nullptr;
                        FacingPrev* empty = nullptr;
                        FacingPrev* oldest = nullptr;
                        const uint64_t now_tick = tooie_diag::gfx_ticks();
                        for (auto& slot : s_face) {
                            if (slot.have && slot.addr == act_addr) {
                                prev = &slot;
                                break;
                            }
                            if (!slot.have && empty == nullptr) {
                                empty = &slot;
                            }
                            if (slot.have &&
                                (oldest == nullptr || slot.last < oldest->last)) {
                                oldest = &slot;
                            }
                        }
                        if (prev == nullptr) {
                            prev = (empty != nullptr) ? empty : oldest;
                            if (prev != nullptr) {
                                *prev = {act_addr, ax, az, ry, now_tick, true};
                            }
                        } else {
                            prev->last = now_tick;
                        }
                        float dx = 0, dz = 0, dist = 0, head = 0, dlt = 0;
                        float dyaw = 0, raw_dyaw = 0;
                        bool moved = false;
                        if (prev != nullptr && prev->addr == act_addr) {
                            dx = ax - prev->x;
                            dz = az - prev->z;
                            dist = std::sqrt(dx * dx + dz * dz);
                            raw_dyaw = ry - prev->yaw;
                            dyaw = shortest(prev->yaw, ry);
                            if (dist > 0.4f) {
                                moved = true;
                                head = wrap360(std::atan2(dx, dz) * (180.0f / 3.14159265f));
                                dlt = std::fabs(shortest(head, ry));
                            }
                            prev->x = ax;
                            prev->z = az;
                            prev->yaw = ry;
                        }
                        const bool interesting =
                            !skip && (intro || digger_g || digger_f) &&
                            (moved || std::fabs(dyaw) > 5.0f);
                        if (interesting) {
                            std::fprintf(stderr,
                                         "[facing] tick=%llu mode=%d map=%d i=%u actor=%08x "
                                         "sys=%08x state=%08x pos=%.2f,%.2f,%.2f dpos=%.2f,%.2f "
                                         "move_head=%.2f yaw=%.2f unk54=%.2f pitch=%.2f "
                                         "raw_delta=%.2f wrapped_delta=%.2f "
                                         "move_vs_yaw=%.2f anim=%08x\n",
                                         static_cast<unsigned long long>(tooie_diag::gfx_ticks()),
                                         mode_v, map_v, i, act_addr, sys_entry, unk74,
                                         ax, ay, az, dx, dz, head, ry, unk54, rx,
                                         raw_dyaw, dyaw, dlt, d1);
                        }
                    }
                }
            }
        }

        // Camera in D_80127728[0] and ps->0x184
        uint32_t cam_addr = 0;
        if (guest_ok(ps_addr)) {
            const gpr ps = static_cast<gpr>(static_cast<int32_t>(ps_addr));
            uint32_t cam_idx = uint32_t(MEM_W(0x184, ps));
            const gpr cam_tab = static_cast<gpr>(static_cast<int32_t>(0x80127728u));
            cam_addr = uint32_t(MEM_W(cam_idx * 4, cam_tab));
        }
        if (!guest_ok(cam_addr)) {
            const gpr cam_tab = static_cast<gpr>(static_cast<int32_t>(0x80127728u));
            cam_addr = uint32_t(MEM_W(0, cam_tab));
        }
        if (guest_ok(cam_addr)) {
            const gpr cam = static_cast<gpr>(static_cast<int32_t>(cam_addr));
            uint32_t sub_cam = uint32_t(MEM_W(0x04, cam));
            if (guest_ok(sub_cam)) {
                const gpr scam = static_cast<gpr>(static_cast<int32_t>(sub_cam));
                float cx = bits_f32(uint32_t(MEM_W(0x74, scam)));
                float cy = bits_f32(uint32_t(MEM_W(0x78, scam)));
                float cz = bits_f32(uint32_t(MEM_W(0x7C, scam)));
                float rx = bits_f32(uint32_t(MEM_W(0x80, scam)));
                float ry = bits_f32(uint32_t(MEM_W(0x84, scam)));
                float rz = bits_f32(uint32_t(MEM_W(0x88, scam)));
                uint32_t target = uint32_t(MEM_W(0x64, scam));
                float tx = 0, ty = 0, tz = 0;
                if (guest_ok(target)) {
                    const gpr tgt = static_cast<gpr>(static_cast<int32_t>(target));
                    tx = bits_f32(uint32_t(MEM_W(0x00, tgt)));
                    ty = bits_f32(uint32_t(MEM_W(0x04, tgt)));
                    tz = bits_f32(uint32_t(MEM_W(0x08, tgt)));
                }
                std::fprintf(stderr,
                             "[tooie-cam] cam=%08x scam=%08x eye=%.2f,%.2f,%.2f rot=%.2f,%.2f,%.2f target=%08x tgt_pos=%.2f,%.2f,%.2f\n",
                             cam_addr, sub_cam, cx, cy, cz, rx, ry, rz, target, tx, ty, tz);
            }
        }

        // Viewport in D_8012D500
        const gpr vp_sym = static_cast<gpr>(static_cast<int32_t>(0x8012D500u));
        const uint32_t vp_addr = uint32_t(MEM_W(0, vp_sym));
        if (guest_ok(vp_addr)) {
            const gpr vp = static_cast<gpr>(static_cast<int32_t>(vp_addr));
            const float vx = bits_f32(uint32_t(MEM_W(0x00, vp)));
            const float vy = bits_f32(uint32_t(MEM_W(0x04, vp)));
            const float vz = bits_f32(uint32_t(MEM_W(0x08, vp)));
            const float vscale = bits_f32(uint32_t(MEM_W(0x164, vp)));
            float vm[16]{};
            for (int k = 0; k < 16; ++k) {
                vm[k] = bits_f32(uint32_t(MEM_W(0xA4 + k * 4, vp)));
            }
            std::fprintf(stderr,
                         "[tooie-vp] vp=%08x trans=%.2f,%.2f,%.2f scale=%.3f\n"
                         "  rot_mtx: [%.4f %.4f %.4f] [%.4f %.4f %.4f] [%.4f %.4f %.4f]\n",
                         vp_addr, vx, vy, vz, vscale,
                         vm[0], vm[1], vm[2],
                         vm[4], vm[5], vm[6],
                         vm[8], vm[9], vm[10]);
        }

        // Viewproj in D_8012D558
        const gpr vp_mtx_sym = static_cast<gpr>(static_cast<int32_t>(0x8012D558u));
        const uint32_t vpmtx_addr = uint32_t(MEM_W(0, vp_mtx_sym));
        if (guest_ok(vpmtx_addr)) {
            std::fprintf(stderr, "[tooie-viewproj] addr=%08x\n", vpmtx_addr);
        }
        std::fflush(stderr);
    };
    dump_movement("enter");
    if (tooie_diag::shot_enabled()) {
        tooie_shot_note_overlay(rdram, ctx, overlay_index, entry_index, 0);
    }

    func(rdram, ctx);

    // The attract gameplay demo (mode==5) consumes one recorded-pad sample per
    // glrecord overlay dispatch (overlay 698 entrypoint 0). This is the TRUE
    // recorded-input consumer / guest logic tick for the demo walk.
    if ((tooie_diag::demo_path_enabled() || tooie_diag::shot_enabled() || tooie_diag::demo_sync_enabled()) &&
        overlay_index == 698u && entry_index == 0u) {
        const uint64_t tick = tooie_diag::note_guest_logic_tick();
        if (tooie_diag::demo_path_enabled()) {
            tooie_demo_path_tick(rdram, tick);
        }
        if (tooie_diag::shot_enabled()) {
            tooie_shot_tick(rdram, tick);
        }
        if (tooie_diag::demo_sync_enabled()) {
            tooie_demo_sync_tick(rdram, tick);
        }
    }
    if (tooie_diag::shot_enabled()) {
        tooie_shot_note_overlay(rdram, ctx, overlay_index, entry_index, 1);
    }

    dump_opening("leave");
    dump_movement("leave");

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

    // Diagnostic spawn on MAP_18B_CS_STARTUP_INTRO, OPT-IN via
    // TOOIE_DIAG_SPAWN_MARKER419=1. Default OFF: it alters the natural flow
    // (proved to be an artifact earlier — the actor vector crash). The
    // regression case is preserved behind the env var.
    // NOTE: func_80108C90 receives a MARKER ID, not an overlay index.
    // Marker 419 == chcollballcontrol (overlay 467) — NOT chintroticker
    // (chintroticker overlay index is 419, but its marker id is 569).
    // 419 is kept deliberately as the relocation regression case for P08;
    // do NOT change to 569 in this stage.
    static const bool s_intro_spawn_enabled = [] {
        const char* raw = std::getenv("TOOIE_DIAG_SPAWN_MARKER419");
        if (raw == nullptr || raw[0] == '\0') return false;
        try {
            return std::stol(raw) != 0;
        } catch (...) {
            return true; // any non-numeric value is treated as ON
        }
    }();
    static bool s_intro_ticker_spawned = false;
    if (s_intro_spawn_enabled && overlay_index == 677u && entry_index == 4u && !s_intro_ticker_spawned) {
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
