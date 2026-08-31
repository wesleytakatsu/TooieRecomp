#include "tintro_projection_policy.h"

#include <cmath>
#include <cstring>

#include "recompui/recompui.h"
#include "tooie_window.h"
#include "ultramodern/config.hpp"

namespace {

constexpr uint32_t kModeAddr = 0x80127632u;
constexpr uint32_t kMapAddr = 0x80127640u;
constexpr int kIntroMode = 3;
constexpr int kIntroMap = 395;
constexpr float kViAspect = 4.0f / 3.0f;

static bool guest_ok(uint32_t address, size_t size) {
    const uint32_t offset = address & 0x007FFFFFu;
    return static_cast<uint64_t>(offset) + size <= 0x00800000u;
}

static int16_t read_s16(const uint8_t* rdram, uint32_t address) {
    if (rdram == nullptr || !guest_ok(address, sizeof(int16_t))) {
        return 0;
    }

    int16_t value = 0;
    const uint32_t offset = (address ^ 2u) & 0x007FFFFFu;
    std::memcpy(&value, rdram + offset, sizeof(value));
    return value;
}

static bool intro_context(const uint8_t* rdram) {
    return read_s16(rdram, kModeAddr) == kIntroMode ||
           read_s16(rdram, kMapAddr) == kIntroMap;
}

static bool cinematic_projection(const float proj[4][4], float& source_aspect) {
    if (proj == nullptr) {
        return false;
    }

    source_aspect = tooie_tintro_projection_policy::source_aspect_from_projection(
        proj[0][0], proj[1][1]);
    // Keep unrelated orthographic/HUD projections out of the T_INTRO seam.
    return source_aspect >= 1.50f && source_aspect <= 2.10f;
}

static float target_aspect() {
    const ultramodern::renderer::GraphicsConfig graphics =
        ultramodern::renderer::get_graphics_config();
    int width = 0;
    int height = 0;
    recompui::get_window_size(width, height);
    return tooie_window::calculate_target_aspect_ratio(
        graphics.ar_option, width, height, kViAspect);
}

} // namespace

namespace tooie_tintro_projection_policy {

void apply(uint8_t* rdram, float proj[4][4]) {
    if (!intro_context(rdram)) {
        return;
    }

    float source_aspect = 0.0f;
    if (!cinematic_projection(proj, source_aspect)) {
        return;
    }

    const float target = target_aspect();
    if (!std::isfinite(target) || target <= kViAspect) {
        return;
    }

    const float factor = compensation_factor(target, source_aspect);
    if (!std::isfinite(factor) || factor <= 0.0f) {
        return;
    }

    // RT64's later automatic adjustment divides the first matrix column by
    // target/(304/228). Preconditioning here makes that result use the
    // actual cinematic source aspect rather than the false VI aspect.
    for (int row = 0; row < 4; ++row) {
        proj[row][0] *= factor;
    }
}

} // namespace tooie_tintro_projection_policy
