//
// RT64
//

#pragma once

#include "shared/rt64_hlsl.h"

#ifdef HLSL_CPU
#include "common/rt64_common.h"

namespace interop {
#endif
    struct RSPViewport {
        // TK64: every float3 starts at a multiple of 16 bytes and the size is a multiple of 16, so this struct has the
        // same layout in C++, in D3D structured buffers and in GLSL std430 storage buffers (docs/specs/shader-translation.md).
        float3 scale;
        float scalePadding;
        float3 translate;
        float translatePadding;

        static RSPViewport identity() {
            RSPViewport viewport;
            viewport.scale = float3(1.0f, 1.0f, 1.0f);
            viewport.translate = float3(0.0f, 0.0f, 0.0f);
            return viewport;
        }

#ifdef HLSL_CPU
        RT64::FixedRect rect(const int16_t *clipRatios) const {
            int32_t x1 = int32_t(lround((translate.x - scale.x * float(clipRatios[0])) * 4.0f));
            int32_t y1 = int32_t(lround((translate.y - scale.y * float(clipRatios[1])) * 4.0f));
            int32_t x2 = int32_t(lround((translate.x - scale.x * float(clipRatios[2])) * 4.0f));
            int32_t y2 = int32_t(lround((translate.y - scale.y * float(clipRatios[3])) * 4.0f));
            return RT64::FixedRect(std::min(x1, x2), std::min(y1, y2), std::max(x1, x2), std::max(y1, y2));
        }

        float minDepth() const {
            return translate.z - scale.z;
        }

        float maxDepth() const {
            return translate.z + scale.z;
        }
#endif
    };
#ifdef HLSL_CPU
    // TK64: keep the layout identical in C++, D3D structured buffers and GLSL std430.
    static_assert((sizeof(RSPViewport) % 16) == 0, "RSPViewport must stay std430-compatible (docs/specs/shader-translation.md)");
    static_assert((offsetof(RSPViewport, scale) % 16) == 0, "RSPViewport::scale must stay std430-compatible (docs/specs/shader-translation.md)");
    static_assert((offsetof(RSPViewport, translate) % 16) == 0, "RSPViewport::translate must stay std430-compatible (docs/specs/shader-translation.md)");
#endif
#ifdef HLSL_CPU
};
#endif