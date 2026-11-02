//
// RT64
//

#pragma once

#include "shared/rt64_hlsl.h"

#ifdef HLSL_CPU
namespace interop {
#endif
#ifdef HLSL_CPU
    union GPUTileFlags {
        struct {
            uint alphaIsCvg : 1;
            uint highRes : 1;
            uint fromCopy : 1;
            uint rawTMEM : 1;
            uint hasMipmaps : 1;
            uint shiftedByHalf : 1;
        };

        uint value;
    };
#else
    // SPIR-V code generation does not seem to like bitfields at the moment, so we work around it by querying the flags manually.
    typedef uint GPUTileFlags;

    bool gpuTileFlagAlphaIsCvg(GPUTileFlags flags) {
        return flags & 0x1;
    }

    bool gpuTileFlagHighRes(GPUTileFlags flags) {
        return flags & 0x2;
    }

    bool gpuTileFlagFromCopy(GPUTileFlags flags) {
        return flags & 0x4;
    }

    bool gpuTileFlagRawTMEM(GPUTileFlags flags) {
        return flags & 0x8;
    }

    bool gpuTileFlagHasMipmaps(GPUTileFlags flags) {
        return flags & 0x10;
    }

    bool gpuTileFlagShiftedByHalf(GPUTileFlags flags) {
        return flags & 0x20;
    }
#endif
    struct GPUTile {
        // TK64: every float3 starts at a multiple of 16 bytes and the size is a multiple of 16, so this struct has the
        // same layout in C++, in D3D structured buffers and in GLSL std430 storage buffers (docs/specs/shader-translation.md).
        float2 ulScale;
        float2 tcScale;
        uint2 texelShift;
        uint2 texelMask;
        float3 textureDimensions;
        uint textureIndex;
        GPUTileFlags flags;
        uint padding0;
        uint padding1;
        uint padding2;
    };
#ifdef HLSL_CPU
    // TK64: keep the layout identical in C++, D3D structured buffers and GLSL std430.
    static_assert((sizeof(GPUTile) % 16) == 0, "GPUTile must stay std430-compatible (docs/specs/shader-translation.md)");
    static_assert((offsetof(GPUTile, textureDimensions) % 16) == 0, "GPUTile::textureDimensions must stay std430-compatible (docs/specs/shader-translation.md)");
#endif
#ifdef HLSL_CPU
};
#endif