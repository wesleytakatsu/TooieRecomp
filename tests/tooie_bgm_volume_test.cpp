#include "tooie_bgm_volume.h"

#include <cstdio>
#include <limits>

namespace {

int g_failures = 0;

void expect(bool condition, const char* name) {
    if (!condition) {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++g_failures;
    }
}

void test_gain() {
    using tooie_bgm::apply_unscaled;
    expect(apply_unscaled(32000, 100) == 32000, "identity_100");
    expect(apply_unscaled(32000, 0) == 0, "mute_0");
    expect(apply_unscaled(0, 40) == 0, "authored_zero");
    expect(apply_unscaled(32000, 50) == 16000, "half");
    expect(apply_unscaled(20000, 1) == 200, "one_percent");
    expect(apply_unscaled(1, 50) == 0, "trunc_toward_zero");
    expect(apply_unscaled(-100, 50) == -50, "negative_trunc");
    expect(apply_unscaled(32767, 150) == 32767, "clamp_high");
    expect(apply_unscaled(4000, -3) == 0, "clamp_low");
    expect(apply_unscaled(apply_unscaled(32000, 50), 50) == 8000, "feeding_applied_compounds");
}

void test_config_number_bounds() {
    tooie_bgm::set_percent(1e300);
    expect(tooie_bgm::get_percent() == 100, "oversized_config_high");
    tooie_bgm::set_percent(-1e300);
    expect(tooie_bgm::get_percent() == 0, "oversized_config_low");
    tooie_bgm::set_percent(std::numeric_limits<double>::quiet_NaN());
    expect(tooie_bgm::get_percent() == 0, "nan_config_mutes");
    tooie_bgm::set_percent(std::numeric_limits<double>::infinity());
    expect(tooie_bgm::get_percent() == 100, "infinite_config_high");
    tooie_bgm::set_percent(49.9);
    expect(tooie_bgm::get_percent() == 49, "fractional_config_truncates");
}

void test_slots_once() {
    tooie_bgm::reset_slots();
    tooie_bgm::set_percent(100);
    expect(tooie_bgm::get_percent() == 100, "atomic_100");
    expect(tooie_bgm::slot_for_seqp(tooie_bgm::seqp_addr(0)) == 0, "slot0");
    expect(tooie_bgm::slot_for_seqp(tooie_bgm::seqp_addr(5)) == 5, "slot5");
    expect(tooie_bgm::slot_for_seqp(0x80020BE0u) == -1, "unknown_seqp");

    int events = 0;
    const int16_t scaled = tooie_bgm::scale_authored(0, 32000);
    ++events;
    expect(scaled == 32000, "csp_100_same");
    int16_t extra = 1;
    expect(!tooie_bgm::take_reapply(0, 32000, extra), "steady_100_no_extra");
    expect(events == 1, "event_count_100");

    tooie_bgm::set_percent(0);
    expect(tooie_bgm::scale_authored(0, 32000) == 0, "csp_zero");
    tooie_bgm::set_percent(50);
    expect(tooie_bgm::scale_authored(0, 32000) == 16000, "csp_50");
    expect(tooie_bgm::scale_authored(0, 32000) == 16000, "repeat_50_no_decay");
    expect(!tooie_bgm::take_reapply(0, 32000, extra), "repeat_50_no_extra");

    tooie_bgm::set_percent(100);
    tooie_bgm::scale_authored(1, 8000);
    tooie_bgm::set_percent(25);
    expect(tooie_bgm::take_reapply(1, 8000, extra), "live_while_idle");
    expect(extra == 2000, "live_quarter");
    expect(!tooie_bgm::take_reapply(1, 8000, extra), "live_holds");

    expect(tooie_bgm::scale_authored(1, 0) == 0, "fade_authored_0");
    expect(tooie_bgm::scale_authored(1, 8000) == 2000, "fade_step");
    expect(tooie_bgm::scale_authored(1, 32000) == 8000, "fade_end");
    tooie_bgm::set_percent(100);
    expect(tooie_bgm::take_reapply(1, 32000, extra), "restore_100");
    expect(extra == 32000, "restore_authored");
    expect(!tooie_bgm::take_reapply(1, 32000, extra), "restore_holds");

    tooie_bgm::clear_slot(1);
    tooie_bgm::set_percent(100);
    expect(!tooie_bgm::take_reapply(1, 32000, extra), "idle_then_steady_100");
}

}

int main() {
    test_gain();
    test_config_number_bounds();
    test_slots_once();
    if (g_failures != 0) {
        std::fprintf(stderr, "%d failures\n", g_failures);
        return 1;
    }
    std::printf("tooie_bgm_volume_test ok\n");
    return 0;
}
