#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>

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
//
// Backpack flap witness (P09): TOOIE_DIAG_BACKPACK_FLAP=1 is independent of
// TOOIE_DIAG. Default-off CPU probe at the common model renderer. It records
// func_800DE448 draws whose model_bin was returned by func_800D674C for the
// babackpack asset IDs (0x621/0x625/0x60A/0x61B/0x831), then samples only
// func_800E2180 dual-matrix submits inside that draw. It writes
// /tmp/tooie-backpack-flap-witness.csv and
// /tmp/tooie-backpack-flap-anomalies.txt on first tracked draw, periodic
// flush, anomaly, or shutdown. The self-verifying status marker is
// /tmp/tooie-backpack-flap-status.txt. It does not alter animation,
// interpolation, culling, or rendering.
//
// Player model + P09 secondary-sway witness: TOOIE_DIAG_SECONDARY_SWAY=1 is
// independent of TOOIE_DIAG and of TOOIE_DIAG_BACKPACK_FLAP. Default-off CPU
// probe. Primary token is KAZOOIE head feather (schema
// P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V6B), not Banjo snout. Compact type-2
// DCF48 still supplies the G_MTX. Durable model_bin is registered at DE448.
// Primary 0x62B plume is model-relative 0x124A0 INDEX 30 boneAnimID 103.
// Fallback solo 0x61C crest is 0xF558 INDEX 11 boneAnimID 69. Alternate
// 0x83A crest is 0xD408 INDEX 8 boneAnimID 69. V2 adds a producer witness at
// func_800AE8A8 (quat+scale compose) and func_800AE820 (final 4x4). V3 adds
// scale-normalized quaternion telemetry. V4 keeps the G_MTX token on a
// modelview-parallel stack so nested bone G_MTX / type-0x18 restore cannot
// drop it before setVertexCommon. RT64 match plumbing remains the existing
// V5 GameFrame::matchTransform path. V6B is a diagnostic-only triangle
// witness at F3DEX2 G_TRI1/G_TRI2 and RSP::drawIndexedTri. Classification
// starts from per-vertex worldIndices; the G_MTX token is not required for
// visibility. Banjo snout 0x11CB8 is closed as the primary token.
//
// Kazooie feather animation sample-time witness (P09):
// TOOIE_DIAG_FEATHER_ANIM_SAMPLE_TIME=1 is independent of TOOIE_DIAG and of
// TOOIE_DIAG_SECONDARY_SWAY. Default-off CPU probe. Schema
// P09_FEATHER_ANIM_SAMPLE_TIME_V1. After baanim_update (0x8008C894) it
// records clip id / sample time / duration / playback / |v_xz| proxy.
// Before func_800AE8A8 compose it records raw 0x28 TRS for bones
// 79,100-105,107,108 and the parent of 107. Status/CSV:
// /tmp/tooie-feather-anim-sample-time-status.txt and
// /tmp/tooie-feather-anim-sample-time.csv. Does not alter animation,
// interpolation, culling, or rendering.
//
// Passive high-rate temporal coherency (P09, RT64 CPU):
// TOOIE_DIAG_FEATHER_TEMPORAL_COHERENCY=1 writes
// /tmp/tooie-feather-temporal-coherency-status.txt (schema VERSION=4).
// Also enables the CPU P/V scale witness in func_800E44FC:
// /tmp/tooie-pv-scale-status.txt. TOOIE_DIAG_PV_SCALE=1 enables only
// the scale witness. Neither mutates weights, matrices, or GPU state.
//
// RSP endpoint sidecar bind proof (P09, RT64 CPU, default-off):
// TOOIE_DIAG_RSP_ENDPOINT_BINDING=1 writes
// /tmp/tooie-rsp-endpoint-binding-status.txt. CPU-only: descriptor
// identities, last-dispatch vs session sidecar/witness W-sign A/B,
// 0x62B target-DL / world-set membership, intermediate-only feather
// crossing counts, live interpolated clip.w vs endpoint W, CPU
// POLICY_A/B/live-sign explosion replay (VERSION=4 geometry), and
// unfiltered POLICY_A/LIVE/TRIANGLE_CURRENT residual edge/NDC
// distributions vs ALL_CURRENT and vs endpoint envelope, plus
// CPU-only CLIP_LERP/NDC_LERP (closed: do not implement), JOIN
// mixed-world relative/phase error, TRANSFORM/DRAW/COMPONENT
// coherent-CURRENT counterfactuals, class A-E cohort correlation,
// COHORT_ALIGNED_LIVE, MAPPING_COHERENT_LIVE (interpolate
// VIEW+WORLD only when both have a valid PREV pair), and
// EXACT_ORDER_VALIDATED_CAMERA_POSE_VIEW (validated camera-pose
// interpolation then inverse VIEW, exact ((V*P)*W)*pos4, default-off).
// No GPU readback.
//
// Shader-local CURRENT tfPos workaround (P09). RT64 reads
// TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT and stays off when it is absent,
// so Banjo is unchanged. Tooie startup sets it to 1 only when unset,
// before renderer init. That forces RSPProcessCS tfPos to the endpoint
// CURRENT clip. Fog follows tfPos. Projection/Transform/Tile/LookAt/Vertex
// weights and present interpolation stay unchanged. Explicit 0 is the
// original-interpolation rollback. WORKAROUND, not a true interpolation
// fix. No GPU readback. Does not enable other diagnostics.
//
// The previous production W-sign snap
// (if prevClipW * curClipW <= 0 then tfPos = curClip) is disabled.
// TOOIE_DIAG_RSP_MARK_W_CROSSING=1 only offsets NDC X when
// FORCE_ENDPOINT_CURRENT is also set. Not a production fix.
//
// Default-off P09 production candidate (WORLD only):
// TOOIE_TEST_P09_SELECTIVE_ELEMENTWISE_WORLD=1 selects exact 16-entry
// elementwise interpolation for WORLD matrices consumed by GameCalls
// that use more than one distinct WORLD. Other WORLDs keep decomposed
// RigidBody::lerp. VIEW/PROJECTION unchanged. No GPU readback.
//
// Default-off selector-granularity audit (CPU only, no interpolation change):
// TOOIE_DIAG_P09_SELECTOR_AUDIT=1 writes bounded GameCall usage aggregates
// for that WORLD-index policy to /tmp/tooie-p09-selector-audit.txt.
//
// Second default-off candidate (does not replace the one above):
// TOOIE_TEST_P09_MULTI_ONLY_ELEMENTWISE_WORLD=1 applies the same 16-entry
// WORLD lerp only to indices used by multi-WORLD GameCalls and by no
// single-WORLD GameCall in that workload. Mixed indices keep RigidBody::lerp.
// If both test toggles are set, the original selector is applied.
// Enable one candidate at a time. VIEW/PROJECTION/shader unchanged.

namespace tooie_diag {

bool enabled();
bool timing_enabled();
bool movement_enabled();
bool input_enabled();
bool frontend_enabled();
bool demo_path_enabled();
bool demo_sync_enabled();
bool demo7_drift_enabled();
bool demo1_parity_enabled();
bool demo1_projectile_enabled();
bool demo1_target_enabled();
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

bool backpack_flap_enabled();
void backpack_note_entry3();
void backpack_note_asset_lookup(uint32_t asset_id, uint32_t model_ptr);
bool backpack_tracked_model_id(uint32_t model_ptr, uint32_t* model_id_out);
void backpack_note_draw_entry(uint32_t model_bin, uint32_t model_id);
void backpack_note_de498();
void backpack_note_e2aa4();
void backpack_draw_begin(uint8_t* rdram, void* ctx, uint32_t model_id,
                         uint32_t model_bin);
void backpack_draw_end();
bool backpack_draw_scope_active();
void backpack_note_dual_mtx(uint8_t* rdram, void* ctx);

// Legacy diagnostic truthiness: non-null, non-empty, first character not '0'.
inline bool tooie_diag_legacy_flag_on(const char* value) {
    return value != nullptr && value[0] != '\0' && value[0] != '0';
}

// Case-insensitive exact token. A longer or shorter value does not match.
inline bool tooie_diag_exact_token_on(const char* value, const char* literal) {
    if (value == nullptr || literal == nullptr) {
        return false;
    }
    while (*literal != '\0') {
        char c = *value;
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
        if (c != *literal) {
            return false;
        }
        ++value;
        ++literal;
    }
    return *value == '\0';
}

// TOOIE_DIAG_P09_SEAM_EDGE_PATHOLOGY_CORRELATION uses exact 1/true/yes/on.
inline bool tooie_diag_correlation_flag_on(const char* value) {
    if (value == nullptr || value[0] == '\0') {
        return false;
    }
    return tooie_diag_exact_token_on(value, "1") ||
           tooie_diag_exact_token_on(value, "true") ||
           tooie_diag_exact_token_on(value, "yes") ||
           tooie_diag_exact_token_on(value, "on");
}

// Consumers that read the durable model bin published by the 80092B04 getter.
// Temporal matches the RT64 envOn fallback: a TOOIE value of null, empty, or
// leading '0' yields to RT64_DIAG_FEATHER_TEMPORAL_COHERENCY.
inline bool getter_durable_metadata_requested(
    bool secondary_sway,
    const char* handoff_v7,
    const char* postprocess_q,
    const char* temporal_tooie,
    const char* temporal_rt64,
    const char* endpoint_binding,
    const char* seam_correlation) {
    if (secondary_sway) {
        return true;
    }
    const char* pair_snap = std::getenv("TOOIE_DIAG_P09_LOCAL_PAIR_SNAPSHOT");
    if (pair_snap != nullptr && pair_snap[0] != '\0' && pair_snap[0] != '0') {
        return true;
    }
    if (tooie_diag_legacy_flag_on(handoff_v7) ||
        tooie_diag_legacy_flag_on(postprocess_q)) {
        return true;
    }
    const char* temporal = temporal_tooie;
    if (temporal == nullptr || temporal[0] == '\0' || temporal[0] == '0') {
        temporal = temporal_rt64;
    }
    return tooie_diag_legacy_flag_on(temporal) ||
           tooie_diag_legacy_flag_on(endpoint_binding) ||
           tooie_diag_correlation_flag_on(seam_correlation);
}

bool secondary_sway_enabled();
bool p09_local_pair_provenance_enabled();
void secondary_sway_note_98ba8(uint8_t* rdram, uint32_t player);
bool secondary_sway_player_draw_begin(uint8_t* rdram, uint32_t player);
void secondary_sway_player_draw_end();
void secondary_sway_note_model_getter(uint8_t* rdram, uint32_t owner,
                                      uint32_t arg1, uint32_t model_bin);
uint64_t v7_model_bin();
void v7_note_model_getter(uint8_t* rdram, uint32_t owner, uint32_t arg1, uint64_t model_bin);
uint64_t v7_model_getter_raw_62a_count();
uint64_t v7_model_getter_raw_62b_count();
uint64_t v7_model_getter_normalized_62b_count();
uint64_t v7_model_bin_bridge_accept_count();
uint64_t v7_model_bin_bridge_reject_count();
uint64_t v7_model_getter_wrapper_call_count();
uint64_t v7_model_getter_reject_arg_filter_count();
uint64_t v7_model_getter_reject_return_zero_count();
uint64_t v7_model_getter_reject_resource_unavailable_count();
uint64_t v7_model_getter_reject_raw_resource_other_count();
uint64_t v7_model_getter_reject_metadata_unavailable_count();
uint64_t v7_model_getter_reject_metadata_mismatch_count();
uint64_t v7_model_getter_reject_other_count();
const char* v7_model_getter_first_rejection_reason();
uint64_t v7_durable_register_total();
uint64_t v7_durable_normalized_62b_count();
uint64_t v7_model_context_publish_count();
uint64_t v7_model_context_clear_count();
uint64_t v7_model_context_generation();
uint64_t v7_model_context_last_generation();
uint32_t v7_model_context_last_physical_base();
uint32_t v7_model_context_last_player();
uint32_t v7_model_bridge_first_raw_resource();
uint32_t v7_model_bridge_first_normalized_resource();
uint64_t v7_model_bridge_first_guest_bin();
uint32_t v7_model_bridge_last_raw_resource();
uint32_t v7_model_bridge_last_normalized_resource();
uint64_t v7_model_bridge_last_guest_bin();
void secondary_sway_note_player_de448(uint8_t* rdram, void* ctx);
void secondary_sway_note_bone_anim(uint8_t* rdram, uint32_t anim_list,
                                   uint32_t bone_anim_id, uint32_t arg2,
                                   uint32_t arg3, uint32_t arg4,
                                   uint32_t arg5);
void secondary_sway_note_bone_anim_entry();
void secondary_sway_note_snout_dual_mtx(uint8_t* rdram, void* ctx);
void secondary_sway_note_snout_dual_mtx_entry();
void secondary_sway_note_e2aa4_entry();
bool secondary_sway_note_e2aa4(uint8_t* rdram, void* ctx);
void secondary_sway_e2aa4_snout_end();
bool secondary_sway_e2aa4_snout_scope_active();
void secondary_sway_note_de2a4_entry();
void secondary_sway_note_de2a4(uint8_t* rdram, void* ctx);
bool secondary_sway_de498_begin(uint8_t* rdram, void* ctx);
void secondary_sway_de498_end();
void secondary_sway_note_dcf48_entry();
bool secondary_sway_note_dcf48(uint8_t* rdram, void* ctx);
void secondary_sway_dcf48_snout_end();
bool secondary_sway_dcf48_snout_scope_active();
void secondary_sway_note_ade2c(uint8_t* rdram, void* ctx);
void secondary_sway_note_19aa0(uint8_t* rdram, uint32_t parent_ptr,
                               uint32_t local_ptr);
void secondary_sway_note_18c28(uint8_t* rdram, uint32_t gfx_mtx_ptr,
                               uint32_t float_src_ptr);
void secondary_sway_note_hip_consume_entry();
void secondary_sway_note_hip_consume(uint8_t* rdram, void* ctx);
void secondary_sway_note_ae8a8(uint8_t* rdram, uint32_t anim_list_ptr,
                               uint32_t skeleton, uint32_t slot_base,
                               uint32_t additive_ctx);
void secondary_sway_note_ae820(uint8_t* rdram, uint32_t anim_list_ptr,
                               uint32_t skeleton, uint32_t slot_base,
                               uint32_t additive_ctx);

bool feather_anim_sample_time_enabled();
void feather_anim_note_baanim_update(uint8_t* rdram, uint32_t player);
void feather_anim_note_ae8a8_before_compose(uint8_t* rdram,
                                            uint32_t anim_list_ptr,
                                            uint32_t skeleton,
                                            uint32_t slot_base,
                                            uint32_t additive_ctx);

} // namespace tooie_diag

// Per-guest-logic-tick demo-path sampler. Called once per glrecord sample
// consumption (overlay 698 ep 0) during the attract gameplay demo (mode==5).
extern "C" void tooie_demo_path_tick(uint8_t* rdram, uint64_t tick);
extern "C" void tooie_shot_tick(uint8_t* rdram, uint64_t tick);
extern "C" void tooie_shot_note_overlay(uint8_t* rdram, void* ctx,
                                       uint32_t ovl, uint32_t ep, int leaving);

// Default-off P09 PREV/CUR snapshot witness list. Count, cap+1 on overflow,
// or 0xFFFFFFFF if a matched static candidate has no bound world.
// Candidate class is not a visual identity.
extern "C" uint32_t tooie_diag_p09_local_pair_witnesses(
    uint64_t workload_id, uint32_t* world_index, uint32_t* resource_id,
    uint32_t* node_rel, uint32_t* bone_index, uint32_t* bone_anim_id,
    uint8_t* candidate_class, uint32_t cap);
extern "C" uint64_t tooie_diag_rt64_matrix(
    uint32_t command_address, uint32_t matrix_address,
    uint32_t physical_address, uint32_t params, uint32_t stack_before,
    uint32_t stack_after, uint32_t group_depth, uint32_t matrix_id,
    uint32_t group_index, uint32_t explicit_group, uint64_t workload_id,
    uint32_t display_list_address, uint64_t display_list_counter);
extern "C" void tooie_diag_rt64_vertex(
    uint64_t token, uint64_t workload_id, uint32_t transform_index,
    uint32_t group_index, uint32_t vertex_start, uint32_t vertex_count,
    uint32_t physical_address, uint32_t segmented_address, uint32_t draw_uid,
    uint32_t draw_call_index, uint32_t draw_min_matrix,
    uint32_t draw_max_matrix, uint32_t projection_type,
    uint32_t projection_index, uint32_t other_mode_h);
extern "C" void tooie_diag_rt64_gbi_tri(uint32_t opcode);
extern "C" void tooie_diag_rt64_tri_handler_enter(uint32_t kind);
extern "C" void tooie_diag_rt64_feather_tri(
    uint64_t token, uint64_t workload_id, uint32_t cur_transform_index,
    uint32_t slot_a, uint32_t slot_b, uint32_t slot_c,
    uint32_t global_a, uint32_t global_b, uint32_t global_c,
    uint32_t world_a, uint32_t world_b, uint32_t world_c,
    uint32_t draw_uid, uint32_t draw_call_index,
    uint32_t draw_min_matrix, uint32_t draw_max_matrix,
    uint32_t current_dl, uint32_t parent_dl, uint32_t dl_depth);
extern "C" void tooie_diag_rt64_match(
    uint64_t workload_id, uint64_t previous_workload_id,
    uint32_t transform_index, uint32_t previous_transform_index,
    uint32_t group_index, uint32_t matrix_id);
extern "C" bool tooie_diag_rt64_match_override(
    uint64_t workload_id, uint32_t transform_index, uint32_t group_index);
extern "C" bool tooie_diag_rt64_no_interpolation(
    uint64_t workload_id, uint32_t transform_index, uint32_t group_index);
extern "C" void tooie_diag_rt64_post_match(
    uint64_t workload_id, uint32_t transform_index, uint32_t previous_transform_index,
    uint32_t group_index, float previous_weight, float current_weight,
    const float* previous_matrix, const float* current_matrix,
    const float* final_matrix, uint32_t no_interpolation);
extern "C" void tooie_diag_rt64_match_observe(
    uint32_t stage, uint64_t current_workload_id,
    uint64_t previous_workload_id, uint64_t current_call_hash,
    uint64_t previous_call_hash, uint32_t current_scene_proj,
    uint32_t previous_scene_proj, uint32_t current_call_index,
    uint32_t previous_call_index, uint32_t current_range_begin,
    uint32_t current_range_end, uint32_t previous_range_begin,
    uint32_t previous_range_end, uint32_t current_transform_index,
    uint32_t previous_transform_index, uint32_t current_group_index,
    uint32_t previous_group_index, uint32_t current_matrix_id,
    uint32_t previous_matrix_id, uint32_t current_projection_type,
    uint32_t previous_projection_type, uint32_t current_projection_index,
    uint32_t previous_projection_index, uint32_t reason,
    uint32_t candidate_valid, float position_difference,
    float orientation_difference, float screen_space_difference);
extern "C" void tooie_diag_rt64_match_finalize();
extern "C" void tooie_diag_rt64_match_flush();
extern "C" uint32_t tooie_diag_rt64_v6c_vertex_load(
    uint32_t slot, uint64_t generation, uint64_t workload_id,
    uint32_t current_dl, uint32_t parent_dl, uint32_t dl_depth,
    uint32_t segmented_source, uint32_t resolved_source,
    uint32_t source_vertex_index, uint32_t world_index,
    uint32_t active_transform, uint32_t stack_depth);
extern "C" void tooie_diag_rt64_v6c_tri(
    uint64_t prim_seq, uint64_t workload_id, uint32_t active_transform,
    const char* prim_kind, uint32_t current_dl, uint32_t parent_dl,
    uint32_t dl_depth, const uint32_t* slots, const uint32_t* globals,
    const uint32_t* worlds, const uint32_t* bones,
    const uint64_t* generations, const uint64_t* load_workloads,
    const uint32_t* load_dl, const uint32_t* load_parent,
    const uint32_t* load_depth, const uint32_t* segmented_sources,
    const uint32_t* resolved_sources, const uint32_t* source_indices,
    const uint32_t* load_active_transforms, const uint32_t* load_stack_depths);
extern "C" void tooie_diag_rt64_v6c_gdl(
    uint32_t raw_address, uint32_t resolved_address, uint32_t parent_address,
    uint32_t depth, uint32_t is_call, uint32_t segment3_base);
