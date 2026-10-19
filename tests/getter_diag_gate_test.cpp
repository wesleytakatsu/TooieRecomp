// Host check for the func_80092B04 durable-metadata gate predicate.
// Compiles the header only. Does not link the game.

#include "tooie_diag.h"

#include <cstdio>
#include <cstdlib>

namespace {

int g_failures = 0;

void expect(bool condition, const char* name) {
    if (!condition) {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++g_failures;
    }
}

bool requested(bool sway, const char* v7, const char* q, const char* temporal_tooie,
               const char* temporal_rt64, const char* endpoint, const char* correlation) {
    return tooie_diag::getter_durable_metadata_requested(
        sway, v7, q, temporal_tooie, temporal_rt64, endpoint, correlation);
}

}  // namespace

int main() {
    expect(!requested(false, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr),
           "all_null_off");
    expect(!requested(false, "", "", "", "", "", ""), "all_empty_off");
    expect(!requested(false, "0", "0", "0", "0", "0", "0"), "all_zero_off");

    expect(requested(false, "1", nullptr, nullptr, nullptr, nullptr, nullptr), "v7_1_on");
    expect(!requested(false, "0", nullptr, nullptr, nullptr, nullptr, nullptr), "v7_0_off");
    expect(requested(false, "2", "0", "0", "0", "0", "0"), "q_or_v7_leading_nonzero_on");
    expect(requested(false, "0", "yes", nullptr, nullptr, nullptr, nullptr), "q_legacy_on");

    expect(requested(false, nullptr, nullptr, "1", nullptr, nullptr, nullptr),
           "temporal_tooie_on");
    expect(requested(false, nullptr, nullptr, "0", "1", nullptr, nullptr),
           "temporal_rt64_fallback_on");
    expect(requested(false, nullptr, nullptr, nullptr, "1", nullptr, nullptr),
           "temporal_rt64_only_on");
    expect(!requested(false, nullptr, nullptr, "0", "0", nullptr, nullptr),
           "temporal_both_zero_off");
    expect(!requested(false, nullptr, nullptr, "0", nullptr, nullptr, nullptr),
           "temporal_tooie_zero_rt64_null_off");
    expect(requested(false, nullptr, nullptr, "2", "0", nullptr, nullptr),
           "temporal_tooie_2_does_not_fall_through");

    expect(requested(true, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr), "sway_on");

    expect(requested(false, nullptr, nullptr, nullptr, nullptr, "1", nullptr),
           "endpoint_binding_on");
    expect(!requested(false, nullptr, nullptr, nullptr, nullptr, "0", nullptr),
           "endpoint_binding_off");

    expect(requested(false, nullptr, nullptr, nullptr, nullptr, nullptr, "yes"),
           "correlation_yes_on");
    expect(requested(false, nullptr, nullptr, nullptr, nullptr, nullptr, "YES"),
           "correlation_YES_on");
    expect(requested(false, nullptr, nullptr, nullptr, nullptr, nullptr, "on"),
           "correlation_on_on");
    expect(requested(false, nullptr, nullptr, nullptr, nullptr, nullptr, "True"),
           "correlation_True_on");
    expect(requested(false, nullptr, nullptr, nullptr, nullptr, nullptr, "1"),
           "correlation_1_on");
    expect(!requested(false, nullptr, nullptr, nullptr, nullptr, nullptr, "2"),
           "correlation_2_off");
    expect(!requested(false, nullptr, nullptr, nullptr, nullptr, nullptr, "trueX"),
           "correlation_trueX_off");
    expect(!requested(false, nullptr, nullptr, nullptr, nullptr, nullptr, " yes"),
           "correlation_padded_off");

    if (g_failures != 0) {
        std::fprintf(stderr, "%d getter gate checks failed\n", g_failures);
        return 1;
    }
    std::printf("PASS getter_diag_gate_test\n");
    return 0;
}
