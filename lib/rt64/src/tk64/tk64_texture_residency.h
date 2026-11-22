//
// TK64
//
// Copyright (c) 2026 The TK64 contributors. Licensed under the MIT license. See LICENSE file for details.
//
// Per-file residency of replacement textures: which mip a file is kept from, chosen from the measured level of detail.
// Phase 4 of per-distance mip streaming. See docs/specs/texture-streaming.md.
//

#pragma once

#include <cstdint>
#include <utility>
#include <vector>

namespace tk64 {
    // Active quality level: 0 is off (whole files, as RT64), 1 (Minimum) to 5 (Maximum). Precedence:
    // TK64_TEXTURE_STREAMING=0 (kill switch) > TK64_TEST_TEXTURE_STREAMING=<0..5> (test) > the host's request
    // (rt64_host_texture_streaming.h) > off. Always 0 unless the backend supports it (D3D12 or Vulkan). Thread-safe.
    uint32_t textureStreamingLevel();
    bool textureStreamingEnabled();

    // Host request: 0 off, 1..5 a level. Takes effect at the next residency decision. Thread-safe.
    void requestTextureStreamingLevel(uint32_t level);

    // Set by RT64 once the device exists: streaming is only allowed on D3D12 and Vulkan.
    void setTextureStreamingSupported(bool supported);

    // Memory budget for replacement textures in use. Precedence: TK64_TEST_TEXTURE_STREAMING_BUDGET_MB=<n> > the host's
    // request (bytes, 0 = automatic) > automatic (RT64's replacement pool size, 2/3 of dedicated video memory).
    uint64_t textureStreamingBudget();
    void requestTextureStreamingBudget(uint64_t bytes);
    void setTextureStreamingAutomaticBudget(uint64_t bytes);

    // What the last residency decision saw, for hosts and the report.
    struct TextureStreamingStats {
        uint32_t level = 0;
        uint64_t usedBytes = 0;     // replacement textures referenced by the game's textures
        uint64_t budgetBytes = 0;
        bool underPressure = false;
        uint64_t allocationFailures = 0;
    };

    TextureStreamingStats textureStreamingStats();
    void setTextureStreamingStats(const TextureStreamingStats &stats);

    // Bias added to a measured level before choosing the top mip, for the active level (texture-lod.md).
    float textureStreamingBias();

    // Measurement is needed for the diagnostic report or for streaming.
    bool textureLodMeasuring();

    // First load of a file not measured yet: start from this mip and move up once it is drawn.
    constexpr uint32_t TextureResidencyInitialTopMip = 2;

    // A file is moved one mip down only after its current top mip went unneeded for this long.
    constexpr double TextureResidencyDowngradeSeconds = 3.0;

    constexpr uint32_t TextureResidencyNoRequest = UINT32_MAX;

    struct TextureResidencyState {
        uint32_t currentTopMip = 0;     // top mip of the texture loaded now
        uint32_t mipCount = 0;          // mips of the whole file
        bool pending = false;           // a load of another top mip is in flight
        double lastNeedSeconds = 0.0;   // last time the current top mip (or a finer one) was needed
        uint32_t rememberedTopMip = TextureResidencyNoRequest; // last top mip asked for, kept across evictions
    };

    // Decides from one update's finest level (TextureLodNone when the file was not drawn) whether to load another top
    // mip. Moves up at once; moves down one mip at a time after TextureResidencyDowngradeSeconds without need. Returns the
    // top mip to load, or TextureResidencyNoRequest.
    // allowUpgrade false (memory pressure) keeps a file from moving up unless it is on the camera (TextureLodFull).
    uint32_t textureResidencyDecision(TextureResidencyState &state, float lod, float bias, double nowSeconds, double downgradeSeconds = TextureResidencyDowngradeSeconds, bool allowUpgrade = true);

    // Top mip for the first load of a file: the remembered one, else TextureResidencyInitialTopMip.
    uint32_t textureResidencyInitialTopMip(const TextureResidencyState *state);

    // Pressure starts when used memory is above the budget and ends below this fraction of it.
    constexpr double TextureResidencyPressureRelease = 0.9;

    // At most this many files are moved down per decision under pressure, so a spike does not reload everything at once.
    constexpr uint32_t TextureResidencyPressureStepsPerUpdate = 4;

    struct TextureResidencyCandidate {
        uint32_t id = 0;            // caller's identifier
        float lod = 0.0f;           // finest level the file was drawn at (TextureLodFull: on the camera)
        uint32_t currentTopMip = 0;
        uint32_t mipCount = 0;
        uint64_t bytes = 0;         // memory of the loaded texture
    };

    // Under pressure: which drawn files to move one mip down to free excessBytes, farthest first (largest level), the
    // ones on the camera (TextureLodFull) last, at most maxSteps. Returns (id, new top mip) pairs. Moving one mip down
    // frees about 3/4 of a texture.
    std::vector<std::pair<uint32_t, uint32_t>> textureResidencyPressureSteps(std::vector<TextureResidencyCandidate> candidates, uint64_t excessBytes, uint32_t maxSteps = TextureResidencyPressureStepsPerUpdate);
};
