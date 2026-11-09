#include "tooie_expanded_memory.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
bool diagnostics() {
    static const bool enabled = [] {
        const char* value = std::getenv("TOOIE_DIAG_HEAP");
        return value && std::strcmp(value, "1") == 0;
    }();
    return enabled;
}
bool guard_enabled() {
    static const bool enabled = [] {
        const char* value = std::getenv("TOOIE_TEST_DISTANCE_HEAP_GUARD");
        return !value || std::strcmp(value, "0") != 0;
    }();
    return enabled;
}
uint32_t word(uint8_t* rdram, uint32_t address) {
    return uint32_t(MEM_W(0, static_cast<int32_t>(address)));
}
}

extern "C" void tooie_heap_end(uint8_t*, recomp_context* ctx) {
    ctx->r2 = static_cast<int32_t>(tooie_memory::boot.end());
    static const bool logged = [] {
        if (diagnostics()) std::fprintf(stderr, "[tooie-heap] base=%08X end=%08X size=%08X expanded=%d\n",
            tooie_memory::boot.end() - tooie_memory::boot.size(), tooie_memory::boot.end(),
            tooie_memory::boot.size(), tooie_memory::boot.active());
        return true;
    }();
    (void)logged;
}

extern "C" void tooie_heap_size(uint8_t*, recomp_context* ctx) {
    ctx->r2 = tooie_memory::boot.size();
}

bool tooie_memory::distance_heap_available(uint8_t* rdram) {
    if (!guard_enabled()) return true;
    if (!rdram) return false;
    const uint32_t size = word(rdram, 0x8007BF00u);
    const uint32_t used = word(rdram, 0x8003F750u);
    const uint32_t base = word(rdram, 0x8007E990u);
    const uint32_t frame = word(rdram, 0x800459C8u);
    struct Snapshot {
        uint8_t* ram = nullptr;
        uint32_t frame = 0, base = 0, size = 0, largest = 0;
        bool previous = false, reported = false;
    };
    static thread_local Snapshot cache;
    if (cache.ram != rdram || cache.frame != frame || cache.base != base || cache.size != size) {
        cache.ram = rdram;
        cache.frame = frame;
        cache.base = base;
        cache.size = size;
        cache.largest = 0;
        // Same list and block sizes as heap_largest_free; no guest calls/stack mutation.
        if (size >= 0x40 && contains(base, size) && aligned(base, 0x20)) {
            const uint32_t last = base + size - 0x20;
            uint32_t block = word(rdram, base + 0x14);
            uint32_t remaining = size / 0x20;
            bool valid = true;
            while (block != last && remaining--) {
                if (block < base + 0x20 || block >= last || !aligned(block, 0x18)) {
                    valid = false;
                    break;
                }
                const uint32_t next = word(rdram, block + 4);
                if (next <= block || next > last) {
                    valid = false;
                    break;
                }
                cache.largest = std::max(cache.largest, next - block);
                block = word(rdram, block + 0x14);
            }
            if (!valid || block != last) cache.largest = 0;
        }
    }
    // Total free bytes are O(1) and checked on every call, including within a frame.
    const bool allowed = used <= size && guard_allows(size - used, cache.largest);
    if (diagnostics() && (!cache.reported || allowed != cache.previous)) {
        std::fprintf(stderr, "[tooie-heap] distance-guard=%s free=%u largest=%u\n",
            allowed ? "extended" : "native", used <= size ? size - used : 0, cache.largest);
        cache.previous = allowed;
        cache.reported = true;
    }
    return allowed;
}
