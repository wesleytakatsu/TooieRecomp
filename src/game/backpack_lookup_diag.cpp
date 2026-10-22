#include <cstdint>
#include <cstring>

#include "recomp.h"
#include "funcs.h"
#include "tooie_diag.h"

extern "C" void __real_func_800D674C(uint8_t* rdram, recomp_context* ctx);
extern "C" void __real_func_800DE448(uint8_t* rdram, recomp_context* ctx);
extern "C" void __real_func_800DE498(uint8_t* rdram, recomp_context* ctx);
extern "C" void __real_func_800E2AA4(uint8_t* rdram, recomp_context* ctx);
extern "C" void __real_func_800DE2A4(uint8_t* rdram, recomp_context* ctx);
extern "C" void __real_func_800DCF48(uint8_t* rdram, recomp_context* ctx);
extern "C" void __real_func_800ADE2C(uint8_t* rdram, recomp_context* ctx);
extern "C" void __real_func_80019AA0(uint8_t* rdram, recomp_context* ctx);
extern "C" void __real_mlMtxApply(uint8_t* rdram, recomp_context* ctx);
extern "C" void __real_func_800E2180(uint8_t* rdram, recomp_context* ctx);
extern "C" void __real_babackpack_entrypoint_3(uint8_t* rdram, recomp_context* ctx);
extern "C" void __real_func_80098BA8(uint8_t* rdram, recomp_context* ctx);
extern "C" void __real_func_80092444(uint8_t* rdram, recomp_context* ctx);
extern "C" void __real_func_80092B04(uint8_t* rdram, recomp_context* ctx);
extern "C" void __real_func_800E0B8C(uint8_t* rdram, recomp_context* ctx);
extern "C" void __real_func_800E0E94(uint8_t* rdram, recomp_context* ctx);
extern "C" void __real_func_800AE8A8(uint8_t* rdram, recomp_context* ctx);
extern "C" void __real_func_800AE820(uint8_t* rdram, recomp_context* ctx);

// __real_<f> is provided by the linker (--wrap on ELF, lld-link /wrap on Windows); list: TOOIE_WRAPPED_SYMBOLS in CMakeLists.txt.

namespace {

uint32_t guest_word(uint8_t* rdram, uint32_t guest_ptr, uint32_t offset) {
    if (rdram == nullptr) {
        return 0;
    }
    const uint32_t physical = (guest_ptr + offset) & 0x1FFFFFFFu;
    if (physical > 0x007FFFFCu) {
        return 0;
    }
    uint32_t value = 0;
    std::memcpy(&value, rdram + physical, sizeof(value));
    return value;
}

} // namespace

extern "C" void __wrap_func_800D674C(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t asset_id = ctx != nullptr ? static_cast<uint32_t>(ctx->r4) : 0;
    __real_func_800D674C(rdram, ctx);
    if (tooie_diag::backpack_flap_enabled() && ctx != nullptr) {
        tooie_diag::backpack_note_asset_lookup(asset_id, static_cast<uint32_t>(ctx->r2));
    }
}

extern "C" void __wrap_func_80098BA8(uint8_t* rdram, recomp_context* ctx) {
    if (tooie_diag::secondary_sway_enabled() && ctx != nullptr) {
        tooie_diag::secondary_sway_note_98ba8(rdram, static_cast<uint32_t>(ctx->r4));
    }
    __real_func_80098BA8(rdram, ctx);
}

extern "C" void __wrap_func_80092444(uint8_t* rdram, recomp_context* ctx) {
    bool scoped = false;
    if (tooie_diag::secondary_sway_enabled() && ctx != nullptr) {
        scoped = tooie_diag::secondary_sway_player_draw_begin(
            rdram, static_cast<uint32_t>(ctx->r4));
    }
    __real_func_80092444(rdram, ctx);
    if (scoped) {
        tooie_diag::secondary_sway_player_draw_end();
    }
}

extern "C" void __wrap_func_80092B04(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t owner = ctx != nullptr ? static_cast<uint32_t>(ctx->r4) : 0;
    const uint32_t arg1 = ctx != nullptr ? static_cast<uint32_t>(ctx->r5) : 0;
    __real_func_80092B04(rdram, ctx);
    if (ctx != nullptr) {
        tooie_diag::v7_note_model_getter(
            rdram, owner, arg1, static_cast<uint64_t>(ctx->r2));
    }
    if (tooie_diag::secondary_sway_enabled() && ctx != nullptr) {
        tooie_diag::secondary_sway_note_model_getter(
            rdram, owner, arg1, static_cast<uint32_t>(ctx->r2));
    }
}

extern "C" void __wrap_babackpack_entrypoint_3(uint8_t* rdram, recomp_context* ctx) {
    if (tooie_diag::backpack_flap_enabled()) {
        tooie_diag::backpack_note_entry3();
    }
    __real_babackpack_entrypoint_3(rdram, ctx);
}

extern "C" void __wrap_func_800DE448(uint8_t* rdram, recomp_context* ctx) {
    if (tooie_diag::secondary_sway_enabled() && ctx != nullptr) {
        tooie_diag::secondary_sway_note_player_de448(rdram, ctx);
    }
    const bool enabled = tooie_diag::backpack_flap_enabled();
    uint32_t model_bin = 0;
    uint32_t model_id = 0;
    bool tracked = false;
    if (enabled && ctx != nullptr) {
        model_bin = guest_word(rdram, static_cast<uint32_t>(ctx->r29), 0x10);
        tracked = tooie_diag::backpack_tracked_model_id(model_bin, &model_id);
        tooie_diag::backpack_note_draw_entry(model_bin, model_id);
    }
    if (enabled) {
        tooie_diag::backpack_draw_begin(rdram, ctx, tracked ? model_id : 0, model_bin);
    }

    __real_func_800DE448(rdram, ctx);

    if (enabled) {
        tooie_diag::backpack_draw_end();
    }
}

extern "C" void __wrap_func_800DE498(uint8_t* rdram, recomp_context* ctx) {
    if (tooie_diag::backpack_flap_enabled()) {
        tooie_diag::backpack_note_de498();
    }
    const bool tree_scope = tooie_diag::secondary_sway_de498_begin(rdram, ctx);
    __real_func_800DE498(rdram, ctx);
    if (tree_scope) {
        tooie_diag::secondary_sway_de498_end();
    }
}

extern "C" void __wrap_func_800DE2A4(uint8_t* rdram, recomp_context* ctx) {
    if (tooie_diag::secondary_sway_enabled() && ctx != nullptr) {
        tooie_diag::secondary_sway_note_de2a4_entry();
        tooie_diag::secondary_sway_note_de2a4(rdram, ctx);
    }
    __real_func_800DE2A4(rdram, ctx);
}

extern "C" void __wrap_func_800DCF48(uint8_t* rdram, recomp_context* ctx) {
    bool snout_scope = false;
    const bool sway = tooie_diag::secondary_sway_enabled();
    const bool provenance = tooie_diag::p09_local_pair_provenance_enabled();
    if ((sway || provenance) && ctx != nullptr) {
        if (sway) {
            tooie_diag::secondary_sway_note_dcf48_entry();
        }
        snout_scope = tooie_diag::secondary_sway_note_dcf48(rdram, ctx);
    }
    __real_func_800DCF48(rdram, ctx);
    if (snout_scope) {
        tooie_diag::secondary_sway_dcf48_snout_end();
    }
}

extern "C" void __wrap_func_800ADE2C(uint8_t* rdram, recomp_context* ctx) {
    const bool snout_scope = (tooie_diag::secondary_sway_enabled() ||
                              tooie_diag::p09_local_pair_provenance_enabled()) &&
                             tooie_diag::secondary_sway_dcf48_snout_scope_active();
    __real_func_800ADE2C(rdram, ctx);
    if (snout_scope) {
        tooie_diag::secondary_sway_note_ade2c(rdram, ctx);
    }
}

extern "C" void __wrap_func_80019AA0(uint8_t* rdram, recomp_context* ctx) {
    const bool snout_scope = (tooie_diag::secondary_sway_enabled() ||
                              tooie_diag::p09_local_pair_provenance_enabled()) &&
                             tooie_diag::secondary_sway_dcf48_snout_scope_active() &&
                             ctx != nullptr;
    const uint32_t parent_ptr = snout_scope ? static_cast<uint32_t>(ctx->r4) : 0;
    const uint32_t local_ptr = snout_scope ? static_cast<uint32_t>(ctx->r5) : 0;
    __real_func_80019AA0(rdram, ctx);
    if (snout_scope) {
        tooie_diag::secondary_sway_note_19aa0(rdram, parent_ptr, local_ptr);
    }
}

extern "C" void __wrap_mlMtxApply(uint8_t* rdram, recomp_context* ctx) {
    const bool snout_scope = (tooie_diag::secondary_sway_enabled() ||
                              tooie_diag::p09_local_pair_provenance_enabled()) &&
                             tooie_diag::secondary_sway_dcf48_snout_scope_active() &&
                             ctx != nullptr;
    // mlMtxApply: a1 = entry a0 (Gfx Mtx dest / G_MTX operand);
    // a0 = MEM_W(D_8007BCB0) (float stack top); guMtxF2L(float, Mtx).
    const uint32_t gfx_mtx_ptr = snout_scope ? static_cast<uint32_t>(ctx->r4) : 0;
    const uint32_t float_src_ptr =
        snout_scope ? guest_word(rdram, 0x8007BCB0u, 0) : 0;
    __real_mlMtxApply(rdram, ctx);
    if (snout_scope) {
        tooie_diag::secondary_sway_note_18c28(rdram, gfx_mtx_ptr, float_src_ptr);
    }
}

extern "C" void __wrap_func_800E2AA4(uint8_t* rdram, recomp_context* ctx) {
    bool snout_scope = false;
    if (tooie_diag::secondary_sway_enabled() && ctx != nullptr) {
        tooie_diag::secondary_sway_note_e2aa4_entry();
        snout_scope = tooie_diag::secondary_sway_note_e2aa4(rdram, ctx);
    }
    if (tooie_diag::backpack_flap_enabled()) {
        tooie_diag::backpack_note_e2aa4();
    }
    __real_func_800E2AA4(rdram, ctx);
    if (snout_scope) {
        tooie_diag::secondary_sway_e2aa4_snout_end();
    }
}

extern "C" void __wrap_func_800E2180(uint8_t* rdram, recomp_context* ctx) {
    if (tooie_diag::secondary_sway_enabled() &&
        tooie_diag::secondary_sway_e2aa4_snout_scope_active()) {
        tooie_diag::secondary_sway_note_snout_dual_mtx_entry();
        tooie_diag::secondary_sway_note_snout_dual_mtx(rdram, ctx);
    }
    if (tooie_diag::backpack_flap_enabled()) {
        tooie_diag::backpack_note_dual_mtx(rdram, ctx);
    }
    __real_func_800E2180(rdram, ctx);
}

extern "C" void __wrap_func_800E0B8C(uint8_t* rdram, recomp_context* ctx) {
    uint32_t anim_list = 0;
    uint32_t bone_anim_id = 0;
    uint32_t arg2 = 0;
    uint32_t arg3 = 0;
    uint32_t arg4 = 0;
    uint32_t arg5 = 0;
    if (ctx != nullptr) {
        anim_list = static_cast<uint32_t>(ctx->r4);
        bone_anim_id = static_cast<uint32_t>(ctx->r5) & 0xFFu;
        arg2 = static_cast<uint32_t>(ctx->r6);
        arg3 = static_cast<uint32_t>(ctx->r7);
        arg4 = guest_word(rdram, static_cast<uint32_t>(ctx->r29), 0x10);
        arg5 = guest_word(rdram, static_cast<uint32_t>(ctx->r29), 0x14);
    }
    if (tooie_diag::secondary_sway_enabled()) {
        tooie_diag::secondary_sway_note_bone_anim_entry();
    }
    __real_func_800E0B8C(rdram, ctx);
    if (tooie_diag::secondary_sway_enabled() && ctx != nullptr &&
        (bone_anim_id == 3u || bone_anim_id == 42u || bone_anim_id == 79u ||
         (bone_anim_id >= 100u && bone_anim_id <= 105u))) {
        tooie_diag::secondary_sway_note_bone_anim(
            rdram, anim_list, bone_anim_id, arg2, arg3, arg4, arg5);
    }
}

extern "C" void __wrap_func_800E0E94(uint8_t* rdram, recomp_context* ctx) {
    if (tooie_diag::secondary_sway_enabled()) {
        tooie_diag::secondary_sway_note_hip_consume_entry();
        tooie_diag::secondary_sway_note_hip_consume(rdram, ctx);
    }
    __real_func_800E0E94(rdram, ctx);
}

extern "C" void __wrap_func_800AE8A8(uint8_t* rdram, recomp_context* ctx) {
    // Keep full GPRs: MEM_* subtracts 0xFFFFFFFF80000000, so KSEG0
    // pointers must stay sign-extended. Truncating to uint32_t zero-extends
    // and crashes in AEAA0/AEBA8 (core 3120935).
    const gpr a0 = ctx != nullptr ? ctx->r4 : 0;
    const gpr a1 = ctx != nullptr ? ctx->r5 : 0;
    const gpr a2 = ctx != nullptr ? ctx->r6 : 0;
    const gpr a3 = ctx != nullptr ? ctx->r7 : 0;
    if (tooie_diag::feather_anim_sample_time_enabled()) {
        tooie_diag::feather_anim_note_ae8a8_before_compose(
            rdram, static_cast<uint32_t>(a0), static_cast<uint32_t>(a1),
            static_cast<uint32_t>(a2), static_cast<uint32_t>(a3));
    }
    __real_func_800AE8A8(rdram, ctx);
    if (tooie_diag::secondary_sway_enabled() && ctx != nullptr) {
        ctx->r4 = a0;
        ctx->r5 = a1;
        ctx->r6 = a2;
        ctx->r7 = a3;
        tooie_diag::secondary_sway_note_ae8a8(
            rdram, static_cast<uint32_t>(a0), static_cast<uint32_t>(a1),
            static_cast<uint32_t>(a2), static_cast<uint32_t>(a3));
    }
}

extern "C" void __wrap_func_800AE820(uint8_t* rdram, recomp_context* ctx) {
    const gpr a0 = ctx != nullptr ? ctx->r4 : 0;
    const gpr a1 = ctx != nullptr ? ctx->r5 : 0;
    const gpr a2 = ctx != nullptr ? ctx->r6 : 0;
    const gpr a3 = ctx != nullptr ? ctx->r7 : 0;
    __real_func_800AE820(rdram, ctx);
    if (tooie_diag::secondary_sway_enabled()) {
        tooie_diag::secondary_sway_note_ae820(
            rdram, static_cast<uint32_t>(a0), static_cast<uint32_t>(a1),
            static_cast<uint32_t>(a2), static_cast<uint32_t>(a3));
    }
}
