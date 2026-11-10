#pragma once

#include <cstdint>
#include <functional>

namespace recomp::config {
    class Config;
}

// Graphics > "Texture Streaming": per-distance mip streaming of texture pack replacements, provided by the renderer
// (TK64, lib/rt64/src/hle/rt64_host_texture_streaming.h). Off by default: replacements load whole, as before.
namespace tooie_texture_streaming {
    inline constexpr char option[] = "texture_streaming";

    // Option values: 0 Off, 1 Minimum, 2 Low, 3 Medium, 4 High, 5 Maximum. They are the renderer's quality levels.
    inline constexpr uint32_t off = 0;
    inline constexpr uint32_t maximum = 5;

    // Receives the level to apply on load and on Apply (never for unapplied UI changes). The game passes
    // RT64::requestTextureStreamingQuality; tests pass their own.
    using LevelSink = std::function<void(uint32_t)>;

    void add_config(recomp::config::Config& config, LevelSink sink);
}
