#pragma once

#include <atomic>
#include <cstdint>

namespace RT64 {
inline std::atomic<uint32_t>& hostRdramMaskSlot() {
    static std::atomic<uint32_t> mask{0x7FFFFFu};
    return mask;
}

// Host sets this once before guest execution; emulator/plugin defaults stay native.
inline void setHostRdramExpanded(bool expanded) {
    hostRdramMaskSlot().store(expanded ? 0xFFFFFFu : 0x7FFFFFu, std::memory_order_release);
}

inline uint32_t hostRdramMask() {
    return hostRdramMaskSlot().load(std::memory_order_acquire);
}
}
