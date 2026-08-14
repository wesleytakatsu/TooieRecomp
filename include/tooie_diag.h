#pragma once

#include <cstddef>
#include <cstdint>

// Runtime diagnostics for boot hang + audio path (P07 residual).
// Enable with TOOIE_DIAG=1 (or any non-empty/non-zero value).
// Optional: TOOIE_DIAG_INTERVAL_MS=2000 (default).
//
// Movement fidelity probe (P08): TOOIE_DIAG_MOVEMENT=1 is independent of
// TOOIE_DIAG so the attract/demo path can be traced without the older
// diagnostic overlay timing side-effects.

namespace tooie_diag {

bool enabled();
bool movement_enabled();

void note_syscall_dispatch(uint32_t ovl, uint32_t ep);
void note_syscall_return(uint32_t ovl, uint32_t ep);
void note_sched_ack();
void note_rsp_audtask();
void note_queue_samples(size_t sample_count, uint32_t queued_bytes_after,
                        int16_t peak_abs = 0, size_t nonzero = 0);
void note_audio_device(uint32_t device_id, const char* driver, const char* device_name);

// gfx/VI-adjacent host tick counter (update_gfx). Always increments when
// movement diagnostics are on so timebase can be compared to movement dumps.
uint64_t gfx_ticks();

// Sample a core1 degree-math / RA-wrapper result. Logs the first N calls,
// then every Nth, plus any non-finite or huge discontinuity.
void note_math(const char* name, float input, float output);

// Call from a frequent host path (e.g. update_gfx) — prints summary on interval.
void tick();

// Force an immediate summary line to stderr.
void report_now(const char* reason);

} // namespace tooie_diag
