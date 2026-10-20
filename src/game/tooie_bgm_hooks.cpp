#include "tooie_bgm_hooks.h"

#include "recomp.h"
#include "tooie_bgm_volume.h"

extern "C" void func_80025ED0(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_800FB968(uint8_t* rdram, recomp_context* ctx);

namespace {

gpr kseg0(uint32_t addr) {
    return static_cast<gpr>(static_cast<int32_t>(addr));
}

gpr s16_gpr(int16_t value) {
    return static_cast<gpr>(static_cast<int32_t>(value));
}

} // namespace

extern "C" void tooie_bgm_on_csp_set_vol(uint8_t* rdram, recomp_context* ctx) {
    const int slot = tooie_bgm::slot_for_seqp(static_cast<uint32_t>(ctx->r4));
    if (slot >= 0) {
        const int16_t authored = static_cast<int16_t>(ctx->r5);
        ctx->r5 = s16_gpr(tooie_bgm::scale_authored(slot, authored));
    }
    func_80025ED0(rdram, ctx);
}

extern "C" void tooie_bgm_on_update(uint8_t* rdram, recomp_context* ctx) {
    func_800FB968(rdram, ctx);
    for (unsigned slot = 0; slot < tooie_bgm::kSlotCount; ++slot) {
        const gpr track = kseg0(tooie_bgm::track_addr(slot) + tooie_bgm::kTrackIdOff);
        if (MEM_H(0, track) < 0) {
            tooie_bgm::clear_slot(slot);
            continue;
        }
        const int16_t unscaled = MEM_H(0, kseg0(tooie_bgm::player_addr(slot)));
        int16_t applied = 0;
        if (!tooie_bgm::take_reapply(static_cast<int>(slot), unscaled, applied)) {
            continue;
        }
        recomp_context local = *ctx;
        local.r4 = kseg0(tooie_bgm::seqp_addr(slot));
        local.r5 = s16_gpr(applied);
        func_80025ED0(rdram, &local);
    }
}
