// T15: C2AB8-shaped preemption microbenchmark. No guest ROM.
// Modes:
//   A  flag OFF
//   B  flag ON, no external publish (hint miss)
//   C  flag ON, host publishes at ~60 Hz
// Functional gates only: A/B never service; C services sparse publishes,
// not one service per backedge. No performance percentage gate.

#include "ultramodern/ultramodern.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>
#include <algorithm>

// ultramodern.a expects these from the host (librecomp / harness).
// T15 never creates guest OSThreads, so the stubs are unused.
std::atomic_bool exited{false};
void run_thread_function(uint8_t* rdram, uint64_t addr, uint64_t sp, uint64_t arg) {
    (void)rdram;
    (void)addr;
    (void)sp;
    (void)arg;
}

namespace {

struct ModeResult {
    const char* name;
    bool flag_on;
    bool publish;
    uint64_t n;
    uint64_t hint_true;
    uint64_t hint_false;
    uint64_t service;
    uint64_t publish_count;
    double wall_ms;
    double ns_per_backedge;
};

constexpr uint64_t kInner = 60;
constexpr uint64_t kOuter = 166667; // 60 * 166667 = 10,000,020
constexpr int kReps = 3;

ModeResult run_mode(const char* name, bool flag_on, bool publish) {
    ultramodern::set_main_thread();
    ultramodern::set_guest_preemption_safepoints(flag_on);

    std::atomic<bool> stop{false};
    std::atomic<uint64_t> publish_count{0};
    std::thread publisher;
    if (publish) {
        publisher = std::thread([&] {
            while (!stop.load(std::memory_order_relaxed)) {
                ultramodern::publish_external_work();
                publish_count.fetch_add(1, std::memory_order_relaxed);
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }
        });
    }

    uint64_t hint_true = 0;
    uint64_t hint_false = 0;
    uint64_t service = 0;
    const auto t0 = std::chrono::steady_clock::now();
    for (uint64_t outer = 0; outer < kOuter; ++outer) {
        for (uint64_t inner = 0; inner < kInner; ++inner) {
            if (ultramodern_preemption_hint()) {
                ++hint_true;
                ultramodern_service_preemption(nullptr, nullptr);
                ++service;
            } else {
                ++hint_false;
            }
        }
    }
    const auto t1 = std::chrono::steady_clock::now();

    if (publish) {
        stop.store(true, std::memory_order_relaxed);
        publisher.join();
    }

    // Leave the process-default OFF after each mode (startup-only contract).
    ultramodern::set_guest_preemption_safepoints(false);

    const double wall_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    const uint64_t n = kInner * kOuter;
    ModeResult r{};
    r.name = name;
    r.flag_on = flag_on;
    r.publish = publish;
    r.n = n;
    r.hint_true = hint_true;
    r.hint_false = hint_false;
    r.service = service;
    r.publish_count = publish_count.load(std::memory_order_relaxed);
    r.wall_ms = wall_ms;
    r.ns_per_backedge = (wall_ms * 1.0e6) / static_cast<double>(n);
    return r;
}

void print_result(const ModeResult& r) {
    std::printf("T15-%s flag=%s publish=%s N=%llu hint_true=%llu hint_false=%llu "
                "service=%llu publish_count=%llu wall_ms=%.3f ns/backedge=%.3f\n",
                r.name,
                r.flag_on ? "ON" : "OFF",
                r.publish ? "YES" : "NO",
                static_cast<unsigned long long>(r.n),
                static_cast<unsigned long long>(r.hint_true),
                static_cast<unsigned long long>(r.hint_false),
                static_cast<unsigned long long>(r.service),
                static_cast<unsigned long long>(r.publish_count),
                r.wall_ms,
                r.ns_per_backedge);
}

double median(std::vector<double> v) {
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}

} // namespace

int main() {
    std::printf("T15 C2AB8-shaped preemption microbenchmark\n");
    std::printf("CPU/OS recorded by the caller. compiler=clang++ opt=Release inner=%llu outer=%llu\n",
                static_cast<unsigned long long>(kInner),
                static_cast<unsigned long long>(kOuter));

    std::vector<ModeResult> a_reps, b_reps, c_reps;
    bool ok = true;
    for (int rep = 0; rep < kReps; ++rep) {
        std::printf("T15 rep %d/%d\n", rep + 1, kReps);
        const ModeResult a = run_mode("A", false, false);
        const ModeResult b = run_mode("B", true, false);
        const ModeResult c = run_mode("C", true, true);
        print_result(a);
        print_result(b);
        print_result(c);

        if (a.hint_true != 0 || a.service != 0) {
            std::printf("T15-A FAIL: expected hint false and service 0\n");
            ok = false;
        }
        if (b.hint_true != 0 || b.service != 0) {
            std::printf("T15-B FAIL: expected hint-miss fast path, service 0\n");
            ok = false;
        }
        if (c.service == 0 && c.publish_count == 0) {
            std::printf("T15-C FAIL: no publishes observed\n");
            ok = false;
        }
        if (c.service == c.n) {
            std::printf("T15-C FAIL: service-per-backedge\n");
            ok = false;
        }
        if (c.publish_count > 0 && c.service > c.publish_count * 4 + 8) {
            std::printf("T15-C FAIL: services (%llu) not sparse vs publishes (%llu)\n",
                        static_cast<unsigned long long>(c.service),
                        static_cast<unsigned long long>(c.publish_count));
            ok = false;
        }
        a_reps.push_back(a);
        b_reps.push_back(b);
        c_reps.push_back(c);
    }

    auto collect = [](const std::vector<ModeResult>& reps) {
        std::vector<double> ns;
        ns.reserve(reps.size());
        for (const auto& r : reps) {
            ns.push_back(r.ns_per_backedge);
        }
        return ns;
    };
    const auto a_ns = collect(a_reps);
    const auto b_ns = collect(b_reps);
    const auto c_ns = collect(c_reps);
    const double a_med = median(a_ns);
    const double b_med = median(b_ns);
    const double c_med = median(c_ns);

    std::printf("T15-A ns/backedge median=%.3f range=[%.3f, %.3f]\n",
                a_med, *std::min_element(a_ns.begin(), a_ns.end()),
                *std::max_element(a_ns.begin(), a_ns.end()));
    std::printf("T15-B ns/backedge median=%.3f range=[%.3f, %.3f]\n",
                b_med, *std::min_element(b_ns.begin(), b_ns.end()),
                *std::max_element(b_ns.begin(), b_ns.end()));
    std::printf("T15-C ns/backedge median=%.3f range=[%.3f, %.3f]\n",
                c_med, *std::min_element(c_ns.begin(), c_ns.end()),
                *std::max_element(c_ns.begin(), c_ns.end()));
    std::printf("T15 B_OVER_A=%.3f\n", a_med > 0.0 ? (b_med / a_med) : 0.0);
    std::printf("T15 last-rep publish_count=%llu service_count=%llu\n",
                static_cast<unsigned long long>(c_reps.back().publish_count),
                static_cast<unsigned long long>(c_reps.back().service));
    std::printf("T15 SERVICE_PER_BACKEDGE_B=NO SERVICE_PER_BACKEDGE_C=%s\n",
                c_reps.back().service == c_reps.back().n ? "YES" : "NO");
    std::printf("T15 %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
