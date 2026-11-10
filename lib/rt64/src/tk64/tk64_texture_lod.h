//
// TK64
//
// Copyright (c) 2026 The TK64 contributors. Licensed under the MIT license. See LICENSE file for details.
//
// Texture level of detail measured on the CPU, per triangle, for replacement textures. Phase 3 of the per-distance mip
// streaming plan: it only measures and reports; nothing in the rendered image changes. See docs/specs/texture-lod.md.
//

#pragma once

#include <cstdint>
#include <limits>
#include <string>

namespace tk64 {
    // A level of detail is log2 of the texels covered by one pixel along the axis the sampler uses (0 means one texel per
    // pixel, 1 means two texels per pixel and mip 1 is enough, negative means magnified).
    constexpr float TextureLodNone = std::numeric_limits<float>::infinity();   // nothing measured
    constexpr float TextureLodFull = -std::numeric_limits<float>::infinity();  // needs the full-resolution mip

    // Anisotropy assumed for the sampler that reads replacement textures.
    constexpr float TextureLodMaxAnisotropy = 16.0f;

    // Added to a measured level before choosing mips: RT64's replacement samplers use a mip bias of -0.25
    // (rt64_shader_library.cpp, TextureSampler.hlsli), and 0.25 more is a margin for the per-triangle estimate against the
    // GPU's per-quad derivatives. With it, the Maximum quality level keeps every mip the GPU would sample.
    constexpr float TextureLodSamplerBias = -0.5f;

    // True when TK64_DIAG_TEXTURE_LOD names a report file. Read once.
    bool textureLodEnabled();

    // TK64_TEST_TEXTURE_FIRST_MIP=<n>: load every replacement DDS from mip n down, for testing partial loads. 0 (the
    // default) loads whole files. Read once; clamped to 0..15.
    uint32_t textureTestFirstMip();

    // First mip actually loaded for a file with mipCount mips: the request, kept within the file's mips.
    uint32_t textureFirstMip(uint32_t requested, uint32_t mipCount);

    // TK64_TEST_FAIL_TEXTURE_ALLOC=<n>: every n-th replacement texture allocation is treated as failed, to test the
    // fallback. 0 or unset: never. Thread-safe.
    bool textureAllocationTestFailure();

    // Streaming events shown in the report's window line. Thread-safe.
    enum class TextureStreamEvent { FirstLoad, Request, Swap };
    void countTextureStreamEvent(TextureStreamEvent event);

    // Replacement allocations that failed, real or simulated. Thread-safe.
    void countTextureAllocationFailure();
    uint64_t textureAllocationFailures();

    // Horizontal clip bound used for the measurement, as a multiple of w. Wider than the native 4:3 volume so geometry in
    // the widescreen strips is measured too; measuring a little outside the image only makes the result finer.
    constexpr float TextureLodGuardX = 2.0f;

    // Level of detail of one triangle. clip holds the clip-space x, y, z, w of each vertex (OpenGL depth range, visible
    // when -w <= z), u/v are texture coordinates in texels of the tile and viewportScale converts normalized device
    // coordinates to native framebuffer pixels. The triangle is clipped to the near plane and to the image (x within
    // TextureLodGuardX * w, y within w); with perspective-correct texture coordinates the level is evaluated exactly at
    // each corner of the visible polygon and the finest one is returned. TextureLodNone when nothing is visible or the
    // triangle is seen edge-on; TextureLodFull when the texture coordinates do not vary (texture generation).
    float triangleTexelLod(const float clip[3][4], const float u[3], const float v[3], float viewportScaleX, float viewportScaleY, float maxAnisotropy = TextureLodMaxAnisotropy);

    // Level of detail of a texture rectangle from its per-pixel steps (s5.10 fixed point, 1 << 10 is one texel per pixel).
    float rectangleTexelLod(int32_t dsdx, int32_t dtdy);

    // Report of the levels the replacement textures needed, written to the TK64_DIAG_TEXTURE_LOD file every few seconds:
    // per file, the lowest level seen and the top mip each of the five quality levels would keep resident.
    struct TextureLodSample {
        std::string relativePath;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t mipmaps = 0;
        uint64_t memorySize = 0;     // whole mip chain of the file
        uint64_t residentSize = 0;   // what is loaded now
        float lod = TextureLodNone;
    };

    // Bias of the five quality levels, from Maximum (index 0) to Minimum (index 4).
    constexpr float TextureLodLevelBias[5] = { 0.0f, 1.0f, 1.5f, 2.0f, 3.0f };

    // Top mip a quality bias would keep for a texture with the given mip count. TextureLodNone and TextureLodFull keep 0.
    uint32_t textureLodTopMip(float lod, float bias, uint32_t mipmaps);

    // Approximate bytes of a texture whose top mip is topMip, from the bytes of the whole mip chain.
    uint64_t textureLodBytes(uint64_t memorySize, uint32_t topMip);

    // Adds a sample from the render thread. Thread-safe.
    void recordTextureLod(const TextureLodSample &sample);
};
