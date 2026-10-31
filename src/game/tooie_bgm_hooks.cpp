#include "tooie_bgm_hooks.h"

#include "recomp.h"
#include "tooie_bgm_volume.h"

extern "C" void func_80025ED0(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_800FB968(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_80016ED0(uint8_t* rdram, recomp_context* ctx);

namespace {
tooie_bgm::GainFilter gain_filter = nullptr;
tooie_bgm::FrameObserver frame_observer = nullptr;
tooie_bgm::SequenceObserver sequence_observer = nullptr;
tooie_bgm::GainObserver gain_observer = nullptr;

gpr kseg0(uint32_t addr) {
    return static_cast<gpr>(static_cast<int32_t>(addr));
}

gpr s16_gpr(int16_t value) {
    return static_cast<gpr>(static_cast<int32_t>(value));
}

} // namespace

void tooie_bgm::set_content_hooks(GainFilter gain, FrameObserver frame, SequenceObserver sequence, GainObserver posted) {
    gain_filter = gain; frame_observer = frame; sequence_observer = sequence;
    gain_observer = posted;
}

extern "C" void tooie_bgm_on_sequence_start(uint8_t* rdram, recomp_context* ctx) {
    const unsigned slot = static_cast<uint8_t>(ctx->r4);
    const int sequence = static_cast<int32_t>(ctx->r5);
    func_80016ED0(rdram, ctx);
    if (sequence_observer && slot < tooie_bgm::kSlotCount) sequence_observer(rdram, slot, sequence);
}

extern "C" void tooie_bgm_on_csp_set_vol(uint8_t* rdram, recomp_context* ctx) {
    const int slot = tooie_bgm::slot_for_seqp(static_cast<uint32_t>(ctx->r4));
    if (slot >= 0) {
        const int16_t authored = static_cast<int16_t>(ctx->r5);
        int16_t applied = tooie_bgm::scale_authored(slot, authored);
        if (gain_filter) applied = gain_filter(rdram, slot, authored, applied);
        ctx->r5 = s16_gpr(applied);
    }
    const int16_t posted = static_cast<int16_t>(ctx->r5);
    func_80025ED0(rdram, ctx);
    if (slot >= 0 && gain_observer) gain_observer(rdram, slot, posted);
}

extern "C" void tooie_bgm_on_update(uint8_t* rdram, recomp_context* ctx) {
    func_800FB968(rdram, ctx);
    if (frame_observer) frame_observer(rdram, ctx);
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
        local.r5 = s16_gpr(gain_filter ? gain_filter(rdram, slot, unscaled, applied) : applied);
        const int16_t posted = static_cast<int16_t>(local.r5);
        func_80025ED0(rdram, &local);
        if (gain_observer) gain_observer(rdram, slot, posted);
    }
}
