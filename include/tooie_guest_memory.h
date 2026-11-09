#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace tooie_memory {
inline constexpr uint32_t native_end = 0x80400000u;
inline constexpr uint32_t native_size = 0x002C8800u;
// First page after the canonical overlay high-water mark (0x8080B5E0).
inline constexpr uint32_t expanded_base = 0x8080C000u;
inline constexpr uint32_t expanded_end = 0x81000000u;
inline constexpr uint32_t expanded_size = expanded_end - expanded_base;

class BootState {
public:
    void latch(bool requested, bool permitted) {
        int expected = -1;
        state.compare_exchange_strong(expected, requested && permitted ? 1 : 0);
    }
    bool active() const { return state.load(std::memory_order_acquire) == 1; }
    bool latched() const { return state.load(std::memory_order_acquire) >= 0; }
    bool restart_required(bool requested) const { return latched() && requested != active(); }
    uint32_t end() const { return active() ? expanded_end : native_end; }
    uint32_t size() const { return active() ? expanded_size : native_size; }
private:
    std::atomic<int> state{-1};
};

inline BootState boot;
inline std::atomic<uint32_t> static_overlay_end{0x80800000u};

inline bool layout_available(uint32_t overlay_end, size_t patch_bytes) {
    return overlay_end <= expanded_base && patch_bytes <= expanded_base - 0x80801000u;
}

inline bool contains(uint32_t address, size_t size = 1) {
    if (address >= 0x80000000u && address < 0x80800000u)
        return size <= 0x80800000u - address;
    return boot.active() && address >= expanded_base && address < expanded_end &&
        size <= expanded_end - address;
}

inline bool contains_alias(uint32_t address, size_t size = 1) {
    return address >= 0x80000000u && address < 0xC0000000u &&
        contains(0x80000000u | (address & 0x1FFFFFFFu), size);
}

inline bool aligned(uint32_t address, size_t size = 4) {
    return (address & 3u) == 0 && contains(address, size);
}
}
