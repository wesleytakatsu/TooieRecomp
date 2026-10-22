#include "ultramodern/ultramodern.hpp"

#include <cstdint>

// PATCH 2: software interrupt-mask state matching libultra osSetIntMask /
// __osDisableInt / __osRestoreInt. No real MI hardware.
//
// Current RCP/MI mask is per native guest OSThread. The scheduler keeps a
// 1:1 OSThread lifetime <-> std::thread affinity, so thread_local matches
// libultra THREAD_RCP save/restore without an explicit resume helper.
// Initial 0x3F is the OS_IM_ALL RCP portion (osCreateThread). A destroyed
// and later recreated OSThread gets a new native thread and therefore a
// fresh 0x3F; it does not inherit the creator mask.
//
// g_global_int_mask (__OSGlobalIntMask) remains process-global.
//
// current_combined_mask / apply_os_set_int_mask / cause_eligible are
// guest-execution-context operations. Host producer threads (VI/AI/SP/DP/
// SI/PI/timer) must not call them. Eligibility is evaluated only on the
// game-token owner via drain_pending_external.

namespace ultramodern::intmask {

thread_local uint32_t current_mi_mask = 0x3Fu;
uint32_t g_global_int_mask = ~0u;

uint32_t status_reg_init() {
    return SR_IE | SR_IMASK;
}

uint32_t current_combined_mask(uint32_t status_reg) {
    const uint32_t ie = status_reg & SR_IE;
    const uint32_t cpu = status_reg & SR_IMASK;
    const uint32_t rcp = (current_mi_mask << 16) & RCP_IMASK;
    return ie | cpu | rcp;
}

uint32_t apply_os_set_int_mask(RDRAM_ARG uint32_t* status_reg, uint32_t a0) {
    const uint32_t prev = current_combined_mask(status_reg != nullptr ? *status_reg : 0u);

    const uint32_t global_rcp = (g_global_int_mask & RCP_IMASK) >> 16;
    current_mi_mask = ((a0 & RCP_IMASK) >> 16) & global_rcp;

    if (status_reg != nullptr) {
        const uint32_t global_im = g_global_int_mask & SR_IMASK;
        const uint32_t new_im = (a0 & SR_IMASK) & global_im;
        // osSetIntMask does not touch IE. Commit IM/MI only; the caller
        // writes architectural r2, then after_interrupt_state_change.
        *status_reg = (*status_reg & ~SR_IMASK) | new_im;
    }

    return prev;
}

uint32_t disable_int(uint32_t* status_reg) {
    if (status_reg == nullptr) {
        return 0;
    }
    const uint32_t old_ie = *status_reg & SR_IE;
    *status_reg &= ~SR_IE;
    return old_ie;
}

void restore_int(RDRAM_ARG uint32_t* status_reg, uint32_t a0) {
    if (status_reg == nullptr) {
        return;
    }
    *status_reg |= (a0 & SR_IE);
    // No r2 to preserve. Service pending causes after IE is committed.
    ultramodern::after_interrupt_state_change(PASS_RDRAM status_reg);
}

static uint32_t mi_bit_for_cause(ultramodern::InterruptCause cause) {
    switch (cause) {
        case ultramodern::InterruptCause::SP: return MI_INTR_SP;
        case ultramodern::InterruptCause::SI: return MI_INTR_SI;
        case ultramodern::InterruptCause::AI: return MI_INTR_AI;
        case ultramodern::InterruptCause::VI: return MI_INTR_VI;
        case ultramodern::InterruptCause::PI: return MI_INTR_PI;
        case ultramodern::InterruptCause::DP: return MI_INTR_DP;
        default: return 0;
    }
}

bool cause_eligible(uint32_t status_reg, ultramodern::InterruptCause cause) {
    if (cause == ultramodern::InterruptCause::None) {
        return true;
    }
    if ((status_reg & SR_IE) == 0) {
        return false;
    }
    if ((status_reg & (SR_EXL | SR_ERL)) != 0) {
        return false;
    }
    if (cause == ultramodern::InterruptCause::Timer) {
        // IM7 analog. Runtime does not model C0_COMPARE.
        return ((status_reg & SR_IM7) != 0) && ((g_global_int_mask & SR_IM7) != 0);
    }
    const uint32_t mi_bit = mi_bit_for_cause(cause);
    if (mi_bit == 0) {
        return true;
    }
    if ((status_reg & SR_IM2) == 0) {
        return false;
    }
    if ((current_mi_mask & mi_bit) == 0) {
        return false;
    }
    if (((g_global_int_mask >> 16) & mi_bit) == 0) {
        return false;
    }
    return true;
}

bool cause_eligible(const uint32_t* status_reg, ultramodern::InterruptCause cause) {
    if (cause == ultramodern::InterruptCause::None) {
        return true;
    }
    if (status_reg == nullptr) {
        return false;
    }
    return cause_eligible(*status_reg, cause);
}

} // namespace ultramodern::intmask
