#include "ultramodern/ultramodern.hpp"

#include <atomic>
#include <cassert>

// PATCH 2: epoch + hint + service protocol. PATCH 3.5: cause-aware drain
// lives in mesgqueue; this file owns the flag, epoch, TLS, and the two
// eligibility-change triggers. Env ULTRAMODERN_GUEST_PREEMPTION remains
// PATCH 4. Feature defaults OFF: hint is false and service returns without
// draining or swapping.

namespace {

std::atomic<bool> g_guest_preemption_enabled{false};
std::atomic<uint64_t> g_work_epoch{0};
thread_local uint64_t tls_last_seen = 0;
thread_local uint8_t* tls_rdram = nullptr;
thread_local uint32_t* tls_status_reg = nullptr;

} // namespace

void ultramodern::set_guest_preemption_safepoints(bool enabled) {
    const bool was = g_guest_preemption_enabled.exchange(enabled, std::memory_order_relaxed);
    // Production treats the flag as startup-immutable. Tests may toggle
    // only while guest service is quiescent. Do not mutate concurrently
    // with an active guest drain: held_ineligible is token-serialized,
    // but this public setter is not. PATCH 4 env is startup init, not
    // hot toggling.
    // If we turn OFF with held hardware events, flush them back onto the
    // MPMC so the legacy drain-all path can see them. Do not strand.
    if (was && !enabled) {
        ultramodern::flush_held_ineligible_to_external();
    }
}

bool ultramodern::guest_preemption_safepoints_enabled() {
    return g_guest_preemption_enabled.load(std::memory_order_relaxed);
}

void ultramodern::publish_external_work() {
    g_work_epoch.fetch_add(1, std::memory_order_release);
}

void ultramodern::set_current_guest_interrupt_tls(uint8_t* rdram, uint32_t* status_reg) {
    tls_rdram = rdram;
    tls_status_reg = status_reg;
}

uint8_t* ultramodern::current_guest_rdram() {
    return tls_rdram;
}

uint32_t* ultramodern::current_guest_status_reg() {
    return tls_status_reg;
}

bool ultramodern::cpu_preemptible(uint32_t status_reg) {
    // PATCH 3.5: hardware causes are class-filtered before they can wake a
    // thread. The swap gate is therefore only CPU execution state.
    // InterruptCause::None (software IPC) may legally schedule while RCP
    // classes are masked, provided IE && !EXL && !ERL.
    if ((status_reg & ultramodern::intmask::SR_IE) == 0) {
        return false;
    }
    if ((status_reg & ultramodern::intmask::SR_EXL) != 0) {
        return false;
    }
    if ((status_reg & ultramodern::intmask::SR_ERL) != 0) {
        return false;
    }
    return true;
}

void ultramodern::service_preemption(RDRAM_ARG uint32_t* status_reg) {
    if (!g_guest_preemption_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    assert(ultramodern::is_game_thread() && "service_preemption is game-thread only");

    if (rdram != nullptr) {
        tls_rdram = rdram;
    }
    if (status_reg != nullptr) {
        tls_status_reg = status_reg;
    }

    for (;;) {
        const uint64_t seen = g_work_epoch.load(std::memory_order_acquire);
        const bool had_requeue = ultramodern::dequeue_external_messages(PASS_RDRAM1);
        if (had_requeue) {
            // Pending work remains. Publish so hint stays true, but do not
            // treat our own requeue as "new work" that requires another spin
            // (a full guest mq would livelock). Do not publish for hold.
            ultramodern::publish_external_work();
        }
        if (status_reg != nullptr && ultramodern::cpu_preemptible(*status_reg)) {
            ultramodern::check_running_queue(PASS_RDRAM1);
        }
        tls_last_seen = seen;
        const uint64_t now = g_work_epoch.load(std::memory_order_relaxed);
        if (now == seen) {
            break;
        }
        if (had_requeue && now == seen + 1) {
            break;
        }
    }
}

void ultramodern::after_interrupt_state_change(RDRAM_ARG uint32_t* status_reg) {
    if (!g_guest_preemption_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    ultramodern::service_preemption(PASS_RDRAM status_reg);
}

void ultramodern::service_pending_interrupts_for_current_guest(RDRAM_ARG1) {
    if (!g_guest_preemption_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    if (tls_status_reg == nullptr) {
        return;
    }
    ultramodern::service_preemption(PASS_RDRAM tls_status_reg);
}

extern "C" bool ultramodern_preemption_hint(void) {
    if (!g_guest_preemption_enabled.load(std::memory_order_relaxed)) {
        return false;
    }
    return g_work_epoch.load(std::memory_order_relaxed) != tls_last_seen;
}

extern "C" void ultramodern_service_preemption(uint8_t* rdram, void* status_reg_opaque) {
    // Generated code passes (void*)&ctx->status_reg. Do not interpret this
    // as a recomp_context* (anonymous typedef; no layout dependency here).
    auto* status_reg = static_cast<uint32_t*>(status_reg_opaque);
    ultramodern::service_preemption(rdram, status_reg);
}
