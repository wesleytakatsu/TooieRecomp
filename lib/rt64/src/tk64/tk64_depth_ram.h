//
// TK64
//
// Copyright (c) 2026 The TK64 contributors. Licensed under the MIT license. See LICENSE file for details.
//
// Depth for games that read the Z-buffer from RAM, without rendering frames twice. See docs/specs/mobile-driver-costs.md.
//

#pragma once

#include <cstdint>
#include <cstddef>
#include <mutex>

namespace plume {
    struct RenderTexture;

    // Starts converting a depth target to the N64's 16-bit format, and publishes the newest earlier conversion the GPU finished
    // in tk64::depthram::frame(). It never waits for the GPU, so the depth is a frame or more old. OpenGL ES backend only.
    void glesReadDepth16(RenderTexture *texture, uint32_t address, uint32_t width, uint32_t height, float scaleX, float scaleY);
}

namespace tk64::depthram {
    // The finished conversion stays mapped until the next one replaces it, and the mutex guards the mapping. Reading the buffer is
    // slow, so the thread that needs the values does it, not the OpenGL thread. Each 32-bit word holds two values, the first in its
    // high half, which is how RDRAM lays them out in this emulator.
    struct Frame {
        std::mutex mutex;
        bool fresh = false;
        uint32_t address = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        const uint8_t *mapped = nullptr;
        size_t rowBytes = 0;
    };

    inline bool &enabled() {
        static bool on = false;
        return on;
    }

    inline Frame &frame() {
        static Frame instance;
        return instance;
    }
}
