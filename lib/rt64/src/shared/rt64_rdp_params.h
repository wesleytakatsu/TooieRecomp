//
// RT64
//

#pragma once

#include "rt64_hlsl.h"

#ifdef HLSL_CPU
namespace interop {
#endif
    struct RDPParams {
        // TK64: every float3 starts at a multiple of 16 bytes and the size is a multiple of 16, so this struct has the
        // same layout in C++, in D3D structured buffers and in GLSL std430 storage buffers (docs/specs/shader-translation.md).
        float4 primColor;
        float2 primLOD;
        float2 primDepth;
        float4 envColor;
        float4 fogColor;
        float4 blendColor;
        float3 keyCenter;
        float keyCenterPadding;
        float3 keyScale;
        float keyScalePadding;
        int convertK[6];
        int padding0;
        int padding1;
    };
#ifdef HLSL_CPU
    // TK64: keep the layout identical in C++, D3D structured buffers and GLSL std430.
    static_assert((sizeof(RDPParams) % 16) == 0, "RDPParams must stay std430-compatible (docs/specs/shader-translation.md)");
    static_assert((offsetof(RDPParams, keyCenter) % 16) == 0, "RDPParams::keyCenter must stay std430-compatible (docs/specs/shader-translation.md)");
    static_assert((offsetof(RDPParams, keyScale) % 16) == 0, "RDPParams::keyScale must stay std430-compatible (docs/specs/shader-translation.md)");
#endif
#ifdef HLSL_CPU
};
#endif