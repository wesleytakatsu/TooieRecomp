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
//
// Demo path precision probe (P08): TOOIE_DIAG_DEMO_PATH=1 is independent of
// TOOIE_DIAG. It implies movement sampling and adds throttled func_80013B70
// caller/RA logs plus player physics/goto fields for one attract clip.
//
// First demo egg-shot probe (P08): TOOIE_DIAG_SHOT=1 is independent of
// TOOIE_DIAG. Compact per-logic-tick window around the first recorded C-up
// fire in the Wooded Hollow gameplay demo. Does not change game behavior.
//
// Forced/cinematic facing probe (P08 USER_BUG_B): TOOIE_DIAG_FORCED_FACING=1
// is independent of TOOIE_DIAG. Compact per-actor [facing] lines for scripted
// movers (filtered; not a full actor dump). Also enables throttled 13B70 RA.
//
// Attract cycle / follow-camera / egg-type probe (P08 provisional):
// TOOIE_DIAG_ATTRACT=1 is independent of TOOIE_DIAG. Implies SHOT + DEMO_SYNC.
// Dumps gsattract D_808003D0, user-demo ordinals, egg spawn type, and
// follow-camera fields. Does not change game behavior.

namespace tooie_diag {

bool enabled();
bool timing_enabled();
bool movement_enabled();
bool input_enabled();
bool frontend_enabled();
bool demo_path_enabled();
bool demo_sync_enabled();
bool shot_enabled();
bool forced_facing_enabled();
bool attract_enabled();

void note_syscall_dispatch(uint32_t ovl, uint32_t ep);
void note_syscall_return(uint32_t ovl, uint32_t ep);
void note_sched_ack();
void note_rsp_audtask();
void note_queue_samples(size_t sample_count, uint32_t queued_bytes_after,
                        int16_t peak_abs = 0, size_t nonzero = 0);
void note_audio_device(uint32_t device_id, const char* driver, const char* device_name);

// Timing counters (TOOIE_DIAG_TIMING=1).
// guest_logic_tick: monotonically increasing counter, incremented exactly once
// per proven main-game-logic boundary (func_800151BC wrapper).
void note_guest_vi();
void note_game_logic(int32_t divisor = -1);
void note_display_list();
void note_animation_update();
void note_host_update_gfx();
void note_guest_input_update();

// gfx/VI-adjacent host tick counter (update_gfx). Always increments when
// movement diagnostics are on so timebase can be compared to movement dumps.
uint64_t gfx_ticks();

// Increment the guest logic tick counter and return the new value. Called
// exactly once per func_800151BC (proven main-game-logic boundary).
uint64_t note_guest_logic_tick();
uint64_t guest_logic_tick();

// Sample a core1 degree-math / RA-wrapper result. Logs the first N calls,
// then every Nth, plus any non-finite or huge discontinuity.
void note_math(const char* name, float input, float output);

// Throttled func_80013B70 sample. ra is the caller return address (ctx->r31).
void note_13b70(uint32_t ra, float y, float x, float mag, float angle);

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

// Gameplay detection for direction/facing diagnosis (TOOIE_DIAG_DIRECTION=1).
bool direction_enabled();
void set_gameplay_detected(bool v);
bool gameplay_detected();

// Read player bastick + yaw + position from rdram and log.
void note_movement_pipeline(uint8_t* rdram, const char* input_name);

} // namespace tooie_diag

// Per-guest-logic-tick demo-path sampler. Called once per glrecord sample
// consumption (overlay 698 ep 0) during the attract gameplay demo (mode==5).
extern "C" void tooie_demo_path_tick(uint8_t* rdram, uint64_t tick);
extern "C" void tooie_shot_tick(uint8_t* rdram, uint64_t tick);
extern "C" void tooie_shot_note_overlay(uint8_t* rdram, void* ctx,
                                       uint32_t ovl, uint32_t ep, int leaving);
