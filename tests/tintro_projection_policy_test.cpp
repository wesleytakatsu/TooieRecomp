#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <array>
#include <cstring>
#include <vector>

#include "tintro_projection_policy.h"
#include "tooie_window.h"

// Use the production apply/target calculation with controlled host services.
static ultramodern::renderer::GraphicsConfig graphics{};
static int window_width = 1600, window_height = 900;
namespace ultramodern::renderer {
const GraphicsConfig& get_graphics_config() { return graphics; }
}
namespace recompui {
void get_window_size(int& width, int& height) {
    width = window_width;
    height = window_height;
}
}

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

using Matrix = std::array<std::array<float, 4>, 4>;
static Matrix projection(float aspect) {
    Matrix matrix{};
    matrix[0][0] = 0.905148f;
    matrix[1][1] = matrix[0][0] * aspect;
    matrix[2][2] = -1.002f;
    matrix[2][3] = -1.0f;
    matrix[3][2] = -20.02f;
    return matrix;
}
static void set_context(std::vector<uint8_t>& memory, int16_t mode, int16_t map) {
    std::memcpy(memory.data() + ((0x80127632u ^ 2u) & 0x7FFFFFu), &mode, sizeof(mode));
    std::memcpy(memory.data() + ((0x80127640u ^ 2u) & 0x7FFFFFu), &map, sizeof(map));
}
static Matrix apply(std::vector<uint8_t>& memory, Matrix matrix) {
    // Keep the production C-array interface; no aliasing between array types.
    float native[4][4];
    std::memcpy(native, matrix.data(), sizeof(native));
    tooie_tintro_projection_policy::apply(memory.data(), native);
    std::memcpy(matrix.data(), native, sizeof(native));
    return matrix;
}
static void test_context_and_window_aspects() {
    std::vector<uint8_t> memory(0x800000);
    constexpr float cinematic = 304.0f / 172.0f;
    const int sizes[][2] = {
        {800, 600}, {1280, 1024}, {600, 800}, {1280, 800}, {1600, 900},
        {2520, 1080}, {3840, 1080}, {1373, 793}, {0, 900}, {1600, 0}, {-1, 900}
    };
    for (auto option : {ultramodern::renderer::AspectRatio::Expand,
                        ultramodern::renderer::AspectRatio::Original}) {
        graphics.ar_option = option;
        for (const auto& size : sizes) {
            // Change dimensions in one process to catch cached/fixed-16:9 policies.
            window_width = size[0]; window_height = size[1];
            for (float source : {4.0f/3.0f, 1.51f, 1.6f, cinematic, 1.9f, 2.09f}) {
                const Matrix input = projection(source);
                for (int map : {322, 323, 0}) {
                    set_context(memory, 3, int16_t(map));
                    TEST_ASSERT(apply(memory, input) == input,
                                "ordinary story projection must remain unchanged at every window aspect");
                }
                for (int mode : {0, 1, 2, 4, 5}) {
                    set_context(memory, int16_t(mode), 395);
                    TEST_ASSERT(apply(memory, input) == input,
                                "T_INTRO map outside its active story context must remain unchanged");
                }
                set_context(memory, 3, 395);
                Matrix expected = input;
                const float target = tooie_window::calculate_target_aspect_ratio(
                    option, window_width, window_height);
                if (source >= 1.50f && source <= 2.10f) {
                    const float actual_source = tooie_tintro_projection_policy::source_aspect_from_projection(
                        input[0][0], input[1][1]);
                    const float factor = tooie_tintro_projection_policy::compensation_factor(target, actual_source);
                    for (auto& row : expected) row[0] *= factor;
                }
                TEST_ASSERT(apply(memory, input) == expected,
                            "intro retains its existing policy using the current window aspect");
            }
        }
    }
    // Gameplay -> cinematic -> gameplay across a map transition retains no policy state.
    graphics.ar_option = ultramodern::renderer::AspectRatio::Expand;
    window_width = 1600; window_height = 900;
    set_context(memory, 3, 395);
    TEST_ASSERT(apply(memory, projection(cinematic)) != projection(cinematic), "intro compensation remains active");
    set_context(memory, 3, 322);
    for (float source : {4.0f/3.0f, cinematic, 4.0f/3.0f}) {
        TEST_ASSERT(apply(memory, projection(source)) == projection(source), "ordinary scene transitions retain shape");
    }
    float native[4][4]{};
    tooie_tintro_projection_policy::apply(nullptr, native);
    set_context(memory, 3, 395);
    tooie_tintro_projection_policy::apply(memory.data(), nullptr);
    Matrix invalid = projection(cinematic);
    invalid[0][0] = 0.0f;
    TEST_ASSERT(apply(memory, invalid) == invalid, "invalid projection bypasses compensation");
}

int main() {
    test_source_derivation();
    test_aspect_policy();
    test_context_and_window_aspects();
    std::printf("=== PASS: %d/%d assertions passed ===\n", g_pass_count, g_test_count);
    return 0;
}
