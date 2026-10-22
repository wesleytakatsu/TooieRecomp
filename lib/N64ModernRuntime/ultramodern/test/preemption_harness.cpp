// PATCH 2+3 standalone harness for Hybrid F' guest preemption.
// T01 repaired (service after enqueue). T02–T10 and T12 implemented.
// T11-F proves the C ABI consumes a status_reg pointer (not whole ctx).
// Static/live generator T11 lives in N64Recomp PreemptionSafepointTest.
// Threaded tests run in a child process so leftover guest std::threads
// cannot poison later cases.

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#include "ultramodern/ultra64.h"
#include "ultramodern/ultramodern.hpp"

extern "C" void yield_self_1ms(uint8_t* rdram);

std::atomic_bool exited{false};

namespace {

int g_pass = 0;
int g_fail = 0;
int g_skip = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (cond) {                                                          \
            ++g_pass;                                                        \
        } else {                                                             \
            ++g_fail;                                                        \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                    \
    } while (0)

void skip(const char* id, const char* why) {
    ++g_skip;
    std::printf("SKIPPED %s — %s\n", id, why);
}

constexpr uint32_t kGuestBase = 0x80000000u;
constexpr size_t kRdramBytes = 0x10000;
constexpr uint32_t kOffQueueHead = 0x0F00;
constexpr uint32_t kOffMainThread = 0x1000;
constexpr uint32_t kOffAudioThread = 0x1100;
constexpr uint32_t kOffWaiterThread = 0x1200;
constexpr uint32_t kOffAudioMq = 0x1300;
constexpr uint32_t kOffDummyMq = 0x1380;
constexpr uint32_t kOffExtraMq = 0x1400;
constexpr uint32_t kOffAudioMsg = 0x1500;
constexpr uint32_t kOffDummyMsg = 0x1540;
constexpr uint32_t kOffExtraMsg = 0x1580;
constexpr uint32_t kOffMainSp = 0x2000;
constexpr uint32_t kOffAudioSp = 0x3000;
constexpr uint32_t kOffWaiterSp = 0x4000;

constexpr int32_t kEntryMainT01 = 1;
constexpr int32_t kEntryAudioOnce = 2;
constexpr int32_t kEntryMainService = 3;
constexpr int32_t kEntryMainMasked = 4;
constexpr int32_t kEntryMainNone = 5;
constexpr int32_t kEntryWaiterOnce = 6;
constexpr int32_t kEntryMainSamePri = 7;
constexpr int32_t kEntryMainBurst = 8;
constexpr int32_t kEntryWaiterVi = 9;
constexpr int32_t kEntryMainRequeue = 10;
constexpr int32_t kEntryMainRepeat = 11;
constexpr int32_t kEntryAudioRepeat = 12;
constexpr int32_t kEntryMainFifo = 13;
constexpr int32_t kEntryAudioFifo = 14;
constexpr int32_t kEntryMainCAbiMasked = 15;
constexpr int32_t kEntryMainCAbiUnmasked = 16;
constexpr int32_t kEntryMainX1A = 17;
constexpr int32_t kEntryMainX1G = 18;
constexpr int32_t kEntryThreadBX1G = 19;
constexpr int32_t kEntryMainX1E = 20;
constexpr int32_t kEntryMainRequeueCause = 21;
constexpr int32_t kEntryMainTls = 22;
constexpr int32_t kEntryAudioTls = 23;
constexpr int32_t kEntryMainP3X6 = 24;
constexpr int32_t kEntryAudioP3X6 = 25;
constexpr int32_t kEntryMainX1F = 26;
constexpr int32_t kEntryMainNonint = 27;
constexpr int32_t kEntryMainRcpX1 = 28;
constexpr int32_t kEntryThreadBRcpX1 = 29;
constexpr int32_t kEntryMainRcpX2 = 30;
constexpr int32_t kEntryThreadBRcpX2 = 31;
constexpr int32_t kEntryThreadARcpX3 = 32;
constexpr int32_t kEntryThreadBRcpX3 = 33;
constexpr int32_t kEntryThreadCRcpX3 = 34;
constexpr int32_t kEntryMainRcpX4 = 35;
constexpr int32_t kEntryThreadBRcpX4 = 36;

constexpr uint32_t kOsImSpDp = ultramodern::intmask::OS_IM_SP | ultramodern::intmask::OS_IM_DP;
constexpr uint32_t kRcpAi = 0x00040000u;
constexpr uint32_t kRcpVi = 0x00080000u;
constexpr uint32_t kRcpSpDp = 0x00210000u;

enum class SwapPhase : int {
    Unset = 0,
    BeforeService = 1,
    InServiceWindow = 2,
    AfterService = 3,
};

constexpr OSPri kPriMain = 20;
constexpr OSPri kPriAudio = 50;
constexpr OSPri kPriMid = 30;
constexpr OSPri kPriLow = 10;

uint8_t* g_rdram = nullptr;

PTR(void) gptr(uint32_t off) {
    return static_cast<int32_t>(kGuestBase + off);
}

template <typename Pred>
bool wait_until(Pred pred, int timeout_ms) {
    const auto start = std::chrono::steady_clock::now();
    while (!pred()) {
        if (std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start)
                .count() > timeout_ms) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return true;
}

struct CoopState {
    std::atomic<bool> audio_blocked{false};
    std::atomic<bool> audio_ran{false};
    std::atomic<bool> service_did_not_wake{false};
    std::atomic<bool> host_enqueued{false};
    std::atomic<bool> main_resumed{false};
    std::atomic<bool> send_recv_ok{false};
    std::atomic<bool> waiter_blocked{false};
    std::atomic<bool> waiter_ran{false};
    std::atomic<bool> swapped_while_masked{false};
    std::atomic<int> audio_count{0};
    std::atomic<int> fifo_next{0};
    std::atomic<bool> fifo_ok{false};
    std::atomic<bool> hint_after_requeue{false};
    std::atomic<bool> started_waiter{false};
    uint32_t status = ultramodern::intmask::status_reg_init();
    uint32_t audio_status = ultramodern::intmask::status_reg_init();
    uint32_t waiter_status = ultramodern::intmask::status_reg_init();
    uint32_t saved_ie = 0;
    uint32_t saved_mask = 0;
    std::atomic<bool> masked_ready{false};
    std::atomic<bool> unmask_now{false};
    std::atomic<bool> b_started{false};
    std::atomic<bool> b_resumed{false};
    std::atomic<bool> toy_survived{false};
    uint32_t toy_gpr2 = 0;
    uint32_t toy_gpr4 = 0;
    uint32_t toy_gpr16 = 0;
    uint32_t toy_hi = 0;
    uint32_t toy_lo = 0;
    uint32_t toy_fpu0 = 0;
    uint32_t toy_fpu1 = 0;
    uint32_t toy_local = 0;
    uint32_t toy_local2 = 0;
    uint32_t toy_status_after = 0;
    std::atomic<int> phase{static_cast<int>(SwapPhase::Unset)};
    std::atomic<int> phase_seen_by_high{static_cast<int>(SwapPhase::Unset)};
    uint32_t rcp_old_r2 = 0;
    uint32_t rcp_mask_a = 0;
    uint32_t rcp_mask_b = 0;
    uint32_t rcp_mask_c = 0;
    std::atomic<int> rcp_cycles_a{0};
    std::atomic<int> rcp_cycles_b{0};
    std::atomic<int> rcp_cycles_c{0};
    std::atomic<int> rcp_ready{0};
    std::atomic<bool> rcp_leak{false};
    std::atomic<size_t> held_at_service{0};
};

CoopState* g_coop = nullptr;

void park_forever(uint8_t* rdram) {
    while (true) {
        yield_self_1ms(rdram);
    }
}

void audio_once(uint8_t* rdram) {
    g_coop->audio_blocked.store(true, std::memory_order_release);
    const s32 rc = osRecvMesg(rdram, gptr(kOffAudioMq), gptr(kOffAudioMsg), OS_MESG_BLOCK);
    if (rc == 0) {
        g_coop->audio_ran.store(true, std::memory_order_release);
        g_coop->audio_count.fetch_add(1, std::memory_order_acq_rel);
    }
    osRecvMesg(rdram, gptr(kOffAudioMq), gptr(kOffAudioMsg), OS_MESG_BLOCK);
}

void waiter_once(uint8_t* rdram, uint32_t mq_off, uint32_t msg_off) {
    g_coop->waiter_blocked.store(true, std::memory_order_release);
    const s32 rc = osRecvMesg(rdram, gptr(mq_off), gptr(msg_off), OS_MESG_BLOCK);
    if (rc == 0) {
        g_coop->waiter_ran.store(true, std::memory_order_release);
    }
    osRecvMesg(rdram, gptr(mq_off), gptr(msg_off), OS_MESG_BLOCK);
}

void main_t01(uint8_t* rdram) {
    const OSMesg sent = 42;
    const s32 send_rc = osSendMesg(rdram, gptr(kOffDummyMq), sent, OS_MESG_NOBLOCK);
    const s32 recv_rc = osRecvMesg(rdram, gptr(kOffDummyMq), gptr(kOffDummyMsg), OS_MESG_NOBLOCK);
    const OSMesg got = *TO_PTR(OSMesg, gptr(kOffDummyMsg));
    g_coop->send_recv_ok.store(send_rc == 0 && recv_rc == 0 && got == sent, std::memory_order_release);

    osStartThread(rdram, gptr(kOffAudioThread));
    while (!g_coop->audio_blocked.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    while (!g_coop->host_enqueued.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }

    // T01-E: service AFTER the host enqueue, feature OFF.
    ultramodern::service_preemption(rdram, &g_coop->status);
    if (!g_coop->audio_ran.load(std::memory_order_acquire)) {
        g_coop->service_did_not_wake.store(true, std::memory_order_release);
    }

    (void)osSendMesg(rdram, gptr(kOffDummyMq), 1, OS_MESG_NOBLOCK);
    g_coop->main_resumed.store(true, std::memory_order_release);
    park_forever(rdram);
}

void main_service_loop(uint8_t* rdram) {
    osStartThread(rdram, gptr(kOffAudioThread));
    while (!g_coop->audio_blocked.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    while (!g_coop->host_enqueued.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    while (!g_coop->audio_ran.load(std::memory_order_acquire)) {
        ultramodern::service_preemption(rdram, &g_coop->status);
    }
    g_coop->main_resumed.store(true, std::memory_order_release);
    park_forever(rdram);
}

void main_masked(uint8_t* rdram) {
    osStartThread(rdram, gptr(kOffAudioThread));
    while (!g_coop->audio_blocked.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    g_coop->saved_ie = ultramodern::intmask::disable_int(&g_coop->status);
    while (!g_coop->host_enqueued.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    ultramodern::service_preemption(rdram, &g_coop->status);
    g_coop->swapped_while_masked.store(g_coop->audio_ran.load(std::memory_order_acquire),
                                       std::memory_order_release);
    ultramodern::intmask::restore_int(rdram, &g_coop->status, g_coop->saved_ie);
    g_coop->main_resumed.store(true, std::memory_order_release);
    park_forever(rdram);
}

void main_os_im_none(uint8_t* rdram) {
    osStartThread(rdram, gptr(kOffAudioThread));
    while (!g_coop->audio_blocked.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    g_coop->saved_mask = ultramodern::intmask::apply_os_set_int_mask(
        rdram, &g_coop->status, ultramodern::intmask::OS_IM_NONE);
    while (!g_coop->host_enqueued.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    ultramodern::service_preemption(rdram, &g_coop->status);
    g_coop->swapped_while_masked.store(g_coop->audio_ran.load(std::memory_order_acquire),
                                       std::memory_order_release);
    (void)ultramodern::intmask::apply_os_set_int_mask(rdram, &g_coop->status, g_coop->saved_mask);
    ultramodern::after_interrupt_state_change(rdram, &g_coop->status);
    g_coop->main_resumed.store(true, std::memory_order_release);
    park_forever(rdram);
}

void main_same_or_low(uint8_t* rdram) {
    // Same/lower priority: osStartThread only queues. Do not wait for the
    // waiter to execute — MAIN keeps the token. Service must deliver and
    // leave the waiter queued without swapping.
    osStartThread(rdram, gptr(kOffWaiterThread));
    g_coop->started_waiter.store(true, std::memory_order_release);
    while (!g_coop->host_enqueued.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    ultramodern::service_preemption(rdram, &g_coop->status);
    g_coop->main_resumed.store(true, std::memory_order_release);
    park_forever(rdram);
}

void main_burst(uint8_t* rdram) {
    osStartThread(rdram, gptr(kOffAudioThread));
    osStartThread(rdram, gptr(kOffWaiterThread));
    while (!g_coop->audio_blocked.load(std::memory_order_acquire) ||
           !g_coop->waiter_blocked.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    while (!g_coop->host_enqueued.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    ultramodern::service_preemption(rdram, &g_coop->status);
    g_coop->main_resumed.store(true, std::memory_order_release);
    park_forever(rdram);
}

void main_requeue(uint8_t* rdram) {
    CHECK(osSendMesg(rdram, gptr(kOffAudioMq), 1, OS_MESG_NOBLOCK) == 0);
    g_coop->audio_blocked.store(true, std::memory_order_release);
    while (!g_coop->host_enqueued.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    ultramodern::service_preemption(rdram, &g_coop->status);
    g_coop->hint_after_requeue.store(ultramodern_preemption_hint(), std::memory_order_release);
    CHECK(osRecvMesg(rdram, gptr(kOffAudioMq), gptr(kOffAudioMsg), OS_MESG_NOBLOCK) == 0);
    ultramodern::service_preemption(rdram, &g_coop->status);
    const s32 rc = osRecvMesg(rdram, gptr(kOffAudioMq), gptr(kOffAudioMsg), OS_MESG_NOBLOCK);
    g_coop->audio_ran.store(rc == 0, std::memory_order_release);
    g_coop->main_resumed.store(true, std::memory_order_release);
    park_forever(rdram);
}

void main_repeat(uint8_t* rdram) {
    osStartThread(rdram, gptr(kOffAudioThread));
    while (g_coop->audio_count.load(std::memory_order_acquire) < 1000) {
        ultramodern::service_preemption(rdram, &g_coop->status);
    }
    g_coop->main_resumed.store(true, std::memory_order_release);
    park_forever(rdram);
}

void audio_repeat(uint8_t* rdram) {
    for (int i = 0; i < 1000; ++i) {
        g_coop->audio_blocked.store(true, std::memory_order_release);
        const s32 rc = osRecvMesg(rdram, gptr(kOffAudioMq), gptr(kOffAudioMsg), OS_MESG_BLOCK);
        if (rc == 0) {
            g_coop->audio_count.fetch_add(1, std::memory_order_acq_rel);
            g_coop->audio_blocked.store(false, std::memory_order_release);
        }
    }
    // Block (do not yield_self) so the token returns to MAIN after the
    // last swap. yield_self_1ms would keep pri-50 running forever.
    osRecvMesg(rdram, gptr(kOffAudioMq), gptr(kOffAudioMsg), OS_MESG_BLOCK);
}

void audio_fifo(uint8_t* rdram) {
    g_coop->audio_blocked.store(true, std::memory_order_release);
    bool ok = true;
    for (int expect = 1; expect <= 3; ++expect) {
        const s32 rc = osRecvMesg(rdram, gptr(kOffAudioMq), gptr(kOffAudioMsg), OS_MESG_BLOCK);
        const OSMesg got = *TO_PTR(OSMesg, gptr(kOffAudioMsg));
        if (rc != 0 || got != expect) {
            ok = false;
        }
        g_coop->fifo_next.store(expect, std::memory_order_release);
    }
    g_coop->fifo_ok.store(ok, std::memory_order_release);
    g_coop->audio_ran.store(true, std::memory_order_release);
    park_forever(rdram);
}

void main_fifo(uint8_t* rdram) {
    osStartThread(rdram, gptr(kOffAudioThread));
    while (!g_coop->audio_blocked.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    while (!g_coop->host_enqueued.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    ultramodern::service_preemption(rdram, &g_coop->status);
    g_coop->main_resumed.store(true, std::memory_order_release);
    park_forever(rdram);
}

void main_cabi_masked(uint8_t* rdram) {
    osStartThread(rdram, gptr(kOffAudioThread));
    while (!g_coop->audio_blocked.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    while (!g_coop->host_enqueued.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    uint32_t masked = 0; // IE=0 — not preemptible if the pointer is honored
    ultramodern_service_preemption(rdram, &masked);
    g_coop->swapped_while_masked.store(g_coop->audio_ran.load(std::memory_order_acquire),
                                       std::memory_order_release);
    g_coop->main_resumed.store(true, std::memory_order_release);
    park_forever(rdram);
}

void main_cabi_unmasked(uint8_t* rdram) {
    osStartThread(rdram, gptr(kOffAudioThread));
    while (!g_coop->audio_blocked.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    while (!g_coop->host_enqueued.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    uint32_t unmasked = ultramodern::intmask::status_reg_init();
    ultramodern_service_preemption(rdram, &unmasked);
    g_coop->main_resumed.store(true, std::memory_order_release);
    park_forever(rdram);
}

void main_x1a(uint8_t* rdram) {
    osStartThread(rdram, gptr(kOffAudioThread));
    while (!g_coop->audio_blocked.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    g_coop->saved_mask = ultramodern::intmask::apply_os_set_int_mask(
        rdram, &g_coop->status, ultramodern::intmask::OS_IM_VI);
    g_coop->masked_ready.store(true, std::memory_order_release);
    while (!g_coop->host_enqueued.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    ultramodern::service_preemption(rdram, &g_coop->status);
    g_coop->swapped_while_masked.store(g_coop->audio_ran.load(std::memory_order_acquire),
                                       std::memory_order_release);
    g_coop->main_resumed.store(true, std::memory_order_release);
    while (!g_coop->unmask_now.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    (void)ultramodern::intmask::apply_os_set_int_mask(
        rdram, &g_coop->status, ultramodern::intmask::OS_IM_AI);
    ultramodern::after_interrupt_state_change(rdram, &g_coop->status);
    park_forever(rdram);
}

void main_x1g(uint8_t* rdram) {
    osStartThread(rdram, gptr(kOffAudioThread));
    while (!g_coop->audio_blocked.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    // Mask AI via this thread's SR.IM2 only so process-global MI stays open.
    // B's own status (IM2 set) can then make the held AI eligible on resume.
    g_coop->status &= ~ultramodern::intmask::SR_IM2;
    g_coop->masked_ready.store(true, std::memory_order_release);
    while (!g_coop->host_enqueued.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    // Hold the masked AI on A. Do not publish. B's resume hook must retry.
    ultramodern::service_preemption(rdram, &g_coop->status);
    osRecvMesg(rdram, gptr(kOffDummyMq), gptr(kOffDummyMsg), OS_MESG_BLOCK);
    g_coop->main_resumed.store(true, std::memory_order_release);
    park_forever(rdram);
}

void thread_b_x1g(uint8_t* rdram) {
    g_coop->b_started.store(true, std::memory_order_release);
    // A is higher priority: this queues B and swaps to A. Resume is a true
    // wait_for_resumed (TLS already set), not a first start.
    osStartThread(rdram, gptr(kOffMainThread));
    g_coop->b_resumed.store(true, std::memory_order_release);
    park_forever(rdram);
}

void main_x1f(uint8_t* rdram) {
    osStartThread(rdram, gptr(kOffAudioThread));
    while (!g_coop->audio_blocked.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    (void)ultramodern::intmask::apply_os_set_int_mask(
        rdram, &g_coop->status, ultramodern::intmask::OS_IM_AI);
    g_coop->status |= ultramodern::intmask::SR_EXL;
    g_coop->masked_ready.store(true, std::memory_order_release);
    while (!g_coop->host_enqueued.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    ultramodern::service_preemption(rdram, &g_coop->status);
    g_coop->swapped_while_masked.store(g_coop->audio_ran.load(std::memory_order_acquire),
                                       std::memory_order_release);
    g_coop->main_resumed.store(true, std::memory_order_release);
    while (!g_coop->unmask_now.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    // M1/X1-F: commit EXL clear, then the same after-commit hook MTC0 uses.
    g_coop->status &= ~ultramodern::intmask::SR_EXL;
    ultramodern::after_interrupt_state_change(rdram, &g_coop->status);
    park_forever(rdram);
}

void main_nonint(uint8_t* rdram) {
    g_coop->saved_mask = ultramodern::intmask::apply_os_set_int_mask(
        rdram, &g_coop->status, ultramodern::intmask::OS_IM_VI);
    g_coop->masked_ready.store(true, std::memory_order_release);
    while (!g_coop->host_enqueued.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    ultramodern::service_preemption(rdram, &g_coop->status);
    g_coop->main_resumed.store(true, std::memory_order_release);
    park_forever(rdram);
}

void main_x1e(uint8_t* rdram) {
    osStartThread(rdram, gptr(kOffAudioThread));
    while (!g_coop->audio_blocked.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    // AI class open, IE cleared.
    (void)ultramodern::intmask::apply_os_set_int_mask(
        rdram, &g_coop->status, ultramodern::intmask::OS_IM_AI);
    g_coop->saved_ie = ultramodern::intmask::disable_int(&g_coop->status);
    g_coop->masked_ready.store(true, std::memory_order_release);
    while (!g_coop->host_enqueued.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    ultramodern::service_preemption(rdram, &g_coop->status);
    g_coop->swapped_while_masked.store(g_coop->audio_ran.load(std::memory_order_acquire),
                                       std::memory_order_release);
    ultramodern::intmask::restore_int(rdram, &g_coop->status, g_coop->saved_ie);
    g_coop->main_resumed.store(true, std::memory_order_release);
    park_forever(rdram);
}

void main_requeue_cause(uint8_t* rdram) {
    CHECK(osSendMesg(rdram, gptr(kOffAudioMq), 1, OS_MESG_NOBLOCK) == 0);
    g_coop->audio_blocked.store(true, std::memory_order_release);
    (void)ultramodern::intmask::apply_os_set_int_mask(
        rdram, &g_coop->status, ultramodern::intmask::OS_IM_VI);
    g_coop->masked_ready.store(true, std::memory_order_release);
    while (!g_coop->host_enqueued.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    ultramodern::service_preemption(rdram, &g_coop->status);
    CHECK(ultramodern::held_ineligible_count() == 1);
    CHECK(ultramodern::held_ineligible_cause_at(0) == ultramodern::InterruptCause::AI);
    (void)ultramodern::intmask::apply_os_set_int_mask(
        rdram, &g_coop->status, ultramodern::intmask::OS_IM_AI);
    ultramodern::after_interrupt_state_change(rdram, &g_coop->status);
    // Full mq requeue must preserve AI. Remask and retry: held again as AI.
    CHECK(ultramodern::held_ineligible_count() == 0);
    (void)ultramodern::intmask::apply_os_set_int_mask(
        rdram, &g_coop->status, ultramodern::intmask::OS_IM_VI);
    ultramodern::service_preemption(rdram, &g_coop->status);
    CHECK(ultramodern::held_ineligible_count() == 1);
    CHECK(ultramodern::held_ineligible_cause_at(0) == ultramodern::InterruptCause::AI);
    CHECK(osRecvMesg(rdram, gptr(kOffAudioMq), gptr(kOffAudioMsg), OS_MESG_NOBLOCK) == 0);
    (void)ultramodern::intmask::apply_os_set_int_mask(
        rdram, &g_coop->status, ultramodern::intmask::OS_IM_AI);
    ultramodern::after_interrupt_state_change(rdram, &g_coop->status);
    const s32 rc = osRecvMesg(rdram, gptr(kOffAudioMq), gptr(kOffAudioMsg), OS_MESG_NOBLOCK);
    g_coop->audio_ran.store(rc == 0, std::memory_order_release);
    g_coop->main_resumed.store(true, std::memory_order_release);
    park_forever(rdram);
}

void main_tls(uint8_t* rdram) {
    osStartThread(rdram, gptr(kOffAudioThread));
    while (!g_coop->audio_blocked.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    while (!g_coop->host_enqueued.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    ultramodern::service_preemption(rdram, &g_coop->status);
    g_coop->main_resumed.store(true, std::memory_order_release);
    park_forever(rdram);
}

void audio_tls(uint8_t* rdram) {
    g_coop->audio_blocked.store(true, std::memory_order_release);
    const s32 rc = osRecvMesg(rdram, gptr(kOffAudioMq), gptr(kOffAudioMsg), OS_MESG_BLOCK);
    if (rc == 0) {
        g_coop->audio_ran.store(true, std::memory_order_release);
        // Redundant empty service with this thread's older tls_last_seen.
        ultramodern::service_preemption(rdram, &g_coop->audio_status);
        ultramodern::service_preemption(rdram, &g_coop->audio_status);
        g_coop->fifo_ok.store(true, std::memory_order_release);
    }
    osRecvMesg(rdram, gptr(kOffAudioMq), gptr(kOffAudioMsg), OS_MESG_BLOCK);
}

struct ToyCtx {
    uint64_t gpr[32]{};
    uint32_t status_reg = 0;
    uint64_t hi = 0;
    uint64_t lo = 0;
    uint32_t fpu[32]{};
};

void toy_generated(uint8_t* rdram, ToyCtx* ctx) {
    const uint32_t local_addend = 0xA5A5u;
    const uint32_t local_scratch = 0x5A5Au;
    ctx->gpr[2] = 0x11111111u;
    ctx->gpr[4] = 0x44444444u;
    ctx->gpr[16] = 0x16161616u;
    ctx->hi = 0x22222222u;
    ctx->lo = 0x33333333u;
    ctx->fpu[0] = 0x4048F5C3u;
    ctx->fpu[1] = 0x3F800000u;
    g_coop->phase.store(static_cast<int>(SwapPhase::BeforeService), std::memory_order_release);
    if (ultramodern_preemption_hint()) {
        // Service window begins here. HIGH must observe InServiceWindow;
        // a later cooperative park after this function returns is AfterService.
        g_coop->phase.store(static_cast<int>(SwapPhase::InServiceWindow), std::memory_order_release);
        ultramodern_service_preemption(rdram, &ctx->status_reg);
        g_coop->phase.store(static_cast<int>(SwapPhase::AfterService), std::memory_order_release);
    }
    g_coop->toy_gpr2 = static_cast<uint32_t>(ctx->gpr[2]);
    g_coop->toy_gpr4 = static_cast<uint32_t>(ctx->gpr[4]);
    g_coop->toy_gpr16 = static_cast<uint32_t>(ctx->gpr[16]);
    g_coop->toy_hi = static_cast<uint32_t>(ctx->hi);
    g_coop->toy_lo = static_cast<uint32_t>(ctx->lo);
    g_coop->toy_fpu0 = ctx->fpu[0];
    g_coop->toy_fpu1 = ctx->fpu[1];
    g_coop->toy_local = local_addend;
    g_coop->toy_local2 = local_scratch;
    g_coop->toy_status_after = ctx->status_reg;
    g_coop->toy_survived.store(
        ctx->gpr[2] == 0x11111111u && ctx->gpr[4] == 0x44444444u && ctx->gpr[16] == 0x16161616u &&
            ctx->hi == 0x22222222u && ctx->lo == 0x33333333u && ctx->fpu[0] == 0x4048F5C3u &&
            ctx->fpu[1] == 0x3F800000u && local_addend == 0xA5A5u && local_scratch == 0x5A5Au,
        std::memory_order_release);
}

void audio_p3x6(uint8_t* rdram) {
    g_coop->audio_blocked.store(true, std::memory_order_release);
    const s32 rc = osRecvMesg(rdram, gptr(kOffAudioMq), gptr(kOffAudioMsg), OS_MESG_BLOCK);
    if (rc == 0) {
        g_coop->phase_seen_by_high.store(g_coop->phase.load(std::memory_order_acquire),
                                         std::memory_order_release);
        g_coop->audio_ran.store(true, std::memory_order_release);
        g_coop->audio_count.fetch_add(1, std::memory_order_acq_rel);
    }
    osRecvMesg(rdram, gptr(kOffAudioMq), gptr(kOffAudioMsg), OS_MESG_BLOCK);
}

void main_p3x6(uint8_t* rdram) {
    osStartThread(rdram, gptr(kOffAudioThread));
    while (!g_coop->audio_blocked.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    while (!g_coop->host_enqueued.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    ToyCtx ctx{};
    ctx.status_reg = g_coop->status;
    toy_generated(rdram, &ctx);
    g_coop->status = ctx.status_reg;
    g_coop->main_resumed.store(true, std::memory_order_release);
    park_forever(rdram);
}

// Mirrors librecomp osSetIntMask_recomp: commit MI/IM, write previous
// combined mask to the architectural return (r2), THEN service. The
// harness does not link librecomp, so this is the production sequence
// against a local ctx analog. r2 must be B's previous mask after A->B.
struct OsSetIntMaskCtx {
    uint32_t status_reg = 0;
    uint64_t r2 = 0;
    uint64_t r4 = 0;
};

void osSetIntMask_recomp_analog(uint8_t* rdram, OsSetIntMaskCtx* ctx) {
    ctx->r2 = ultramodern::intmask::apply_os_set_int_mask(
        rdram, &ctx->status_reg, static_cast<uint32_t>(ctx->r4));
    ultramodern::after_interrupt_state_change(rdram, &ctx->status_reg);
}

void note_rcp_leak(uint32_t observed, uint32_t want_rcp) {
    if ((observed & ultramodern::intmask::RCP_IMASK) != want_rcp) {
        g_coop->rcp_leak.store(true, std::memory_order_release);
    }
    if ((observed & ultramodern::intmask::SR_IE) == 0) {
        g_coop->rcp_leak.store(true, std::memory_order_release);
    }
    if ((observed & ultramodern::intmask::SR_IM2) == 0) {
        g_coop->rcp_leak.store(true, std::memory_order_release);
    }
}

void main_rcp_x1(uint8_t* rdram) {
    osStartThread(rdram, gptr(kOffAudioThread));
    while (!g_coop->audio_blocked.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    // IE=1 and SR.IM2=1 stay set. Only RCP AI is closed on THIS thread.
    (void)ultramodern::intmask::apply_os_set_int_mask(
        rdram, &g_coop->status, ultramodern::intmask::OS_IM_VI);
    g_coop->masked_ready.store(true, std::memory_order_release);
    while (!g_coop->host_enqueued.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    ultramodern::service_preemption(rdram, &g_coop->status);
    g_coop->held_at_service.store(ultramodern::held_ineligible_count(), std::memory_order_release);
    g_coop->swapped_while_masked.store(g_coop->audio_ran.load(std::memory_order_acquire),
                                       std::memory_order_release);
    osRecvMesg(rdram, gptr(kOffDummyMq), gptr(kOffDummyMsg), OS_MESG_BLOCK);
    g_coop->main_resumed.store(true, std::memory_order_release);
    park_forever(rdram);
}

void thread_b_rcp_x1(uint8_t* rdram) {
    // B's TLS starts at 0x3F. Pin MI.AI open explicitly. Do not touch the
    // mask again after resume — TLS must restore it automatically.
    (void)ultramodern::intmask::apply_os_set_int_mask(
        rdram, &g_coop->waiter_status, ultramodern::intmask::OS_IM_AI);
    g_coop->b_started.store(true, std::memory_order_release);
    osStartThread(rdram, gptr(kOffMainThread));
    g_coop->b_resumed.store(true, std::memory_order_release);
    park_forever(rdram);
}

void main_rcp_x2(uint8_t* rdram) {
    (void)ultramodern::intmask::apply_os_set_int_mask(
        rdram, &g_coop->status, ultramodern::intmask::OS_IM_VI);
    osRecvMesg(rdram, gptr(kOffDummyMq), gptr(kOffDummyMsg), OS_MESG_BLOCK);
    park_forever(rdram);
}

void thread_b_rcp_x2(uint8_t* rdram) {
    (void)ultramodern::intmask::apply_os_set_int_mask(
        rdram, &g_coop->waiter_status, ultramodern::intmask::OS_IM_AI);
    osStartThread(rdram, gptr(kOffMainThread));
    OsSetIntMaskCtx ctx{};
    ctx.status_reg = g_coop->waiter_status;
    ctx.r4 = ultramodern::intmask::OS_IM_ALL;
    osSetIntMask_recomp_analog(rdram, &ctx);
    g_coop->waiter_status = ctx.status_reg;
    g_coop->rcp_old_r2 = static_cast<uint32_t>(ctx.r2);
    g_coop->b_resumed.store(true, std::memory_order_release);
    park_forever(rdram);
}

void thread_a_rcp_x3(uint8_t* rdram) {
    (void)ultramodern::intmask::apply_os_set_int_mask(
        rdram, &g_coop->status, ultramodern::intmask::OS_IM_AI);
    g_coop->rcp_ready.fetch_add(1, std::memory_order_acq_rel);
    osRecvMesg(rdram, gptr(kOffDummyMq), gptr(kOffDummyMsg), OS_MESG_BLOCK);
    for (int i = 0; i < 8; ++i) {
        const uint32_t m = ultramodern::intmask::current_combined_mask(g_coop->status);
        g_coop->rcp_mask_a = m;
        note_rcp_leak(m, kRcpAi);
        g_coop->rcp_cycles_a.fetch_add(1, std::memory_order_acq_rel);
        (void)osSendMesg(rdram, gptr(kOffExtraMq), 1, OS_MESG_NOBLOCK);
        if (i + 1 < 8) {
            osRecvMesg(rdram, gptr(kOffDummyMq), gptr(kOffDummyMsg), OS_MESG_BLOCK);
        }
    }
    g_coop->main_resumed.store(true, std::memory_order_release);
    // Drop below B/C so they can finish their last recv. park_forever would
    // keep this thread highest-priority and runnable, starving the ring.
    osSetThreadPri(rdram, NULLPTR, 0);
    park_forever(rdram);
}

void thread_b_rcp_x3(uint8_t* rdram) {
    (void)ultramodern::intmask::apply_os_set_int_mask(
        rdram, &g_coop->audio_status, ultramodern::intmask::OS_IM_VI);
    osStartThread(rdram, gptr(kOffMainThread));
    g_coop->rcp_ready.fetch_add(1, std::memory_order_acq_rel);
    osRecvMesg(rdram, gptr(kOffExtraMq), gptr(kOffExtraMsg), OS_MESG_BLOCK);
    for (int i = 0; i < 8; ++i) {
        const uint32_t m = ultramodern::intmask::current_combined_mask(g_coop->audio_status);
        g_coop->rcp_mask_b = m;
        note_rcp_leak(m, kRcpVi);
        g_coop->rcp_cycles_b.fetch_add(1, std::memory_order_acq_rel);
        (void)osSendMesg(rdram, gptr(kOffAudioMq), 1, OS_MESG_NOBLOCK);
        if (i + 1 < 8) {
            osRecvMesg(rdram, gptr(kOffExtraMq), gptr(kOffExtraMsg), OS_MESG_BLOCK);
        }
    }
    osSetThreadPri(rdram, NULLPTR, 0);
    park_forever(rdram);
}

void thread_c_rcp_x3(uint8_t* rdram) {
    (void)ultramodern::intmask::apply_os_set_int_mask(
        rdram, &g_coop->waiter_status, kOsImSpDp);
    osStartThread(rdram, gptr(kOffAudioThread));
    while (g_coop->rcp_ready.load(std::memory_order_acquire) < 2) {
        std::this_thread::yield();
    }
    for (int i = 0; i < 8; ++i) {
        const uint32_t m = ultramodern::intmask::current_combined_mask(g_coop->waiter_status);
        g_coop->rcp_mask_c = m;
        note_rcp_leak(m, kRcpSpDp);
        g_coop->rcp_cycles_c.fetch_add(1, std::memory_order_acq_rel);
        (void)osSendMesg(rdram, gptr(kOffDummyMq), 1, OS_MESG_NOBLOCK);
        osRecvMesg(rdram, gptr(kOffAudioMq), gptr(kOffAudioMsg), OS_MESG_BLOCK);
    }
    park_forever(rdram);
}

void main_rcp_x4(uint8_t* rdram) {
    osStartThread(rdram, gptr(kOffAudioThread));
    while (!g_coop->audio_blocked.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    (void)ultramodern::intmask::apply_os_set_int_mask(
        rdram, &g_coop->status, ultramodern::intmask::OS_IM_VI);
    g_coop->masked_ready.store(true, std::memory_order_release);
    while (!g_coop->host_enqueued.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    ultramodern::service_preemption(rdram, &g_coop->status);
    g_coop->held_at_service.store(ultramodern::held_ineligible_count(), std::memory_order_release);
    g_coop->swapped_while_masked.store(g_coop->audio_ran.load(std::memory_order_acquire),
                                       std::memory_order_release);
    osRecvMesg(rdram, gptr(kOffDummyMq), gptr(kOffDummyMsg), OS_MESG_BLOCK);
    g_coop->main_resumed.store(true, std::memory_order_release);
    park_forever(rdram);
}

void thread_b_rcp_x4(uint8_t* rdram) {
    (void)ultramodern::intmask::apply_os_set_int_mask(
        rdram, &g_coop->waiter_status, ultramodern::intmask::OS_IM_AI);
    g_coop->b_started.store(true, std::memory_order_release);
    osStartThread(rdram, gptr(kOffMainThread));
    g_coop->b_resumed.store(true, std::memory_order_release);
    park_forever(rdram);
}

} // namespace

void run_thread_function(uint8_t* rdram, uint64_t addr, uint64_t /*sp*/, uint64_t arg) {
    uint32_t* sr = &g_coop->status;
    switch (static_cast<int32_t>(addr)) {
        case kEntryAudioOnce:
        case kEntryAudioRepeat:
        case kEntryAudioFifo:
        case kEntryAudioTls:
        case kEntryAudioP3X6:
            sr = &g_coop->audio_status;
            break;
        case kEntryWaiterOnce:
        case kEntryWaiterVi:
        case kEntryThreadBX1G:
        case kEntryThreadBRcpX1:
        case kEntryThreadBRcpX2:
        case kEntryThreadCRcpX3:
        case kEntryThreadBRcpX4:
            sr = &g_coop->waiter_status;
            break;
        case kEntryThreadBRcpX3:
            sr = &g_coop->audio_status;
            break;
        default:
            break;
    }
    ultramodern::set_current_guest_interrupt_tls(rdram, sr);
    ultramodern::service_pending_interrupts_for_current_guest(rdram);

    switch (static_cast<int32_t>(addr)) {
        case kEntryMainT01:
            main_t01(rdram);
            break;
        case kEntryAudioOnce:
            audio_once(rdram);
            break;
        case kEntryMainService:
            main_service_loop(rdram);
            break;
        case kEntryMainMasked:
            main_masked(rdram);
            break;
        case kEntryMainNone:
            main_os_im_none(rdram);
            break;
        case kEntryWaiterOnce:
            waiter_once(rdram, kOffDummyMq, kOffDummyMsg);
            break;
        case kEntryMainSamePri:
            main_same_or_low(rdram);
            break;
        case kEntryMainBurst:
            main_burst(rdram);
            break;
        case kEntryWaiterVi:
            waiter_once(rdram, kOffExtraMq, kOffExtraMsg);
            break;
        case kEntryMainRequeue:
            main_requeue(rdram);
            break;
        case kEntryMainRepeat:
            main_repeat(rdram);
            break;
        case kEntryAudioRepeat:
            audio_repeat(rdram);
            break;
        case kEntryMainFifo:
            main_fifo(rdram);
            break;
        case kEntryAudioFifo:
            audio_fifo(rdram);
            break;
        case kEntryMainCAbiMasked:
            main_cabi_masked(rdram);
            break;
        case kEntryMainCAbiUnmasked:
            main_cabi_unmasked(rdram);
            break;
        case kEntryMainX1A:
            main_x1a(rdram);
            break;
        case kEntryMainX1G:
            main_x1g(rdram);
            break;
        case kEntryThreadBX1G:
            thread_b_x1g(rdram);
            break;
        case kEntryMainX1E:
            main_x1e(rdram);
            break;
        case kEntryMainX1F:
            main_x1f(rdram);
            break;
        case kEntryMainNonint:
            main_nonint(rdram);
            break;
        case kEntryMainRequeueCause:
            main_requeue_cause(rdram);
            break;
        case kEntryMainTls:
            main_tls(rdram);
            break;
        case kEntryAudioTls:
            audio_tls(rdram);
            break;
        case kEntryMainP3X6:
            main_p3x6(rdram);
            break;
        case kEntryAudioP3X6:
            audio_p3x6(rdram);
            break;
        case kEntryMainRcpX1:
            main_rcp_x1(rdram);
            break;
        case kEntryThreadBRcpX1:
            thread_b_rcp_x1(rdram);
            break;
        case kEntryMainRcpX2:
            main_rcp_x2(rdram);
            break;
        case kEntryThreadBRcpX2:
            thread_b_rcp_x2(rdram);
            break;
        case kEntryThreadARcpX3:
            thread_a_rcp_x3(rdram);
            break;
        case kEntryThreadBRcpX3:
            thread_b_rcp_x3(rdram);
            break;
        case kEntryThreadCRcpX3:
            thread_c_rcp_x3(rdram);
            break;
        case kEntryMainRcpX4:
            main_rcp_x4(rdram);
            break;
        case kEntryThreadBRcpX4:
            thread_b_rcp_x4(rdram);
            break;
        default:
            std::fprintf(stderr, "unknown harness entry %d arg=%llu\n",
                         static_cast<int32_t>(addr), static_cast<unsigned long long>(arg));
            std::abort();
    }
}

namespace {

void setup_common(CoopState* coop) {
    g_rdram = new uint8_t[kRdramBytes]{};
    g_coop = coop;
    osCreateMesgQueue(g_rdram, gptr(kOffAudioMq), gptr(kOffAudioMsg), 4);
    osCreateMesgQueue(g_rdram, gptr(kOffDummyMq), gptr(kOffDummyMsg), 4);
    osCreateMesgQueue(g_rdram, gptr(kOffExtraMq), gptr(kOffExtraMsg), 4);
}

void t01_flag_and_hint() {
    std::printf("T01-A flag/hint/service isolation\n");
    CHECK(ultramodern::guest_preemption_safepoints_enabled() == false);
    CHECK(ultramodern_preemption_hint() == false);

    ultramodern::set_guest_preemption_safepoints(true);
    CHECK(ultramodern::guest_preemption_safepoints_enabled() == true);
    CHECK(ultramodern_preemption_hint() == false); // no publish yet

    ultramodern::set_guest_preemption_safepoints(false);
    CHECK(ultramodern::guest_preemption_safepoints_enabled() == false);
    CHECK(ultramodern_preemption_hint() == false);

    ultramodern_service_preemption(nullptr, nullptr);
}

void t01_cooperative_scheduler() {
    std::printf("T01-B cooperative scheduler regression (feature OFF)\n");
    CoopState coop{};
    setup_common(&coop);
    osCreateThread(g_rdram, gptr(kOffMainThread), 6, kEntryMainT01, 0, gptr(kOffMainSp), kPriMain);
    osCreateThread(g_rdram, gptr(kOffAudioThread), 4, kEntryAudioOnce, 0, gptr(kOffAudioSp), kPriAudio);

    CHECK(ultramodern_preemption_hint() == false);
    CHECK(ultramodern::guest_preemption_safepoints_enabled() == false);

    osStartThread(g_rdram, gptr(kOffMainThread));
    CHECK(wait_until([&] { return coop.audio_blocked.load(std::memory_order_acquire); }, 2000));
    CHECK(coop.audio_ran.load(std::memory_order_acquire) == false);

    ultramodern::enqueue_external_message(gptr(kOffAudioMq), 7, false, false);
    coop.host_enqueued.store(true, std::memory_order_release);

    CHECK(wait_until([&] { return coop.audio_ran.load(std::memory_order_acquire); }, 2000));
    CHECK(wait_until([&] { return coop.main_resumed.load(std::memory_order_acquire); }, 2000));
    CHECK(coop.service_did_not_wake.load(std::memory_order_acquire));
    CHECK(coop.send_recv_ok.load(std::memory_order_acquire));
    CHECK(ultramodern_preemption_hint() == false);
}

void t02_high_pri() {
    std::printf("T02 high-priority async preemption\n");
    ultramodern::set_guest_preemption_safepoints(true);
    CoopState coop{};
    setup_common(&coop);
    osCreateThread(g_rdram, gptr(kOffMainThread), 6, kEntryMainService, 0, gptr(kOffMainSp), kPriMain);
    osCreateThread(g_rdram, gptr(kOffAudioThread), 4, kEntryAudioOnce, 0, gptr(kOffAudioSp), kPriAudio);
    osStartThread(g_rdram, gptr(kOffMainThread));
    CHECK(wait_until([&] { return coop.audio_blocked.load(std::memory_order_acquire); }, 2000));
    CHECK(coop.audio_ran.load(std::memory_order_acquire) == false);
    ultramodern::enqueue_external_message(gptr(kOffAudioMq), 7, false, false);
    coop.host_enqueued.store(true, std::memory_order_release);
    CHECK(wait_until([&] { return coop.audio_ran.load(std::memory_order_acquire); }, 2000));
    CHECK(wait_until([&] { return coop.main_resumed.load(std::memory_order_acquire); }, 2000));
}

void t03_ie_zero() {
    std::printf("T03 interrupts-disabled no-preempt\n");
    ultramodern::set_guest_preemption_safepoints(true);
    CoopState coop{};
    setup_common(&coop);
    osCreateThread(g_rdram, gptr(kOffMainThread), 6, kEntryMainMasked, 0, gptr(kOffMainSp), kPriMain);
    osCreateThread(g_rdram, gptr(kOffAudioThread), 4, kEntryAudioOnce, 0, gptr(kOffAudioSp), kPriAudio);
    osStartThread(g_rdram, gptr(kOffMainThread));
    CHECK(wait_until([&] { return coop.audio_blocked.load(std::memory_order_acquire); }, 2000));
    ultramodern::enqueue_external_message(gptr(kOffAudioMq), 7, false, false);
    coop.host_enqueued.store(true, std::memory_order_release);
    CHECK(wait_until([&] { return coop.main_resumed.load(std::memory_order_acquire); }, 2000));
    CHECK(coop.swapped_while_masked.load(std::memory_order_acquire) == false);
    CHECK(wait_until([&] { return coop.audio_ran.load(std::memory_order_acquire); }, 2000));
    CHECK(coop.saved_ie == ultramodern::intmask::SR_IE);
}

void t04_restore_and_none() {
    std::printf("T04 restore + OS_IM_NONE\n");
    ultramodern::set_guest_preemption_safepoints(true);
    uint32_t sr = ultramodern::intmask::status_reg_init();
    CHECK(ultramodern::cpu_preemptible(sr) == true);
    const uint32_t old_ie = ultramodern::intmask::disable_int(&sr);
    CHECK(old_ie == ultramodern::intmask::SR_IE);
    CHECK(ultramodern::cpu_preemptible(sr) == false);

    CoopState coop{};
    setup_common(&coop);
    osCreateThread(g_rdram, gptr(kOffMainThread), 6, kEntryMainNone, 0, gptr(kOffMainSp), kPriMain);
    osCreateThread(g_rdram, gptr(kOffAudioThread), 4, kEntryAudioOnce, 0, gptr(kOffAudioSp), kPriAudio);
    osStartThread(g_rdram, gptr(kOffMainThread));
    CHECK(wait_until([&] { return coop.audio_blocked.load(std::memory_order_acquire); }, 2000));
    ultramodern::enqueue_external_message(gptr(kOffAudioMq), 7, false, false, ultramodern::InterruptCause::AI);
    coop.host_enqueued.store(true, std::memory_order_release);
    CHECK(wait_until([&] { return coop.main_resumed.load(std::memory_order_acquire); }, 2000));
    CHECK(coop.swapped_while_masked.load(std::memory_order_acquire) == false);
    CHECK((coop.saved_mask & ultramodern::intmask::OS_IM_NONE) != 0);
    CHECK(wait_until([&] { return coop.audio_ran.load(std::memory_order_acquire); }, 2000));
}

void t05_same_pri() {
    std::printf("T05 same-priority no-preempt\n");
    ultramodern::set_guest_preemption_safepoints(true);
    CoopState coop{};
    setup_common(&coop);
    osCreateThread(g_rdram, gptr(kOffMainThread), 6, kEntryMainSamePri, 0, gptr(kOffMainSp), kPriMain);
    osCreateThread(g_rdram, gptr(kOffWaiterThread), 5, kEntryWaiterOnce, 0, gptr(kOffWaiterSp), kPriMain);
    osStartThread(g_rdram, gptr(kOffMainThread));
    CHECK(wait_until([&] { return coop.started_waiter.load(std::memory_order_acquire); }, 2000));
    ultramodern::enqueue_external_message(gptr(kOffDummyMq), 9, false, false);
    coop.host_enqueued.store(true, std::memory_order_release);
    CHECK(wait_until([&] { return coop.main_resumed.load(std::memory_order_acquire); }, 2000));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    CHECK(coop.waiter_ran.load(std::memory_order_acquire) == false);
    CHECK(ultramodern::thread_queue_peek(g_rdram, ultramodern::running_queue) == gptr(kOffWaiterThread));
}

void t06_lower_pri() {
    std::printf("T06 lower-priority no-preempt\n");
    ultramodern::set_guest_preemption_safepoints(true);
    CoopState coop{};
    setup_common(&coop);
    osCreateThread(g_rdram, gptr(kOffMainThread), 6, kEntryMainSamePri, 0, gptr(kOffMainSp), kPriMain);
    osCreateThread(g_rdram, gptr(kOffWaiterThread), 5, kEntryWaiterOnce, 0, gptr(kOffWaiterSp), kPriLow);
    osStartThread(g_rdram, gptr(kOffMainThread));
    CHECK(wait_until([&] { return coop.started_waiter.load(std::memory_order_acquire); }, 2000));
    ultramodern::enqueue_external_message(gptr(kOffDummyMq), 9, false, false);
    coop.host_enqueued.store(true, std::memory_order_release);
    CHECK(wait_until([&] { return coop.main_resumed.load(std::memory_order_acquire); }, 2000));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    CHECK(coop.waiter_ran.load(std::memory_order_acquire) == false);
    CHECK(ultramodern::thread_queue_peek(g_rdram, ultramodern::running_queue) == gptr(kOffWaiterThread));
}

void t07_burst() {
    std::printf("T07 simultaneous VI+AI\n");
    ultramodern::set_guest_preemption_safepoints(true);
    CoopState coop{};
    setup_common(&coop);
    osCreateThread(g_rdram, gptr(kOffMainThread), 6, kEntryMainBurst, 0, gptr(kOffMainSp), kPriMain);
    osCreateThread(g_rdram, gptr(kOffAudioThread), 4, kEntryAudioOnce, 0, gptr(kOffAudioSp), kPriAudio);
    osCreateThread(g_rdram, gptr(kOffWaiterThread), 5, kEntryWaiterVi, 0, gptr(kOffWaiterSp), kPriMid);
    osStartThread(g_rdram, gptr(kOffMainThread));
    CHECK(wait_until([&] {
        return coop.audio_blocked.load(std::memory_order_acquire) &&
               coop.waiter_blocked.load(std::memory_order_acquire);
    }, 2000));
    std::thread ai([&] { ultramodern::enqueue_external_message(gptr(kOffAudioMq), 1, false, false); });
    std::thread vi([&] { ultramodern::enqueue_external_message(gptr(kOffExtraMq), 2, false, false); });
    ai.join();
    vi.join();
    coop.host_enqueued.store(true, std::memory_order_release);
    CHECK(wait_until([&] { return coop.audio_ran.load(std::memory_order_acquire); }, 2000));
    CHECK(wait_until([&] { return coop.waiter_ran.load(std::memory_order_acquire); }, 2000));
    CHECK(wait_until([&] { return coop.main_resumed.load(std::memory_order_acquire); }, 2000));
}

void t08_requeue() {
    std::printf("T08 requeue lost-wakeup\n");
    ultramodern::set_guest_preemption_safepoints(true);
    CoopState coop{};
    setup_common(&coop);
    osCreateMesgQueue(g_rdram, gptr(kOffAudioMq), gptr(kOffAudioMsg), 1);
    osCreateThread(g_rdram, gptr(kOffMainThread), 6, kEntryMainRequeue, 0, gptr(kOffMainSp), kPriMain);
    osStartThread(g_rdram, gptr(kOffMainThread));
    CHECK(wait_until([&] { return coop.audio_blocked.load(std::memory_order_acquire); }, 2000));
    ultramodern::enqueue_external_message(gptr(kOffAudioMq), 99, false, true);
    coop.host_enqueued.store(true, std::memory_order_release);
    CHECK(wait_until([&] { return coop.main_resumed.load(std::memory_order_acquire); }, 2000));
    CHECK(coop.hint_after_requeue.load(std::memory_order_acquire));
    CHECK(coop.audio_ran.load(std::memory_order_acquire));
}

void t09_repeat() {
    std::printf("T09 repeated thread swaps\n");
    ultramodern::set_guest_preemption_safepoints(true);
    CoopState coop{};
    setup_common(&coop);
    osCreateThread(g_rdram, gptr(kOffMainThread), 6, kEntryMainRepeat, 0, gptr(kOffMainSp), kPriMain);
    osCreateThread(g_rdram, gptr(kOffAudioThread), 4, kEntryAudioRepeat, 0, gptr(kOffAudioSp), kPriAudio);
    osStartThread(g_rdram, gptr(kOffMainThread));
    for (int i = 0; i < 1000; ++i) {
        CHECK(wait_until([&] { return coop.audio_blocked.load(std::memory_order_acquire); }, 2000));
        ultramodern::enqueue_external_message(gptr(kOffAudioMq), i + 1, false, false);
        CHECK(wait_until([&] { return coop.audio_count.load(std::memory_order_acquire) > i; }, 2000));
    }
    CHECK(wait_until([&] { return coop.main_resumed.load(std::memory_order_acquire); }, 2000));
    CHECK(coop.audio_count.load(std::memory_order_acquire) == 1000);
}

void t10_fifo() {
    std::printf("T10 per-producer FIFO\n");
    ultramodern::set_guest_preemption_safepoints(true);
    CoopState coop{};
    setup_common(&coop);
    osCreateThread(g_rdram, gptr(kOffMainThread), 6, kEntryMainFifo, 0, gptr(kOffMainSp), kPriMain);
    osCreateThread(g_rdram, gptr(kOffAudioThread), 4, kEntryAudioFifo, 0, gptr(kOffAudioSp), kPriAudio);
    osStartThread(g_rdram, gptr(kOffMainThread));
    CHECK(wait_until([&] { return coop.audio_blocked.load(std::memory_order_acquire); }, 2000));
    ultramodern::enqueue_external_message(gptr(kOffAudioMq), 1, false, false);
    ultramodern::enqueue_external_message(gptr(kOffAudioMq), 2, false, false);
    ultramodern::enqueue_external_message(gptr(kOffAudioMq), 3, false, false);
    coop.host_enqueued.store(true, std::memory_order_release);
    CHECK(wait_until([&] { return coop.audio_ran.load(std::memory_order_acquire); }, 2000));
    CHECK(coop.fifo_ok.load(std::memory_order_acquire));
    CHECK(coop.fifo_next.load(std::memory_order_acquire) == 3);
}

void t11_abi_masked() {
    std::printf("T11-F C ABI honors IE=0 status_reg pointer\n");
    ultramodern::set_guest_preemption_safepoints(true);
    CoopState coop{};
    setup_common(&coop);
    osCreateThread(g_rdram, gptr(kOffMainThread), 6, kEntryMainCAbiMasked, 0, gptr(kOffMainSp), kPriMain);
    osCreateThread(g_rdram, gptr(kOffAudioThread), 4, kEntryAudioOnce, 0, gptr(kOffAudioSp), kPriAudio);
    osStartThread(g_rdram, gptr(kOffMainThread));
    CHECK(wait_until([&] { return coop.audio_blocked.load(std::memory_order_acquire); }, 2000));
    ultramodern::enqueue_external_message(gptr(kOffAudioMq), 7, false, false);
    coop.host_enqueued.store(true, std::memory_order_release);
    CHECK(wait_until([&] { return coop.main_resumed.load(std::memory_order_acquire); }, 2000));
    CHECK(coop.swapped_while_masked.load(std::memory_order_acquire) == false);
}

void t11_abi_unmasked() {
    std::printf("T11-F C ABI unmasked status_reg pointer preempts\n");
    ultramodern::set_guest_preemption_safepoints(true);
    CoopState coop{};
    setup_common(&coop);
    osCreateThread(g_rdram, gptr(kOffMainThread), 6, kEntryMainCAbiUnmasked, 0, gptr(kOffMainSp), kPriMain);
    osCreateThread(g_rdram, gptr(kOffAudioThread), 4, kEntryAudioOnce, 0, gptr(kOffAudioSp), kPriAudio);
    osStartThread(g_rdram, gptr(kOffMainThread));
    CHECK(wait_until([&] { return coop.audio_blocked.load(std::memory_order_acquire); }, 2000));
    ultramodern::enqueue_external_message(gptr(kOffAudioMq), 7, false, false);
    coop.host_enqueued.store(true, std::memory_order_release);
    CHECK(wait_until([&] { return coop.audio_ran.load(std::memory_order_acquire); }, 2000));
    CHECK(wait_until([&] { return coop.main_resumed.load(std::memory_order_acquire); }, 2000));
}

void t11_abi_flag_off() {
    std::printf("T11-G C ABI flag OFF does not service\n");
    CHECK(ultramodern::guest_preemption_safepoints_enabled() == false);
    uint32_t unmasked = ultramodern::intmask::status_reg_init();
    ultramodern_service_preemption(nullptr, &unmasked);
    CHECK(ultramodern_preemption_hint() == false);
}

void t12_threadqueue() {
    std::printf("T12 Fix B threadqueue contract\n");
    uint8_t* rdram = new uint8_t[kRdramBytes]{};
    auto init_thr = [&](uint32_t off, OSId id, OSPri pri) {
        OSThread* t = TO_PTR(OSThread, static_cast<int32_t>(kGuestBase + off));
        std::memset(t, 0, sizeof(*t));
        t->next = NULLPTR;
        t->queue = NULLPTR;
        t->priority = pri;
        t->id = id;
        t->state = static_cast<uint16_t>(OSThreadState::STOPPED);
    };
    init_thr(kOffMainThread, 1, 20);
    init_thr(kOffAudioThread, 2, 50);
    init_thr(kOffWaiterThread, 3, 30);
    *TO_PTR(PTR(OSThread), gptr(kOffQueueHead)) = NULLPTR;
    const PTR(PTR(OSThread)) q = gptr(kOffQueueHead);
    const PTR(OSThread) a = gptr(kOffAudioThread);
    const PTR(OSThread) b = gptr(kOffMainThread);
    const PTR(OSThread) c = gptr(kOffWaiterThread);

    ultramodern::thread_queue_insert(rdram, q, a);
    ultramodern::thread_queue_insert(rdram, q, b);
    ultramodern::thread_queue_insert(rdram, q, c);
    CHECK(ultramodern::thread_queue_peek(rdram, q) == a);

    ultramodern::thread_queue_insert(rdram, q, a);
    OSThread* ath = TO_PTR(OSThread, a);
    CHECK(ath->next != a);
    CHECK(ultramodern::thread_queue_peek(rdram, q) == a);

    CHECK(ultramodern::thread_queue_remove(rdram, q, b) == true);
    CHECK(ultramodern::thread_queue_peek(rdram, q) == a);
    CHECK(ultramodern::thread_queue_remove(rdram, q, b) == false);

    CHECK(ultramodern::thread_queue_remove(rdram, q, a) == true);
    CHECK(ultramodern::thread_queue_peek(rdram, q) == c);
    CHECK(ultramodern::thread_queue_remove(rdram, q, c) == true);
    CHECK(ultramodern::thread_queue_empty(rdram, q));
    delete[] rdram;
}

s32 mq_count(uint32_t mq_off) {
    uint8_t* rdram = g_rdram;
    return TO_PTR(OSMesgQueue, gptr(mq_off))->validCount;
}

void t04_plus_masks() {
    std::printf("T04+ exact combined masks / nested disable-restore\n");
    uint8_t dummy = 0;
    uint32_t sr = ultramodern::intmask::status_reg_init();
    CHECK(ultramodern::cpu_preemptible(sr) == true);
    CHECK(ultramodern::intmask::current_combined_mask(sr) == ultramodern::intmask::OS_IM_ALL);
    CHECK((sr & ultramodern::intmask::SR_IE) != 0);

    uint32_t prev = ultramodern::intmask::apply_os_set_int_mask(&dummy, &sr, ultramodern::intmask::OS_IM_NONE);
    CHECK(prev == ultramodern::intmask::OS_IM_ALL);
    CHECK((sr & ultramodern::intmask::SR_IE) != 0);
    CHECK((sr & ultramodern::intmask::SR_IMASK) == 0);
    CHECK(ultramodern::intmask::current_combined_mask(sr) == ultramodern::intmask::OS_IM_NONE);
    CHECK(ultramodern::cpu_preemptible(sr) == true); // IE still on

    prev = ultramodern::intmask::apply_os_set_int_mask(&dummy, &sr, ultramodern::intmask::OS_IM_AI);
    CHECK(prev == ultramodern::intmask::OS_IM_NONE);
    CHECK((sr & ultramodern::intmask::SR_IE) != 0);
    CHECK((sr & ultramodern::intmask::SR_IM2) != 0);
    CHECK(ultramodern::intmask::current_combined_mask(sr) == ultramodern::intmask::OS_IM_AI);

    prev = ultramodern::intmask::apply_os_set_int_mask(&dummy, &sr, ultramodern::intmask::OS_IM_VI);
    CHECK(prev == ultramodern::intmask::OS_IM_AI);
    CHECK(ultramodern::intmask::current_combined_mask(sr) == ultramodern::intmask::OS_IM_VI);

    prev = ultramodern::intmask::apply_os_set_int_mask(&dummy, &sr, 0x00000C01u); // IE|IM2|IM3 partial CPU
    CHECK((sr & ultramodern::intmask::SR_IE) != 0);
    CHECK((sr & 0x00000C00u) == 0x00000C00u);

    prev = ultramodern::intmask::apply_os_set_int_mask(&dummy, &sr, ultramodern::intmask::OS_IM_ALL);
    CHECK(ultramodern::intmask::current_combined_mask(sr) == ultramodern::intmask::OS_IM_ALL);

    const uint32_t ie1 = ultramodern::intmask::disable_int(&sr);
    CHECK(ie1 == ultramodern::intmask::SR_IE);
    CHECK((sr & ultramodern::intmask::SR_IE) == 0);
    const uint32_t ie2 = ultramodern::intmask::disable_int(&sr);
    CHECK(ie2 == 0);
    ultramodern::intmask::restore_int(&dummy, &sr, ie2);
    CHECK((sr & ultramodern::intmask::SR_IE) == 0);
    ultramodern::intmask::restore_int(&dummy, &sr, ie1);
    CHECK((sr & ultramodern::intmask::SR_IE) != 0);
}

void t07_plus_causes() {
    std::printf("T07+ cause metadata + per-producer FIFO\n");
    ultramodern::set_guest_preemption_safepoints(true);
    CoopState coop{};
    setup_common(&coop);
    osCreateThread(g_rdram, gptr(kOffMainThread), 6, kEntryMainBurst, 0, gptr(kOffMainSp), kPriMain);
    osCreateThread(g_rdram, gptr(kOffAudioThread), 4, kEntryAudioOnce, 0, gptr(kOffAudioSp), kPriAudio);
    osCreateThread(g_rdram, gptr(kOffWaiterThread), 5, kEntryWaiterVi, 0, gptr(kOffWaiterSp), kPriMid);
    osStartThread(g_rdram, gptr(kOffMainThread));
    CHECK(wait_until([&] {
        return coop.audio_blocked.load(std::memory_order_acquire) &&
               coop.waiter_blocked.load(std::memory_order_acquire);
    }, 2000));
    std::thread ai([&] {
        ultramodern::enqueue_external_message(gptr(kOffAudioMq), 1, false, false, ultramodern::InterruptCause::AI);
    });
    std::thread vi([&] {
        ultramodern::enqueue_external_message(gptr(kOffExtraMq), 2, false, false, ultramodern::InterruptCause::VI);
    });
    ai.join();
    vi.join();
    coop.host_enqueued.store(true, std::memory_order_release);
    CHECK(wait_until([&] { return coop.audio_ran.load(std::memory_order_acquire); }, 2000));
    CHECK(wait_until([&] { return coop.waiter_ran.load(std::memory_order_acquire); }, 2000));
    CHECK(wait_until([&] { return coop.main_resumed.load(std::memory_order_acquire); }, 2000));
}

void t_x1a_b() {
    std::printf("X1-A/B AI held while VI open, then unmask\n");
    ultramodern::set_guest_preemption_safepoints(true);
    CoopState coop{};
    setup_common(&coop);
    osCreateThread(g_rdram, gptr(kOffMainThread), 6, kEntryMainX1A, 0, gptr(kOffMainSp), kPriMain);
    osCreateThread(g_rdram, gptr(kOffAudioThread), 4, kEntryAudioOnce, 0, gptr(kOffAudioSp), kPriAudio);
    osStartThread(g_rdram, gptr(kOffMainThread));
    CHECK(wait_until([&] { return coop.masked_ready.load(std::memory_order_acquire); }, 2000));
    ultramodern::enqueue_external_message(gptr(kOffAudioMq), 7, false, false, ultramodern::InterruptCause::AI);
    coop.host_enqueued.store(true, std::memory_order_release);
    CHECK(wait_until([&] { return coop.main_resumed.load(std::memory_order_acquire); }, 2000));
    CHECK(coop.swapped_while_masked.load(std::memory_order_acquire) == false);
    CHECK(coop.audio_ran.load(std::memory_order_acquire) == false);
    CHECK(mq_count(kOffAudioMq) == 0);
    CHECK(ultramodern::held_ineligible_count() == 1);
    CHECK(ultramodern::held_ineligible_cause_at(0) == ultramodern::InterruptCause::AI);
    coop.unmask_now.store(true, std::memory_order_release);
    CHECK(wait_until([&] { return coop.audio_ran.load(std::memory_order_acquire); }, 2000));
    CHECK(ultramodern::held_ineligible_count() == 0);
}

void t_x1c() {
    std::printf("X1-C VI cause preemption\n");
    ultramodern::set_guest_preemption_safepoints(true);
    CoopState coop{};
    setup_common(&coop);
    osCreateThread(g_rdram, gptr(kOffMainThread), 6, kEntryMainService, 0, gptr(kOffMainSp), kPriMain);
    osCreateThread(g_rdram, gptr(kOffAudioThread), 4, kEntryAudioOnce, 0, gptr(kOffAudioSp), kPriAudio);
    osStartThread(g_rdram, gptr(kOffMainThread));
    CHECK(wait_until([&] { return coop.audio_blocked.load(std::memory_order_acquire); }, 2000));
    ultramodern::enqueue_external_message(gptr(kOffAudioMq), 7, false, false, ultramodern::InterruptCause::VI);
    coop.host_enqueued.store(true, std::memory_order_release);
    CHECK(wait_until([&] { return coop.audio_ran.load(std::memory_order_acquire); }, 2000));
    CHECK(wait_until([&] { return coop.main_resumed.load(std::memory_order_acquire); }, 2000));
}

void t_x1d() {
    std::printf("X1-D OS_IM_NONE holds hardware cause\n");
    ultramodern::set_guest_preemption_safepoints(true);
    CoopState coop{};
    setup_common(&coop);
    osCreateThread(g_rdram, gptr(kOffMainThread), 6, kEntryMainNone, 0, gptr(kOffMainSp), kPriMain);
    osCreateThread(g_rdram, gptr(kOffAudioThread), 4, kEntryAudioOnce, 0, gptr(kOffAudioSp), kPriAudio);
    osStartThread(g_rdram, gptr(kOffMainThread));
    CHECK(wait_until([&] { return coop.audio_blocked.load(std::memory_order_acquire); }, 2000));
    ultramodern::enqueue_external_message(gptr(kOffAudioMq), 7, false, false, ultramodern::InterruptCause::AI);
    coop.host_enqueued.store(true, std::memory_order_release);
    CHECK(wait_until([&] { return coop.main_resumed.load(std::memory_order_acquire); }, 2000));
    CHECK(coop.swapped_while_masked.load(std::memory_order_acquire) == false);
    CHECK(wait_until([&] { return coop.audio_ran.load(std::memory_order_acquire); }, 2000));
}

void t_x1e() {
    std::printf("X1-E IE=0 holds AI, restore delivers\n");
    ultramodern::set_guest_preemption_safepoints(true);
    CoopState coop{};
    setup_common(&coop);
    osCreateThread(g_rdram, gptr(kOffMainThread), 6, kEntryMainX1E, 0, gptr(kOffMainSp), kPriMain);
    osCreateThread(g_rdram, gptr(kOffAudioThread), 4, kEntryAudioOnce, 0, gptr(kOffAudioSp), kPriAudio);
    osStartThread(g_rdram, gptr(kOffMainThread));
    CHECK(wait_until([&] { return coop.masked_ready.load(std::memory_order_acquire); }, 2000));
    ultramodern::enqueue_external_message(gptr(kOffAudioMq), 7, false, false, ultramodern::InterruptCause::AI);
    coop.host_enqueued.store(true, std::memory_order_release);
    CHECK(wait_until([&] { return coop.main_resumed.load(std::memory_order_acquire); }, 2000));
    CHECK(coop.swapped_while_masked.load(std::memory_order_acquire) == false);
    CHECK(wait_until([&] { return coop.audio_ran.load(std::memory_order_acquire); }, 2000));
}

void t_x1f_m1_m2() {
    std::printf("X1-F / M1 / M2 MTC0-analog Status commit then service\n");
    ultramodern::set_guest_preemption_safepoints(true);
    CoopState coop{};
    setup_common(&coop);
    osCreateThread(g_rdram, gptr(kOffMainThread), 6, kEntryMainX1F, 0, gptr(kOffMainSp), kPriMain);
    osCreateThread(g_rdram, gptr(kOffAudioThread), 4, kEntryAudioOnce, 0, gptr(kOffAudioSp), kPriAudio);
    osStartThread(g_rdram, gptr(kOffMainThread));
    CHECK(wait_until([&] { return coop.masked_ready.load(std::memory_order_acquire); }, 2000));
    ultramodern::enqueue_external_message(gptr(kOffAudioMq), 7, false, false, ultramodern::InterruptCause::AI);
    coop.host_enqueued.store(true, std::memory_order_release);
    CHECK(wait_until([&] { return coop.main_resumed.load(std::memory_order_acquire); }, 2000));
    CHECK(coop.swapped_while_masked.load(std::memory_order_acquire) == false);
    CHECK(ultramodern::held_ineligible_count() == 1);
    coop.unmask_now.store(true, std::memory_order_release);
    CHECK(wait_until([&] { return coop.audio_ran.load(std::memory_order_acquire); }, 2000));
    CHECK(ultramodern::held_ineligible_count() == 0);
}

void t_x1g() {
    std::printf("X1-G thread-dispatch resume hook\n");
    ultramodern::set_guest_preemption_safepoints(true);
    CoopState coop{};
    setup_common(&coop);
    osCreateThread(g_rdram, gptr(kOffMainThread), 6, kEntryMainX1G, 0, gptr(kOffMainSp), kPriMid);
    osCreateThread(g_rdram, gptr(kOffAudioThread), 4, kEntryAudioOnce, 0, gptr(kOffAudioSp), kPriAudio);
    osCreateThread(g_rdram, gptr(kOffWaiterThread), 5, kEntryThreadBX1G, 0, gptr(kOffWaiterSp), kPriLow);
    osStartThread(g_rdram, gptr(kOffWaiterThread));
    CHECK(wait_until([&] { return coop.masked_ready.load(std::memory_order_acquire); }, 2000));
    ultramodern::enqueue_external_message(gptr(kOffAudioMq), 7, false, false, ultramodern::InterruptCause::AI);
    CHECK(ultramodern::held_ineligible_count() == 0); // not yet drained
    coop.host_enqueued.store(true, std::memory_order_release);
    CHECK(wait_until([&] { return coop.audio_ran.load(std::memory_order_acquire); }, 2000));
    CHECK(wait_until([&] { return coop.b_resumed.load(std::memory_order_acquire); }, 2000));
}

void t_requeue_cause() {
    std::printf("REQUEUE_CAUSE preserves AI through full-mq requeue\n");
    ultramodern::set_guest_preemption_safepoints(true);
    CoopState coop{};
    setup_common(&coop);
    osCreateMesgQueue(g_rdram, gptr(kOffAudioMq), gptr(kOffAudioMsg), 1);
    osCreateThread(g_rdram, gptr(kOffMainThread), 6, kEntryMainRequeueCause, 0, gptr(kOffMainSp), kPriMain);
    osStartThread(g_rdram, gptr(kOffMainThread));
    CHECK(wait_until([&] { return coop.masked_ready.load(std::memory_order_acquire); }, 2000));
    ultramodern::enqueue_external_message(gptr(kOffAudioMq), 99, false, true, ultramodern::InterruptCause::AI);
    coop.host_enqueued.store(true, std::memory_order_release);
    CHECK(wait_until([&] { return coop.main_resumed.load(std::memory_order_acquire); }, 2000));
    CHECK(coop.audio_ran.load(std::memory_order_acquire));
}

void t_noninterrupt() {
    std::printf("NONINTERRUPT None delivered while AI masked\n");
    ultramodern::set_guest_preemption_safepoints(true);
    CoopState coop{};
    setup_common(&coop);
    osCreateThread(g_rdram, gptr(kOffMainThread), 6, kEntryMainNonint, 0, gptr(kOffMainSp), kPriMain);
    osStartThread(g_rdram, gptr(kOffMainThread));
    CHECK(wait_until([&] { return coop.masked_ready.load(std::memory_order_acquire); }, 2000));
    ultramodern::enqueue_external_message(gptr(kOffAudioMq), 7, false, false, ultramodern::InterruptCause::None);
    coop.host_enqueued.store(true, std::memory_order_release);
    CHECK(wait_until([&] { return coop.main_resumed.load(std::memory_order_acquire); }, 2000));
    CHECK(mq_count(kOffAudioMq) == 1);
    CHECK(ultramodern::held_ineligible_count() == 0);
    coop.unmask_now.store(true, std::memory_order_release);
}

void t_tls() {
    std::printf("TLS per-thread epoch no reorder/livelock\n");
    ultramodern::set_guest_preemption_safepoints(true);
    CoopState coop{};
    setup_common(&coop);
    osCreateThread(g_rdram, gptr(kOffMainThread), 6, kEntryMainTls, 0, gptr(kOffMainSp), kPriMain);
    osCreateThread(g_rdram, gptr(kOffAudioThread), 4, kEntryAudioTls, 0, gptr(kOffAudioSp), kPriAudio);
    osStartThread(g_rdram, gptr(kOffMainThread));
    CHECK(wait_until([&] { return coop.audio_blocked.load(std::memory_order_acquire); }, 2000));
    ultramodern::enqueue_external_message(gptr(kOffAudioMq), 1, false, false, ultramodern::InterruptCause::None);
    coop.host_enqueued.store(true, std::memory_order_release);
    CHECK(wait_until([&] { return coop.audio_ran.load(std::memory_order_acquire); }, 2000));
    CHECK(wait_until([&] { return coop.fifo_ok.load(std::memory_order_acquire); }, 2000));
    CHECK(wait_until([&] { return coop.main_resumed.load(std::memory_order_acquire); }, 2000));
    CHECK(mq_count(kOffAudioMq) == 0);
}

void t_p3x6() {
    std::printf("P3-X6+ real safepoint swap with phase-ordered HIGH ownership\n");
    ultramodern::set_guest_preemption_safepoints(true);
    CoopState coop{};
    setup_common(&coop);
    osCreateThread(g_rdram, gptr(kOffMainThread), 6, kEntryMainP3X6, 0, gptr(kOffMainSp), kPriMain);
    osCreateThread(g_rdram, gptr(kOffAudioThread), 4, kEntryAudioP3X6, 0, gptr(kOffAudioSp), kPriAudio);
    osStartThread(g_rdram, gptr(kOffMainThread));
    CHECK(wait_until([&] { return coop.audio_blocked.load(std::memory_order_acquire); }, 2000));
    ultramodern::enqueue_external_message(gptr(kOffAudioMq), 7, false, false, ultramodern::InterruptCause::None);
    coop.host_enqueued.store(true, std::memory_order_release);
    CHECK(wait_until([&] { return coop.audio_ran.load(std::memory_order_acquire); }, 2000));
    CHECK(wait_until([&] { return coop.toy_survived.load(std::memory_order_acquire); }, 2000));
    CHECK(wait_until([&] { return coop.main_resumed.load(std::memory_order_acquire); }, 2000));
    CHECK(coop.phase_seen_by_high.load(std::memory_order_acquire) ==
          static_cast<int>(SwapPhase::InServiceWindow));
    CHECK(coop.phase.load(std::memory_order_acquire) == static_cast<int>(SwapPhase::AfterService));
    CHECK(coop.toy_gpr2 == 0x11111111u);
    CHECK(coop.toy_gpr4 == 0x44444444u);
    CHECK(coop.toy_gpr16 == 0x16161616u);
    CHECK(coop.toy_hi == 0x22222222u);
    CHECK(coop.toy_lo == 0x33333333u);
    CHECK(coop.toy_fpu0 == 0x4048F5C3u);
    CHECK(coop.toy_fpu1 == 0x3F800000u);
    CHECK(coop.toy_local == 0xA5A5u);
    CHECK(coop.toy_local2 == 0x5A5Au);
    CHECK((coop.toy_status_after & ultramodern::intmask::SR_IE) != 0);
}

void t_rcp_x1() {
    std::printf("RCP-X1 RCP-only dispatch: A masks MI.AI, B open, same SR.IM2\n");
    ultramodern::set_guest_preemption_safepoints(true);
    CoopState coop{};
    setup_common(&coop);
    osCreateThread(g_rdram, gptr(kOffMainThread), 6, kEntryMainRcpX1, 0, gptr(kOffMainSp), kPriMid);
    osCreateThread(g_rdram, gptr(kOffAudioThread), 4, kEntryAudioOnce, 0, gptr(kOffAudioSp), kPriAudio);
    osCreateThread(g_rdram, gptr(kOffWaiterThread), 5, kEntryThreadBRcpX1, 0, gptr(kOffWaiterSp), kPriLow);
    osStartThread(g_rdram, gptr(kOffWaiterThread));
    CHECK(wait_until([&] { return coop.masked_ready.load(std::memory_order_acquire); }, 2000));
    ultramodern::enqueue_external_message(gptr(kOffAudioMq), 7, false, false, ultramodern::InterruptCause::AI);
    CHECK(ultramodern::held_ineligible_count() == 0);
    coop.host_enqueued.store(true, std::memory_order_release);
    CHECK(wait_until([&] { return coop.audio_ran.load(std::memory_order_acquire); }, 2000));
    CHECK(wait_until([&] { return coop.b_resumed.load(std::memory_order_acquire); }, 2000));
    CHECK(coop.swapped_while_masked.load(std::memory_order_acquire) == false);
    CHECK(coop.held_at_service.load(std::memory_order_acquire) == 1);
    CHECK(ultramodern::held_ineligible_count() == 0);
}

void t_rcp_x2() {
    std::printf("RCP-X2 osSetIntMask old return is current thread RCP\n");
    ultramodern::set_guest_preemption_safepoints(true);
    CoopState coop{};
    setup_common(&coop);
    osCreateThread(g_rdram, gptr(kOffMainThread), 6, kEntryMainRcpX2, 0, gptr(kOffMainSp), kPriMid);
    osCreateThread(g_rdram, gptr(kOffWaiterThread), 5, kEntryThreadBRcpX2, 0, gptr(kOffWaiterSp), kPriLow);
    osStartThread(g_rdram, gptr(kOffWaiterThread));
    CHECK(wait_until([&] { return coop.b_resumed.load(std::memory_order_acquire); }, 2000));
    CHECK((coop.rcp_old_r2 & kRcpAi) != 0);
    CHECK((coop.rcp_old_r2 & kRcpVi) == 0);
    CHECK((coop.rcp_old_r2 & ultramodern::intmask::SR_IE) != 0);
    CHECK((coop.rcp_old_r2 & ultramodern::intmask::SR_IM2) != 0);
    CHECK(coop.rcp_old_r2 == ultramodern::intmask::OS_IM_AI);
}

void t_rcp_x3() {
    std::printf("RCP-X3 three-thread RCP isolation over real swaps\n");
    ultramodern::set_guest_preemption_safepoints(true);
    CoopState coop{};
    setup_common(&coop);
    osCreateMesgQueue(g_rdram, gptr(kOffAudioMq), gptr(kOffAudioMsg), 16);
    osCreateMesgQueue(g_rdram, gptr(kOffDummyMq), gptr(kOffDummyMsg), 16);
    osCreateMesgQueue(g_rdram, gptr(kOffExtraMq), gptr(kOffExtraMsg), 16);
    osCreateThread(g_rdram, gptr(kOffMainThread), 6, kEntryThreadARcpX3, 0, gptr(kOffMainSp), kPriMid);
    osCreateThread(g_rdram, gptr(kOffAudioThread), 4, kEntryThreadBRcpX3, 0, gptr(kOffAudioSp), kPriMain);
    osCreateThread(g_rdram, gptr(kOffWaiterThread), 5, kEntryThreadCRcpX3, 0, gptr(kOffWaiterSp), kPriLow);
    osStartThread(g_rdram, gptr(kOffWaiterThread));
    const bool progressed = wait_until([&] {
        return coop.rcp_cycles_a.load(std::memory_order_acquire) == 8 &&
               coop.rcp_cycles_b.load(std::memory_order_acquire) == 8 &&
               coop.rcp_cycles_c.load(std::memory_order_acquire) == 8;
    }, 4000);
    CHECK(progressed);
    CHECK(coop.rcp_leak.load(std::memory_order_acquire) == false);
    CHECK((coop.rcp_mask_a & ultramodern::intmask::RCP_IMASK) == kRcpAi);
    CHECK((coop.rcp_mask_b & ultramodern::intmask::RCP_IMASK) == kRcpVi);
    CHECK((coop.rcp_mask_c & ultramodern::intmask::RCP_IMASK) == kRcpSpDp);
}

void t_rcp_x4() {
    std::printf("RCP-X4 held AI delivers on B resume via TLS RCP, no re-enqueue\n");
    ultramodern::set_guest_preemption_safepoints(true);
    CoopState coop{};
    setup_common(&coop);
    osCreateThread(g_rdram, gptr(kOffMainThread), 6, kEntryMainRcpX4, 0, gptr(kOffMainSp), kPriMid);
    osCreateThread(g_rdram, gptr(kOffAudioThread), 4, kEntryAudioOnce, 0, gptr(kOffAudioSp), kPriAudio);
    osCreateThread(g_rdram, gptr(kOffWaiterThread), 5, kEntryThreadBRcpX4, 0, gptr(kOffWaiterSp), kPriLow);
    osStartThread(g_rdram, gptr(kOffWaiterThread));
    CHECK(wait_until([&] { return coop.masked_ready.load(std::memory_order_acquire); }, 2000));
    ultramodern::enqueue_external_message(gptr(kOffAudioMq), 7, false, false, ultramodern::InterruptCause::AI);
    coop.host_enqueued.store(true, std::memory_order_release);
    CHECK(wait_until([&] { return coop.audio_ran.load(std::memory_order_acquire); }, 2000));
    CHECK(wait_until([&] { return coop.b_resumed.load(std::memory_order_acquire); }, 2000));
    CHECK(coop.swapped_while_masked.load(std::memory_order_acquire) == false);
    CHECK(coop.held_at_service.load(std::memory_order_acquire) == 1);
    CHECK(ultramodern::held_ineligible_count() == 0);
    CHECK(mq_count(kOffAudioMq) == 0);
}

void t_mtc0_flag_off() {
    std::printf("MTC0 feature-OFF is a no-op for async service\n");
    CHECK(ultramodern::guest_preemption_safepoints_enabled() == false);
    uint32_t sr = ultramodern::intmask::status_reg_init();
    ultramodern::after_interrupt_state_change(nullptr, &sr);
    CHECK(ultramodern_preemption_hint() == false);
    CHECK(ultramodern::held_ineligible_count() == 0);
}

using test_fn = void (*)();

int run_isolated(const char* id, test_fn fn) {
    std::fflush(stdout);
    std::fflush(stderr);
    const pid_t pid = fork();
    if (pid < 0) {
        std::perror("fork");
        ++g_fail;
        return 1;
    }
    if (pid == 0) {
        g_pass = 0;
        g_fail = 0;
        fn();
        std::fflush(stdout);
        std::fflush(stderr);
        std::_Exit(g_fail == 0 ? 0 : 1);
    }
    int st = 0;
    if (waitpid(pid, &st, 0) < 0) {
        std::perror("waitpid");
        ++g_fail;
        return 1;
    }
    if (WIFEXITED(st) && WEXITSTATUS(st) == 0) {
        ++g_pass;
        std::printf("%s PASS\n", id);
        return 0;
    }
    ++g_fail;
    std::printf("%s FAIL status=%d\n", id, st);
    return 1;
}

} // namespace

int main() {
    t01_flag_and_hint();

    run_isolated("T01-B", t01_cooperative_scheduler);
    run_isolated("T02", t02_high_pri);
    run_isolated("T03", t03_ie_zero);
    run_isolated("T04", t04_restore_and_none);
    run_isolated("T05", t05_same_pri);
    run_isolated("T06", t06_lower_pri);
    run_isolated("T07", t07_burst);
    run_isolated("T08", t08_requeue);
    run_isolated("T09", t09_repeat);
    run_isolated("T10", t10_fifo);
    run_isolated("T11-F-masked", t11_abi_masked);
    run_isolated("T11-F-unmasked", t11_abi_unmasked);
    t11_abi_flag_off();
    run_isolated("T12", t12_threadqueue);
    run_isolated("T04+", t04_plus_masks);
    run_isolated("T07+", t07_plus_causes);
    run_isolated("X1-A/B", t_x1a_b);
    run_isolated("X1-C", t_x1c);
    run_isolated("X1-D", t_x1d);
    run_isolated("X1-E", t_x1e);
    run_isolated("X1-F/M1/M2", t_x1f_m1_m2);
    run_isolated("X1-G", t_x1g);
    run_isolated("REQUEUE_CAUSE", t_requeue_cause);
    run_isolated("NONINTERRUPT", t_noninterrupt);
    run_isolated("TLS", t_tls);
    run_isolated("P3-X6+", t_p3x6);
    run_isolated("RCP-X1", t_rcp_x1);
    run_isolated("RCP-X2", t_rcp_x2);
    run_isolated("RCP-X3", t_rcp_x3);
    run_isolated("RCP-X4", t_rcp_x4);
    t_mtc0_flag_off();

    std::printf("PATCH35 %s  (pass=%d fail=%d skipped=%d)\n",
                g_fail == 0 ? "PASS" : "FAIL", g_pass, g_fail, g_skip);
    std::fflush(stdout);
    std::fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
