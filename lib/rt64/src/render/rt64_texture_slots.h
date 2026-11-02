//
// TK64
//

#pragma once

#include <cstdlib>
#include <cstring>

#include "common/rt64_plume.h"

namespace RT64 {
    // TK64: whether the renderer binds each draw's textures to fixed slots instead of using the bindless texture array
    // (docs/specs/texture-slots.md). Used on devices without descriptor indexing; TK64_FORCE_TEXTURE_SLOTS=1 forces it
    // on any device for testing.
    inline bool textureSlotsForced() {
        static const bool forced = []() {
            const char *value = std::getenv("TK64_FORCE_TEXTURE_SLOTS");
            return (value != nullptr) && (std::strcmp(value, "1") == 0);
        }();

        return forced;
    }

    inline bool useTextureSlots(const RenderDevice *device) {
        return textureSlotsForced() || !device->getCapabilities().descriptorIndexing;
    }
};
