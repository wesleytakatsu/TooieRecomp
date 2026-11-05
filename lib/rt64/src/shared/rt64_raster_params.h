//
// RT64
//

#pragma once

#include "shared/rt64_hlsl.h"

#ifdef HLSL_CPU
namespace interop {
#endif
    struct RasterParams {
        uint renderIndex;
#ifdef TK64_MOBILE
        // TK64: what the vertex shader needs from the draw call, so it reads no storage buffer. Mobile drivers may offer
        // none to the vertex stage (Panfrost, docs/specs/storage-buffer-limits.md).
        uint vsRect;
        uint vsFixedDepth;
        float vsFixedDepthValue;
#else
        uint3 padding;
#endif
        float2 screenScale;
        float2 screenOffset;
    };
#ifdef HLSL_CPU
};
#endif