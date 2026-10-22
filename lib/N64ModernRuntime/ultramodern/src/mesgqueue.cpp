#include <thread>
#include <vector>

#include "blockingconcurrentqueue.h"

#include "ultramodern/ultra64.h"
#include "ultramodern/ultramodern.hpp"

struct QueuedMessage {
    PTR(OSMesgQueue) mq;
    OSMesg mesg;
    bool jam;
    bool requeue_if_blocked;
    ultramodern::InterruptCause cause = ultramodern::InterruptCause::None;
};

static moodycamel::BlockingConcurrentQueue<QueuedMessage> external_messages {};

// Process-global: a pending hardware interrupt is device state, not
// per-native-thread state. Only the current guest token owner mutates it.
// Host producers never touch this vector; they only enqueue into the MPMC.
static std::vector<QueuedMessage> held_ineligible {};

struct DrainResult {
    bool delivered = false;
    bool full_requeue = false;
};

bool do_send(RDRAM_ARG PTR(OSMesgQueue) mq_, OSMesg msg, bool jam, bool block);

static DrainResult drain_pending_external(RDRAM_ARG std::vector<QueuedMessage> incoming) {
    const bool gate = ultramodern::guest_preemption_safepoints_enabled();
    const uint32_t* status_reg = ultramodern::current_guest_status_reg();

    std::vector<QueuedMessage> work;
    work.swap(held_ineligible);
    work.insert(work.end(), incoming.begin(), incoming.end());

    std::vector<QueuedMessage> still_held;
    std::vector<QueuedMessage> full_requeue;
    DrainResult result;

    for (QueuedMessage& cur : work) {
        const bool eligible = !gate || ultramodern::intmask::cause_eligible(status_reg, cur.cause);
        if (eligible) {
            if (do_send(PASS_RDRAM cur.mq, cur.mesg, cur.jam, false)) {
                result.delivered = true;
            } else if (cur.requeue_if_blocked) {
                // Full guest mq is NOT an interrupt hold. Preserve cause.
                full_requeue.push_back(cur);
            }
        } else {
            still_held.push_back(cur);
        }
    }

    held_ineligible = std::move(still_held);
    for (QueuedMessage& cur : full_requeue) {
        external_messages.enqueue(cur);
    }
    result.full_requeue = !full_requeue.empty();
    return result;
}

void ultramodern::enqueue_external_message(PTR(OSMesgQueue) mq, OSMesg msg, bool jam, bool requeue_if_blocked,
                                          InterruptCause cause) {
    external_messages.enqueue({mq, msg, jam, requeue_if_blocked, cause});
    ultramodern::publish_external_work();
}

bool ultramodern::dequeue_external_messages(RDRAM_ARG1) {
    std::vector<QueuedMessage> incoming;
    QueuedMessage to_send;
    while (external_messages.try_dequeue(to_send)) {
        incoming.push_back(to_send);
    }
    return drain_pending_external(PASS_RDRAM std::move(incoming)).full_requeue;
}

void ultramodern::flush_held_ineligible_to_external() {
    if (held_ineligible.empty()) {
        return;
    }
    for (QueuedMessage& cur : held_ineligible) {
        external_messages.enqueue(cur);
    }
    held_ineligible.clear();
    ultramodern::publish_external_work();
}

size_t ultramodern::held_ineligible_count() {
    return held_ineligible.size();
}

ultramodern::InterruptCause ultramodern::held_ineligible_cause_at(size_t index) {
    return held_ineligible.at(index).cause;
}

void ultramodern::wait_for_external_message(RDRAM_ARG1) {
    if (!ultramodern::guest_preemption_safepoints_enabled()) {
        QueuedMessage to_send;
        external_messages.wait_dequeue(to_send);
        if (!do_send(PASS_RDRAM to_send.mq, to_send.mesg, to_send.jam, false) && to_send.requeue_if_blocked) {
            external_messages.enqueue(to_send);
            ultramodern::publish_external_work();
        }
        return;
    }

    for (;;) {
        std::vector<QueuedMessage> incoming;
        QueuedMessage to_send;
        while (external_messages.try_dequeue(to_send)) {
            incoming.push_back(to_send);
        }
        DrainResult result = drain_pending_external(PASS_RDRAM std::move(incoming));
        if (result.full_requeue) {
            ultramodern::publish_external_work();
        }
        if (result.delivered || result.full_requeue) {
            return;
        }
        // Nothing eligible (held-only or idle). Block for a new host event;
        // do not treat a held masked cause as wake work and do not spin.
        incoming.clear();
        external_messages.wait_dequeue(to_send);
        incoming.push_back(to_send);
        while (external_messages.try_dequeue(to_send)) {
            incoming.push_back(to_send);
        }
        result = drain_pending_external(PASS_RDRAM std::move(incoming));
        if (result.full_requeue) {
            ultramodern::publish_external_work();
        }
        if (result.delivered || result.full_requeue) {
            return;
        }
    }
}

void ultramodern::wait_for_external_message_timed(RDRAM_ARG u32 millis) {
    if (!ultramodern::guest_preemption_safepoints_enabled()) {
        QueuedMessage to_send;
        if (external_messages.wait_dequeue_timed(to_send, std::chrono::milliseconds{millis})) {
            if (!do_send(PASS_RDRAM to_send.mq, to_send.mesg, to_send.jam, false) && to_send.requeue_if_blocked) {
                external_messages.enqueue(to_send);
                ultramodern::publish_external_work();
            }
        }
        return;
    }

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds{millis};
    for (;;) {
        std::vector<QueuedMessage> incoming;
        QueuedMessage to_send;
        while (external_messages.try_dequeue(to_send)) {
            incoming.push_back(to_send);
        }
        if (!incoming.empty() || !held_ineligible.empty()) {
            const DrainResult result = drain_pending_external(PASS_RDRAM std::move(incoming));
            if (result.full_requeue) {
                ultramodern::publish_external_work();
            }
            if (result.delivered || result.full_requeue) {
                return;
            }
        }

        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            return;
        }
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
        if (!external_messages.wait_dequeue_timed(to_send, remaining)) {
            return;
        }
        incoming.clear();
        incoming.push_back(to_send);
        while (external_messages.try_dequeue(to_send)) {
            incoming.push_back(to_send);
        }
        const DrainResult result = drain_pending_external(PASS_RDRAM std::move(incoming));
        if (result.full_requeue) {
            ultramodern::publish_external_work();
        }
        if (result.delivered || result.full_requeue) {
            return;
        }
    }
}

extern "C" void osCreateMesgQueue(RDRAM_ARG PTR(OSMesgQueue) mq_, PTR(OSMesg) msg, s32 count) {
    OSMesgQueue *mq = TO_PTR(OSMesgQueue, mq_);
    mq->blocked_on_recv = NULLPTR;
    mq->blocked_on_send = NULLPTR;
    mq->msgCount = count;
    mq->msg = msg;
    mq->validCount = 0;
    mq->first = 0;
}

s32 MQ_GET_COUNT(OSMesgQueue *mq) {
    return mq->validCount;
}

s32 MQ_IS_EMPTY(OSMesgQueue *mq) {
    return mq->validCount == 0;
}

s32 MQ_IS_FULL(OSMesgQueue* mq) {
    return MQ_GET_COUNT(mq) >= mq->msgCount;
}

bool do_send(RDRAM_ARG PTR(OSMesgQueue) mq_, OSMesg msg, bool jam, bool block) {
    OSMesgQueue* mq = TO_PTR(OSMesgQueue, mq_);
    if (!block) {
        // If non-blocking, fail if the queue is full.
        if (MQ_IS_FULL(mq)) {
            return false;
        }
    }
    else {
        // Otherwise, yield this thread until the queue has room.
        while (MQ_IS_FULL(mq)) {
            debug_printf("[Message Queue] Thread %d is blocked on send\n", TO_PTR(OSThread, ultramodern::this_thread())->id);
            ultramodern::thread_queue_insert(PASS_RDRAM GET_MEMBER(OSMesgQueue, mq_, blocked_on_send), ultramodern::this_thread());
            ultramodern::run_next_thread_and_wait(PASS_RDRAM1);
        }
    }
    
    if (jam) {
        // Jams insert at the head of the message queue's buffer.
        mq->first = (mq->first + mq->msgCount - 1) % mq->msgCount;
        TO_PTR(OSMesg, mq->msg)[mq->first] = msg;
        mq->validCount++;
    }
    else {
        // Sends insert at the tail of the message queue's buffer.
        s32 last = (mq->first + mq->validCount) % mq->msgCount;
        TO_PTR(OSMesg, mq->msg)[last] = msg;
        mq->validCount++;
    }

    // If any threads were blocked on receiving from this message queue, pop the first one and schedule it.
    PTR(PTR(OSThread)) blocked_queue = GET_MEMBER(OSMesgQueue, mq_, blocked_on_recv);
    if (!ultramodern::thread_queue_empty(PASS_RDRAM blocked_queue)) {
        ultramodern::schedule_running_thread(PASS_RDRAM ultramodern::thread_queue_pop(PASS_RDRAM blocked_queue));
    }
    
    return true;
}

bool do_recv(RDRAM_ARG PTR(OSMesgQueue) mq_, PTR(OSMesg) msg_, bool block) {
    OSMesgQueue* mq = TO_PTR(OSMesgQueue, mq_);
    if (!block) {
        // If non-blocking, fail if the queue is empty
        if (MQ_IS_EMPTY(mq)) {
            return false;
        }
    } else {
        // Otherwise, yield this thread in a loop until the queue is no longer full
        while (MQ_IS_EMPTY(mq)) {
            debug_printf("[Message Queue] Thread %d is blocked on receive\n", TO_PTR(OSThread, ultramodern::this_thread())->id);
            ultramodern::thread_queue_insert(PASS_RDRAM GET_MEMBER(OSMesgQueue, mq_, blocked_on_recv), ultramodern::this_thread());
            ultramodern::run_next_thread_and_wait(PASS_RDRAM1);
        }
    }

    if (msg_ != NULLPTR) {
        *TO_PTR(OSMesg, msg_) = TO_PTR(OSMesg, mq->msg)[mq->first];
    }
    
    mq->first = (mq->first + 1) % mq->msgCount;
    mq->validCount--;

    // If any threads were blocked on sending to this message queue, pop the first one and schedule it.
    PTR(PTR(OSThread)) blocked_queue = GET_MEMBER(OSMesgQueue, mq_, blocked_on_send);
    if (!ultramodern::thread_queue_empty(PASS_RDRAM blocked_queue)) {
        ultramodern::schedule_running_thread(PASS_RDRAM ultramodern::thread_queue_pop(PASS_RDRAM blocked_queue));
    }

    return true;
}

extern "C" s32 osSendMesg(RDRAM_ARG PTR(OSMesgQueue) mq_, OSMesg msg, s32 flags) {
    OSMesgQueue *mq = TO_PTR(OSMesgQueue, mq_);
    bool jam = false;
    
    // Don't directly send to the message queue if this isn't a game thread to avoid contention.
    if (!ultramodern::is_game_thread()) {
        ultramodern::enqueue_external_message(mq_, msg, jam, false);
        return 0;
    }
    
    // Handle any messages that have been received from an external thread.
    if (ultramodern::dequeue_external_messages(PASS_RDRAM1)) {
        ultramodern::publish_external_work();
    }

    // Try to send the message.
    bool sent = do_send(PASS_RDRAM mq_, msg, jam, flags == OS_MESG_BLOCK);
    
    // Check the queue to see if this thread should swap execution to another.
    ultramodern::check_running_queue(PASS_RDRAM1);

    return sent ? 0 : -1;
}

extern "C" s32 osJamMesg(RDRAM_ARG PTR(OSMesgQueue) mq_, OSMesg msg, s32 flags) {
    OSMesgQueue *mq = TO_PTR(OSMesgQueue, mq_);
    bool jam = true;
    
    // Don't directly send to the message queue if this isn't a game thread to avoid contention.
    if (!ultramodern::is_game_thread()) {
        ultramodern::enqueue_external_message(mq_, msg, jam, false);
        return 0;
    }
    
    // Handle any messages that have been received from an external thread.
    if (ultramodern::dequeue_external_messages(PASS_RDRAM1)) {
        ultramodern::publish_external_work();
    }

    // Try to send the message.
    bool sent = do_send(PASS_RDRAM mq_, msg, jam, flags == OS_MESG_BLOCK);
    
    // Check the queue to see if this thread should swap execution to another.
    ultramodern::check_running_queue(PASS_RDRAM1);

    return sent ? 0 : -1;
}

extern "C" s32 osRecvMesg(RDRAM_ARG PTR(OSMesgQueue) mq_, PTR(OSMesg) msg_, s32 flags) {
    OSMesgQueue *mq = TO_PTR(OSMesgQueue, mq_);
    
    assert(ultramodern::is_game_thread() && "RecvMesg not allowed outside of game threads.");
    
    // Handle any messages that have been received from an external thread.
    if (ultramodern::dequeue_external_messages(PASS_RDRAM1)) {
        ultramodern::publish_external_work();
    }

    // Try to receive a message.
    bool received = do_recv(PASS_RDRAM mq_, msg_, flags == OS_MESG_BLOCK);
    
    // Check the queue to see if this thread should swap execution to another.
    ultramodern::check_running_queue(PASS_RDRAM1);

    return received ? 0 : -1;
}
