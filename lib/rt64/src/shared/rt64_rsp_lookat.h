//
// RT64
//

#pragma once

#include "shared/rt64_hlsl.h"

#define RSP_LOOKAT_INDEX_ENABLED 0x1
#define RSP_LOOKAT_INDEX_LINEAR 0x2
#define RSP_LOOKAT_INDEX_SHIFT 2

#ifdef HLSL_CPU
namespace interop {
#endif
    struct RSPLookAt {
        // TK64: every float3 starts at a multiple of 16 bytes and the size is a multiple of 16, so this struct has the
        // same layout in C++, in D3D structured buffers and in GLSL std430 storage buffers (docs/specs/shader-translation.md).
        float3 x;
        float xPadding;
        float3 y;
        float yPadding;
    };
#ifdef HLSL_CPU
    // TK64: keep the layout identical in C++, D3D structured buffers and GLSL std430.
    static_assert((sizeof(RSPLookAt) % 16) == 0, "RSPLookAt must stay std430-compatible (docs/specs/shader-translation.md)");
    static_assert((offsetof(RSPLookAt, x) % 16) == 0, "RSPLookAt::x must stay std430-compatible (docs/specs/shader-translation.md)");
    static_assert((offsetof(RSPLookAt, y) % 16) == 0, "RSPLookAt::y must stay std430-compatible (docs/specs/shader-translation.md)");
#endif
#ifdef HLSL_CPU
};
#endif