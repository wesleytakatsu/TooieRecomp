//
// RT64
//

#pragma once

#include <set>
#include <unordered_map>

#include "hle/rt64_draw_call.h"
#include "render/rt64_texture_cache.h"

namespace RT64 {
    struct State;

    struct TextureManager {
        struct TextureHashKey {
            uint64_t low;
            uint64_t high;

            bool operator==(const TextureHashKey &other) const {
                return (low == other.low) && (high == other.high);
            }
        };

        struct TextureHashKeyHasher {
            size_t operator()(const TextureHashKey &key) const {
                return static_cast<size_t>(key.low ^ key.high);
            }
        };

        std::unordered_map<TextureHashKey, uint64_t, TextureHashKeyHasher> textureHashCache;
        std::set<uint64_t> hashSet;
        std::set<uint64_t> dumpedSet;

        void uploadEmpty(State *state, TextureCache *textureCache, uint64_t creationFrame, uint16_t width, uint16_t height, uint64_t replacementHash);
        uint64_t uploadTMEM(State *state, const LoadTile &loadTile, TextureCache *textureCache, uint64_t creationFrame, uint16_t byteOffset, uint16_t byteCount, uint16_t width, uint16_t height, uint32_t tlut);
        uint64_t uploadTexture(State *state, const LoadTile &loadTile, TextureCache *textureCache, uint64_t creationFrame, uint16_t width, uint16_t height, uint32_t tlut);
        void dumpTexture(uint64_t hash, State *state, const LoadTile &loadTile, uint16_t width, uint16_t height, uint32_t tlut);
        void removeHashes(const std::vector<uint64_t> &hashes);
    };
};