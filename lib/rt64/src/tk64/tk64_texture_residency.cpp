//
// TK64
//
// Copyright (c) 2026 The TK64 contributors. Licensed under the MIT license. See LICENSE file for details.
//

#include "tk64_texture_residency.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <mutex>

#include "tk64_texture_lod.h"

namespace tk64 {
    static std::atomic<int> hostStreamingLevel{ -1 };
    static std::atomic<bool> streamingSupported{ false };
    static std::atomic<uint64_t> hostStreamingBudget{ 0 };
    static std::atomic<uint64_t> automaticStreamingBudget{ 0 };

    // -1 when the variable is absent or empty.
    static long environmentNumber(const char *name) {
        const char *value = std::getenv(name);
        if ((value == nullptr) || (value[0] == '\0')) {
            return -1;
        }

        return std::strtol(value, nullptr, 10);
    }

    uint32_t textureStreamingLevel() {
        static const long killSwitch = environmentNumber("TK64_TEXTURE_STREAMING");
        static const long testLevel = environmentNumber("TK64_TEST_TEXTURE_STREAMING");
        if (!streamingSupported.load(std::memory_order_acquire) || (killSwitch == 0)) {
            return 0;
        }

        if (testLevel >= 0) {
            return uint32_t(std::clamp(testLevel, 0L, 5L));
        }

        const int host = hostStreamingLevel.load(std::memory_order_acquire);
        return (host > 0) ? uint32_t(std::min(host, 5)) : 0;
    }

    void requestTextureStreamingLevel(uint32_t level) {
        hostStreamingLevel.store(int(std::min(level, 5U)), std::memory_order_release);
    }

    void setTextureStreamingSupported(bool supported) {
        streamingSupported.store(supported, std::memory_order_release);
    }

    uint64_t textureStreamingBudget() {
        static const long testMegabytes = environmentNumber("TK64_TEST_TEXTURE_STREAMING_BUDGET_MB");
        if (testMegabytes > 0) {
            return uint64_t(testMegabytes) * 1024ULL * 1024ULL;
        }

        const uint64_t host = hostStreamingBudget.load(std::memory_order_acquire);
        return (host > 0) ? host : automaticStreamingBudget.load(std::memory_order_acquire);
    }

    void requestTextureStreamingBudget(uint64_t bytes) {
        hostStreamingBudget.store(bytes, std::memory_order_release);
    }

    void setTextureStreamingAutomaticBudget(uint64_t bytes) {
        automaticStreamingBudget.store(bytes, std::memory_order_release);
    }

    static std::mutex statsMutex;
    static TextureStreamingStats lastStats;

    TextureStreamingStats textureStreamingStats() {
        std::unique_lock lock(statsMutex);
        return lastStats;
    }

    void setTextureStreamingStats(const TextureStreamingStats &stats) {
        std::unique_lock lock(statsMutex);
        lastStats = stats;
    }

    bool textureStreamingEnabled() {
        return textureStreamingLevel() > 0;
    }

    float textureStreamingBias() {
        // Level 5 (Maximum) uses TextureLodLevelBias[0], level 1 (Minimum) uses TextureLodLevelBias[4].
        const uint32_t level = std::max(textureStreamingLevel(), 1U);
        return TextureLodLevelBias[5 - level];
    }

    bool textureLodMeasuring() {
        return textureLodEnabled() || textureStreamingEnabled();
    }

    uint32_t textureResidencyDecision(TextureResidencyState &state, float lod, float bias, double nowSeconds, double downgradeSeconds, bool allowUpgrade) {
        if (lod == TextureLodNone) {
            // Not drawn in this update: keep what is loaded; a later update decides.
            return TextureResidencyNoRequest;
        }

        const uint32_t desired = textureLodTopMip(lod, bias, state.mipCount);
        if (desired <= state.currentTopMip) {
            state.lastNeedSeconds = nowSeconds;
        }

        if (state.pending) {
            return TextureResidencyNoRequest;
        }

        if ((desired < state.currentTopMip) && (allowUpgrade || (lod == TextureLodFull))) {
            state.pending = true;
            state.rememberedTopMip = desired;
            return desired;
        }

        if ((desired > state.currentTopMip) && ((nowSeconds - state.lastNeedSeconds) >= downgradeSeconds)) {
            const uint32_t lastMip = (state.mipCount > 0) ? (state.mipCount - 1) : 0;
            const uint32_t next = std::min(state.currentTopMip + 1, lastMip);
            if (next != state.currentTopMip) {
                state.pending = true;
                state.rememberedTopMip = next;
                state.lastNeedSeconds = nowSeconds;
                return next;
            }
        }

        return TextureResidencyNoRequest;
    }

    std::vector<std::pair<uint32_t, uint32_t>> textureResidencyPressureSteps(std::vector<TextureResidencyCandidate> candidates, uint64_t excessBytes, uint32_t maxSteps) {
        // Farthest first; on the camera last; larger textures first among equal levels.
        std::stable_sort(candidates.begin(), candidates.end(), [](const TextureResidencyCandidate &a, const TextureResidencyCandidate &b) {
            if (a.lod != b.lod) {
                return a.lod > b.lod;
            }

            return a.bytes > b.bytes;
        });

        std::vector<std::pair<uint32_t, uint32_t>> steps;
        uint64_t freed = 0;
        for (const TextureResidencyCandidate &c : candidates) {
            if ((freed >= excessBytes) || (steps.size() >= maxSteps)) {
                break;
            }

            if ((c.mipCount == 0) || (c.currentTopMip + 1 >= c.mipCount)) {
                continue;
            }

            steps.emplace_back(c.id, c.currentTopMip + 1);
            freed += c.bytes - c.bytes / 4;
        }

        return steps;
    }

    uint32_t textureResidencyInitialTopMip(const TextureResidencyState *state) {
        if ((state != nullptr) && (state->rememberedTopMip != TextureResidencyNoRequest)) {
            return state->rememberedTopMip;
        }

        return TextureResidencyInitialTopMip;
    }
};
