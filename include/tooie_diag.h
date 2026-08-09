#pragma once

#include <cstddef>
#include <cstdint>

// Runtime diagnostics for boot hang + audio path (P07 residual).
// Enable with TOOIE_DIAG=1 (or any non-empty/non-zero value).
// Optional: TOOIE_DIAG_INTERVAL_MS=2000 (default).

namespace tooie_diag {

bool enabled();

void note_syscall_dispatch(uint32_t ovl, uint32_t ep);
void note_syscall_return(uint32_t ovl, uint32_t ep);
void note_sched_ack();
void note_rsp_audtask();
void note_queue_samples(size_t sample_count, uint32_t queued_bytes_after,
                        int16_t peak_abs = 0, size_t nonzero = 0);
void note_audio_device(uint32_t device_id, const char* driver, const char* device_name);

// Call from a frequent host path (e.g. update_gfx) — prints summary on interval.
void tick();

// Force an immediate summary line to stderr.
void report_now(const char* reason);

} // namespace tooie_diag
