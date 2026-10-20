#pragma once

#include <cstdint>

// BGM gain for the six sequence slots. Guest thread reads the atomic only.
//
// Wrap kCspSetVolVram (80025ED0), not 80017404. 17404 already stored the
// authored s16 at player+0 before the jal, and 80016ED0 calls setvol directly.
// Known seqp: replace a1 with apply_unscaled(authored) once, then call the
// original func_80025ED0 symbol (not the lookup). Unknown seqp: unchanged.
// After original kUpdateVram (800FB968), for track_id >= 0 read player+0 and
// post the original symbol from a copied recomp_context only when the gain
// changed. Steady 100 adds no event. track_id < 0 clears that slot.
// UI calls set_percent from the option callback. Do not call
// banjo::get_bgm_volume on the guest thread.
// Song-id vs voice is still unclassified. Do not filter ids here.

namespace tooie_bgm {

constexpr unsigned kSlotCount = 6;
constexpr uint32_t kTrackBase = 0x801357D0u;
constexpr uint32_t kTrackStride = 0x50u;
constexpr uint32_t kTrackIdOff = 0x28u;
constexpr uint32_t kPlayerBase = 0x80079FA0u;
constexpr uint32_t kPlayerStride = 0x274u;
constexpr uint32_t kPlayerSeqOff = 0x1E0u;
constexpr uint32_t kCspSetVolVram = 0x80025ED0u;
constexpr uint32_t kUpdateVram = 0x800FB968u;

constexpr uint32_t player_addr(unsigned slot) {
    return kPlayerBase + slot * kPlayerStride;
}

constexpr uint32_t seqp_addr(unsigned slot) {
    return player_addr(slot) + kPlayerSeqOff;
}

constexpr uint32_t track_addr(unsigned slot) {
    return kTrackBase + slot * kTrackStride;
}

// Clamp config numbers before integer conversion; NaN mutes rather than overflowing.
void set_percent(double percent);
int get_percent();

int16_t apply_unscaled(int16_t unscaled, int percent);

int slot_for_seqp(uint32_t seqp);
int16_t scale_authored(int slot, int16_t authored);
bool take_reapply(int slot, int16_t unscaled, int16_t& applied);
void clear_slot(unsigned slot);
void reset_slots();

}
