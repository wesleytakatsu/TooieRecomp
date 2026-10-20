#include "tooie_bgm_volume.h"

#include <atomic>

namespace tooie_bgm {
namespace {

struct Posted {
    bool have = false;
    int16_t unscaled = 0;
    int percent = 100;
};

std::atomic<int> g_percent{100};
Posted g_slots[kSlotCount];

int clamp_percent(int percent) {
    if (percent < 0) {
        return 0;
    }
    if (percent > 100) {
        return 100;
    }
    return percent;
}

} // namespace

void set_percent(double percent) {
    const int bounded = percent >= 100.0 ? 100 : (percent > 0.0 ? static_cast<int>(percent) : 0);
    g_percent.store(bounded, std::memory_order_relaxed);
}

int get_percent() {
    return g_percent.load(std::memory_order_relaxed);
}

int16_t apply_unscaled(int16_t unscaled, int percent) {
    percent = clamp_percent(percent);
    if (percent == 0 || unscaled == 0) {
        return 0;
    }
    if (percent == 100) {
        return unscaled;
    }
    return static_cast<int16_t>(static_cast<int32_t>(unscaled) * percent / 100);
}

int slot_for_seqp(uint32_t seqp) {
    for (unsigned slot = 0; slot < kSlotCount; ++slot) {
        if (seqp == seqp_addr(slot)) {
            return static_cast<int>(slot);
        }
    }
    return -1;
}

int16_t scale_authored(int slot, int16_t authored) {
    const int percent = get_percent();
    Posted& posted = g_slots[slot];
    posted.have = true;
    posted.unscaled = authored;
    posted.percent = percent;
    return apply_unscaled(authored, percent);
}

bool take_reapply(int slot, int16_t unscaled, int16_t& applied) {
    const int percent = get_percent();
    Posted& posted = g_slots[slot];
    applied = apply_unscaled(unscaled, percent);
    if (posted.have && posted.unscaled == unscaled && posted.percent == percent) {
        return false;
    }
    // Steady 100 already went out on the original CSP event.
    if (percent == 100 && !posted.have) {
        return false;
    }
    posted.have = true;
    posted.unscaled = unscaled;
    posted.percent = percent;
    return true;
}

void clear_slot(unsigned slot) {
    g_slots[slot] = {};
}

void reset_slots() {
    for (unsigned slot = 0; slot < kSlotCount; ++slot) {
        clear_slot(slot);
    }
}

}
