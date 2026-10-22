//
// RT64
//

#pragma once

#include <atomic>
#include <cstdint>

namespace RT64 {
    // Count of swapchain presents that returned success. Written by the present thread only, read by anyone.
    // Presents skipped by the decimator or dropped on a failed acquire/present are not counted.
    inline std::atomic<uint64_t> presentSuccessCount{ 0 };
};
