#include "tooie_distant_detail.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace {
std::atomic<bool> selected{false};

// TOOIE_DIAG_DISTANT_DETAIL=1 (default off): every 5 s, stderr gets how many LOD node and player level distances
// were seen and how many of them the option changed. Counting only; no per-call output.
struct Diag {
    bool on = [] { const char* v = std::getenv("TOOIE_DIAG_DISTANT_DETAIL"); return v && v[0] == '1'; }();
    std::atomic<uint32_t> lod_seen{0}, lod_forced{0}, player_seen{0}, player_forced{0};
    std::atomic<int64_t> last{0};
    void count(std::atomic<uint32_t>& seen, std::atomic<uint32_t>& forced, bool changed) {
        if (!on) {
            return;
        }
        seen.fetch_add(1, std::memory_order_relaxed);
        if (changed) {
            forced.fetch_add(1, std::memory_order_relaxed);
        }
        const int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        int64_t previous = last.load(std::memory_order_relaxed);
        if (now - previous >= 5 && last.compare_exchange_strong(previous, now)) {
            std::fprintf(stderr, "[distant-detail] lod %u/%u forced, player %u/%u forced\n",
                lod_forced.exchange(0), lod_seen.exchange(0), player_forced.exchange(0), player_seen.exchange(0));
        }
    }
} diag;

// One pending distance call per wrapped invocation, matched by the stack pointer the native code passes to it.
struct Pending {
    gpr stack;
    bool consumed = false;
};
thread_local Pending* lod = nullptr;
thread_local Pending* player = nullptr;

struct Scope {
    Pending*& slot;
    Pending* previous;
    Scope(Pending*& s, Pending& value) : slot(s), previous(s) { slot = &value; }
    ~Scope() { slot = previous; }
};

// Takes the pending call when this is it: the first call made with the expected stack, never a nested one.
bool take(Pending* pending, recomp_context* ctx) {
    if (!pending || pending->consumed || ctx->r29 != pending->stack) {
        return false;
    }
    pending->consumed = true;
    return true;
}
}

namespace tooie_distant_detail {
void set_enabled(bool enabled) { selected.store(enabled, std::memory_order_relaxed); }
bool enabled() { return selected.load(std::memory_order_relaxed); }

void lod_node(uint8_t* rdram, recomp_context* ctx, recomp_func_t* next) {
    if (!enabled()) {
        next(rdram, ctx);
        return;
    }
    // 0x800DD214, 0x800DD2E8 and 0x800DD37C all open a 0x18-byte frame and call func_800EEB40 from it.
    Pending frame{ADD32(ctx->r29, -0x18)};
    Scope scope(lod, frame);
    next(rdram, ctx);
}

void lod_distance_squared(uint8_t* rdram, recomp_context* ctx, recomp_func_t* next) {
    const bool applies = take(lod, ctx);
    next(rdram, ctx);
    if (!applies) {
        return;
    }
    const bool change = enabled() && std::isfinite(ctx->f0.fl) && ctx->f0.fl > near_distance_squared;
    if (change) {
        ctx->f0.fl = near_distance_squared;
    }
    diag.count(diag.lod_seen, diag.lod_forced, change);
}

void player_model_level(uint8_t* rdram, recomp_context* ctx, recomp_func_t* next) {
    if (!enabled()) {
        next(rdram, ctx);
        return;
    }
    // 0x80094070 opens a 0x40-byte frame and calls func_800EEAD4 from it.
    Pending frame{ADD32(ctx->r29, -0x40)};
    Scope scope(player, frame);
    next(rdram, ctx);
}

void player_distance(uint8_t* rdram, recomp_context* ctx, recomp_func_t* next) {
    const bool applies = take(player, ctx);
    next(rdram, ctx);
    if (!applies) {
        return;
    }
    const bool change = enabled() && std::isfinite(ctx->f0.fl) && ctx->f0.fl != 0.0f;
    if (change) {
        ctx->f0.fl = 0.0f;
    }
    diag.count(diag.player_seen, diag.player_forced, change);
}
}
