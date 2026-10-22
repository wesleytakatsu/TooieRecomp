#include <cassert>

#include "ultramodern/ultramodern.hpp"

static PTR(OSThread) running_queue_impl = NULLPTR;

static PTR(OSThread)* queue_to_ptr(RDRAM_ARG PTR(PTR(OSThread)) queue) {
    if (queue == ultramodern::running_queue) {
        return &running_queue_impl;
    }
    return TO_PTR(PTR(OSThread), queue);
}

void ultramodern::thread_queue_insert(RDRAM_ARG PTR(PTR(OSThread)) queue_, PTR(OSThread) toadd_) {
    PTR(OSThread)* cur = queue_to_ptr(PASS_RDRAM queue_);
    OSThread* toadd = TO_PTR(OSThread, toadd_); 
    debug_printf("[Thread Queue] Inserting thread %d into queue 0x%08X\n", toadd->id, (uintptr_t)queue_);
    // A thread can be woken by two host-side event sources before the first
    // wakeup has run. Inserting it into the same queue twice makes next point
    // back to itself and permanently wedges the cooperative scheduler.
    // Membership is decided by the REAL links in the queue, not by the queue
    // field: the field is a reinsert handle and stays stale after remove, so
    // checking it would wrongly reject a legitimate osSetThreadPri reinsert.
    // Scan the links as well before mutating them.
    PTR(OSThread)* scan = cur;
    for (uint32_t hops = 0; *scan != NULLPTR; hops++) {
        if (*scan == toadd_) {
            debug_printf("  Thread %d is already linked in queue 0x%08X\n", toadd->id, (uintptr_t)queue_);
            return;
        }
        if (hops == 255) {
            debug_printf("  Refusing to insert into malformed queue 0x%08X\n", (uintptr_t)queue_);
            return;
        }
        scan = &TO_PTR(OSThread, *scan)->next;
    }

    while (*cur && TO_PTR(OSThread, *cur)->priority > toadd->priority) {
        cur = &TO_PTR(OSThread, *cur)->next;
    }
    toadd->next = (*cur);
    toadd->queue = queue_;
    *cur = toadd_;

    debug_printf("  Contains:");
    cur = queue_to_ptr(PASS_RDRAM queue_);
    while (*cur) {
        debug_printf("%d (%d) ", TO_PTR(OSThread, *cur)->id, TO_PTR(OSThread, *cur)->priority);
        cur = &TO_PTR(OSThread, *cur)->next;
    }
    debug_printf("\n");
}

PTR(OSThread) ultramodern::thread_queue_pop(RDRAM_ARG PTR(PTR(OSThread)) queue_) {
    PTR(OSThread)* queue = queue_to_ptr(PASS_RDRAM queue_);
    PTR(OSThread) ret = *queue;
    *queue = TO_PTR(OSThread, ret)->next;
    TO_PTR(OSThread, ret)->queue = NULLPTR;
    debug_printf("[Thread Queue] Popped thread %d from queue 0x%08X\n", TO_PTR(OSThread, ret)->id, (uintptr_t)queue_);
    return ret;
}

bool ultramodern::thread_queue_remove(RDRAM_ARG PTR(PTR(OSThread)) queue_, PTR(OSThread) t_) {
    debug_printf("[Thread Queue] Removing thread %d from queue 0x%08X\n", TO_PTR(OSThread, t_)->id, (uintptr_t)queue_);

    if (queue_ == NULLPTR) {
        return false;
    }

    PTR(OSThread)* pred = queue_to_ptr(PASS_RDRAM queue_);

    while (*pred != NULLPTR) {
        if (*pred == t_) {
            *pred = TO_PTR(OSThread, *pred)->next;
            return true;
        }

        pred = &TO_PTR(OSThread, *pred)->next;
    }

    return false;
}

bool ultramodern::thread_queue_empty(RDRAM_ARG PTR(PTR(OSThread)) queue_) {
    PTR(OSThread)* queue = queue_to_ptr(PASS_RDRAM queue_);
    return *queue == NULLPTR;
}

PTR(OSThread) ultramodern::thread_queue_peek(RDRAM_ARG PTR(PTR(OSThread)) queue_) {
    PTR(OSThread)* queue = queue_to_ptr(PASS_RDRAM queue_);
    return *queue;
}
