#include "tooie_diag.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <thread>

#include <SDL2/SDL.h>

namespace tooie_diag {

// Defined below; used by ensure_init's background thread.
void report_now(const char* reason);

namespace {

std::atomic<bool> g_enabled{false};
std::atomic<bool> g_inited{false};
std::atomic<uint64_t> g_syscall_dispatch{0};
std::atomic<uint64_t> g_syscall_return{0};
std::atomic<uint64_t> g_sched_ack{0};
std::atomic<uint64_t> g_rsp_aud{0};
std::atomic<uint64_t> g_queue_calls{0};
std::atomic<uint64_t> g_queue_samples{0};
std::atomic<uint64_t> g_queue_nonzero{0};
std::atomic<uint32_t> g_last_queue_bytes{0};
std::atomic<int32_t> g_last_peak{0};
std::atomic<int32_t> g_max_peak{0};
std::atomic<uint32_t> g_audio_device{0};
std::atomic<uint32_t> g_last_ovl{0};
std::atomic<uint32_t> g_last_ep{0};
std::atomic<uint32_t> g_depth{0};
std::atomic<uint32_t> g_max_depth{0};

// Nested stack of in-flight syscalls (ovl<<16|ep), up to 8 levels.
constexpr int kStackMax = 8;
std::atomic<uint32_t> g_stack[kStackMax]{};
char g_driver[64]{};
char g_device_name[128]{};
std::mutex g_name_mu;

std::chrono::steady_clock::time_point g_last_report{};
int g_interval_ms = 2000;

void ensure_init() {
    bool expected = false;
    if (!g_inited.compare_exchange_strong(expected, true)) {
        return;
    }
    const char* e = std::getenv("TOOIE_DIAG");
    g_enabled.store(e != nullptr && e[0] != '\0' && e[0] != '0', std::memory_order_relaxed);
    const char* iv = std::getenv("TOOIE_DIAG_INTERVAL_MS");
    if (iv && iv[0]) {
        int v = std::atoi(iv);
        if (v >= 200) {
            g_interval_ms = v;
        }
    }
    g_last_report = std::chrono::steady_clock::now();
    if (g_enabled.load(std::memory_order_relaxed)) {
        std::fprintf(stderr,
                     "[tooie-diag] enabled interval=%dms "
                     "(syscall/rsp/queue/sched counters + hang stack)\n",
                     g_interval_ms);
        std::fflush(stderr);
        // Background ticker so we still report when the game thread is stuck
        // and update_gfx is not running.
        std::thread([] {
            while (true) {
                std::this_thread::sleep_for(std::chrono::milliseconds(g_interval_ms));
                if (!g_enabled.load(std::memory_order_relaxed)) {
                    continue;
                }
                report_now("bg");
            }
        }).detach();
    }
}

void format_stack(char* buf, size_t buflen) {
    const uint32_t depth = g_depth.load(std::memory_order_relaxed);
    size_t off = 0;
    if (depth == 0) {
        std::snprintf(buf, buflen, "(idle)");
        return;
    }
    for (uint32_t i = 0; i < depth && i < kStackMax && off + 1 < buflen; ++i) {
        const uint32_t v = g_stack[i].load(std::memory_order_relaxed);
        const int n = std::snprintf(buf + off, buflen - off, "%s%u:%u",
                                    (i ? ">" : ""), v >> 16, v & 0xFFFFu);
        if (n < 0) {
            break;
        }
        off += static_cast<size_t>(n);
    }
}

} // namespace

bool enabled() {
    ensure_init();
    return g_enabled.load(std::memory_order_relaxed);
}

void note_syscall_dispatch(uint32_t ovl, uint32_t ep) {
    ensure_init();
    g_syscall_dispatch.fetch_add(1, std::memory_order_relaxed);
    g_last_ovl.store(ovl, std::memory_order_relaxed);
    g_last_ep.store(ep, std::memory_order_relaxed);
    const uint32_t d = g_depth.fetch_add(1, std::memory_order_relaxed);
    if (d < kStackMax) {
        g_stack[d].store((ovl << 16) | (ep & 0xFFFFu), std::memory_order_relaxed);
    }
    uint32_t maxd = g_max_depth.load(std::memory_order_relaxed);
    while (d + 1 > maxd &&
           !g_max_depth.compare_exchange_weak(maxd, d + 1, std::memory_order_relaxed)) {
    }
}

void note_syscall_return(uint32_t ovl, uint32_t ep) {
    ensure_init();
    (void)ovl;
    (void)ep;
    g_syscall_return.fetch_add(1, std::memory_order_relaxed);
    uint32_t d = g_depth.load(std::memory_order_relaxed);
    if (d > 0) {
        g_depth.store(d - 1, std::memory_order_relaxed);
    }
}

void note_sched_ack() {
    ensure_init();
    g_sched_ack.fetch_add(1, std::memory_order_relaxed);
}

void note_rsp_audtask() {
    ensure_init();
    g_rsp_aud.fetch_add(1, std::memory_order_relaxed);
}

void note_queue_samples(size_t sample_count, uint32_t queued_bytes_after,
                        int16_t peak_abs, size_t nonzero) {
    ensure_init();
    g_queue_calls.fetch_add(1, std::memory_order_relaxed);
    g_queue_samples.fetch_add(sample_count, std::memory_order_relaxed);
    g_queue_nonzero.fetch_add(nonzero, std::memory_order_relaxed);
    g_last_queue_bytes.store(queued_bytes_after, std::memory_order_relaxed);
    g_last_peak.store(peak_abs, std::memory_order_relaxed);
    int32_t maxp = g_max_peak.load(std::memory_order_relaxed);
    while (peak_abs > maxp &&
           !g_max_peak.compare_exchange_weak(maxp, peak_abs, std::memory_order_relaxed)) {
    }
}

void note_audio_device(uint32_t device_id, const char* driver, const char* device_name) {
    ensure_init();
    g_audio_device.store(device_id, std::memory_order_relaxed);
    std::lock_guard<std::mutex> lock(g_name_mu);
    if (driver) {
        std::snprintf(g_driver, sizeof(g_driver), "%s", driver);
    }
    if (device_name) {
        std::snprintf(g_device_name, sizeof(g_device_name), "%s", device_name);
    }
    if (enabled()) {
        std::fprintf(stderr,
                     "[tooie-diag] audio device id=%u driver=%s name=\"%s\"\n",
                     device_id,
                     g_driver[0] ? g_driver : "?",
                     g_device_name[0] ? g_device_name : "?");
        std::fflush(stderr);
    }
}

void report_now(const char* reason) {
    ensure_init();
    if (!enabled()) {
        return;
    }
    char stack[160];
    format_stack(stack, sizeof(stack));
    char driver[64];
    char dname[128];
    {
        std::lock_guard<std::mutex> lock(g_name_mu);
        std::snprintf(driver, sizeof(driver), "%s", g_driver[0] ? g_driver : "?");
        std::snprintf(dname, sizeof(dname), "%s", g_device_name[0] ? g_device_name : "?");
    }
    std::fprintf(stderr,
                 "[tooie-diag] %s "
                 "syscall d=%llu r=%llu inflight=%lld "
                 "sched_ack=%llu rsp_aud=%llu "
                 "queue_calls=%llu queue_samp=%llu nonzero=%llu peak=%d maxpeak=%d queue_bytes=%u "
                 "audio_dev=%u/%s/\"%s\" "
                 "last_ovl=%u:%u depth=%u max_depth=%u stack=%s\n",
                 reason ? reason : "report",
                 static_cast<unsigned long long>(g_syscall_dispatch.load()),
                 static_cast<unsigned long long>(g_syscall_return.load()),
                 static_cast<long long>(g_syscall_dispatch.load() - g_syscall_return.load()),
                 static_cast<unsigned long long>(g_sched_ack.load()),
                 static_cast<unsigned long long>(g_rsp_aud.load()),
                 static_cast<unsigned long long>(g_queue_calls.load()),
                 static_cast<unsigned long long>(g_queue_samples.load()),
                 static_cast<unsigned long long>(g_queue_nonzero.load()),
                 g_last_peak.load(),
                 g_max_peak.load(),
                 g_last_queue_bytes.load(),
                 g_audio_device.load(),
                 driver,
                 dname,
                 g_last_ovl.load(),
                 g_last_ep.load(),
                 g_depth.load(),
                 g_max_depth.load(),
                 stack);
    std::fflush(stderr);
}

void tick() {
    ensure_init();
    if (!enabled()) {
        return;
    }
    const auto now = std::chrono::steady_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_last_report).count();
    if (ms < g_interval_ms) {
        return;
    }
    g_last_report = now;
    report_now("tick");
}

} // namespace tooie_diag
