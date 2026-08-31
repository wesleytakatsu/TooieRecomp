#ifndef TOOIE_TINTRO_PROJECTION_POLICY_H
#define TOOIE_TINTRO_PROJECTION_POLICY_H

#include <cmath>
#include <cstdint>
#include <algorithm>

namespace tooie_tintro_projection_policy {

// Perspective matrices encode horizontal/vertical scale as P00/P11. The
// ratio is therefore the source aspect represented by the guest projection.
inline float source_aspect_from_projection(float p00, float p11) {
    if (!std::isfinite(p00) || !std::isfinite(p11) || std::fabs(p00) < 0.001f) {
        return 0.0f;
    }
    const float aspect = std::fabs(p11 / p00);
    return std::isfinite(aspect) ? aspect : 0.0f;
}

// RT64 currently derives its automatic source from the 4:3 VI dimensions.
// This factor preconditions the guest matrix so the later automatic step
// yields the actual cinematic-source policy instead of a false 4:3 policy.
inline float compensation_factor(float target_aspect, float source_aspect) {
    constexpr float kViAspect = 4.0f / 3.0f;
    if (!std::isfinite(target_aspect) || !std::isfinite(source_aspect) ||
        target_aspect <= kViAspect || source_aspect <= 0.0f) {
        return 1.0f;
    }
    return std::min(target_aspect, source_aspect) / kViAspect;
}

void apply(uint8_t* rdram, float proj[4][4]);

} // namespace tooie_tintro_projection_policy

#endif // TOOIE_TINTRO_PROJECTION_POLICY_H
