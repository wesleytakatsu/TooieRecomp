//
// RT64 (TK64)
//
// Host-controlled texture streaming (docs/specs/texture-streaming.md). Off unless the host asks: a host that never calls
// these functions loads replacement textures exactly as RT64 did. Any thread; takes effect at the next residency decision.
// Only D3D12 and Vulkan stream; on other backends the request is kept but does nothing. TK64_TEXTURE_STREAMING=0 forces it
// off and TK64_TEST_TEXTURE_STREAMING overrides the level, for testing.
//

#pragma once

#include <cstdint>

#include "tk64/tk64_texture_residency.h"

namespace RT64 {
    // 0: off (whole files). 1 (Minimum) to 5 (Maximum): quality level.
    inline void requestTextureStreamingQuality(uint32_t level) {
        tk64::requestTextureStreamingLevel(level);
    }

    // Memory budget for replacement textures in use, in bytes. 0: automatic (2/3 of dedicated video memory).
    inline void requestTextureStreamingBudget(uint64_t bytes) {
        tk64::requestTextureStreamingBudget(bytes);
    }

    // Level in effect, memory in use, budget, pressure and allocation failures, as of the last decision.
    inline tk64::TextureStreamingStats getTextureStreamingStats() {
        return tk64::textureStreamingStats();
    }
};
