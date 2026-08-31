#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "tintro_projection_policy.h"

static int g_test_count = 0;
static int g_pass_count = 0;

#define TEST_ASSERT(cond, msg) do { \
    ++g_test_count; \
    if (cond) { \
        ++g_pass_count; \
    } else { \
        std::fprintf(stderr, "FAIL: %s (line %d): %s\n", __FUNCTION__, __LINE__, msg); \
        std::exit(1); \
    } \
} while (0)

static void test_source_derivation() {
    constexpr float guest_p00 = 0.905148f;
    constexpr float expected_source = 1.76744186f;
    const float guest_p11 = guest_p00 * expected_source;
    const float source = tooie_tintro_projection_policy::source_aspect_from_projection(
        guest_p00, guest_p11);
    TEST_ASSERT(std::fabs(source - expected_source) < 1.0e-5f,
                "source aspect comes from the cinematic projection");
}

static void test_aspect_policy() {
    constexpr float source = 1.76744186f;
    constexpr float vi = 4.0f / 3.0f;
    const float target_4_3 = 4.0f / 3.0f;
    const float target_16_10 = 16.0f / 10.0f;
    const float target_16_9 = 16.0f / 9.0f;
    const float target_21_9 = 21.0f / 9.0f;
    const float target_32_9 = 32.0f / 9.0f;

    TEST_ASSERT(std::fabs(tooie_tintro_projection_policy::compensation_factor(
                             target_4_3, source) - 1.0f) < 1.0e-5f,
                "4:3 keeps the guest projection");
    TEST_ASSERT(std::fabs(tooie_tintro_projection_policy::compensation_factor(
                             target_16_10, source) - 1.2f) < 1.0e-5f,
                "16:10 compensates the VI-derived automatic source");
    const float factor_16_9 = tooie_tintro_projection_policy::compensation_factor(
        target_16_9, source);
    TEST_ASSERT(std::fabs(factor_16_9 - source / vi) < 1.0e-5f,
                "16:9 uses the cinematic source, not 4:3");
    TEST_ASSERT(std::fabs(factor_16_9 / (target_16_9 / vi) - source / target_16_9) < 1.0e-5f,
                "16:9 post-AUTO scale is source/target");
    TEST_ASSERT(std::fabs(tooie_tintro_projection_policy::compensation_factor(
                             target_21_9, source) - source / vi) < 1.0e-5f,
                "21:9 remains source-clamped");
    TEST_ASSERT(std::fabs(tooie_tintro_projection_policy::compensation_factor(
                             target_32_9, source) - source / vi) < 1.0e-5f,
                "32:9 remains source-clamped");
}

int main() {
    test_source_derivation();
    test_aspect_policy();
    std::printf("=== PASS: %d/%d assertions passed ===\n", g_pass_count, g_test_count);
    return 0;
}
