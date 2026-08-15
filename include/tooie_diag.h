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
//
// Directional input probe (P08): TOOIE_DIAG_INPUT=1 logs host/pad/guest
// stick layers. Optional TOOIE_DIAG_INPUT_VECTOR=center|right|left|up|down|
// up-right|up-left|down-right|down-left forces synthetic host axes after
// the real frontend read. No effect when both are unset.

namespace tooie_diag {

bool enabled();
bool movement_enabled();
bool input_enabled();
bool frontend_enabled();

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

// If TOOIE_DIAG_INPUT_VECTOR is set, replace *x/*y with that unit vector.
void apply_input_vector_override(float* x, float* y);

// Applies synthetic input sequence (e.g. START, stick, buttons) when
// TOOIE_DIAG_FRONTEND=1 or TOOIE_DIAG_INPUT_SCRIPT is set.
void apply_synthetic_input(int controller, uint16_t* buttons, float* x, float* y);

// Note frontend / menu state transition (e.g. from chattract / chgameselect / mode changes).
void note_frontend_state(uint32_t ovl, uint32_t ep, int32_t mode, int32_t map, uint32_t warp);

// Throttled host-axis log (frontend normalized stick, after override).
void note_host_input(int controller, float x, float y, uint16_t buttons);

// Throttled guest-boundary log (OSContPad + func_80015D60 scale word).
void note_guest_pad(int8_t stick_x, int8_t stick_y, uint16_t buttons,
                    int32_t scale_word, float remap_x, float remap_y);

// Last host axes observed by note_host_input (0,0 if never called).
void last_host_input(float* x, float* y);

} // namespace tooie_diag
