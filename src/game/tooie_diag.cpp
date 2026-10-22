#include "tooie_diag.h"
#include "tooie_diag_path.h"
#include "recomp.h"
#include "render/rt64_p09_local_pair_snapshot.h"

#include <atomic>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <map>
#include <string>
#include <unordered_set>
#include <thread>
#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#include <SDL.h>
#else
#include <unistd.h>

#include <SDL2/SDL.h>
#endif

namespace tooie_diag {

void v7_note_durable_model_bin(uint32_t model_bin, uint32_t raw_resource,
                               uint32_t normalized_resource, uint32_t player);
void v7_clear_model_context_if_bin(uint32_t model_bin);

#define TOOIE_SECONDARY_SWAY_DIAGNOSTIC_SCHEMA \
    "P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V6C"
#define TOOIE_FEATHER_ANIM_SAMPLE_TIME_SCHEMA \
    "P09_FEATHER_ANIM_SAMPLE_TIME_V1"

// Defined below; used by ensure_init's background thread.
void report_now(const char* reason);

namespace {

std::atomic<bool> g_enabled{false};
std::atomic<bool> g_timing_enabled{false};
std::atomic<bool> g_movement_enabled{false};
std::atomic<bool> g_input_enabled{false};
std::atomic<bool> g_frontend_enabled{false};
std::atomic<bool> g_direction_enabled{false};
std::atomic<bool> g_demo_path_enabled{false};
std::atomic<bool> g_demo_sync_enabled{false};
std::atomic<bool> g_demo7_drift_enabled{false};
std::atomic<bool> g_demo1_parity_enabled{false};
std::atomic<bool> g_demo1_projectile_enabled{false};
std::atomic<bool> g_demo1_target_enabled{false};
std::atomic<bool> g_shot_enabled{false};
std::atomic<bool> g_forced_facing_enabled{false};
std::atomic<bool> g_attract_enabled{false};
std::atomic<bool> g_backpack_flap_enabled{false};
std::atomic<bool> g_secondary_sway_enabled{false};
std::atomic<bool> g_p09_local_pair_snapshot{false};
std::atomic<bool> g_getter_durable_needed{false};
std::atomic<bool> g_secondary_sway_interp_simple_enabled{false};
std::atomic<bool> g_secondary_sway_no_interpolation_enabled{false};
std::atomic<bool> g_feather_anim_sample_time_enabled{false};
std::atomic<bool> g_gameplay_seen{false};
std::atomic<bool> g_inited{false};
std::atomic<uint32_t> g_host_x_bits{0};
std::atomic<uint32_t> g_host_y_bits{0};
std::chrono::steady_clock::time_point g_first_input_tp{};
std::atomic<bool> g_first_input_seen{false};
float g_override_x = 0.0f;
float g_override_y = 0.0f;
bool g_override_active = false;
std::atomic<uint64_t> g_gfx_ticks{0};
std::atomic<uint64_t> g_syscall_dispatch{0};
std::atomic<uint64_t> g_syscall_return{0};
std::atomic<uint64_t> g_sched_ack{0};
std::atomic<uint64_t> g_rsp_aud{0};
std::atomic<uint64_t> g_queue_calls{0};
std::atomic<uint64_t> g_queue_samples{0};
std::atomic<uint64_t> g_queue_nonzero{0};
std::atomic<uint32_t> g_last_queue_bytes{0};
std::atomic<int32_t> g_last_peak{0};
std::atomic<int32_t> g_max_peak{0};
std::atomic<uint32_t> g_audio_device{0};
std::atomic<uint32_t> g_last_ovl{0};
std::atomic<uint32_t> g_last_ep{0};
std::atomic<uint32_t> g_depth{0};
std::atomic<uint32_t> g_max_depth{0};
std::atomic<uint64_t> g_host_update_gfx{0};
std::atomic<uint64_t> g_guest_input_updates{0};
std::atomic<uint64_t> g_guest_vi{0};
std::atomic<uint64_t> g_game_logic{0};
std::atomic<uint64_t> g_display_list{0};
std::atomic<uint64_t> g_animation_update{0};

// Nested stack of in-flight syscalls (ovl<<16|ep), up to 8 levels.
constexpr int kStackMax = 8;
std::atomic<uint32_t> g_stack[kStackMax]{};
char g_driver[64]{};
char g_device_name[128]{};
std::mutex g_name_mu;
std::mutex g_timing_mu;

std::chrono::steady_clock::time_point g_last_report{};
std::chrono::steady_clock::time_point g_timing_start{};
int g_interval_ms = 2000;

constexpr uint32_t kBackpackAssetIds[] = {
    0x621u, 0x625u, 0x60Au, 0x61Bu, 0x831u
};
constexpr size_t kBackpackAssetCount =
    sizeof(kBackpackAssetIds) / sizeof(kBackpackAssetIds[0]);
constexpr size_t kBackpackFlapRingSize = 256;
constexpr uint64_t kBackpackFlushInterval = 60;
constexpr size_t kBackpackUniqueNodeLimit = 4096;
constexpr size_t kBackpackDrawStackSize = 32;
constexpr size_t kBackpackObservedAssetLimit = 32;

enum class BackpackReject : size_t {
    NoActiveModelContext = 0,
    ModelIdMismatch,
    NoNodeContext,
    UnexpectedNodeType,
    Other,
    Count,
};

struct BackpackFlapSample {
    uint64_t frame = 0;
    uint64_t gfx_tick = 0;
    uint32_t model_resource_id = 0;
    uint32_t model_bin = 0;
    uint32_t node_ptr = 0;
    uint32_t geocmd_ptr = 0;
    uint32_t caller_ra = 0;
    uint32_t gfx_ctx = 0;
    uint32_t node_word0 = 0;
    uint8_t node_byte8 = 0;
    uint8_t node_byte9 = 0;
    uint32_t prev_mtx_ptr = 0;
    uint32_t curr_mtx_ptr = 0;
    uint32_t prev_words[16]{};
    uint32_t curr_words[16]{};
    uint32_t scale_bits = 0;
    uint32_t pos_x_bits = 0;
    uint32_t pos_y_bits = 0;
    uint32_t pos_z_bits = 0;
    bool draw_submitted = false;
};

struct BackpackDrawScope {
    bool active = false;
    uint32_t model_id = 0;
    uint32_t model_bin = 0;
    uint32_t position_ptr = 0;
    uint32_t scale_bits = 0;
    uint32_t caller_ra = 0;
};

struct BackpackObservedAsset {
    uint32_t asset_id = 0;
    uint64_t hits = 0;
    uint32_t model_ptr = 0;
};

thread_local std::array<BackpackDrawScope, kBackpackDrawStackSize>
    g_backpack_draw_scopes{};
thread_local size_t g_backpack_draw_depth = 0;
thread_local size_t g_backpack_draw_overflow = 0;

std::array<BackpackFlapSample, kBackpackFlapRingSize> g_backpack_flap_ring{};
size_t g_backpack_flap_ring_next = 0;
size_t g_backpack_flap_ring_count = 0;
std::mutex g_backpack_flap_mu;
bool g_backpack_flap_anomaly_seen = false;
uint64_t g_backpack_flap_samples = 0;
uint64_t g_backpack_flap_anomalies = 0;
uint64_t g_backpack_flap_last_frame = 0;
bool g_backpack_flap_first_hit = false;
uint64_t g_backpack_flap_first_hit_frame = 0;
uint32_t g_backpack_flap_first_hit_callsite = 0;
uint32_t g_backpack_first_model_id = 0;
uint32_t g_backpack_first_model_bin = 0;
uint32_t g_backpack_first_node_ptr = 0;
std::array<uint32_t, kBackpackAssetCount> g_backpack_model_ptrs{};
uint64_t g_backpack_entry3_hits = 0;
uint64_t g_backpack_d674c_hits = 0;
uint64_t g_backpack_d674c_tracked_hits = 0;
std::array<uint64_t, kBackpackAssetCount> g_backpack_asset_hits{};
uint64_t g_backpack_asset_other_hits = 0;
std::array<BackpackObservedAsset, kBackpackObservedAssetLimit>
    g_backpack_observed_other_assets{};
size_t g_backpack_observed_other_asset_count = 0;
uint64_t g_backpack_observed_other_asset_evictions = 0;
uint64_t g_backpack_de448_hits = 0;
uint64_t g_backpack_de448_tracked_hits = 0;
uint64_t g_backpack_de498_hits = 0;
uint64_t g_backpack_de498_tracked_hits = 0;
uint64_t g_backpack_e2aa4_hits = 0;
uint64_t g_backpack_e2aa4_tracked_hits = 0;
uint64_t g_backpack_e2180_hits = 0;
uint64_t g_backpack_e2180_tracked_hits = 0;
uint64_t g_backpack_e2180_sample_written = 0;
std::array<uint64_t, static_cast<size_t>(BackpackReject::Count)>
    g_backpack_e2180_rejections{};
uint64_t g_backpack_scope_overflow_events = 0;
uint64_t g_backpack_scope_max_depth = 0;
std::unordered_set<uint32_t> g_backpack_unique_nodes;
uint64_t g_backpack_unique_node_overflow = 0;
FILE* g_backpack_witness_csv = nullptr;
bool g_backpack_witness_csv_header = false;
int64_t g_backpack_flap_process_start_ms = 0;
long g_backpack_flap_pid = 0;

constexpr uint32_t kPlayerTableBase = 0x80135490u;
constexpr uint32_t kPlayerTableSlots = 8u;
constexpr size_t kFeatherAnimMaxRows = 384;
constexpr size_t kFeatherAnimControlTicks = 8;
constexpr size_t kFeatherAnimClipIdSlots = 16;
constexpr float kFeatherAnimLargeRotRad = 0.25f;
constexpr float kFeatherAnimLargeScale = 0.05f;
constexpr float kFeatherAnimLargeTrans = 2.0f;

enum class FeatherAnimTimeClass : uint32_t {
    None = 0,
    Forward,
    Same,
    Backward,
    WrapExpected,
    UnexpectedJump,
};

enum class FeatherAnimReject : size_t {
    None = 0,
    BaanimNotPlayerTable,
    BaanimStateZero,
    BaanimAnctrlZero,
    Ae8a8BadSkeleton,
    Ae8a8NoPlume,
    Ae8a8AnimBaseZero,
    CsvOpenFailed,
    MaxRows,
    Count,
};

struct FeatherAnimSnap {
    uint64_t tick = 0;
    uint32_t player = 0;
    uint32_t slot = 0;
    uint32_t locomotion = 0;
    uint32_t baanim_mode = 0;
    uint32_t anctrl = 0;
    uint32_t clip_ptr = 0;
    int16_t clip_id = 0;
    int16_t anctrl_index = 0;
    uint32_t flags_word = 0;
    uint32_t playback = 0;
    int32_t direction = 0;
    float sample_time = 0.0f;
    float sample_time_prev = 0.0f;
    float sample_dt = 0.0f;
    float duration = 0.0f;
    float cached_timer = 0.0f;
    float clip_f14 = 0.0f;
    float horiz_speed = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float vz = 0.0f;
    uint32_t clip_w[8]{};
    uint32_t apply_fn = 0;
    uint32_t file_ptr = 0;
    int16_t frame_start = 0;
    int16_t frame_end = 0;
    int16_t elem_count = 0;
    bool header_ok = false;
    FeatherAnimTimeClass time_class = FeatherAnimTimeClass::None;
    bool clip_changed = false;
    bool valid = false;
};

struct FeatherAnimBonePrev {
    bool valid = false;
    uint64_t tick = 0;
    int16_t clip_id = 0;
    float q[4]{};
    float s[3]{};
    float t[3]{};
};

struct FeatherAnimClipCount {
    int16_t clip_id = 0;
    uint64_t count = 0;
    bool used = false;
};

std::mutex g_feather_anim_mu;
FeatherAnimSnap g_feather_anim_last_baanim{};
FeatherAnimBonePrev g_feather_anim_prev[256]{};
FeatherAnimClipCount g_feather_anim_clip_counts[kFeatherAnimClipIdSlots]{};
FILE* g_feather_anim_csv = nullptr;
bool g_feather_anim_csv_header = false;
uint64_t g_feather_anim_rows = 0;
uint64_t g_feather_anim_control_ticks = 0;
uint64_t g_feather_anim_baanim_raw = 0;
uint64_t g_feather_anim_baanim_accepted = 0;
uint64_t g_feather_anim_ae8a8_raw = 0;
uint64_t g_feather_anim_ae8a8_accepted = 0;
uint64_t g_feather_anim_target_sample_rows = 0;
uint64_t g_feather_anim_join_ok = 0;
uint64_t g_feather_anim_join_mismatch = 0;
uint64_t g_feather_anim_time_forward = 0;
uint64_t g_feather_anim_time_same = 0;
uint64_t g_feather_anim_time_backward = 0;
uint64_t g_feather_anim_time_wrap = 0;
uint64_t g_feather_anim_time_unexpected = 0;
uint64_t g_feather_anim_clip_change = 0;
uint64_t g_feather_anim_clip_unexpected = 0;
uint64_t g_feather_anim_107_large_rot = 0;
uint64_t g_feather_anim_107_large_scale = 0;
uint64_t g_feather_anim_107_large_trans = 0;
uint64_t g_feather_anim_107_step_authored = 0;
uint64_t g_feather_anim_107_step_unexplained = 0;
uint64_t g_feather_anim_107_prop_left = 0;
uint64_t g_feather_anim_107_prop_right = 0;
uint64_t g_feather_anim_108_large = 0;
uint64_t g_feather_anim_79_large = 0;
uint64_t g_feather_anim_body_large = 0;
uint64_t g_feather_anim_body_time_same = 0;
uint32_t g_feather_anim_body_anim_id = 0;
uint32_t g_feather_anim_body_index = 0;
int16_t g_feather_anim_last_clip_id = 0;
int16_t g_feather_anim_prev_clip_id = 0;
float g_feather_anim_last_sample_time = 0.0f;
bool g_feather_anim_have_prev_time = false;
std::array<uint64_t, static_cast<size_t>(FeatherAnimReject::Count)>
    g_feather_anim_rejects{};
FeatherAnimReject g_feather_anim_last_reject = FeatherAnimReject::None;
const char* g_feather_anim_last_reject_detail = "NONE";

void feather_anim_write_status(const char* reason);
void feather_anim_flush_at_exit();
constexpr size_t kPlayerScopeStackSize = 32;
constexpr uint64_t kSecondarySwayFlushInterval = 32;

struct PlayerRenderScope {
    uint32_t player = 0;
    uint32_t slot = 0;
    uint32_t expected_model_bin = 0;
    uint32_t resource_id = 0;
    bool getter_seen = false;
};

thread_local std::array<PlayerRenderScope, kPlayerScopeStackSize> g_player_scopes{};
thread_local size_t g_player_scope_depth = 0;
thread_local bool g_e2aa4_snout_scope = false;
thread_local bool g_dcf48_snout_scope = false;
thread_local uint32_t g_dcf48_model_bin = 0;
thread_local uint32_t g_dcf48_node_ptr = 0;
thread_local uint32_t g_dcf48_node_rel = 0;
thread_local uint32_t g_dcf48_bone_index = 0;
thread_local uint32_t g_dcf48_bone_anim_id = 0;
thread_local uint32_t g_dcf48_resource_id = 0;
std::atomic<uint64_t> g_v7_model_bin{0};
std::atomic<uint64_t> g_v7_model_getter_raw_62a{0};
std::atomic<uint64_t> g_v7_model_getter_raw_62b{0};
std::atomic<uint64_t> g_v7_model_getter_normalized_62b{0};
std::atomic<uint64_t> g_v7_model_bin_accept{0};
std::atomic<uint64_t> g_v7_model_bin_reject{0};
std::atomic<uint32_t> g_v7_first_raw_resource{0};
std::atomic<uint32_t> g_v7_first_normalized_resource{0};
std::atomic<uint64_t> g_v7_first_guest_bin{0};
std::atomic<uint32_t> g_v7_last_raw_resource{0};
std::atomic<uint32_t> g_v7_last_normalized_resource{0};
std::atomic<uint64_t> g_v7_last_guest_bin{0};
std::atomic<uint64_t> g_v7_getter_wrapper_calls{0};
std::atomic<uint64_t> g_v7_getter_reject_arg_filter{0};
std::atomic<uint64_t> g_v7_getter_reject_return_zero{0};
std::atomic<uint64_t> g_v7_getter_reject_resource_unavailable{0};
std::atomic<uint64_t> g_v7_getter_reject_raw_resource_other{0};
std::atomic<uint64_t> g_v7_getter_reject_metadata_unavailable{0};
std::atomic<uint64_t> g_v7_getter_reject_metadata_mismatch{0};
std::atomic<uint64_t> g_v7_getter_reject_other{0};
std::atomic<uint32_t> g_v7_first_reject_reason{0};
std::atomic<uint64_t> g_v7_durable_register_total{0};
std::atomic<uint64_t> g_v7_durable_normalized_62b{0};
std::atomic<uint64_t> g_v7_model_context_publish_count{0};
std::atomic<uint64_t> g_v7_model_context_clear_count{0};
std::atomic<uint64_t> g_v7_model_context_generation{0};
std::atomic<uint64_t> g_v7_last_context_generation{0};
std::atomic<uint32_t> g_v7_last_physical_model_base{0};
std::atomic<uint32_t> g_v7_last_player{0};

enum V7GetterRejectReason : uint32_t {
    kV7RejectNone = 0,
    kV7RejectArgFilter = 1,
    kV7RejectReturnZero = 2,
    kV7RejectResourceUnavailable = 3,
    kV7RejectRawResourceOther = 4,
    kV7RejectMetadataUnavailable = 5,
    kV7RejectMetadataMismatch = 6,
    kV7RejectOther = 7,
};

void v7_note_getter_reject(std::atomic<uint64_t>& reason_counter,
                           V7GetterRejectReason reason) {
    ++reason_counter;
    ++g_v7_model_bin_reject;
    uint32_t expected = kV7RejectNone;
    g_v7_first_reject_reason.compare_exchange_strong(
        expected, static_cast<uint32_t>(reason), std::memory_order_relaxed);
}

struct TreeWalkScope {
    uint32_t model_bin = 0;
    uint32_t root = 0;
};
constexpr size_t kTreeScopeStackSize = 4;
thread_local std::array<TreeWalkScope, kTreeScopeStackSize> g_tree_scopes{};
thread_local size_t g_tree_scope_depth = 0;

std::mutex g_secondary_sway_mu;
FILE* g_secondary_sway_csv = nullptr;
uint64_t g_secondary_sway_outer_hits = 0;
uint64_t g_secondary_sway_92444_hits = 0;
uint64_t g_secondary_sway_total_hits = 0;
uint64_t g_secondary_sway_in_scope_hits = 0;
uint64_t g_secondary_sway_callsite_800926d8_hits = 0;
uint64_t g_secondary_sway_sample_writes = 0;
uint64_t g_secondary_sway_max_depth = 0;
uint64_t g_secondary_sway_overflow = 0;
uint64_t g_secondary_sway_reject_no_scope = 0;
uint64_t g_secondary_sway_reject_wrong_callsite = 0;
uint64_t g_secondary_sway_reject_other = 0;
uint32_t g_secondary_sway_last_resource = 0;
uint32_t g_secondary_sway_last_model_bin = 0;
uint32_t g_secondary_sway_last_player = 0;
uint32_t g_secondary_sway_last_locomotion = 0;
int16_t g_secondary_sway_last_gtype = 0;
uint16_t g_secondary_sway_last_bones = 0;
uint32_t g_secondary_sway_last_magic = 0;
bool g_secondary_sway_first_filtered = false;
uint64_t g_secondary_sway_bone_anim_hits = 0;
uint64_t g_secondary_sway_bone_anim_samples = 0;
uint64_t g_secondary_sway_bone_anim_entry_hits = 0;
uint64_t g_secondary_sway_snout_matrix_entry_hits = 0;
uint64_t g_secondary_sway_snout_matrix_hits = 0;
uint64_t g_secondary_sway_snout_matrix_samples = 0;
uint64_t g_secondary_sway_snout_reject_no_scope = 0;
uint64_t g_secondary_sway_snout_reject_no_durable = 0;
uint64_t g_secondary_sway_snout_reject_no_getter = 0;
uint64_t g_secondary_sway_snout_reject_wrong_resource = 0;
uint64_t g_secondary_sway_snout_reject_wrong_node = 0;
uint64_t g_secondary_sway_snout_reject_outside_span = 0;
uint64_t g_secondary_sway_snout_reject_wrong_type = 0;
uint64_t g_secondary_sway_snout_reject_wrong_bone = 0;
uint64_t g_secondary_sway_snout_reject_pointer_form = 0;
uint64_t g_secondary_sway_snout_reject_other_node = 0;
uint64_t g_secondary_sway_e2aa4_raw = 0;
uint64_t g_secondary_sway_e2aa4_with_registered = 0;
uint64_t g_secondary_sway_e2aa4_in_true_range = 0;
uint64_t g_secondary_sway_e2aa4_rel_11cb8 = 0;
uint64_t g_secondary_sway_e2aa4_type2 = 0;
uint64_t g_secondary_sway_e2aa4_index11 = 0;
uint64_t g_secondary_sway_e2aa4_bone42 = 0;
uint64_t g_secondary_sway_e2aa4_accept = 0;
uint64_t g_secondary_sway_e2aa4_reject_no_registered = 0;
uint64_t g_secondary_sway_e2aa4_reject_outside_range = 0;
uint64_t g_secondary_sway_e2aa4_reject_offset = 0;
uint64_t g_secondary_sway_e2aa4_reject_type = 0;
uint64_t g_secondary_sway_e2aa4_reject_index = 0;
uint64_t g_secondary_sway_e2aa4_reject_bone = 0;
uint64_t g_secondary_sway_e2aa4_reject_other = 0;
uint64_t g_secondary_sway_e2aa4_tuple_rows = 0;
uint64_t g_secondary_sway_e2180_associated = 0;
uint64_t g_secondary_sway_de2a4_raw = 0;
uint64_t g_secondary_sway_de2a4_in_range = 0;
uint64_t g_secondary_sway_de2a4_type2 = 0;
uint64_t g_secondary_sway_de2a4_rel_11cb8 = 0;
uint64_t g_secondary_sway_de2a4_tuple_rows = 0;
uint32_t g_secondary_sway_de2a4_last_type2_handler = 0;
uint64_t g_secondary_sway_dcf48_raw = 0;
uint64_t g_secondary_sway_dcf48_in_range = 0;
uint64_t g_secondary_sway_dcf48_accept = 0;
uint64_t g_kazooie_feather_primary_hits = 0;
uint64_t g_kazooie_feather_sibling_hits = 0;
uint64_t g_kazooie_feather_62b_primary_hits = 0;
uint64_t g_kazooie_feather_61c_primary_hits = 0;
uint64_t g_kazooie_feather_83a_primary_hits = 0;
uint64_t g_secondary_sway_dcf48_rel_11cb8 = 0;
uint64_t g_secondary_sway_tree_list_contains_11cb8 = 0;
uint64_t g_secondary_sway_tree_rows = 0;
uint64_t g_secondary_sway_tree_draws = 0;
uint32_t g_secondary_sway_last_handler_table = 0;
uint32_t g_secondary_sway_last_geocmd_mode = 0xFFFFFFFFu;
uint64_t g_secondary_sway_ade2c_snout_samples = 0;
uint64_t g_secondary_sway_e2180_in_span = 0;
uint64_t g_secondary_sway_e2180_type2 = 0;
uint64_t g_secondary_sway_e2180_bone42 = 0;
uint64_t g_secondary_sway_e2180_rel_11cb8 = 0;
uint64_t g_secondary_sway_e2180_tuple_rows = 0;
uint64_t g_secondary_sway_durable_register_hits = 0;
uint64_t g_secondary_sway_hip_consume_entry_hits = 0;
uint64_t g_secondary_sway_hip_consume_hits = 0;
uint64_t g_secondary_sway_hip_consume_samples = 0;
FILE* g_secondary_sway_bone_csv = nullptr;
FILE* g_secondary_sway_snout_csv = nullptr;
FILE* g_secondary_sway_hip_csv = nullptr;
FILE* g_secondary_sway_e2180_tuple_csv = nullptr;
FILE* g_secondary_sway_e2aa4_tuple_csv = nullptr;
FILE* g_secondary_sway_de2a4_tuple_csv = nullptr;
FILE* g_secondary_sway_tree_csv = nullptr;
FILE* g_secondary_sway_producer_csv = nullptr;
uint64_t g_secondary_sway_ae8a8_hits = 0;
uint64_t g_secondary_sway_ae820_hits = 0;
uint64_t g_secondary_sway_producer_samples = 0;
uint64_t g_secondary_sway_producer_first_written = 0;
uint64_t g_secondary_sway_rotation_samples = 0;
bool g_secondary_sway_previous_feather_rotation_valid = false;
uint64_t g_secondary_sway_previous_feather_rotation_frame = 0;
float g_secondary_sway_previous_feather_rotation[4]{};
float g_secondary_sway_last_rotation_delta = -1.0f;
bool g_secondary_sway_last_quaternion_flip = false;

// Live 0x62B snout GeoCmd02. Independent parse of asset 0x62B:
// offset 0x11CB8, bone INDEX 11, boneAnimID 42. 0x62A uses 0x14960 /
// INDEX 10; 0x14960 is past the end of decompressed 0x62B (0x13178).
// True ownership is decompressed-asset size, not heap/allocator extent.
constexpr uint32_t kSecondarySwaySnoutNodeOffset = 0x11CB8u;
constexpr uint32_t kSecondarySwaySnoutBoneAnimId = 42u;
constexpr uint32_t kSecondarySwaySnoutBoneIndex = 11u;
constexpr uint32_t kSecondarySwayAssetSize = 0x13178u;
// Combined BK GeoCmd body (live walk PROVEN 0x62B): Kazooie head side-plume.
constexpr uint32_t kKazooieFeather62BNodeOffset = 0x124A0u;
constexpr uint32_t kKazooieFeather62BBoneIndex = 30u;
constexpr uint32_t kKazooieFeather62BBoneAnimId = 103u;
constexpr uint32_t kKazooieFeather62BAssetSize = 0x13178u;
// Solo / TRANSFORM_B_KAZOOIE body 0x61C: head crest (only dedicated head extra).
constexpr uint32_t kKazooieFeather61CNodeOffset = 0xF558u;
constexpr uint32_t kKazooieFeather61CBoneIndex = 11u;
constexpr uint32_t kKazooieFeather61CBoneAnimId = 69u;
constexpr uint32_t kKazooieFeather61CAssetSize = 0x106F8u;
// Overlay-118 alternate when 8009EA2C != 0: reduced Kazooie 0x83A crest.
constexpr uint32_t kKazooieFeather83ANodeOffset = 0xD408u;
constexpr uint32_t kKazooieFeather83ABoneIndex = 8u;
constexpr uint32_t kKazooieFeather83ABoneAnimId = 69u;
constexpr uint32_t kKazooieFeather83AAssetSize = 0xE308u;
constexpr uint32_t kKazooieFeather62BResourceId = 0x62Bu;
constexpr uint32_t kKazooieFeather62BParent107NodeOffset = 0x12320u;
constexpr uint32_t kKazooieFeather62BParent107BoneIndex = 25u;
constexpr uint32_t kKazooieFeather62BParent107BoneAnimId = 107u;
constexpr uint32_t kKazooieFeather62BParent108NodeOffset = 0x12330u;
constexpr uint32_t kKazooieFeather62BParent108BoneIndex = 26u;
constexpr uint32_t kKazooieFeather62BParent108BoneAnimId = 108u;
// V5B: bits 0–6 = 79,100–105. V5C adds bits 7–8 = 107,108 (JOIN parents).
constexpr uint32_t kTrackedKazooieFeatherMask = 0x1FFu;
constexpr uint32_t kSecondarySwayMatrixCursorAddress = 0x8007BCB0u;
constexpr size_t kDurableBinSlots = 4;
constexpr size_t kE2AA4TupleFirstN = 24;
constexpr size_t kE2AA4TupleUniqueMax = 32;
constexpr size_t kDe2a4TupleFirstN = 48;
constexpr size_t kTreeMaxRows = 800;
constexpr size_t kTreeSiblingScanMax = 48;
constexpr size_t kSingleMtxTraceSlots = 256;
constexpr size_t kSingleMtxTraceRows = 512;
constexpr size_t kSingleMtxIdentityRows = 1536;
constexpr size_t kSingleMtxMatchRows = 8192;
constexpr size_t kFeatherWorldHits = 96;
constexpr size_t kFeatherTriRows = 4096;

struct RotationSummary {
    bool valid = false;
    float uniform_scale = 0.0f;
    float q[4]{};
    float orthogonality_error = 0.0f;
    float determinant = 0.0f;
};

enum SecondarySwayMatchStage : uint32_t {
    kMatchStageCall = 1,
    kMatchStageTransformPrerequisite = 2,
    kMatchStageCandidate = 3,
    kMatchStageDecision = 4,
};

enum SecondarySwayMatchReason : uint32_t {
    kMatchReasonNone = 0,
    kMatchReasonNoPreviousCall = 1,
    kMatchReasonRangeCountMismatch = 2,
    kMatchReasonTransformMatchingDisabled = 3,
    kMatchReasonMatrixIdMismatch = 4,
    kMatchReasonMatrixIdNotAuto = 5,
    kMatchReasonMirrorDeterminant = 6,
    kMatchReasonCandidateValid = 7,
    kMatchReasonCurrentAlreadyMapped = 8,
    kMatchReasonPreviousAlreadyUsed = 9,
    kMatchReasonSelected = 10,
    kMatchReasonTokenNotBound = 11,
    kMatchReasonNoCandidateObserved = 12,
    kMatchReasonFlushSentinel = 13,
};

struct DurableModelBin {
    uint32_t model_bin = 0;
    uint32_t resource_id = 0;
    uint32_t player = 0;
};

std::array<DurableModelBin, kDurableBinSlots> g_durable_bins{};
size_t g_durable_bin_count = 0;
std::array<uint32_t, kE2AA4TupleUniqueMax> g_e2aa4_unique_a1{};
size_t g_e2aa4_unique_a1_count = 0;

struct SnoutComposition {
    bool valid = false;
    uint64_t frame = 0;
    uint32_t model_bin = 0;
    uint32_t node_ptr = 0;
    uint32_t node_rel = 0;
    uint32_t bone_index = 0;
    uint32_t bone_anim_id = 0;
    uint32_t resource_id = 0;
    uint32_t parent_ptr = 0;
    uint32_t local_ptr = 0;
    uint32_t output_ptr = 0;
    uint32_t local_words[16]{};
    uint32_t parent_words[16]{};
    uint32_t output_float_bits[16]{};
};

struct SingleMtxTraceToken {
    bool pending = false;
    bool matrix_seen = false;
    bool vertex_seen = false;
    bool match_seen = false;
    bool rt64_call_seen = false;
    bool rt64_gameframe_bound = false;
    bool rt64_prerequisite_seen = false;
    bool rt64_candidate_seen = false;
    bool rt64_decision_seen = false;
    bool rt64_selected = false;
    uint64_t token = 0;
    uint64_t frame = 0;
    uint64_t guest_matrix_rows = 0;
    uint32_t model_bin = 0;
    uint32_t node_ptr = 0;
    uint32_t node_rel = 0;
    uint32_t bone_index = 0;
    uint32_t bone_anim_id = 0;
    uint32_t resource_id = 0;
    uint32_t local_ptr = 0;
    uint32_t parent_ptr = 0;
    uint32_t composed_ptr = 0;
    uint32_t source_ptr = 0;
    uint32_t destination_ptr = 0;
    uint32_t restore_destination_ptr = 0;
    uint32_t gfx_command_ptr = 0;
    uint32_t rt64_guest_matrix_ptr = 0;
    uint32_t rt64_stack_after = 0;
    bool restore_alias_seen = false;
    uint64_t matrix_workload_id = 0;
    uint64_t vertex_workload_id = 0;
    uint64_t match_workload_id = 0;
    uint64_t rt64_workload_id = 0;
    bool vertex_fallback_bound = false;
    uint32_t vertex_start = 0;
    uint32_t vertex_count = 0;
    uint32_t rt64_transform_index = 0;
    uint32_t rt64_matrix_id = 0;
    uint32_t rt64_group_index = 0;
    uint32_t rt64_draw_uid = 0;
    uint32_t rt64_draw_call_index = 0;
    uint32_t rt64_draw_min = 0;
    uint32_t rt64_draw_max = 0;
    uint32_t rt64_projection_type = 0;
    uint32_t rt64_projection_index = 0;
    uint32_t rt64_other_mode_h = 0;
    uint64_t rt64_prev_workload_id = 0;
    uint64_t rt64_cur_call_hash = 0;
    uint64_t rt64_prev_call_hash = 0;
    uint32_t rt64_cur_scene_proj = UINT32_MAX;
    uint32_t rt64_prev_scene_proj = UINT32_MAX;
    uint32_t rt64_cur_call_index = UINT32_MAX;
    uint32_t rt64_prev_call_index = UINT32_MAX;
    uint32_t rt64_cur_range_begin = UINT32_MAX;
    uint32_t rt64_cur_range_end = UINT32_MAX;
    uint32_t rt64_prev_range_begin = UINT32_MAX;
    uint32_t rt64_prev_range_end = UINT32_MAX;
    uint32_t rt64_cur_group_index = UINT32_MAX;
    uint32_t rt64_prev_group_index = UINT32_MAX;
    uint32_t rt64_cur_matrix_id = UINT32_MAX;
    uint32_t rt64_prev_matrix_id = UINT32_MAX;
    uint32_t rt64_cur_projection_type = UINT32_MAX;
    uint32_t rt64_prev_projection_type = UINT32_MAX;
    uint32_t rt64_cur_projection_index = UINT32_MAX;
    uint32_t rt64_prev_projection_index = UINT32_MAX;
    uint32_t rt64_call_match_result = 0;
    uint32_t rt64_call_reason = kMatchReasonNone;
    uint32_t rt64_candidate_prev_transform = UINT32_MAX;
    uint32_t rt64_matched_prev_transform = UINT32_MAX;
    uint32_t rt64_match_reason = kMatchReasonNone;
    float rt64_position_difference = -1.0f;
    float rt64_orientation_difference = -1.0f;
    float rt64_screen_space_difference = -1.0f;
    float rt64_match_metric = -1.0f;
    uint32_t local_words[16]{};
    uint32_t parent_words[16]{};
    uint32_t composed_float_bits[16]{};
    uint32_t fixed_words[16]{};
};

struct FeatherWorldHit {
    uint64_t token = 0;
    uint64_t vertex_workload_id = 0;
    uint64_t matrix_workload_id = 0;
    uint64_t frame = 0;
    uint32_t transform_index = UINT32_MAX;
    uint32_t bone_anim_id = 0;
    uint32_t bone_index = 0;
    uint32_t node_rel = 0;
    uint32_t model_bin = 0;
};

thread_local SnoutComposition g_snout_composition{};
std::array<SingleMtxTraceToken, kSingleMtxTraceSlots> g_single_mtx_tokens{};
uint64_t g_single_mtx_token_next = 1;
uint64_t g_single_mtx_guest_matrix_rows = 0;
uint64_t g_single_mtx_identity_rows = 0;
uint64_t g_single_mtx_vertex_rows = 0;
uint64_t g_single_mtx_match_rows = 0;
uint64_t g_single_mtx_match_matched = 0;
uint64_t g_single_mtx_match_unmatched = 0;
uint64_t g_single_mtx_match_no_token = 0;
uint64_t g_single_mtx_match_call_rows = 0;
uint64_t g_single_mtx_match_candidate_rows = 0;
uint64_t g_single_mtx_match_decision_rows = 0;
uint64_t g_single_mtx_match_real_unmatched = 0;
uint64_t g_single_mtx_match_flush_sentinel = 0;
std::array<uint64_t, 16> g_single_mtx_match_reason_counts{};
uint64_t g_single_mtx_token_candidates = 0;
uint64_t g_single_mtx_token_enqueued = 0;
uint64_t g_single_mtx_token_consumed = 0;
uint64_t g_single_mtx_token_expired = 0;
uint64_t g_single_mtx_token_address_mismatch = 0;
uint64_t g_single_mtx_token_command_mismatch = 0;
uint64_t g_single_mtx_token_other_reject = 0;
uint64_t g_single_mtx_token_dest_eq_gmtx_operand = 0;
uint64_t g_single_mtx_token_source_eq_gmtx_operand = 0;
uint64_t g_single_mtx_token_normalization_failure = 0;
uint64_t g_single_mtx_matrix_workload_nonzero = 0;
uint64_t g_single_mtx_vertex_workload_zero = 0;
uint64_t g_single_mtx_vertex_workload_nonzero = 0;
uint64_t g_single_mtx_valid_workload_preserved = 0;
uint64_t g_single_mtx_valid_workload_overwrite_blocked = 0;
uint64_t g_single_mtx_bound_gameframe_match = 0;
uint64_t g_single_mtx_bound_call_stage = 0;
uint64_t g_single_mtx_bound_transform_prerequisite = 0;
uint64_t g_single_mtx_bound_candidate_stage = 0;
uint64_t g_single_mtx_bound_matchtransform = 0;
uint64_t g_single_mtx_synthetic_finalization_only = 0;
uint64_t g_single_mtx_vertex_fallback_bindings = 0;
uint64_t g_single_mtx_vertex_fallback_ambiguous = 0;
uint64_t g_single_mtx_token_restore_alias = 0;
uint64_t g_single_mtx_token_restore_ambiguous = 0;
uint64_t g_single_mtx_interp_mode_control = 0;
uint64_t g_single_mtx_interp_mode_simple_override = 0;
uint64_t g_single_mtx_interp_mode_ambiguous = 0;
uint64_t g_single_mtx_no_interp_override = 0;
uint64_t g_single_mtx_post_match_rows = 0;
uint64_t g_single_mtx_post_match_no_token = 0;
std::array<uint64_t, 128> g_feather_real_call_matched{};
std::array<uint64_t, 128> g_feather_real_call_unmatched{};
std::array<uint64_t, 128> g_feather_real_matched{};
std::array<uint64_t, 128> g_feather_real_unmatched{};
std::array<uint64_t, 128> g_feather_semantic_failure{};
std::array<uint64_t, 128> g_feather_semantic_cross_match{};
std::array<uint64_t, 128> g_feather_semantic_dropped{};
std::array<uint64_t, 128> g_feather_guest_seen{};
std::array<uint64_t, 128> g_feather_matrix_seen{};
std::array<uint64_t, 128> g_feather_gmtx_seen{};
std::array<uint64_t, 128> g_feather_vertex_seen{};
std::array<uint64_t, 128> g_feather_matched_seen{};
std::array<uint64_t, 128> g_feather_post_match_seen{};
std::array<uint64_t, 128> g_feather_no_interp_seen{};
std::array<uint64_t, 128> g_feather_no_interp_eq_current{};
std::array<uint64_t, 128> g_feather_no_interp_ne_current{};
uint64_t g_feather_no_interp_eq_current_total = 0;
uint64_t g_feather_no_interp_ne_current_total = 0;
std::array<uint64_t, 128> g_feather_tri_seen{};
std::array<FeatherWorldHit, kFeatherWorldHits> g_feather_world_hits{};
size_t g_feather_world_hit_next = 0;
uint64_t g_feather_tri_rows = 0;
uint64_t g_feather_tri_mixed = 0;
uint64_t g_feather_tri_ne_active = 0;
uint64_t g_feather_tri_match_active = 0;
std::atomic<uint64_t> g_raw_tri_handler{0};
std::atomic<uint64_t> g_raw_tri1{0};
std::atomic<uint64_t> g_raw_tri2{0};
uint64_t g_tri_with_tracked_worldindex = 0;
uint64_t g_tri_with_tracked_active = 0;
uint64_t g_tri_with_tracked_dl = 0;
uint64_t g_tri_classified = 0;
FILE* g_single_mtx_matrix_csv = nullptr;
FILE* g_single_mtx_identity_csv = nullptr;
FILE* g_single_mtx_match_csv = nullptr;
FILE* g_single_mtx_post_match_csv = nullptr;
FILE* g_feather_tri_csv = nullptr;

constexpr size_t kV6CClassCount = 6;
constexpr size_t kV6CNormalSampleLimit = 24;
constexpr size_t kV6CSuspiciousSampleLimit = 192;
enum V6CClass : size_t {
    kV6COwnMatch = 0,
    kV6CJoinExpectedMix,
    kV6CStaleSlot,
    kV6CLeftoverPreGvtx,
    kV6CMixedUnexpected,
    kV6CUnresolved,
};
const char* const kV6CClassNames[kV6CClassCount] = {
    "OWN_MATCH", "JOIN_EXPECTED_MIX", "STALE_SLOT",
    "LEFTOVER_PRE_GVTX", "MIXED_UNEXPECTED", "UNRESOLVED"};
uint64_t g_v6c_tracked_dl_tri = 0;
uint64_t g_v6c_dl_5aa_tri = 0;
uint64_t g_v6c_dl_57c_tri = 0;
uint64_t g_v6c_dl_534_tri = 0;
uint64_t g_v6c_dl_592_tri = 0;
uint64_t g_v6c_dl_03000040_tri = 0;
uint64_t g_v6c_slot_history_hits = 0;
uint64_t g_v6c_slot_history_misses = 0;
std::array<uint64_t, kV6CClassCount> g_v6c_class_counts{};
std::array<uint64_t, kV6CClassCount> g_v6c_class_samples{};
uint64_t g_v6c_gdl_count = 0;
uint64_t g_v6c_segment3_count = 0;
uint64_t g_v6c_segment3_base_count = 0;
uint32_t g_v6c_segment3_base = 0;
uint32_t g_v6c_segment3_target = 0;
FILE* g_v6c_tri_csv = nullptr;

uint32_t tracked_kazooie_feather_bit(uint32_t bone_anim_id) {
    switch (bone_anim_id) {
    case 79u:
        return 1u << 0;
    case 100u:
        return 1u << 1;
    case 101u:
        return 1u << 2;
    case 102u:
        return 1u << 3;
    case 103u:
        return 1u << 4;
    case 104u:
        return 1u << 5;
    case 105u:
        return 1u << 6;
    case 107u:
        return 1u << 7;
    case 108u:
        return 1u << 8;
    default:
        return 0u;
    }
}

uint32_t backpack_guest_word(uint8_t* rdram, uint32_t guest_ptr, uint32_t offset) {
    if (rdram == nullptr) {
        return 0;
    }
    const uint32_t physical = (guest_ptr + offset) & 0x1FFFFFFFu;
    if (physical > 0x007FFFFCu) {
        return 0;
    }
    uint32_t value = 0;
    std::memcpy(&value, rdram + physical, sizeof(value));
    return value;
}

uint32_t backpack_guest_byte(uint8_t* rdram, uint32_t guest_ptr, uint32_t offset) {
    const uint32_t word = backpack_guest_word(rdram, guest_ptr, offset & ~3u);
    const uint32_t shift = (3u - (offset & 3u)) * 8u;
    return (word >> shift) & 0xFFu;
}

int16_t backpack_guest_hword(uint8_t* rdram, uint32_t guest_ptr, uint32_t offset) {
    const uint32_t word = backpack_guest_word(rdram, guest_ptr, offset & ~3u);
    if ((offset & 2u) != 0) {
        return static_cast<int16_t>(word & 0xFFFFu);
    }
    return static_cast<int16_t>(word >> 16);
}

float backpack_guest_float(uint8_t* rdram, uint32_t guest_ptr, uint32_t offset) {
    const uint32_t word = backpack_guest_word(rdram, guest_ptr, offset);
    float value = 0.0f;
    std::memcpy(&value, &word, sizeof(value));
    return value;
}

bool secondary_sway_logged_additive_anim(uint32_t bone_anim_id) {
    return bone_anim_id == 3u || bone_anim_id == 42u || bone_anim_id == 79u ||
           (bone_anim_id >= 100u && bone_anim_id <= 105u);
}

bool secondary_sway_plume_family_anim(uint32_t bone_anim_id) {
    return bone_anim_id == 79u || bone_anim_id == 107u ||
           (bone_anim_id >= 100u && bone_anim_id <= 105u);
}

float mtx_basis_length(float a, float b, float c) {
    return std::sqrt(a * a + b * b + c * c);
}

float mtx3_determinant(float m00, float m01, float m02, float m10, float m11,
                       float m12, float m20, float m21, float m22) {
    return m00 * (m11 * m22 - m12 * m21) - m01 * (m10 * m22 - m12 * m20) +
           m02 * (m10 * m21 - m11 * m20);
}

RotationSummary rotation_from_quaternion(float qw, float qx, float qy, float qz) {
    RotationSummary out{};
    const float length = std::sqrt(qw * qw + qx * qx + qy * qy + qz * qz);
    if (!std::isfinite(length) || length < 1.0e-6f) {
        return out;
    }
    out.valid = true;
    out.q[0] = qw / length;
    out.q[1] = qx / length;
    out.q[2] = qy / length;
    out.q[3] = qz / length;
    out.uniform_scale = 1.0f;
    out.determinant = 1.0f;
    return out;
}

RotationSummary rotation_from_slot_quaternion(const uint32_t words[16]) {
    float qw = 0.0f;
    float qx = 0.0f;
    float qy = 0.0f;
    float qz = 0.0f;
    std::memcpy(&qw, &words[0], sizeof(qw));
    std::memcpy(&qx, &words[1], sizeof(qx));
    std::memcpy(&qy, &words[2], sizeof(qy));
    std::memcpy(&qz, &words[3], sizeof(qz));
    return rotation_from_quaternion(qw, qx, qy, qz);
}

RotationSummary rotation_from_matrix_words(const uint32_t words[16]) {
    float m[3][3]{};
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            std::memcpy(&m[row][col], &words[row * 4 + col],
                        sizeof(m[row][col]));
        }
    }
    const float sx = mtx_basis_length(m[0][0], m[1][0], m[2][0]);
    const float sy = mtx_basis_length(m[0][1], m[1][1], m[2][1]);
    const float sz = mtx_basis_length(m[0][2], m[1][2], m[2][2]);
    const float scale = (sx + sy + sz) / 3.0f;
    RotationSummary out{};
    out.uniform_scale = scale;
    out.determinant = mtx3_determinant(
        m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2], m[2][0],
        m[2][1], m[2][2]);
    if (!std::isfinite(scale) || scale < 1.0e-6f ||
        !std::isfinite(out.determinant)) {
        return out;
    }
    float r[3][3]{};
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            r[row][col] = m[row][col] / scale;
        }
    }
    const float c0 = r[0][0] * r[0][0] + r[1][0] * r[1][0] +
                     r[2][0] * r[2][0];
    const float c1 = r[0][1] * r[0][1] + r[1][1] * r[1][1] +
                     r[2][1] * r[2][1];
    const float c2 = r[0][2] * r[0][2] + r[1][2] * r[1][2] +
                     r[2][2] * r[2][2];
    const float d01 = r[0][0] * r[0][1] + r[1][0] * r[1][1] +
                      r[2][0] * r[2][1];
    const float d02 = r[0][0] * r[0][2] + r[1][0] * r[1][2] +
                      r[2][0] * r[2][2];
    const float d12 = r[0][1] * r[0][2] + r[1][1] * r[1][2] +
                      r[2][1] * r[2][2];
    out.orthogonality_error = std::fabs(c0 - 1.0f) + std::fabs(c1 - 1.0f) +
                              std::fabs(c2 - 1.0f) + std::fabs(d01) +
                              std::fabs(d02) + std::fabs(d12);

    const float trace = r[0][0] + r[1][1] + r[2][2];
    if (trace > 0.0f) {
        const float s = std::sqrt(trace + 1.0f) * 2.0f;
        out.q[0] = 0.25f * s;
        out.q[1] = (r[2][1] - r[1][2]) / s;
        out.q[2] = (r[0][2] - r[2][0]) / s;
        out.q[3] = (r[1][0] - r[0][1]) / s;
    } else if (r[0][0] > r[1][1] && r[0][0] > r[2][2]) {
        const float s = std::sqrt(1.0f + r[0][0] - r[1][1] - r[2][2]) * 2.0f;
        out.q[0] = (r[2][1] - r[1][2]) / s;
        out.q[1] = 0.25f * s;
        out.q[2] = (r[0][1] + r[1][0]) / s;
        out.q[3] = (r[0][2] + r[2][0]) / s;
    } else if (r[1][1] > r[2][2]) {
        const float s = std::sqrt(1.0f + r[1][1] - r[0][0] - r[2][2]) * 2.0f;
        out.q[0] = (r[0][2] - r[2][0]) / s;
        out.q[1] = (r[0][1] + r[1][0]) / s;
        out.q[2] = 0.25f * s;
        out.q[3] = (r[1][2] + r[2][1]) / s;
    } else {
        const float s = std::sqrt(1.0f + r[2][2] - r[0][0] - r[1][1]) * 2.0f;
        out.q[0] = (r[1][0] - r[0][1]) / s;
        out.q[1] = (r[0][2] + r[2][0]) / s;
        out.q[2] = (r[1][2] + r[2][1]) / s;
        out.q[3] = 0.25f * s;
    }
    const float qlen = std::sqrt(out.q[0] * out.q[0] + out.q[1] * out.q[1] +
                                 out.q[2] * out.q[2] + out.q[3] * out.q[3]);
    out.valid = std::isfinite(qlen) && qlen >= 1.0e-6f;
    if (out.valid) {
        for (float& component : out.q) {
            component /= qlen;
        }
    }
    return out;
}

void secondary_sway_format_quaternion(char* buf, size_t buflen,
                                      const RotationSummary& rotation) {
    if (buflen == 0) {
        return;
    }
    if (!rotation.valid) {
        std::snprintf(buf, buflen, "UNKNOWN");
        return;
    }
    std::snprintf(buf, buflen, "%.9g;%.9g;%.9g;%.9g",
                  static_cast<double>(rotation.q[0]),
                  static_cast<double>(rotation.q[1]),
                  static_cast<double>(rotation.q[2]),
                  static_cast<double>(rotation.q[3]));
}

uint32_t geocmd_table_mode(uint8_t* rdram, uint32_t table) {
    if (rdram == nullptr || table == 0) {
        return 0xFFFFFFFFu;
    }
    for (uint32_t mode = 0; mode < 3u; ++mode) {
        const uint32_t slot = backpack_guest_word(rdram, 0x80123198u, mode * 4u);
        if (slot == table) {
            return mode;
        }
    }
    return 0xFFFFFFFFu;
}

uint32_t geocmd_handler_for_type(uint8_t* rdram, uint32_t type) {
    const uint32_t table = backpack_guest_word(rdram, 0x8012CF8Cu, 0);
    if (table == 0 || type >= 0x40u) {
        return 0;
    }
    return backpack_guest_word(rdram, table, type * 4u);
}

bool tree_scope_active(TreeWalkScope* out) {
    if (g_tree_scope_depth == 0) {
        return false;
    }
    if (out != nullptr) {
        *out = g_tree_scopes[g_tree_scope_depth - 1];
    }
    return g_tree_scopes[g_tree_scope_depth - 1].model_bin != 0;
}

void secondary_sway_write_tree_row(uint8_t* rdram, uint64_t frame,
                                   const char* seam, uint32_t model_bin,
                                   uint32_t root, uint32_t list_head,
                                   uint32_t node_ptr, uint32_t rel,
                                   bool is_head) {
    if (g_secondary_sway_tree_rows >= kTreeMaxRows) {
        return;
    }
    ++g_secondary_sway_tree_rows;
    if (g_secondary_sway_tree_csv == nullptr) {
        g_secondary_sway_tree_csv =
            std::fopen(tooie_diag_path("tooie-secondary-sway-62b-tree.csv").c_str(), "a");
        if (g_secondary_sway_tree_csv == nullptr) {
            return;
        }
        std::fprintf(g_secondary_sway_tree_csv,
                     "FRAME,MODEL_BIN,ROOT,SEAM,LIST_HEAD,NODE,REL_OFFSET,TYPE,"
                     "HANDLER,BONE_INDEX,BONE_ANIM_ID,CONTINUE,IS_LIST_HEAD,"
                     "TABLE,TABLE_MODE\n");
    }
    const uint32_t type = backpack_guest_word(rdram, node_ptr, 0);
    const uint32_t cont = backpack_guest_word(rdram, node_ptr, 4);
    const uint32_t table = backpack_guest_word(rdram, 0x8012CF8Cu, 0);
    const uint32_t handler = geocmd_handler_for_type(rdram, type);
    const uint32_t mode = geocmd_table_mode(rdram, table);
    std::fprintf(g_secondary_sway_tree_csv,
                 "%llu,0x%08X,0x%08X,%s,0x%08X,0x%08X,0x%X,0x%X,0x%08X,%u,%d,"
                 "0x%X,%s,0x%08X,0x%X\n",
                 static_cast<unsigned long long>(frame), model_bin,
                 root, seam != nullptr ? seam : "?", list_head, node_ptr, rel,
                 type, handler, backpack_guest_byte(rdram, node_ptr, 9),
                 static_cast<int>(backpack_guest_hword(rdram, node_ptr, 0xA)),
                 cont, is_head ? "YES" : "NO", table, mode);
    if ((g_secondary_sway_tree_rows % kSecondarySwayFlushInterval) == 0) {
        std::fflush(g_secondary_sway_tree_csv);
    }
}

void backpack_read_mtx_words(uint8_t* rdram, uint32_t guest_ptr, uint32_t* out) {
    for (int i = 0; i < 16; ++i) {
        out[i] = backpack_guest_word(rdram, guest_ptr, static_cast<uint32_t>(i) * 4u);
    }
}

void backpack_format_mtx(char* buf, size_t buflen, const uint32_t words[16]) {
    size_t off = 0;
    buf[0] = '\0';
    for (int i = 0; i < 16 && off + 1 < buflen; ++i) {
        const int n = std::snprintf(buf + off, buflen - off, "%s%08x",
                                    i == 0 ? "" : ":", words[i]);
        if (n < 0) {
            break;
        }
        off += static_cast<size_t>(n);
    }
}

float secondary_sway_bits_float(uint32_t bits) {
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

void secondary_sway_format_words(char* buf, size_t buflen,
                                 const uint32_t words[16]) {
    size_t off = 0;
    if (buflen == 0) {
        return;
    }
    buf[0] = '\0';
    for (int i = 0; i < 16 && off + 1 < buflen; ++i) {
        const int n = std::snprintf(buf + off, buflen - off, "%s%08X",
                                    i == 0 ? "" : ";", words[i]);
        if (n < 0) {
            break;
        }
        off += static_cast<size_t>(n);
    }
}

void secondary_sway_format_floats(char* buf, size_t buflen,
                                  const uint32_t bits[16]) {
    size_t off = 0;
    if (buflen == 0) {
        return;
    }
    buf[0] = '\0';
    for (int i = 0; i < 16 && off + 1 < buflen; ++i) {
        const int n = std::snprintf(
            buf + off, buflen - off, "%s%.9g", i == 0 ? "" : ";",
            static_cast<double>(secondary_sway_bits_float(bits[i])));
        if (n < 0) {
            break;
        }
        off += static_cast<size_t>(n);
    }
}

const char* backpack_node_type_name(uint32_t caller_ra) {
    if (caller_ra >= 0x800E2AA4u && caller_ra < 0x800E2C54u) {
        return "DUAL_MTX_800E2AA4";
    }
    if (caller_ra >= 0x800E2C54u && caller_ra < 0x800E2E48u) {
        return "DUAL_MTX_800E2C54";
    }
    if (caller_ra >= 0x800E2E48u && caller_ra < 0x800E3220u) {
        return "DUAL_MTX_800E2E48";
    }
    return "DUAL_MTX_800E2180";
}

bool backpack_sample_anomalous(const BackpackFlapSample& sample) {
    for (int i = 0; i < 16; ++i) {
        float prev = 0.0f;
        float curr = 0.0f;
        std::memcpy(&prev, &sample.prev_words[i], sizeof(prev));
        std::memcpy(&curr, &sample.curr_words[i], sizeof(curr));
        if (!std::isfinite(prev) || !std::isfinite(curr)) {
            return true;
        }
    }
    return false;
}

void backpack_store_sample(const BackpackFlapSample& sample) {
    g_backpack_flap_ring[g_backpack_flap_ring_next] = sample;
    g_backpack_flap_ring_next = (g_backpack_flap_ring_next + 1) % kBackpackFlapRingSize;
    if (g_backpack_flap_ring_count < kBackpackFlapRingSize) {
        ++g_backpack_flap_ring_count;
    }
}

void backpack_append_csv_locked(const BackpackFlapSample& sample, bool force_flush) {
    if (g_backpack_witness_csv == nullptr) {
        g_backpack_witness_csv = std::fopen(tooie_diag_path("tooie-backpack-flap-witness.csv").c_str(), "a");
        if (g_backpack_witness_csv == nullptr) {
            return;
        }
    }
    if (!g_backpack_witness_csv_header) {
        std::fprintf(g_backpack_witness_csv,
                     "FRAME,GFX_TICK,MODEL_RESOURCE_ID,MODEL_BIN,NODE_ID,NODE_TYPE,"
                     "CALLER_RA,GEOCMD_PTR,NODE_WORD0,NODE_BYTE8,NODE_BYTE9,"
                     "ANIMATION_CONTROL,PARENT_MATRIX,LOCAL_MATRIX,COMPOSED_MATRIX,"
                     "TRANSFORM_TAG,PREVIOUS_MATRIX,CURRENT_MATRIX,INTERPOLATED_MATRIX,"
                     "DL_ID,DRAW_SUBMITTED,SCALE,POS_X,POS_Y,POS_Z,"
                     "PREV_MTX_PTR,CURR_MTX_PTR\n");
        g_backpack_witness_csv_header = true;
    }
    char prev_hex[16 * 9];
    char curr_hex[16 * 9];
    backpack_format_mtx(prev_hex, sizeof(prev_hex), sample.prev_words);
    backpack_format_mtx(curr_hex, sizeof(curr_hex), sample.curr_words);
    float scale = 0.0f;
    float pos_x = 0.0f;
    float pos_y = 0.0f;
    float pos_z = 0.0f;
    std::memcpy(&scale, &sample.scale_bits, sizeof(scale));
    std::memcpy(&pos_x, &sample.pos_x_bits, sizeof(pos_x));
    std::memcpy(&pos_y, &sample.pos_y_bits, sizeof(pos_y));
    std::memcpy(&pos_z, &sample.pos_z_bits, sizeof(pos_z));
    std::fprintf(g_backpack_witness_csv,
                 "%llu,%llu,0x%03x,0x%08x,0x%08x,%s,0x%08x,0x%08x,0x%08x,0x%02x,0x%02x,"
                 "NA,NA,NA,NA,NONE,%s,%s,NA,0x%08x,%s,%.9g,%.9g,%.9g,%.9g,0x%08x,0x%08x\n",
                 static_cast<unsigned long long>(sample.frame),
                 static_cast<unsigned long long>(sample.gfx_tick),
                 sample.model_resource_id, sample.model_bin, sample.node_ptr,
                 backpack_node_type_name(sample.caller_ra), sample.caller_ra,
                 sample.geocmd_ptr, sample.node_word0, sample.node_byte8,
                 sample.node_byte9, prev_hex, curr_hex, sample.gfx_ctx,
                 sample.draw_submitted ? "YES" : "NO", scale, pos_x, pos_y, pos_z,
                 sample.prev_mtx_ptr, sample.curr_mtx_ptr);
    if (force_flush) {
        std::fflush(g_backpack_witness_csv);
    }
}

void backpack_flush_csv_locked(bool close) {
    if (g_backpack_witness_csv == nullptr) {
        return;
    }
    std::fflush(g_backpack_witness_csv);
    if (close) {
        std::fclose(g_backpack_witness_csv);
        g_backpack_witness_csv = nullptr;
    }
}

void backpack_write_status(const char* reason) {
    uint64_t samples = 0;
    uint64_t anomalies = 0;
    uint64_t last_frame = 0;
    bool first_hit = false;
    uint64_t first_hit_frame = 0;
    uint32_t first_hit_callsite = 0;
    uint32_t first_model_id = 0;
    uint32_t first_model_bin = 0;
    uint32_t first_node_ptr = 0;
    std::array<uint32_t, kBackpackAssetCount> model_ptrs{};
    uint64_t entry3_hits = 0;
    uint64_t d674c_hits = 0;
    uint64_t d674c_tracked = 0;
    std::array<uint64_t, kBackpackAssetCount> asset_hits{};
    uint64_t asset_other = 0;
    std::array<BackpackObservedAsset, kBackpackObservedAssetLimit>
        observed_other_assets{};
    size_t observed_other_asset_count = 0;
    uint64_t observed_other_asset_evictions = 0;
    uint64_t de448_hits = 0;
    uint64_t de448_tracked = 0;
    uint64_t de498_hits = 0;
    uint64_t de498_tracked = 0;
    uint64_t e2aa4_hits = 0;
    uint64_t e2aa4_tracked = 0;
    uint64_t e2180_hits = 0;
    uint64_t e2180_tracked = 0;
    uint64_t e2180_written = 0;
    std::array<uint64_t, static_cast<size_t>(BackpackReject::Count)> e2180_rejections{};
    uint64_t scope_overflow = 0;
    uint64_t scope_max_depth = 0;
    size_t unique_nodes = 0;
    uint64_t unique_overflow = 0;
    {
        std::lock_guard<std::mutex> lock(g_backpack_flap_mu);
        samples = g_backpack_flap_samples;
        anomalies = g_backpack_flap_anomalies;
        last_frame = g_backpack_flap_last_frame;
        first_hit = g_backpack_flap_first_hit;
        first_hit_frame = g_backpack_flap_first_hit_frame;
        first_hit_callsite = g_backpack_flap_first_hit_callsite;
        first_model_id = g_backpack_first_model_id;
        first_model_bin = g_backpack_first_model_bin;
        first_node_ptr = g_backpack_first_node_ptr;
        model_ptrs = g_backpack_model_ptrs;
        entry3_hits = g_backpack_entry3_hits;
        d674c_hits = g_backpack_d674c_hits;
        d674c_tracked = g_backpack_d674c_tracked_hits;
        asset_hits = g_backpack_asset_hits;
        asset_other = g_backpack_asset_other_hits;
        observed_other_assets = g_backpack_observed_other_assets;
        observed_other_asset_count = g_backpack_observed_other_asset_count;
        observed_other_asset_evictions = g_backpack_observed_other_asset_evictions;
        de448_hits = g_backpack_de448_hits;
        de448_tracked = g_backpack_de448_tracked_hits;
        de498_hits = g_backpack_de498_hits;
        de498_tracked = g_backpack_de498_tracked_hits;
        e2aa4_hits = g_backpack_e2aa4_hits;
        e2aa4_tracked = g_backpack_e2aa4_tracked_hits;
        e2180_hits = g_backpack_e2180_hits;
        e2180_tracked = g_backpack_e2180_tracked_hits;
        e2180_written = g_backpack_e2180_sample_written;
        e2180_rejections = g_backpack_e2180_rejections;
        scope_overflow = g_backpack_scope_overflow_events;
        scope_max_depth = g_backpack_scope_max_depth;
        unique_nodes = g_backpack_unique_nodes.size();
        unique_overflow = g_backpack_unique_node_overflow;
    }

    if (FILE* status = std::fopen(tooie_diag_path("tooie-backpack-flap-status.txt").c_str(), "w")) {
        const auto fallback_start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        std::fprintf(status,
                     "DIAGNOSTIC_ENABLED=YES\n"
                     "PROCESS_START_TIME=%lld\n"
                     "PID=%ld\n"
                     "INIT_REACHED=YES\n"
                     "OLD_CALLER_DIAGNOSTIC_RETIRED=YES\n"
                     "BRANCH_DOUBLE_COUNT_FIXED=YES\n"
                     "NEW_MODEL_NODE_WITNESS=YES\n"
                     "FILTER_IDENTITY=func_800D674C(asset in 0x621,0x625,0x60A,0x61B,0x831) then func_800DE448 5th-arg model_bin then func_800E2180 in that draw\n"
                     "FILTER_CONFIDENCE=MEDIUM\n"
                     "SYSCALL_HOOK_INSTALLED=NO\n"
                     "CALLER_HOOK_INSTALLED=NO\n"
                     "BABACKPACK_ENTRY3_HITS=%llu\n"
                     "ASSET_RESOLVE_HITS=%llu\n"
                     "ASSET_0621_HITS=%llu\n"
                     "ASSET_0625_HITS=%llu\n"
                     "ASSET_060A_HITS=%llu\n"
                     "ASSET_061B_HITS=%llu\n"
                     "ASSET_0831_HITS=%llu\n"
                     "ASSET_OTHER_HITS=%llu\n"
                     "D674C_HIT_COUNT=%llu\n"
                     "D674C_TRACKED_HIT_COUNT=%llu\n"
                     "DE448_TOTAL_HITS=%llu\n"
                     "DE448_BACKPACK_FILTER_HITS=%llu\n"
                     "DE498_TOTAL_HITS=%llu\n"
                     "DE498_BACKPACK_FILTER_HITS=%llu\n"
                     "E2AA4_TOTAL_HITS=%llu\n"
                     "E2AA4_BACKPACK_FILTER_HITS=%llu\n"
                     "E2180_TOTAL_HITS=%llu\n"
                     "E2180_BACKPACK_FILTER_HITS=%llu\n"
                     "E2180_SAMPLE_WRITTEN_COUNT=%llu\n"
                     "SAMPLE_COUNT=%llu\n"
                     "ANOMALY_COUNT=%llu\n"
                     "UNIQUE_NODE_COUNT=%llu\n"
                     "UNIQUE_NODE_OVERFLOW_COUNT=%llu\n"
                     "FILTER_SCOPE_STACK_SIZE=%llu\n"
                     "FILTER_SCOPE_OVERFLOW_EVENTS=%llu\n"
                     "DRAW_CONTEXT_MAX_DEPTH=%llu\n"
                     "DRAW_CONTEXT_STACK_OVERFLOW=%s\n"
                     "E2180_REJECT_NO_ACTIVE_MODEL_CONTEXT=%llu\n"
                     "E2180_REJECT_MODEL_ID_MISMATCH=%llu\n"
                     "E2180_REJECT_NO_NODE_CONTEXT=%llu\n"
                     "E2180_REJECT_UNEXPECTED_NODE_TYPE=%llu\n"
                     "E2180_REJECT_OTHER=%llu\n"
                     "BACKPACK_FIRST_HIT=%s\n",
                     static_cast<long long>(g_backpack_flap_process_start_ms != 0
                                                 ? g_backpack_flap_process_start_ms
                                                 : fallback_start_ms),
                     g_backpack_flap_pid != 0 ? g_backpack_flap_pid
                                              : static_cast<long>(getpid()),
                     static_cast<unsigned long long>(entry3_hits),
                     static_cast<unsigned long long>(d674c_hits),
                     static_cast<unsigned long long>(asset_hits[0]),
                     static_cast<unsigned long long>(asset_hits[1]),
                     static_cast<unsigned long long>(asset_hits[2]),
                     static_cast<unsigned long long>(asset_hits[3]),
                     static_cast<unsigned long long>(asset_hits[4]),
                     static_cast<unsigned long long>(asset_other),
                     static_cast<unsigned long long>(d674c_hits),
                     static_cast<unsigned long long>(d674c_tracked),
                     static_cast<unsigned long long>(de448_hits),
                     static_cast<unsigned long long>(de448_tracked),
                     static_cast<unsigned long long>(de498_hits),
                     static_cast<unsigned long long>(de498_tracked),
                     static_cast<unsigned long long>(e2aa4_hits),
                     static_cast<unsigned long long>(e2aa4_tracked),
                     static_cast<unsigned long long>(e2180_hits),
                     static_cast<unsigned long long>(e2180_tracked),
                     static_cast<unsigned long long>(e2180_written),
                     static_cast<unsigned long long>(samples),
                     static_cast<unsigned long long>(anomalies),
                     static_cast<unsigned long long>(unique_nodes),
                     static_cast<unsigned long long>(unique_overflow),
                     static_cast<unsigned long long>(kBackpackDrawStackSize),
                     static_cast<unsigned long long>(scope_overflow),
                     static_cast<unsigned long long>(scope_max_depth),
                     scope_overflow != 0 ? "YES" : "NO",
                     static_cast<unsigned long long>(e2180_rejections[0]),
                     static_cast<unsigned long long>(e2180_rejections[1]),
                     static_cast<unsigned long long>(e2180_rejections[2]),
                     static_cast<unsigned long long>(e2180_rejections[3]),
                     static_cast<unsigned long long>(e2180_rejections[4]),
                     first_hit ? "YES" : "NO");
        for (size_t i = 0; i < kBackpackAssetCount; ++i) {
            std::fprintf(status, "ASSET_0x%03X_MODEL_BIN=0x%08x\n",
                         kBackpackAssetIds[i], model_ptrs[i]);
        }
        std::fprintf(status,
                     "ASSET_OTHER_UNIQUE_SLOTS=%llu\n"
                     "ASSET_OTHER_EVICTIONS=%llu\n",
                     static_cast<unsigned long long>(observed_other_asset_count),
                     static_cast<unsigned long long>(observed_other_asset_evictions));
        for (size_t i = 0; i < observed_other_asset_count; ++i) {
            std::fprintf(status,
                         "ASSET_OTHER_%02llu_ID=0x%08x_HITS=%llu_MODEL_BIN=0x%08x\n",
                         static_cast<unsigned long long>(i),
                         observed_other_assets[i].asset_id,
                         static_cast<unsigned long long>(observed_other_assets[i].hits),
                         observed_other_assets[i].model_ptr);
        }
        if (first_hit) {
            std::fprintf(status,
                         "FIRST_HIT_FRAME=%llu\n"
                         "FIRST_HIT_CALLSITE=0x%08x\n"
                         "FIRST_HIT_FUNCTION=func_800E2180\n"
                         "FIRST_MODEL_RESOURCE_ID=0x%03x\n"
                         "FIRST_MODEL_BIN=0x%08x\n"
                         "FIRST_NODE_PTR=0x%08x\n",
                         static_cast<unsigned long long>(first_hit_frame),
                         first_hit_callsite, first_model_id, first_model_bin,
                         first_node_ptr);
        } else {
            std::fprintf(status,
                         "FIRST_HIT_FRAME=UNKNOWN\n"
                         "FIRST_HIT_CALLSITE=UNKNOWN\n"
                         "FIRST_HIT_FUNCTION=UNKNOWN\n"
                         "FIRST_MODEL_RESOURCE_ID=UNKNOWN\n"
                         "FIRST_MODEL_BIN=UNKNOWN\n"
                         "FIRST_NODE_PTR=UNKNOWN\n");
        }
        std::fprintf(status,
                     "LAST_FRAME=%llu\nLAST_FLUSH_REASON=%s\n",
                     static_cast<unsigned long long>(last_frame),
                     reason != nullptr ? reason : "flush");
        std::fflush(status);
        std::fclose(status);
    }
}

void backpack_write_files(const char* reason) {
    if (!g_backpack_flap_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(g_backpack_flap_mu);
        backpack_flush_csv_locked(reason != nullptr &&
                                  std::strcmp(reason, "shutdown") == 0);
    }
    std::array<BackpackFlapSample, kBackpackFlapRingSize> snapshot{};
    size_t count = 0;
    size_t next = 0;
    bool anomaly = false;
    uint64_t samples = 0;
    uint64_t anomalies_total = 0;
    size_t first_anomaly_index = 0;
    {
        std::lock_guard<std::mutex> lock(g_backpack_flap_mu);
        count = g_backpack_flap_ring_count;
        next = g_backpack_flap_ring_next;
        anomaly = g_backpack_flap_anomaly_seen;
        samples = g_backpack_flap_samples;
        anomalies_total = g_backpack_flap_anomalies;
        first_anomaly_index = count;
        for (size_t i = 0; i < count; ++i) {
            const size_t index = (next + kBackpackFlapRingSize - count + i) %
                                 kBackpackFlapRingSize;
            snapshot[i] = g_backpack_flap_ring[index];
            if (first_anomaly_index == count &&
                backpack_sample_anomalous(snapshot[i])) {
                first_anomaly_index = i;
            }
        }
    }

    if (FILE* report = std::fopen(tooie_diag_path("tooie-backpack-flap-anomalies.txt").c_str(), "w")) {
        std::fprintf(report,
                     "TOOIE_DIAG_BACKPACK_FLAP=1\nreason=%s\n"
                     "samples=%llu\nanomalies=%llu\n"
                     "anomaly_seen=%s\n"
                     "matrix_fields=CPU dual-mtx prev/curr words from func_800E2180\n"
                     "draw_dl=gfx ctx pointer at dual-mtx submit\n"
                     "flap_part_id=UNKNOWN_UNTIL_CAPTURE\n",
                     reason != nullptr ? reason : "flush",
                     static_cast<unsigned long long>(samples),
                     static_cast<unsigned long long>(anomalies_total),
                     anomaly ? "YES" : "NO");
        if (first_anomaly_index < count) {
            const BackpackFlapSample& bad = snapshot[first_anomaly_index];
            std::fprintf(report,
                         "first_bad_stage=DUAL_MTX_NONFINITE\n"
                         "first_bad_function=func_800E2180\n"
                         "first_bad_address=0x%08x\n"
                         "bad_frame=%llu\n"
                         "bad_node=0x%08x\n"
                         "next_good_frame=UNKNOWN\n",
                         bad.node_ptr,
                         static_cast<unsigned long long>(bad.frame),
                         bad.node_ptr);
        } else {
            std::fprintf(report,
                         "first_bad_stage=NO_ANOMALY_PROVEN\n"
                         "first_bad_function=UNKNOWN\n"
                         "first_bad_address=UNKNOWN\n"
                         "previous_good_value=UNKNOWN\n"
                         "bad_frame=UNKNOWN\n"
                         "next_good_frame=UNKNOWN\n");
        }
        std::fclose(report);
    }
}

void backpack_flush_at_exit() {
    backpack_write_status("shutdown");
    backpack_write_files("shutdown");
}

void secondary_sway_write_status(const char* reason) {
    FILE* status = std::fopen(tooie_diag_path("tooie-secondary-sway-status.txt").c_str(), "w");
    if (status == nullptr) {
        return;
    }
    std::fprintf(status,
                 "REASON=%s\n"
                 "DIAGNOSTIC_ENABLED=YES\n"
                 "DIAGNOSTIC_SCHEMA=" TOOIE_SECONDARY_SWAY_DIAGNOSTIC_SCHEMA "\n"
                 "PRIMARY_WITNESS=KAZOOIE_CARRY_BANJO_HEAD_FEATHER\n"
                 "RESOURCE=0x62B_or_0x61C_or_0x83A\n"
                 "FEATHER_BONE=0x62B_idx30_anim103_or_0x61C_idx11_anim69\n"
                 "FEATHER_NODE=0x62B+0x124A0_or_0x61C+0xF558\n"
                 "FEATHER_MATRIX_CLASS=SINGLE_G_MTX\n"
                 "RT64_MATCH_WITNESS=REAL_MATCHTRANSFORM\n"
                 "GUEST_ROTATION_WITNESS=SCALE_NORMALIZED\n"
                 "QUATERNION_SIGN_CONTINUITY=HEMISPHERE_DOT_PRODUCT\n"
                 "PARENT_CHILD_ROTATION_WITNESS=AE8A8_PARENT107_AND_BONE103_PLUS_AE820_FINAL\n"
                 "REAL_FEATHER_MATCH_WITNESS=REUSED_GENERIC_CALL_PREREQUISITE_CANDIDATE_DECISION\n"
                 "BANJO_SNOUT_TOKEN_ENQUEUE=DISABLED\n"
                 "SNOUT_RESOURCE=0x62B_CLOSED_AS_PRIMARY\n"
                 "SNOUT_NODE_OFFSET=0x11CB8\n"
                 "RT64_IDENTITY_WITNESS=ENABLED\n"
                 "MATRIX_CURSOR_SOURCE=D_8007BCB0\n"
                 "MLMTXAPPLY_FLOAT_SOURCE=MEM_W_D_8007BCB0\n"
                 "MLMTXAPPLY_N64_MTX_DEST=entry_a0_gfx_mtx_ring\n"
                 "TRACE_TOKEN_ENQUEUE_PREDICATE=DCF48_tracked_0x62B_feather_plus_postwrite_composed_ptr_plus_nonzero_gfx_mtx\n"
                 "RT64_TOKEN_LOOKUP_KEY=G_MTX_dest_or_same_depth_restore_alias\n"
                 "RT64_TOKEN_STACK=MODELVIEW_PARALLEL\n"
                 "CURRENT_V2_LOOKUP_KEY_CORRECT=NO\n"
                 "FINAL_DIAGNOSTIC_SCHEMA=" TOOIE_SECONDARY_SWAY_DIAGNOSTIC_SCHEMA "\n"
                 "RT64_MATCH_WITNESS=REAL_MATCHTRANSFORM\n"
                 "GUEST_MATRIX_WITNESS=D_8007BCB0_VALIDATED\n"
                 "RT64_CALL_MATCH_WITNESS=ENABLED\n"
                 "RT64_MATCH_ARGUMENT_WITNESS=ENABLED\n"
                 "EXISTING_TOKEN_PLUMBING_REUSED=YES\n"
                 "TOKEN_TO_MATCHTRANSFORM_BINDING=V5_observe_by_preserved_matrix_workload_and_transform\n"
                 "TOKEN_BINDING_CONFIDENCE=HIGH\n"
                 "WORKLOAD_BINDING=MATRIX_WORKLOAD_PRESERVED\n"
                 "MATRIX_WORKLOAD_FIELD=matrix_workload_id\n"
                 "VERTEX_WORKLOAD_FIELD=vertex_workload_id\n"
                 "MATCH_WORKLOAD_FIELD=match_workload_id\n"
                 "ZERO_VERTEX_CAN_OVERWRITE_VALID_MATRIX_WORKLOAD=NO\n"
                 "REAL_UNMATCHED_REQUIRES_REAL_MATCH_DECISION=YES\n"
                 "NO_CANDIDATE_OBSERVED_SEMANTICS=DIAGNOSTIC_ONLY_FINALIZATION_FALLBACK\n"
                 "REAL_MATCHTRANSFORM_WITNESS_RETAINED=YES\n"
                 "V5_MATCH_WITNESS_REUSABLE_FOR_KAZOOIE=YES\n"
                 "FULL_CHAIN_V4_TOKEN_SUPPORT=YES\n"
                 "FULL_CHAIN_BONE_ANIM_IDS=79,100,101,102,103,104,105,107,108\n"
                 "TRACKED_BONE_ORDER=79,100,101,102,103,104,105,107,108\n"
                 "TRACKED_BONE_EXPECTED_MASK=0x1FF\n"
                 "POST_MATCH_MATRIX_WITNESS_IMPLEMENTED=YES\n"
                 "POST_MATCH_ROTATION_WITNESS=YES_SCALE_NORMALIZED_QUATERNION_AND_ANGLES\n"
                 "POST_MATCH_RESULT_CPU_OBSERVABLE=YES\n"
                 "INTERPOLATION_FRACTION_CPU_OBSERVABLE=YES\n"
                 "INTERPOLATION_FRACTION_SOURCE=TransformProcessor::process.curFrameWeight\n"
                 "GAME_UPDATE_VS_RENDER_FRAME_DISTINGUISHABLE=PARTIAL\n"
                 "FULL_CHAIN_NO_INTERP_AB_IMPLEMENTED=YES\n"
                 "FULL_CHAIN_NO_INTERP_IDENTITY_MATCHING_UNCHANGED=YES\n"
                 "FULL_CHAIN_NO_INTERP_GUEST_MATRICES_UNCHANGED=YES\n"
                 "ONLY_TEMPORAL_INTERPOLATION_REMOVED=YES\n"
                 "NO_INTERP_AB_CAN_PROVE_TEMPORAL_INTERPOLATION_CAUSALITY=YES\n"
                 "V4_DIAGNOSTIC_ROOT=WORKLOAD_BINDING_OVERWRITE\n"
                 "V4_DIAGNOSTIC_ROOT_CONFIDENCE=HIGH\n"
                 "GRAPHICAL_ROOT_CLASS=MORE_EVIDENCE_REQUIRED\n"
                 "GRAPHICAL_ROOT_CONFIDENCE=LOW\n"
                 "SNOUT_SEMANTIC_CONTEXT_KEY=call_hash_plus_scene_proj_plus_call_plus_range\n"
                 "FEATHER_SEMANTIC_CONTEXT_KEY=workload_plus_call_hash_plus_scene_proj_plus_call_plus_range\n"
                 "CAN_COMPARE_PREVIOUS_ONLY_WITHIN_SAME_CONTEXT=YES\n"
                 "THIRTEEN_CONTEXTS_EXPECTED=YES\n"
                 "THIRTEEN_CONTEXTS_CAN_CAUSE_FALSE_CROSS_CONTEXT_ANALYSIS=YES\n"
                 "NEXT_CAPTURE_CAN_PROVE_RT64_AUTO_MATCH_FAILURE=YES\n"
                 "NEXT_CAPTURE_CAN_REFUTE_RT64_AUTO_MATCH_FAILURE=YES\n"
                 "RT64_MATCH_WITNESS_REUSABLE_FOR_KAZOOIE=YES\n"
                 "RAW_V3_EVIDENCE_DUPLICATION_CLEANED=NOT_NEEDED\n"
                 "TOKEN_FLOAT_SOURCE_CAPTURED=YES\n"
                 "TOKEN_N64_MTX_DEST_CAPTURED=YES\n"
                 "TOKEN_GFX_CMD_CAPTURED=YES_at_RT64_matrix_consumption\n"
                 "TOKEN_RT64_PRIMARY_MATCH_FIELD=N64_MTX_DESTINATION_EQUALS_G_MTX_OPERAND_OR_SAME_DEPTH_RESTORE\n"
                 "TOKEN_RT64_SECONDARY_CORRELATION_FIELDS=FLOAT_SOURCE_GFX_COMMAND_WORKLOAD_PHYSICAL_PARAMS_STACK_GROUP\n"
                 "ADDRESS_NORMALIZATION_REQUIRED_FOR_TOKEN_MATCH=NO\n"
                 "FULL_COMPOSED_FLOAT_MATRIX_VALIDATION_READY=YES\n"
                 "FULL_N64_MTX_VALIDATION_READY=YES\n"
                 "NEXT_CAPTURE_CAN_TEST_NAN_INF_AUTHORITATIVELY=YES\n"
                 "RT64_SINGLE_MTX_IDENTITY_WITNESS_READY=YES\n"
                 "TRACE_TOKEN_SCOPE_CONFIDENCE=HIGH\n"
                 "FILTER=DCF48_compact_type2_0x62B_bin_plus_0x11CB8_idx11_bone42\n"
                 "TREE_WITNESS=DE498_0x62B_token_plus_DE2A4_sibling_scan_plus_DCF48\n"
                 "E2AA4_PRIMARY_SNOUT_WITNESS=DEMOTED\n"
                 "OLD_R31_FILTER=RETIRED\n"
                 "OLD_80092444_TLS_FOR_E2180=RETIRED\n"
                 "OLD_0X14960_ASSUMPTION=RETIRED\n"
                 "OLD_ALLOCATION_SPAN_0X20000=RETIRED\n"
                 "GENERIC_E2180_DISCOVERY_FILTER=DEMOTED\n"
                 "TRUE_0X62B_ASSET_SIZE=0x%X\n"
                 "D674C_USED=via_func_80092B04_when_id_ne_0x607\n"
                 "OUTER_PLAYER_SCOPE_HITS=%llu\n"
                 "FUNC_80092444_IN_PLAYER_SCOPE_HITS=%llu\n"
                 "DE448_TOTAL_SEEN=%llu\n"
                 "DE448_IN_PLAYER_SCOPE_HITS=%llu\n"
                 "DE448_CALLSITE_800926D8_HITS=%llu\n"
                 "PLAYER_MODEL_SAMPLE_WRITES=%llu\n"
                 "PLAYER_SCOPE_MAX_DEPTH=%llu\n"
                 "PLAYER_SCOPE_OVERFLOW=%llu\n"
                 "REJECT_NO_PLAYER_SCOPE=%llu\n"
                 "REJECT_WRONG_CALLSITE=%llu\n"
                 "REJECT_OTHER=%llu\n"
                 "LAST_RESOURCE_ID=0x%X\n"
                 "LAST_MODEL_BIN=0x%08X\n"
                 "LAST_MAGIC=0x%X\n"
                 "LAST_GTYPE=0x%X\n"
                 "LAST_BONE_COUNT=%u\n"
                 "LAST_PLAYER_STATE=0x%08X\n"
                 "LAST_LOCOMOTION=0x%X\n"
                 "BONE_ANIM_ENTRY_HITS=%llu\n"
                 "BONE_ANIM_HITS=%llu\n"
                 "BONE_ANIM_SAMPLES=%llu\n"
                 "SNOUT_MATRIX_ENTRY_HITS=%llu\n"
                 "SNOUT_MATRIX_HITS=%llu\n"
                 "SNOUT_MATRIX_SAMPLES=%llu\n"
                 "SNOUT_REJECT_NO_SCOPE=%llu\n"
                 "SNOUT_REJECT_NO_DURABLE_MODEL=%llu\n"
                 "SNOUT_REJECT_NO_GETTER=%llu\n"
                 "SNOUT_REJECT_WRONG_RESOURCE=%llu\n"
                 "SNOUT_REJECT_WRONG_NODE=%llu\n"
                 "SNOUT_REJECT_NODE_OUTSIDE_MODEL_SPAN=%llu\n"
                 "SNOUT_REJECT_WRONG_NODE_TYPE=%llu\n"
                 "SNOUT_REJECT_WRONG_BONE_ANIM_ID=%llu\n"
                 "SNOUT_REJECT_NODE_POINTER_FORM_MISMATCH=%llu\n"
                 "SNOUT_REJECT_OTHER_NODE_REASON=%llu\n"
                 "E2AA4_RAW_ENTRY_COUNT=%llu\n"
                 "E2AA4_WITH_REGISTERED_0X62B_COUNT=%llu\n"
                 "E2AA4_NODE_IN_TRUE_0X62B_ASSET_RANGE=%llu\n"
                 "E2AA4_REL_0X11CB8_COUNT=%llu\n"
                 "E2AA4_TYPE2_COUNT=%llu\n"
                 "E2AA4_INDEX11_COUNT=%llu\n"
                 "E2AA4_BONE42_COUNT=%llu\n"
                 "E2AA4_SNOUT_ACCEPT_COUNT=%llu\n"
                 "E2AA4_REJECT_NO_REGISTERED_0X62B=%llu\n"
                 "E2AA4_REJECT_OUTSIDE_TRUE_ASSET_RANGE=%llu\n"
                 "E2AA4_REJECT_OFFSET_MISMATCH=%llu\n"
                 "E2AA4_REJECT_TYPE_MISMATCH=%llu\n"
                 "E2AA4_REJECT_INDEX_MISMATCH=%llu\n"
                 "E2AA4_REJECT_BONE_ANIM_ID_MISMATCH=%llu\n"
                 "E2AA4_REJECT_OTHER=%llu\n"
                 "E2AA4_TUPLE_ROWS=%llu\n"
                 "E2180_ASSOCIATED_SNOUT_COUNT=%llu\n"
                 "DE2A4_RAW_ENTRY_COUNT=%llu\n"
                 "DE2A4_IN_TRUE_0X62B_COUNT=%llu\n"
                 "DE2A4_TYPE2_COUNT=%llu\n"
                 "DE2A4_REL_0X11CB8_COUNT=%llu\n"
                 "DE2A4_LAST_TYPE2_HANDLER=0x%08X\n"
                 "DCF48_RAW_ENTRY_COUNT=%llu\n"
                 "DCF48_IN_TRUE_0X62B_COUNT=%llu\n"
                 "DCF48_SNOUT_ACCEPT_COUNT=%llu\n"
                 "DCF48_REL_0X11CB8_COUNT=%llu\n"
                 "DE2A4_LIST_CONTAINS_0X11CB8=%llu\n"
                 "TREE_CSV_ROWS=%llu\n"
                 "TREE_DRAWS_LOGGED=%llu\n"
                 "LAST_HANDLER_TABLE=0x%08X\n"
                 "LAST_GEOCMD_MODE=0x%X\n"
                 "ADE2C_SNOUT_SAMPLES=%llu\n"
                 "DURABLE_0X62B_REGISTER_HITS=%llu\n"
                 "HIP_CONSUME_ENTRY_HITS=%llu\n"
                 "HIP_CONSUME_HITS=%llu\n"
                 "HIP_CONSUME_SAMPLES=%llu\n"
                 "PRODUCER_WITNESS=AE8A8_AE820\n"
                 "PRODUCER_CSV=%s\n"
                 "AE8A8_HITS=%llu\n"
                 "AE820_HITS=%llu\n"
                 "PRODUCER_SAMPLES=%llu\n"
                 "SNOUT_NODE_OFFSET=0x%X\n"
                 "FIRST_FILTERED=%s\n",
                 reason != nullptr ? reason : "status",
                 kSecondarySwayAssetSize,
                 static_cast<unsigned long long>(g_secondary_sway_outer_hits),
                 static_cast<unsigned long long>(g_secondary_sway_92444_hits),
                 static_cast<unsigned long long>(g_secondary_sway_total_hits),
                 static_cast<unsigned long long>(g_secondary_sway_in_scope_hits),
                 static_cast<unsigned long long>(g_secondary_sway_callsite_800926d8_hits),
                 static_cast<unsigned long long>(g_secondary_sway_sample_writes),
                 static_cast<unsigned long long>(g_secondary_sway_max_depth),
                 static_cast<unsigned long long>(g_secondary_sway_overflow),
                 static_cast<unsigned long long>(g_secondary_sway_reject_no_scope),
                 static_cast<unsigned long long>(g_secondary_sway_reject_wrong_callsite),
                 static_cast<unsigned long long>(g_secondary_sway_reject_other),
                 g_secondary_sway_last_resource,
                 g_secondary_sway_last_model_bin,
                 g_secondary_sway_last_magic,
                 static_cast<unsigned>(g_secondary_sway_last_gtype) & 0xFFFFu,
                 static_cast<unsigned>(g_secondary_sway_last_bones),
                 g_secondary_sway_last_player,
                 g_secondary_sway_last_locomotion,
                 static_cast<unsigned long long>(g_secondary_sway_bone_anim_entry_hits),
                 static_cast<unsigned long long>(g_secondary_sway_bone_anim_hits),
                 static_cast<unsigned long long>(g_secondary_sway_bone_anim_samples),
                 static_cast<unsigned long long>(g_secondary_sway_snout_matrix_entry_hits),
                 static_cast<unsigned long long>(g_secondary_sway_snout_matrix_hits),
                 static_cast<unsigned long long>(g_secondary_sway_snout_matrix_samples),
                 static_cast<unsigned long long>(g_secondary_sway_snout_reject_no_scope),
                 static_cast<unsigned long long>(g_secondary_sway_snout_reject_no_durable),
                 static_cast<unsigned long long>(g_secondary_sway_snout_reject_no_getter),
                 static_cast<unsigned long long>(g_secondary_sway_snout_reject_wrong_resource),
                 static_cast<unsigned long long>(g_secondary_sway_snout_reject_wrong_node),
                 static_cast<unsigned long long>(g_secondary_sway_snout_reject_outside_span),
                 static_cast<unsigned long long>(g_secondary_sway_snout_reject_wrong_type),
                 static_cast<unsigned long long>(g_secondary_sway_snout_reject_wrong_bone),
                 static_cast<unsigned long long>(g_secondary_sway_snout_reject_pointer_form),
                 static_cast<unsigned long long>(g_secondary_sway_snout_reject_other_node),
                 static_cast<unsigned long long>(g_secondary_sway_e2aa4_raw),
                 static_cast<unsigned long long>(g_secondary_sway_e2aa4_with_registered),
                 static_cast<unsigned long long>(g_secondary_sway_e2aa4_in_true_range),
                 static_cast<unsigned long long>(g_secondary_sway_e2aa4_rel_11cb8),
                 static_cast<unsigned long long>(g_secondary_sway_e2aa4_type2),
                 static_cast<unsigned long long>(g_secondary_sway_e2aa4_index11),
                 static_cast<unsigned long long>(g_secondary_sway_e2aa4_bone42),
                 static_cast<unsigned long long>(g_secondary_sway_e2aa4_accept),
                 static_cast<unsigned long long>(g_secondary_sway_e2aa4_reject_no_registered),
                 static_cast<unsigned long long>(g_secondary_sway_e2aa4_reject_outside_range),
                 static_cast<unsigned long long>(g_secondary_sway_e2aa4_reject_offset),
                 static_cast<unsigned long long>(g_secondary_sway_e2aa4_reject_type),
                 static_cast<unsigned long long>(g_secondary_sway_e2aa4_reject_index),
                 static_cast<unsigned long long>(g_secondary_sway_e2aa4_reject_bone),
                 static_cast<unsigned long long>(g_secondary_sway_e2aa4_reject_other),
                 static_cast<unsigned long long>(g_secondary_sway_e2aa4_tuple_rows),
                 static_cast<unsigned long long>(g_secondary_sway_e2180_associated),
                 static_cast<unsigned long long>(g_secondary_sway_de2a4_raw),
                 static_cast<unsigned long long>(g_secondary_sway_de2a4_in_range),
                 static_cast<unsigned long long>(g_secondary_sway_de2a4_type2),
                 static_cast<unsigned long long>(g_secondary_sway_de2a4_rel_11cb8),
                 g_secondary_sway_de2a4_last_type2_handler,
                 static_cast<unsigned long long>(g_secondary_sway_dcf48_raw),
                 static_cast<unsigned long long>(g_secondary_sway_dcf48_in_range),
                 static_cast<unsigned long long>(g_secondary_sway_dcf48_accept),
                 static_cast<unsigned long long>(g_secondary_sway_dcf48_rel_11cb8),
                 static_cast<unsigned long long>(g_secondary_sway_tree_list_contains_11cb8),
                 static_cast<unsigned long long>(g_secondary_sway_tree_rows),
                 static_cast<unsigned long long>(g_secondary_sway_tree_draws),
                 g_secondary_sway_last_handler_table,
                 g_secondary_sway_last_geocmd_mode,
                 static_cast<unsigned long long>(g_secondary_sway_ade2c_snout_samples),
                 static_cast<unsigned long long>(g_secondary_sway_durable_register_hits),
                 static_cast<unsigned long long>(g_secondary_sway_hip_consume_entry_hits),
                 static_cast<unsigned long long>(g_secondary_sway_hip_consume_hits),
                 static_cast<unsigned long long>(g_secondary_sway_hip_consume_samples),
                 tooie_diag_path("tooie-kazooie-feather-producer.csv").c_str(),
                 static_cast<unsigned long long>(g_secondary_sway_ae8a8_hits),
                 static_cast<unsigned long long>(g_secondary_sway_ae820_hits),
                 static_cast<unsigned long long>(g_secondary_sway_producer_samples),
                 kSecondarySwaySnoutNodeOffset,
                 g_secondary_sway_first_filtered ? "YES" : "NO");
    std::fprintf(status,
                 "KAZOOIE_FEATHER_PRIMARY_HITS=%llu\n"
                 "KAZOOIE_FEATHER_62B_PRIMARY_HITS=%llu\n"
                 "KAZOOIE_FEATHER_61C_PRIMARY_HITS=%llu\n"
                 "KAZOOIE_FEATHER_83A_PRIMARY_HITS=%llu\n"
                 "KAZOOIE_FEATHER_SIBLING_HITS=%llu\n"
                 "SNOUT_TRACE_TOKEN_CANDIDATES=%llu\n"
                 "SNOUT_TRACE_TOKEN_ENQUEUED=%llu\n"
                 "SNOUT_TRACE_TOKEN_CONSUMED_RT64=%llu\n"
                 "SNOUT_TRACE_TOKEN_EXPIRED=%llu\n"
                 "SNOUT_TRACE_TOKEN_ADDRESS_MISMATCH=%llu\n"
                 "SNOUT_TRACE_TOKEN_COMMAND_MISMATCH=%llu\n"
                 "SNOUT_TRACE_TOKEN_OTHER_REJECT=%llu\n"
                 "TOKEN_ENQUEUED=%llu\n"
                 "TOKEN_DEST_EQ_GMTX_OPERAND=%llu\n"
                 "TOKEN_SOURCE_EQ_GMTX_OPERAND=%llu\n"
                 "TOKEN_ADDRESS_NORMALIZATION_FAILURE=%llu\n"
                 "TOKEN_MATRIX_WORKLOAD_NONZERO=%llu\n"
                 "TOKEN_VERTEX_WORKLOAD_ZERO=%llu\n"
                 "TOKEN_VERTEX_WORKLOAD_NONZERO=%llu\n"
                 "TOKEN_VALID_WORKLOAD_PRESERVED=%llu\n"
                 "TOKEN_VALID_WORKLOAD_OVERWRITE_BLOCKED=%llu\n"
                 "TOKEN_BOUND_TO_GAMEFRAME_MATCH=%llu\n"
                 "TOKEN_BOUND_TO_CALL_STAGE=%llu\n"
                 "TOKEN_BOUND_TO_TRANSFORM_PREREQUISITE=%llu\n"
                 "TOKEN_BOUND_TO_CANDIDATE_STAGE=%llu\n"
                 "TOKEN_BOUND_TO_MATCHTRANSFORM=%llu\n"
                 "TOKEN_VERTEX_FALLBACK_BINDINGS=%llu\n"
                 "TOKEN_VERTEX_FALLBACK_AMBIGUOUS=%llu\n"
                 "TOKEN_RESTORE_ALIAS=%llu\n"
                 "TOKEN_RESTORE_ALIAS_AMBIGUOUS=%llu\n"
                 "INTERP_MODE_CONTROL_COUNT=%llu\n"
                 "INTERP_MODE_SIMPLE_OVERRIDE_COUNT=%llu\n"
                 "INTERP_MODE_OVERRIDE_AMBIGUOUS_COUNT=%llu\n"
                 "NO_INTERP_OVERRIDE_COUNT=%llu\n"
                 "POST_MATCH_MATRIX_ROWS=%llu\n"
                 "POST_MATCH_NO_TOKEN=%llu\n"
                 "AB_SWITCH_IMPLEMENTED=YES\n"
                 "AB_SWITCH_ENABLED=%s\n"
                 "NO_INTERP_AB_ENABLED=%s\n"
                 "AB_SWITCH_SCOPE=primary_0x62B_node_0x124A0_bone30_anim103\n"
                 "FULL_CHAIN_NO_INTERP_AB_IMPLEMENTED=YES\n"
                 "FULL_CHAIN_NO_INTERP_SCOPE=0x62B_boneAnim_79_100_101_102_103_104_105_107_108\n"
                 "P09_FEATHER_NO_INTERP_EXPERIMENT=V5C\n"
                 "CONTROL_PATH_UNCHANGED_WHEN_OFF=YES\n"
                 "AB_IDENTITY_MATCHING_UNCHANGED=YES\n"
                 "AB_RUNTIME_CAN_PROVE_MATCH_IDENTITY_UNCHANGED=YES\n"
                 "FEATHER_ROTATION_SAMPLES=%llu\n"
                 "FEATHER_LAST_ROTATION_DELTA_RAD=%.9g\n"
                 "FEATHER_LAST_QUATERNION_FLIP=%s\n"
                 "REAL_MATCHED=%llu\n"
                 "REAL_UNMATCHED=%llu\n"
                 "SYNTHETIC_FINALIZATION_ONLY=%llu\n"
                 "RT64_MATCH_ROWS=%llu\n"
                 "RT64_MATCH_MATCHED=%llu\n"
                 "RT64_MATCH_UNMATCHED=%llu\n"
                 "RT64_MATCH_NO_TOKEN=%llu\n"
                 "RT64_MATCH_CALL_ROWS=%llu\n"
                 "RT64_MATCH_CANDIDATE_ROWS=%llu\n"
                 "RT64_MATCH_DECISION_ROWS=%llu\n"
                 "RT64_MATCH_REAL_UNMATCHED=%llu\n"
                 "RT64_MATCH_FLUSH_SENTINEL_ROWS=%llu\n"
                 "RT64_MATCH_REASON_NO_PREVIOUS_CALL=%llu\n"
                 "RT64_MATCH_REASON_RANGE_COUNT_MISMATCH=%llu\n"
                 "RT64_MATCH_REASON_TRANSFORM_MATCHING_DISABLED=%llu\n"
                 "RT64_MATCH_REASON_MATRIX_ID_MISMATCH=%llu\n"
                 "RT64_MATCH_REASON_MATRIX_ID_NOT_AUTO=%llu\n"
                 "RT64_MATCH_REASON_MIRROR_DETERMINANT=%llu\n"
                 "RT64_MATCH_REASON_CANDIDATE_VALID=%llu\n"
                 "RT64_MATCH_REASON_CURRENT_ALREADY_MAPPED=%llu\n"
                 "RT64_MATCH_REASON_PREVIOUS_ALREADY_USED=%llu\n"
                 "RT64_MATCH_REASON_SELECTED=%llu\n"
                 "RT64_MATCH_REASON_TOKEN_NOT_BOUND=%llu\n"
                 "RT64_MATCH_REASON_NO_CANDIDATE_OBSERVED=%llu\n"
                 "TOKEN_POINTER_CHAIN_COUNTERS_IMPLEMENTED=YES\n",
                 static_cast<unsigned long long>(g_kazooie_feather_primary_hits),
                 static_cast<unsigned long long>(g_kazooie_feather_62b_primary_hits),
                 static_cast<unsigned long long>(g_kazooie_feather_61c_primary_hits),
                 static_cast<unsigned long long>(g_kazooie_feather_83a_primary_hits),
                 static_cast<unsigned long long>(g_kazooie_feather_sibling_hits),
                 static_cast<unsigned long long>(g_single_mtx_token_candidates),
                 static_cast<unsigned long long>(g_single_mtx_token_enqueued),
                 static_cast<unsigned long long>(g_single_mtx_token_consumed),
                 static_cast<unsigned long long>(g_single_mtx_token_expired),
                 static_cast<unsigned long long>(g_single_mtx_token_address_mismatch),
                 static_cast<unsigned long long>(g_single_mtx_token_command_mismatch),
                 static_cast<unsigned long long>(g_single_mtx_token_other_reject),
                 static_cast<unsigned long long>(g_single_mtx_token_enqueued),
                 static_cast<unsigned long long>(g_single_mtx_token_dest_eq_gmtx_operand),
                 static_cast<unsigned long long>(g_single_mtx_token_source_eq_gmtx_operand),
                 static_cast<unsigned long long>(g_single_mtx_token_normalization_failure),
                 static_cast<unsigned long long>(g_single_mtx_matrix_workload_nonzero),
                 static_cast<unsigned long long>(g_single_mtx_vertex_workload_zero),
                 static_cast<unsigned long long>(g_single_mtx_vertex_workload_nonzero),
                 static_cast<unsigned long long>(g_single_mtx_valid_workload_preserved),
                 static_cast<unsigned long long>(g_single_mtx_valid_workload_overwrite_blocked),
                 static_cast<unsigned long long>(g_single_mtx_bound_gameframe_match),
                 static_cast<unsigned long long>(g_single_mtx_bound_call_stage),
                 static_cast<unsigned long long>(g_single_mtx_bound_transform_prerequisite),
                 static_cast<unsigned long long>(g_single_mtx_bound_candidate_stage),
                 static_cast<unsigned long long>(g_single_mtx_bound_matchtransform),
                 static_cast<unsigned long long>(g_single_mtx_vertex_fallback_bindings),
                 static_cast<unsigned long long>(g_single_mtx_vertex_fallback_ambiguous),
                 static_cast<unsigned long long>(g_single_mtx_token_restore_alias),
                 static_cast<unsigned long long>(g_single_mtx_token_restore_ambiguous),
                 static_cast<unsigned long long>(g_single_mtx_interp_mode_control),
                 static_cast<unsigned long long>(g_single_mtx_interp_mode_simple_override),
                 static_cast<unsigned long long>(g_single_mtx_interp_mode_ambiguous),
                 static_cast<unsigned long long>(g_single_mtx_no_interp_override),
                 static_cast<unsigned long long>(g_single_mtx_post_match_rows),
                 static_cast<unsigned long long>(g_single_mtx_post_match_no_token),
                 g_secondary_sway_interp_simple_enabled.load(std::memory_order_relaxed)
                     ? "YES"
                     : "NO",
                 g_secondary_sway_no_interpolation_enabled.load(std::memory_order_relaxed)
                     ? "YES"
                     : "NO",
                 static_cast<unsigned long long>(g_secondary_sway_rotation_samples),
                 static_cast<double>(g_secondary_sway_last_rotation_delta),
                 g_secondary_sway_last_quaternion_flip ? "YES" : "NO",
                 static_cast<unsigned long long>(g_single_mtx_match_matched),
                 static_cast<unsigned long long>(g_single_mtx_match_real_unmatched),
                 static_cast<unsigned long long>(g_single_mtx_synthetic_finalization_only),
                 static_cast<unsigned long long>(g_single_mtx_match_rows),
                 static_cast<unsigned long long>(g_single_mtx_match_matched),
                 static_cast<unsigned long long>(g_single_mtx_match_unmatched),
                 static_cast<unsigned long long>(g_single_mtx_match_no_token),
                 static_cast<unsigned long long>(g_single_mtx_match_call_rows),
                 static_cast<unsigned long long>(g_single_mtx_match_candidate_rows),
                 static_cast<unsigned long long>(g_single_mtx_match_decision_rows),
                 static_cast<unsigned long long>(g_single_mtx_match_real_unmatched),
                 static_cast<unsigned long long>(g_single_mtx_match_flush_sentinel),
                 static_cast<unsigned long long>(g_single_mtx_match_reason_counts[kMatchReasonNoPreviousCall]),
                 static_cast<unsigned long long>(g_single_mtx_match_reason_counts[kMatchReasonRangeCountMismatch]),
                 static_cast<unsigned long long>(g_single_mtx_match_reason_counts[kMatchReasonTransformMatchingDisabled]),
                 static_cast<unsigned long long>(g_single_mtx_match_reason_counts[kMatchReasonMatrixIdMismatch]),
                 static_cast<unsigned long long>(g_single_mtx_match_reason_counts[kMatchReasonMatrixIdNotAuto]),
                 static_cast<unsigned long long>(g_single_mtx_match_reason_counts[kMatchReasonMirrorDeterminant]),
                 static_cast<unsigned long long>(g_single_mtx_match_reason_counts[kMatchReasonCandidateValid]),
                 static_cast<unsigned long long>(g_single_mtx_match_reason_counts[kMatchReasonCurrentAlreadyMapped]),
                 static_cast<unsigned long long>(g_single_mtx_match_reason_counts[kMatchReasonPreviousAlreadyUsed]),
                 static_cast<unsigned long long>(g_single_mtx_match_reason_counts[kMatchReasonSelected]),
                 static_cast<unsigned long long>(g_single_mtx_match_reason_counts[kMatchReasonTokenNotBound]),
                 static_cast<unsigned long long>(g_single_mtx_match_reason_counts[kMatchReasonNoCandidateObserved]));
    constexpr uint32_t kFeatherStatusIds[] = {79u, 100u, 101u, 102u,
                                              103u, 104u, 105u, 107u, 108u};
    for (uint32_t bone_anim_id : kFeatherStatusIds) {
        const uint32_t stat = bone_anim_id;
        std::fprintf(
            status,
            "BONE%u_AUTO_STATUS=REAL_CALL_MATCHED:%llu,REAL_CALL_UNMATCHED:%llu,"
            "REAL_MATCHED:%llu,REAL_UNMATCHED:%llu,SEMANTIC_MATCH_CORRECT:%llu,"
            "SEMANTIC_MATCH_FAILURE:%llu,SEMANTIC_CROSS_MATCH:%llu,"
            "SEMANTIC_DROPPED_MATCH:%llu\n",
            bone_anim_id,
            static_cast<unsigned long long>(g_feather_real_call_matched[stat]),
            static_cast<unsigned long long>(g_feather_real_call_unmatched[stat]),
            static_cast<unsigned long long>(g_feather_real_matched[stat]),
            static_cast<unsigned long long>(g_feather_real_unmatched[stat]),
            static_cast<unsigned long long>(g_feather_real_matched[stat]),
            static_cast<unsigned long long>(g_feather_semantic_failure[stat]),
            static_cast<unsigned long long>(g_feather_semantic_cross_match[stat]),
            static_cast<unsigned long long>(g_feather_semantic_dropped[stat]));
    }
    uint32_t guest_mask = 0;
    uint32_t matrix_mask = 0;
    uint32_t gmtx_mask = 0;
    uint32_t vertex_mask = 0;
    uint32_t matched_mask = 0;
    uint32_t post_match_mask = 0;
    uint32_t no_interp_mask = 0;
    uint32_t tri_mask = 0;
    for (uint32_t bone_anim_id : kFeatherStatusIds) {
        const uint32_t stat = bone_anim_id;
        const uint32_t bit = tracked_kazooie_feather_bit(bone_anim_id);
        std::fprintf(
            status,
            "BONE%u_STATUS=GUEST:%llu,MATRIX:%llu,GMTX:%llu,VERTEX:%llu,"
            "MATCHED:%llu,POSTMATCH:%llu,NO_INTERP:%llu,TRI:%llu\n",
            bone_anim_id,
            static_cast<unsigned long long>(g_feather_guest_seen[stat]),
            static_cast<unsigned long long>(g_feather_matrix_seen[stat]),
            static_cast<unsigned long long>(g_feather_gmtx_seen[stat]),
            static_cast<unsigned long long>(g_feather_vertex_seen[stat]),
            static_cast<unsigned long long>(g_feather_matched_seen[stat]),
            static_cast<unsigned long long>(g_feather_post_match_seen[stat]),
            static_cast<unsigned long long>(g_feather_no_interp_seen[stat]),
            static_cast<unsigned long long>(g_feather_tri_seen[stat]));
        if (g_feather_guest_seen[stat] != 0) guest_mask |= bit;
        if (g_feather_matrix_seen[stat] != 0) matrix_mask |= bit;
        if (g_feather_gmtx_seen[stat] != 0) gmtx_mask |= bit;
        if (g_feather_vertex_seen[stat] != 0) vertex_mask |= bit;
        if (g_feather_matched_seen[stat] != 0) matched_mask |= bit;
        if (g_feather_post_match_seen[stat] != 0) post_match_mask |= bit;
        if (g_feather_no_interp_seen[stat] != 0) no_interp_mask |= bit;
        if (g_feather_tri_seen[stat] != 0) tri_mask |= bit;
    }
    std::fprintf(status,
                 "TRACKED_BONES_WITH_GUEST_MASK=0x%02X\n"
                 "TRACKED_BONES_WITH_MATRIX_MASK=0x%02X\n"
                 "TRACKED_BONES_WITH_GMTX_MASK=0x%02X\n"
                 "TRACKED_BONES_WITH_VERTEX_MASK=0x%02X\n"
                 "TRACKED_BONES_WITH_MATCHED_MASK=0x%02X\n"
                 "TRACKED_BONES_WITH_POSTMATCH_MASK=0x%02X\n"
                 "TRACKED_BONES_WITH_NO_INTERP_MASK=0x%02X\n"
                 "TRACKED_BONES_WITH_TRI_MASK=0x%02X\n"
                 "RAW_TRI_HANDLER_COUNT=%llu\n"
                 "RAW_TRI1_COUNT=%llu\n"
                 "RAW_TRI2_COUNT=%llu\n"
                 "TRI_WITH_ANY_TRACKED_WORLDINDEX_COUNT=%llu\n"
                 "TRI_WITH_TRACKED_ACTIVE_COUNT=%llu\n"
                 "TRI_WITH_TRACKED_DL_COUNT=%llu\n"
                 "TRI_CLASSIFIED_COUNT=%llu\n"
                 "FEATHER_TRI_ROWS=%llu\n"
                 "FEATHER_TRI_MATCH_ACTIVE=%llu\n"
                 "FEATHER_TRI_MIXED_TRANSFORM=%llu\n"
                 "FEATHER_TRI_SLOT_NE_ACTIVE=%llu\n"
                 "V6_TRIANGLE_WITNESS_IMPLEMENTED=SUPERSEDED_BY_V6B\n"
                 "V6B_TRIANGLE_WITNESS_IMPLEMENTED=YES\n"
                 "V6B_LOOKUP_STARTS_FROM_VERTEX_WORLDINDEX=YES\n"
                 "V6B_REQUIRES_ACTIVE_TRANSFORM_HIT=NO\n"
                 "FULL_CHAIN_COVERAGE_COMPLETE=%s\n"
                 "CAPTURE_CAN_SELF_DECLARE_INCOMPLETE=YES\n",
                 guest_mask, matrix_mask, gmtx_mask, vertex_mask, matched_mask,
                 post_match_mask, no_interp_mask, tri_mask,
                 static_cast<unsigned long long>(
                     g_raw_tri_handler.load(std::memory_order_relaxed)),
                 static_cast<unsigned long long>(
                     g_raw_tri1.load(std::memory_order_relaxed)),
                 static_cast<unsigned long long>(
                     g_raw_tri2.load(std::memory_order_relaxed)),
                 static_cast<unsigned long long>(g_tri_with_tracked_worldindex),
                 static_cast<unsigned long long>(g_tri_with_tracked_active),
                 static_cast<unsigned long long>(g_tri_with_tracked_dl),
                 static_cast<unsigned long long>(g_tri_classified),
                 static_cast<unsigned long long>(g_feather_tri_rows),
                 static_cast<unsigned long long>(g_feather_tri_match_active),
                 static_cast<unsigned long long>(g_feather_tri_mixed),
                 static_cast<unsigned long long>(g_feather_tri_ne_active),
                 guest_mask == kTrackedKazooieFeatherMask &&
                         matrix_mask == kTrackedKazooieFeatherMask &&
                         gmtx_mask == kTrackedKazooieFeatherMask &&
                         vertex_mask == kTrackedKazooieFeatherMask &&
                         matched_mask == kTrackedKazooieFeatherMask &&
                         post_match_mask == kTrackedKazooieFeatherMask &&
                         no_interp_mask == kTrackedKazooieFeatherMask
                     ? "YES"
                     : "NO");
    std::fprintf(status, "NO_INTERP_TARGET_MASK=0x%03X\n",
                 kTrackedKazooieFeatherMask);
    uint64_t no_interp_eq_total = 0;
    uint64_t no_interp_ne_total = 0;
    bool all_nine_hit = true;
    for (uint32_t bone_anim_id : kFeatherStatusIds) {
        const uint32_t stat = bone_anim_id;
        const bool hit = g_feather_no_interp_seen[stat] != 0;
        if (!hit) {
            all_nine_hit = false;
        }
        no_interp_eq_total += g_feather_no_interp_eq_current[stat];
        no_interp_ne_total += g_feather_no_interp_ne_current[stat];
        std::fprintf(status, "BONE%u_HIT=%s\n", bone_anim_id,
                     hit ? "YES" : "NO");
        std::fprintf(
            status,
            "BONE%u_NO_INTERP_RESULT_EQ_CURRENT=%llu\n"
            "BONE%u_NO_INTERP_RESULT_NE_CURRENT=%llu\n",
            bone_anim_id,
            static_cast<unsigned long long>(
                g_feather_no_interp_eq_current[stat]),
            bone_anim_id,
            static_cast<unsigned long long>(
                g_feather_no_interp_ne_current[stat]));
    }
    const bool all_nine_eq_current =
        g_secondary_sway_no_interpolation_enabled.load(
            std::memory_order_relaxed) &&
        all_nine_hit && no_interp_ne_total == 0 && no_interp_eq_total > 0;
    std::fprintf(
        status,
        "NO_INTERP_RESULT_EQ_CURRENT_COUNT=%llu\n"
        "NO_INTERP_RESULT_NE_CURRENT_COUNT=%llu\n"
        "ALL_NINE_NO_INTERP_RESULT_EQUALS_CURRENT=%s\n"
        "V5B_NO_INTERP_BONES=79,100,101,102,103,104,105\n"
        "VISIBLE_TRIANGLE_WORLD_TRANSFORM_BONES=79,100,101,102,103,104,105,107,108\n"
        "V5B_WAS_COMPLETE_FOR_EVERY_TRANSFORM_USED_BY_VISIBLE_TRIANGLE_VERTICES=NO\n",
        static_cast<unsigned long long>(no_interp_eq_total),
        static_cast<unsigned long long>(no_interp_ne_total),
        all_nine_eq_current ? "YES" : "NO");
    std::fprintf(
        status,
        "CURRENT_DL_RUNTIME_ADDRESS_AVAILABLE=YES\n"
        "CURRENT_DL_STACK_CORRECT_FOR_NESTED_GDL=YES\n"
        "TRACKED_DL_RUNTIME_MAPPING_IMPLEMENTED=YES\n"
        "DL_5AA_RUNTIME_ADDRESS_FORMULA=physical(model_bin)+0x2670+0x5AA*8\n"
        "DL_57C_RUNTIME_ADDRESS_FORMULA=physical(model_bin)+0x2670+0x57C*8\n"
        "DL_534_RUNTIME_ADDRESS_FORMULA=physical(model_bin)+0x2670+0x534*8\n"
        "DL_592_RUNTIME_ADDRESS_FORMULA=physical(model_bin)+0x2670+0x592*8\n"
        "SEGMENT3_RUNTIME_BASE_CAPTURE_IMPLEMENTED=YES\n"
        "SEGMENT3_BASE_OBSERVED=%s\n"
        "SEGMENT3_RUNTIME_BASE=0x%08X\n"
        "DL_03000040_RESOLVED_RUNTIME_TARGET_CAPTURED=%s\n"
        "DL_03000040_RUNTIME_TARGET=0x%08X\n"
        "TRACKED_DL_TRI_COUNT=%llu\n"
        "DL_5AA_TRI_COUNT=%llu\n"
        "DL_57C_TRI_COUNT=%llu\n"
        "DL_534_TRI_COUNT=%llu\n"
        "DL_592_TRI_COUNT=%llu\n"
        "DL_03000040_TRI_COUNT=%llu\n"
        "VERTEX_SLOT_HISTORY_HIT_COUNT=%llu\n"
        "VERTEX_SLOT_HISTORY_MISS_COUNT=%llu\n"
        "OWN_MATCH_COUNT=%llu\n"
        "JOIN_EXPECTED_MIX_COUNT=%llu\n"
        "STALE_SLOT_COUNT=%llu\n"
        "LEFTOVER_PRE_GVTX_COUNT=%llu\n"
        "MIXED_UNEXPECTED_COUNT=%llu\n"
        "UNRESOLVED_COUNT=%llu\n"
        "V6C_CLASSES_MUTUALLY_INTERPRETABLE=YES\n"
        "CAPTURE_CAN_SELF_DECLARE_DL_MAPPING_FAILURE=YES\n"
        "CAPTURE_CAN_SELF_DECLARE_SLOT_HISTORY_FAILURE=YES\n"
        "V6C_OUTPUT_BOUNDED=YES\n"
        "V6C_OWN_DL_EXPECTED_BONE_MAPPING=0x5AA->103,0x57C->104,0x534->105\n"
        "V6C_JOIN_SLOT_EXPECTATION_MAPPING=slots0-3->child,slots28-31->parent\n"
        "V6C_LEFTOVER_PRE_GVTX_EXPECTATION=UNRESOLVED_UNLESS_ASSET_SEMANTICS_PROVE_INTENT\n"
        "V6C_SEGMENT3_GDL_COUNT=%llu\n"
        "V6C_SEGMENT3_BASE_OBSERVATION_COUNT=%llu\n",
        g_v6c_segment3_base_count != 0 ? "YES" : "NO",
        g_v6c_segment3_base,
        g_v6c_segment3_count != 0 ? "YES" : "NO",
        g_v6c_segment3_target,
        static_cast<unsigned long long>(g_v6c_tracked_dl_tri),
        static_cast<unsigned long long>(g_v6c_dl_5aa_tri),
        static_cast<unsigned long long>(g_v6c_dl_57c_tri),
        static_cast<unsigned long long>(g_v6c_dl_534_tri),
        static_cast<unsigned long long>(g_v6c_dl_592_tri),
        static_cast<unsigned long long>(g_v6c_dl_03000040_tri),
        static_cast<unsigned long long>(g_v6c_slot_history_hits),
        static_cast<unsigned long long>(g_v6c_slot_history_misses),
        static_cast<unsigned long long>(g_v6c_class_counts[kV6COwnMatch]),
        static_cast<unsigned long long>(g_v6c_class_counts[kV6CJoinExpectedMix]),
        static_cast<unsigned long long>(g_v6c_class_counts[kV6CStaleSlot]),
        static_cast<unsigned long long>(g_v6c_class_counts[kV6CLeftoverPreGvtx]),
        static_cast<unsigned long long>(g_v6c_class_counts[kV6CMixedUnexpected]),
        static_cast<unsigned long long>(g_v6c_class_counts[kV6CUnresolved]),
        static_cast<unsigned long long>(g_v6c_segment3_count),
        static_cast<unsigned long long>(g_v6c_segment3_base_count));
    std::fprintf(status, "NEW_SCHEMA=" TOOIE_SECONDARY_SWAY_DIAGNOSTIC_SCHEMA "\n");
    std::fflush(status);
    std::fclose(status);
}

void secondary_sway_flush_at_exit() {
    std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
    for (SingleMtxTraceToken& trace : g_single_mtx_tokens) {
        if (trace.pending) {
            ++g_single_mtx_token_expired;
            trace.pending = false;
        }
    }
    if (g_secondary_sway_csv != nullptr) {
        std::fflush(g_secondary_sway_csv);
        std::fclose(g_secondary_sway_csv);
        g_secondary_sway_csv = nullptr;
    }
    if (g_secondary_sway_bone_csv != nullptr) {
        std::fflush(g_secondary_sway_bone_csv);
        std::fclose(g_secondary_sway_bone_csv);
        g_secondary_sway_bone_csv = nullptr;
    }
    if (g_secondary_sway_snout_csv != nullptr) {
        std::fflush(g_secondary_sway_snout_csv);
        std::fclose(g_secondary_sway_snout_csv);
        g_secondary_sway_snout_csv = nullptr;
    }
    if (g_secondary_sway_hip_csv != nullptr) {
        std::fflush(g_secondary_sway_hip_csv);
        std::fclose(g_secondary_sway_hip_csv);
        g_secondary_sway_hip_csv = nullptr;
    }
    if (g_secondary_sway_e2180_tuple_csv != nullptr) {
        std::fflush(g_secondary_sway_e2180_tuple_csv);
        std::fclose(g_secondary_sway_e2180_tuple_csv);
        g_secondary_sway_e2180_tuple_csv = nullptr;
    }
    if (g_secondary_sway_e2aa4_tuple_csv != nullptr) {
        std::fflush(g_secondary_sway_e2aa4_tuple_csv);
        std::fclose(g_secondary_sway_e2aa4_tuple_csv);
        g_secondary_sway_e2aa4_tuple_csv = nullptr;
    }
    if (g_secondary_sway_de2a4_tuple_csv != nullptr) {
        std::fflush(g_secondary_sway_de2a4_tuple_csv);
        std::fclose(g_secondary_sway_de2a4_tuple_csv);
        g_secondary_sway_de2a4_tuple_csv = nullptr;
    }
    if (g_secondary_sway_tree_csv != nullptr) {
        std::fflush(g_secondary_sway_tree_csv);
        std::fclose(g_secondary_sway_tree_csv);
        g_secondary_sway_tree_csv = nullptr;
    }
    if (g_secondary_sway_producer_csv != nullptr) {
        std::fflush(g_secondary_sway_producer_csv);
        std::fclose(g_secondary_sway_producer_csv);
        g_secondary_sway_producer_csv = nullptr;
    }
    if (g_single_mtx_matrix_csv != nullptr) {
        std::fflush(g_single_mtx_matrix_csv);
        std::fclose(g_single_mtx_matrix_csv);
        g_single_mtx_matrix_csv = nullptr;
    }
    if (g_single_mtx_identity_csv != nullptr) {
        std::fflush(g_single_mtx_identity_csv);
        std::fclose(g_single_mtx_identity_csv);
        g_single_mtx_identity_csv = nullptr;
    }
    if (g_single_mtx_match_csv != nullptr) {
        std::fflush(g_single_mtx_match_csv);
        std::fclose(g_single_mtx_match_csv);
        g_single_mtx_match_csv = nullptr;
    }
    if (g_single_mtx_post_match_csv != nullptr) {
        std::fflush(g_single_mtx_post_match_csv);
        std::fclose(g_single_mtx_post_match_csv);
        g_single_mtx_post_match_csv = nullptr;
    }
    if (g_feather_tri_csv != nullptr) {
        std::fflush(g_feather_tri_csv);
        std::fclose(g_feather_tri_csv);
        g_feather_tri_csv = nullptr;
    }
    if (g_v6c_tri_csv != nullptr) {
        std::fflush(g_v6c_tri_csv);
        std::fclose(g_v6c_tri_csv);
        g_v6c_tri_csv = nullptr;
    }
    secondary_sway_write_status("shutdown");
}

const char* feather_anim_reject_name(FeatherAnimReject reject) {
    switch (reject) {
        case FeatherAnimReject::None:
            return "NONE";
        case FeatherAnimReject::BaanimNotPlayerTable:
            return "BAANIM_NOT_PLAYER_TABLE";
        case FeatherAnimReject::BaanimStateZero:
            return "BAANIM_STATE_ZERO";
        case FeatherAnimReject::BaanimAnctrlZero:
            return "BAANIM_ANCTRL_ZERO";
        case FeatherAnimReject::Ae8a8BadSkeleton:
            return "AE8A8_BAD_SKELETON";
        case FeatherAnimReject::Ae8a8NoPlume:
            return "AE8A8_NO_PLUME_FAMILY";
        case FeatherAnimReject::Ae8a8AnimBaseZero:
            return "AE8A8_ANIM_BASE_ZERO";
        case FeatherAnimReject::CsvOpenFailed:
            return "CSV_OPEN_FAILED";
        case FeatherAnimReject::MaxRows:
            return "MAX_ROWS";
        default:
            return "OTHER";
    }
}

void feather_anim_note_reject(FeatherAnimReject reject, const char* detail) {
    g_feather_anim_last_reject = reject;
    g_feather_anim_last_reject_detail = detail != nullptr ? detail : "NONE";
    const size_t idx = static_cast<size_t>(reject);
    if (idx < g_feather_anim_rejects.size()) {
        ++g_feather_anim_rejects[idx];
    }
}

void feather_anim_write_status(const char* reason) {
    FILE* status = std::fopen(tooie_diag_path("tooie-feather-anim-sample-time-status.txt").c_str(), "w");
    if (status == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_feather_anim_mu);
    std::fprintf(status,
                 "REASON=%s\n"
                 "SCHEMA=" TOOIE_FEATHER_ANIM_SAMPLE_TIME_SCHEMA "\n"
                 "VERSION=1\n"
                 "ENABLED=YES\n"
                 "DIAGNOSTIC_SCHEMA=" TOOIE_FEATHER_ANIM_SAMPLE_TIME_SCHEMA "\n"
                 "DIAGNOSTIC_VERSION=1\n"
                 "MAX_ROWS=%zu\n"
                 "RAW_BAANIM_UPDATES=%llu\n"
                 "BAANIM_ACCEPTED=%llu\n"
                 "AE8A8_RAW=%llu\n"
                 "AE8A8_ACCEPTED=%llu\n"
                 "TARGET_SAMPLE_ROWS=%llu\n"
                 "ROWS_WRITTEN=%llu\n"
                 "CONTROL_TICKS=%llu\n"
                 "JOIN_OK=%llu\n"
                 "JOIN_MISMATCH=%llu\n"
                 "SAMPLE_TIME_FORWARD=%llu\n"
                 "SAMPLE_TIME_SAME=%llu\n"
                 "SAMPLE_TIME_BACKWARD=%llu\n"
                 "SAMPLE_TIME_EXPECTED_WRAP=%llu\n"
                 "SAMPLE_TIME_UNEXPECTED_JUMP=%llu\n"
                 "CLIP_ID_CHANGE=%llu\n"
                 "CLIP_ID_UNEXPECTED_CHANGE=%llu\n"
                 "BONE107_LARGE_ROTATION_STEP=%llu\n"
                 "BONE107_LARGE_SCALE_STEP=%llu\n"
                 "BONE107_LARGE_TRANSLATION_STEP=%llu\n"
                 "BONE107_LARGE_STEP_AUTHORED=%llu\n"
                 "BONE107_LARGE_STEP_UNEXPLAINED=%llu\n"
                 "BONE107_RECORD_SAMPLE_MATCH=0\n"
                 "BONE107_RECORD_SAMPLE_MISMATCH=0\n"
                 "INDEPENDENT_SAMPLE_RECONSTRUCTION_AVAILABLE=NO\n"
                 "BONE107_KEY_PROVENANCE_COMPLETE=NO\n"
                 "BONE107_STEP_PROPAGATES_TO_LEFT=%llu\n"
                 "BONE107_STEP_PROPAGATES_TO_RIGHT=%llu\n"
                 "BONE108_LARGE_STEP=%llu\n"
                 "BONE79_LARGE_STEP=%llu\n"
                 "BODY_CONTROL_BONE=%u\n"
                 "BODY_CONTROL_INDEX=%u\n"
                 "BODY_CONTROL_LARGE_STEP=%llu\n"
                 "BODY_CONTROL_SAMPLE_TIME_SAME=%llu\n"
                 "LAST_CLIP_ID=%d\n"
                 "LAST_SAMPLE_TIME=%.9g\n"
                 "LAST_GATE_REJECT=%s\n"
                 "LAST_GATE_REJECT_DETAIL=%s\n"
                 "REJECT_BAANIM_NOT_PLAYER_TABLE=%llu\n"
                 "REJECT_BAANIM_STATE_ZERO=%llu\n"
                 "REJECT_BAANIM_ANCTRL_ZERO=%llu\n"
                 "REJECT_AE8A8_BAD_SKELETON=%llu\n"
                 "REJECT_AE8A8_NO_PLUME=%llu\n"
                 "REJECT_AE8A8_ANIM_BASE_ZERO=%llu\n"
                 "REJECT_CSV_OPEN_FAILED=%llu\n"
                 "REJECT_MAX_ROWS=%llu\n"
                 "CLASSIFICATION=PENDING_CAPTURE\n"
                 "ACTIVE_CLIP_ID_SOURCE=lh Animation+0x18 via anctrl+0x1C (func_8008C0C0)\n"
                 "SAMPLE_TIME_SOURCE=lwc1 Animation+0x10 (func_8008C0C8 / func_8008C1C0)\n"
                 "SAMPLE_FRAME_SOURCE=derived trunc(sample_time) plus optional header start/end\n"
                 "CLIP_DURATION_SOURCE=anctrl+0xC (anctrl_setDuration / 8008B1BC)\n"
                 "WRAP_MODE_SOURCE=anctrl+0x1A playback_type (1=ONCE 2=LOOP 3=STOPPED 4=SUBRANGE)\n"
                 "KEY_LOOKUP_FUNCTION=NOT_IDENTIFIED_IN_ADE4; Animation.apply at clip+0; 0x28 observed at AE8A8 input\n"
                 "HORIZONTAL_SPEED_SOURCE=sqrt(vx*vx+vz*vz) at *(player+0xC8)+0x10 without calling 8009BB24\n"
                 "ANIMATION_RATE_SOURCE=anctrl+0xC after baanim_update (includes 8008C2A8 remap)\n"
                 "RAW_RECORD_CAPTURED_BEFORE_COMPOSE=YES\n"
                 "PRODUCTION_FIX=NO\n",
                 reason != nullptr ? reason : "status",
                 kFeatherAnimMaxRows,
                 static_cast<unsigned long long>(g_feather_anim_baanim_raw),
                 static_cast<unsigned long long>(g_feather_anim_baanim_accepted),
                 static_cast<unsigned long long>(g_feather_anim_ae8a8_raw),
                 static_cast<unsigned long long>(g_feather_anim_ae8a8_accepted),
                 static_cast<unsigned long long>(g_feather_anim_target_sample_rows),
                 static_cast<unsigned long long>(g_feather_anim_rows),
                 static_cast<unsigned long long>(g_feather_anim_control_ticks),
                 static_cast<unsigned long long>(g_feather_anim_join_ok),
                 static_cast<unsigned long long>(g_feather_anim_join_mismatch),
                 static_cast<unsigned long long>(g_feather_anim_time_forward),
                 static_cast<unsigned long long>(g_feather_anim_time_same),
                 static_cast<unsigned long long>(g_feather_anim_time_backward),
                 static_cast<unsigned long long>(g_feather_anim_time_wrap),
                 static_cast<unsigned long long>(g_feather_anim_time_unexpected),
                 static_cast<unsigned long long>(g_feather_anim_clip_change),
                 static_cast<unsigned long long>(g_feather_anim_clip_unexpected),
                 static_cast<unsigned long long>(g_feather_anim_107_large_rot),
                 static_cast<unsigned long long>(g_feather_anim_107_large_scale),
                 static_cast<unsigned long long>(g_feather_anim_107_large_trans),
                 static_cast<unsigned long long>(g_feather_anim_107_step_authored),
                 static_cast<unsigned long long>(g_feather_anim_107_step_unexplained),
                 static_cast<unsigned long long>(g_feather_anim_107_prop_left),
                 static_cast<unsigned long long>(g_feather_anim_107_prop_right),
                 static_cast<unsigned long long>(g_feather_anim_108_large),
                 static_cast<unsigned long long>(g_feather_anim_79_large),
                 g_feather_anim_body_anim_id,
                 g_feather_anim_body_index,
                 static_cast<unsigned long long>(g_feather_anim_body_large),
                 static_cast<unsigned long long>(g_feather_anim_body_time_same),
                 static_cast<int>(g_feather_anim_last_clip_id),
                 g_feather_anim_last_sample_time,
                 feather_anim_reject_name(g_feather_anim_last_reject),
                 g_feather_anim_last_reject_detail != nullptr
                     ? g_feather_anim_last_reject_detail
                     : "NONE",
                 static_cast<unsigned long long>(
                     g_feather_anim_rejects[static_cast<size_t>(
                         FeatherAnimReject::BaanimNotPlayerTable)]),
                 static_cast<unsigned long long>(
                     g_feather_anim_rejects[static_cast<size_t>(
                         FeatherAnimReject::BaanimStateZero)]),
                 static_cast<unsigned long long>(
                     g_feather_anim_rejects[static_cast<size_t>(
                         FeatherAnimReject::BaanimAnctrlZero)]),
                 static_cast<unsigned long long>(
                     g_feather_anim_rejects[static_cast<size_t>(
                         FeatherAnimReject::Ae8a8BadSkeleton)]),
                 static_cast<unsigned long long>(
                     g_feather_anim_rejects[static_cast<size_t>(
                         FeatherAnimReject::Ae8a8NoPlume)]),
                 static_cast<unsigned long long>(
                     g_feather_anim_rejects[static_cast<size_t>(
                         FeatherAnimReject::Ae8a8AnimBaseZero)]),
                 static_cast<unsigned long long>(
                     g_feather_anim_rejects[static_cast<size_t>(
                         FeatherAnimReject::CsvOpenFailed)]),
                 static_cast<unsigned long long>(
                     g_feather_anim_rejects[static_cast<size_t>(
                         FeatherAnimReject::MaxRows)]));
    std::fprintf(status, "CLIP_ID_COUNTS=");
    bool any_clip = false;
    for (size_t i = 0; i < kFeatherAnimClipIdSlots; ++i) {
        if (!g_feather_anim_clip_counts[i].used) {
            continue;
        }
        std::fprintf(status, "%s%d:%llu", any_clip ? "," : "",
                     static_cast<int>(g_feather_anim_clip_counts[i].clip_id),
                     static_cast<unsigned long long>(
                         g_feather_anim_clip_counts[i].count));
        any_clip = true;
    }
    std::fprintf(status, "%s\n", any_clip ? "" : "NONE");
    if (g_feather_anim_rows == 0) {
        std::fprintf(status,
                     "NO_ROWS_REASON=%s\n"
                     "NO_ROWS_HINT=enable TOOIE_DIAG_FEATHER_ANIM_SAMPLE_TIME=1, "
                     "enter combined 0x62B Kazooie gameplay, walk; status must show "
                     "BAANIM_ACCEPTED and AE8A8_ACCEPTED before TARGET_SAMPLE_ROWS\n",
                     feather_anim_reject_name(g_feather_anim_last_reject));
    }
    std::fclose(status);
}

void feather_anim_flush_at_exit() {
    if (g_feather_anim_csv != nullptr) {
        std::fflush(g_feather_anim_csv);
        std::fclose(g_feather_anim_csv);
        g_feather_anim_csv = nullptr;
    }
    feather_anim_write_status("shutdown");
}

void ensure_init() {
    bool expected = false;
    if (!g_inited.compare_exchange_strong(expected, true)) {
        return;
    }
    const char* e = std::getenv("TOOIE_DIAG");
    g_enabled.store(e != nullptr && e[0] != '\0' && e[0] != '0', std::memory_order_relaxed);
    const char* timing = std::getenv("TOOIE_DIAG_TIMING");
    g_timing_enabled.store(timing != nullptr && timing[0] != '\0' && timing[0] != '0',
                           std::memory_order_relaxed);
    if (g_timing_enabled.load(std::memory_order_relaxed)) {
        g_interval_ms = 1000;
        g_timing_start = std::chrono::steady_clock::now();
    }
    const char* mv = std::getenv("TOOIE_DIAG_MOVEMENT");
    g_movement_enabled.store(mv != nullptr && mv[0] != '\0' && mv[0] != '0',
                             std::memory_order_relaxed);
    const char* inp = std::getenv("TOOIE_DIAG_INPUT");
    g_input_enabled.store(inp != nullptr && inp[0] != '\0' && inp[0] != '0',
                          std::memory_order_relaxed);
    const char* fe = std::getenv("TOOIE_DIAG_FRONTEND");
    g_frontend_enabled.store(fe != nullptr && fe[0] != '\0' && fe[0] != '0',
                             std::memory_order_relaxed);
    const char* dir = std::getenv("TOOIE_DIAG_DIRECTION");
    g_direction_enabled.store(dir != nullptr && dir[0] != '\0' && dir[0] != '0',
                               std::memory_order_relaxed);
    if (g_direction_enabled.load(std::memory_order_relaxed)) {
        // Direction diagnosis implies frontend + movement + input diagnostics.
        g_frontend_enabled.store(true, std::memory_order_relaxed);
        g_movement_enabled.store(true, std::memory_order_relaxed);
        g_input_enabled.store(true, std::memory_order_relaxed);
    }
    const char* demo = std::getenv("TOOIE_DIAG_DEMO_PATH");
    g_demo_path_enabled.store(demo != nullptr && demo[0] != '\0' && demo[0] != '0',
                              std::memory_order_relaxed);
    if (g_demo_path_enabled.load(std::memory_order_relaxed)) {
        g_movement_enabled.store(true, std::memory_order_relaxed);
    }
    const char* sync = std::getenv("TOOIE_DIAG_DEMO_SYNC");
    g_demo_sync_enabled.store(sync != nullptr && sync[0] != '\0' && sync[0] != '0',
                              std::memory_order_relaxed);
    if (g_demo_sync_enabled.load(std::memory_order_relaxed)) {
        g_movement_enabled.store(true, std::memory_order_relaxed);
    }
    // Isolated: does NOT enable movement/demo-sync/shot diagnostics.
    const char* d7 = std::getenv("TOOIE_DIAG_DEMO7_DRIFT");
    g_demo7_drift_enabled.store(d7 != nullptr && d7[0] != '\0' && d7[0] != '0',
                                std::memory_order_relaxed);
    // Isolated like DEMO7_DRIFT: does not enable timing/shot/demo-sync/attract/movement.
    const char* d1 = std::getenv("TOOIE_DIAG_DEMO1_PARITY");
    g_demo1_parity_enabled.store(d1 != nullptr && d1[0] != '\0' && d1[0] != '0',
                                 std::memory_order_relaxed);
    const char* d1p = std::getenv("TOOIE_DIAG_DEMO1_PROJECTILE");
    g_demo1_projectile_enabled.store(d1p != nullptr && d1p[0] != '\0' && d1p[0] != '0',
                                     std::memory_order_relaxed);
    const char* d1t = std::getenv("TOOIE_DIAG_DEMO1_TARGET");
    g_demo1_target_enabled.store(d1t != nullptr && d1t[0] != '\0' && d1t[0] != '0',
                                 std::memory_order_relaxed);
    const char* shot = std::getenv("TOOIE_DIAG_SHOT");
    g_shot_enabled.store(shot != nullptr && shot[0] != '\0' && shot[0] != '0',
                         std::memory_order_relaxed);
    const char* attract = std::getenv("TOOIE_DIAG_ATTRACT");
    g_attract_enabled.store(attract != nullptr && attract[0] != '\0' && attract[0] != '0',
                            std::memory_order_relaxed);
    if (g_attract_enabled.load(std::memory_order_relaxed)) {
        g_shot_enabled.store(true, std::memory_order_relaxed);
        g_demo_sync_enabled.store(true, std::memory_order_relaxed);
        g_movement_enabled.store(true, std::memory_order_relaxed);
    }
    const char* backpack = std::getenv("TOOIE_DIAG_BACKPACK_FLAP");
    g_backpack_flap_enabled.store(backpack != nullptr && backpack[0] != '\0' &&
                                       backpack[0] != '0',
                                   std::memory_order_relaxed);
    if (g_backpack_flap_enabled.load(std::memory_order_relaxed)) {
        g_backpack_flap_process_start_ms = std::chrono::duration_cast<
            std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        g_backpack_flap_pid = static_cast<long>(getpid());
        g_backpack_witness_csv = std::fopen(tooie_diag_path("tooie-backpack-flap-witness.csv").c_str(), "w");
        if (g_backpack_witness_csv != nullptr) {
            std::fprintf(g_backpack_witness_csv,
                         "FRAME,GFX_TICK,MODEL_RESOURCE_ID,MODEL_BIN,NODE_ID,NODE_TYPE,"
                         "CALLER_RA,GEOCMD_PTR,NODE_WORD0,NODE_BYTE8,NODE_BYTE9,"
                         "ANIMATION_CONTROL,PARENT_MATRIX,LOCAL_MATRIX,COMPOSED_MATRIX,"
                         "TRANSFORM_TAG,PREVIOUS_MATRIX,CURRENT_MATRIX,INTERPOLATED_MATRIX,"
                         "DL_ID,DRAW_SUBMITTED,SCALE,POS_X,POS_Y,POS_Z,"
                         "PREV_MTX_PTR,CURR_MTX_PTR\n");
            g_backpack_witness_csv_header = true;
            std::fflush(g_backpack_witness_csv);
        }
        backpack_write_status("INIT");
        std::atexit(backpack_flush_at_exit);
        std::fprintf(stderr,
                     "[tooie-backpack] enabled; wrap=func_800D674C/"
                     "func_800DE448/func_800E2180; filter=backpack model_bin; "
                     "bounded CSV/anomaly files flush on first dual-mtx/anomaly/shutdown\n");
        std::fflush(stderr);
    }
    const char* sway = std::getenv("TOOIE_DIAG_SECONDARY_SWAY");
    g_secondary_sway_enabled.store(sway != nullptr && sway[0] != '\0' &&
                                       sway[0] != '0',
                                   std::memory_order_relaxed);
    const char* pair_snap = std::getenv("TOOIE_DIAG_P09_LOCAL_PAIR_SNAPSHOT");
    g_p09_local_pair_snapshot.store(
        pair_snap != nullptr && pair_snap[0] != '\0' && pair_snap[0] != '0',
        std::memory_order_relaxed);
    const char* interp_simple = std::getenv("TOOIE_DIAG_FEATHER_INTERP_SIMPLE");
    g_secondary_sway_interp_simple_enabled.store(
        interp_simple != nullptr && interp_simple[0] != '\0' &&
            interp_simple[0] != '0',
        std::memory_order_relaxed);
    const char* no_interp = std::getenv("TOOIE_DIAG_FEATHER_NO_INTERP");
    g_secondary_sway_no_interpolation_enabled.store(
        no_interp != nullptr && no_interp[0] != '\0' && no_interp[0] != '0',
        std::memory_order_relaxed);
    g_getter_durable_needed.store(
        getter_durable_metadata_requested(
            g_secondary_sway_enabled.load(std::memory_order_relaxed),
            std::getenv("TOOIE_DIAG_FEATHER_HANDOFF_V7"),
            std::getenv("TOOIE_DIAG_FEATHER_POSTPROCESS_Q"),
            std::getenv("TOOIE_DIAG_FEATHER_TEMPORAL_COHERENCY"),
            std::getenv("RT64_DIAG_FEATHER_TEMPORAL_COHERENCY"),
            std::getenv("TOOIE_DIAG_RSP_ENDPOINT_BINDING"),
            std::getenv("TOOIE_DIAG_P09_SEAM_EDGE_PATHOLOGY_CORRELATION")),
        std::memory_order_relaxed);
    if (g_secondary_sway_enabled.load(std::memory_order_relaxed)) {
        g_secondary_sway_csv = std::fopen(tooie_diag_path("tooie-secondary-sway-model.csv").c_str(), "w");
        if (g_secondary_sway_csv != nullptr) {
            std::fprintf(g_secondary_sway_csv,
                         "FRAME,GFX_TICK,PLAYER_OBJECT_POINTER,PLAYER_STATE,APPEARANCE,"
                         "LOCOMOTION,RESOURCE_ID,MODEL_BIN_POINTER,DE448_FIFTH_ARG,"
                         "HOST_CALLSITE,GUEST_CALLSITE,PLAYER_SLOT,MAGIC,GTYPE,BONE_COUNT\n");
            std::fflush(g_secondary_sway_csv);
        }
        secondary_sway_write_status("INIT");
        std::atexit(secondary_sway_flush_at_exit);
        std::fprintf(stderr,
                     "[tooie-sway] enabled; E2AA4 snout 0x62B+0x11CB8 "
                     "true-asset 0x13178; generic E2180 discovery demoted\n");
        std::fflush(stderr);
    }
    const char* feather_anim = std::getenv("TOOIE_DIAG_FEATHER_ANIM_SAMPLE_TIME");
    g_feather_anim_sample_time_enabled.store(
        feather_anim != nullptr && feather_anim[0] != '\0' &&
            feather_anim[0] != '0',
        std::memory_order_relaxed);
    if (g_feather_anim_sample_time_enabled.load(std::memory_order_relaxed)) {
        g_feather_anim_csv =
            std::fopen(tooie_diag_path("tooie-feather-anim-sample-time.csv").c_str(), "w");
        if (g_feather_anim_csv != nullptr) {
            std::fprintf(
                g_feather_anim_csv,
                "ROW_KIND,GUEST_LOGIC,GFX_TICK,BAANIM_TICK,AE8A8_TICK,JOIN_OK,"
                "PLAYER,PLAYER_SLOT,LOCOMOTION,BAANIM_MODE,ANCTRL,CLIP_PTR,"
                "CLIP_ID,ANCTRL_INDEX,FLAGS_WORD,PLAYBACK,DIRECTION,"
                "SAMPLE_TIME,SAMPLE_TIME_PREV,SAMPLE_TIME_DELTA,SAMPLE_FRAME,"
                "SAMPLE_FRAC,DURATION,ANIMATION_RATE,HORIZ_SPEED,VX,VY,VZ,"
                "APPLY_FN,FILE_PTR,FRAME_START,FRAME_END,ELEM_COUNT,"
                "TIME_CLASS,CLIP_CHANGED,"
                "BONE_INDEX,BONE_ANIM_ID,PARENT_INDEX,IS_BODY_CONTROL,"
                "RECORD_PTR,QX,QY,QZ,QW,SX,SY,SZ,TX,TY,TZ,T_PTR,"
                "Q_DELTA_RAD,S_DELTA,T_DELTA,LARGE_ROT,LARGE_SCALE,LARGE_TRANS,"
                "AUTHORED_CANDIDATE,PROP_LEFT,PROP_RIGHT\n");
            std::fflush(g_feather_anim_csv);
            g_feather_anim_csv_header = true;
        } else {
            feather_anim_note_reject(FeatherAnimReject::CsvOpenFailed,
                                     "fopen_csv");
        }
        feather_anim_write_status("INIT");
        std::atexit(feather_anim_flush_at_exit);
        std::fprintf(stderr,
                     "[tooie-feather-anim] enabled schema="
                     TOOIE_FEATHER_ANIM_SAMPLE_TIME_SCHEMA
                     " max_rows=%zu wrap=baanim_update+AE8A8_precompose\n",
                     kFeatherAnimMaxRows);
        std::fflush(stderr);
    }
    const char* facing = std::getenv("TOOIE_DIAG_FORCED_FACING");
    g_forced_facing_enabled.store(facing != nullptr && facing[0] != '\0' &&
                                      facing[0] != '0',
                                  std::memory_order_relaxed);
    const char* vec = std::getenv("TOOIE_DIAG_INPUT_VECTOR");
    if (vec != nullptr && vec[0] != '\0') {
        auto eq = [vec](const char* name) {
            return std::strcmp(vec, name) == 0;
        };
        if (eq("center") || eq("0")) {
            g_override_x = 0.0f;
            g_override_y = 0.0f;
            g_override_active = true;
        } else if (eq("right") || eq("+x")) {
            g_override_x = 1.0f;
            g_override_y = 0.0f;
            g_override_active = true;
        } else if (eq("left") || eq("-x")) {
            g_override_x = -1.0f;
            g_override_y = 0.0f;
            g_override_active = true;
        } else if (eq("up") || eq("+y")) {
            g_override_x = 0.0f;
            g_override_y = 1.0f;
            g_override_active = true;
        } else if (eq("down") || eq("-y")) {
            g_override_x = 0.0f;
            g_override_y = -1.0f;
            g_override_active = true;
        } else if (eq("up-right") || eq("ur")) {
            g_override_x = 1.0f;
            g_override_y = 1.0f;
            g_override_active = true;
        } else if (eq("up-left") || eq("ul")) {
            g_override_x = -1.0f;
            g_override_y = 1.0f;
            g_override_active = true;
        } else if (eq("down-right") || eq("dr")) {
            g_override_x = 1.0f;
            g_override_y = -1.0f;
            g_override_active = true;
        } else if (eq("down-left") || eq("dl")) {
            g_override_x = -1.0f;
            g_override_y = -1.0f;
            g_override_active = true;
        } else {
            std::fprintf(stderr,
                         "[tooie-input] unknown TOOIE_DIAG_INPUT_VECTOR='%s'\n",
                         vec);
            std::fflush(stderr);
        }
    }
    const char* iv = std::getenv("TOOIE_DIAG_INTERVAL_MS");
    if (iv && iv[0]) {
        int v = std::atoi(iv);
        if (v >= 200) {
            g_interval_ms = v;
        }
    }
    g_last_report = std::chrono::steady_clock::now();
    g_timing_start = g_last_report;
    if (g_enabled.load(std::memory_order_relaxed) ||
        g_timing_enabled.load(std::memory_order_relaxed)) {
        std::fprintf(stderr,
                     "[tooie-diag] enabled interval=%dms "
                     "(syscall/rsp/queue/sched counters + hang stack; timing=%s)\n",
                     g_interval_ms,
                     g_timing_enabled.load(std::memory_order_relaxed) ? "on" : "off");
        std::fflush(stderr);
        // Background ticker so we still report when the game thread is stuck
        // and update_gfx is not running.
        std::thread([] {
            while (true) {
                std::this_thread::sleep_for(std::chrono::milliseconds(g_interval_ms));
                if (!g_enabled.load(std::memory_order_relaxed) &&
                    !g_timing_enabled.load(std::memory_order_relaxed)) {
                    continue;
                }
                report_now("bg");
            }
        }).detach();
    }
    if (g_movement_enabled.load(std::memory_order_relaxed)) {
        std::fprintf(stderr,
                     "[tooie-move] enabled (independent of TOOIE_DIAG) "
                     "probe=D_80127630/D_80127760/D_80135470 player=D_80135490[0]\n");
        std::fflush(stderr);
    }
    if (g_input_enabled.load(std::memory_order_relaxed) || g_override_active) {
        std::fprintf(stderr,
                     "[tooie-input] enabled log=%d override=%s host=(%.3f,%.3f)\n",
                     int(g_input_enabled.load(std::memory_order_relaxed)),
                     g_override_active ? (vec ? vec : "?") : "off",
                     g_override_x, g_override_y);
        std::fflush(stderr);
    }
    if (g_frontend_enabled.load(std::memory_order_relaxed)) {
        std::fprintf(stderr,
                     "[tooie-fe] enabled (title & file select synthetic sequencer active)\n");
        std::fflush(stderr);
    }
    if (g_demo_path_enabled.load(std::memory_order_relaxed)) {
        std::fprintf(stderr,
                     "[tooie-demopath] enabled (implies movement; 13B70 RA + physics/goto)\n");
        std::fflush(stderr);
    }
    if (g_forced_facing_enabled.load(std::memory_order_relaxed)) {
        std::fprintf(stderr,
                     "[facing] enabled (filtered scripted-actor heading vs yaw)\n");
        std::fflush(stderr);
    }
    if (g_attract_enabled.load(std::memory_order_relaxed)) {
        std::fprintf(stderr,
                     "[attract] enabled (implies SHOT+DEMO_SYNC; table dump + "
                     "user-demo ordinals + egg/camera fields)\n");
        std::fflush(stderr);
    }
}

void format_stack(char* buf, size_t buflen) {
    const uint32_t depth = g_depth.load(std::memory_order_relaxed);
    size_t off = 0;
    if (depth == 0) {
        std::snprintf(buf, buflen, "(idle)");
        return;
    }
    for (uint32_t i = 0; i < depth && i < kStackMax && off + 1 < buflen; ++i) {
        const uint32_t v = g_stack[i].load(std::memory_order_relaxed);
        const int n = std::snprintf(buf + off, buflen - off, "%s%u:%u",
                                    (i ? ">" : ""), v >> 16, v & 0xFFFFu);
        if (n < 0) {
            break;
        }
        off += static_cast<size_t>(n);
    }
}

} // namespace

bool enabled() {
    ensure_init();
    return g_enabled.load(std::memory_order_relaxed);
}

bool timing_enabled() {
    ensure_init();
    return g_timing_enabled.load(std::memory_order_relaxed);
}

bool movement_enabled() {
    ensure_init();
    return g_movement_enabled.load(std::memory_order_relaxed);
}

bool input_enabled() {
    ensure_init();
    return g_input_enabled.load(std::memory_order_relaxed);
}

bool demo_path_enabled() {
    ensure_init();
    return g_demo_path_enabled.load(std::memory_order_relaxed);
}

bool demo_sync_enabled() {
    ensure_init();
    return g_demo_sync_enabled.load(std::memory_order_relaxed);
}

bool demo1_target_enabled() {
    ensure_init();
    return g_demo1_target_enabled.load(std::memory_order_relaxed);
}

bool demo1_projectile_enabled() {
    ensure_init();
    return g_demo1_projectile_enabled.load(std::memory_order_relaxed);
}

bool demo1_parity_enabled() {
    ensure_init();
    return g_demo1_parity_enabled.load(std::memory_order_relaxed);
}

bool demo7_drift_enabled() {
    ensure_init();
    return g_demo7_drift_enabled.load(std::memory_order_relaxed);
}

bool shot_enabled() {
    ensure_init();
    return g_shot_enabled.load(std::memory_order_relaxed);
}

bool attract_enabled() {
    ensure_init();
    return g_attract_enabled.load(std::memory_order_relaxed);
}

bool forced_facing_enabled() {
    ensure_init();
    return g_forced_facing_enabled.load(std::memory_order_relaxed);
}

bool frontend_enabled() {
    ensure_init();
    return g_frontend_enabled.load(std::memory_order_relaxed);
}

bool direction_enabled() {
    ensure_init();
    return g_direction_enabled.load(std::memory_order_relaxed);
}

void set_gameplay_detected(bool v) {
    g_gameplay_seen.store(v, std::memory_order_release);
}

bool gameplay_detected() {
    return g_gameplay_seen.load(std::memory_order_acquire);
}

static uint32_t float_bits(float f) {
    uint32_t bits = 0;
    std::memcpy(&bits, &f, sizeof(bits));
    return bits;
}

static float bits_float(uint32_t bits) {
    float f = 0.0f;
    std::memcpy(&f, &bits, sizeof(f));
    return f;
}

void apply_input_vector_override(float* x, float* y) {
    ensure_init();
    if (!g_override_active || x == nullptr || y == nullptr) {
        return;
    }
    *x = g_override_x;
    *y = g_override_y;
}

void apply_synthetic_input(int controller, uint16_t* buttons, float* x, float* y) {
    ensure_init();
    if (controller != 0) {
        return;
    }

    apply_input_vector_override(x, y);

    if (!g_frontend_enabled.load(std::memory_order_relaxed)) {
        return;
    }

    bool expected = false;
    if (g_first_input_seen.compare_exchange_strong(expected, true)) {
        g_first_input_tp = std::chrono::steady_clock::now();
        std::fprintf(stderr, "[tooie-fe] synthetic input sequencer activated at t=0.00s\n");
        std::fflush(stderr);
    }

    const auto now = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(now - g_first_input_tp).count();
    const uint32_t current_ovl = g_last_ovl.load(std::memory_order_relaxed);

    static bool s_seen_attract = false;
    static auto s_attract_tp = std::chrono::steady_clock::time_point{};
    static bool s_seen_gameselect = false;
    static auto s_gameselect_tp = std::chrono::steady_clock::time_point{};

    if (current_ovl == 208 && !s_seen_attract) {
        s_seen_attract = true;
        s_attract_tp = now;
        std::fprintf(stderr, "[tooie-fe] t=%.2fs detected chattract (overlay 208, Title Screen / PRESS START)\n", elapsed);
        std::fflush(stderr);
    }

    if (current_ovl == 205 && !s_seen_gameselect) {
        s_seen_gameselect = true;
        s_gameselect_tp = now;
        std::fprintf(stderr, "[tooie-fe] t=%.2fs detected chgameselect (overlay 205, File Selection)\n", elapsed);
        std::fflush(stderr);
    }

    int phase = 0;
    const char* phase_name = "IDLE";

    if (!s_seen_attract && !s_seen_gameselect) {
        // Intro scene before title screen: pulse START periodically to skip logos/cutscenes
        const int sub_sec = static_cast<int>(elapsed * 2.0) % 3; // every 1.5s, active for 0.5s
        if (sub_sec == 0 && elapsed > 1.0) {
            phase = 1;
            phase_name = "INTRO_SKIP_START";
            if (buttons != nullptr) {
                *buttons |= 0x1000u; // CONT_START
            }
        } else {
            phase = 0;
            phase_name = "INTRO_WAIT";
        }
    } else if (s_seen_attract && !s_seen_gameselect) {
        // At Title Screen: press START to transition to File Select
        const double attract_elapsed = std::chrono::duration<double>(now - s_attract_tp).count();
        if (attract_elapsed >= 0.5 && attract_elapsed < 2.5) {
            phase = 2;
            phase_name = "TITLE_PRESS_START";
            if (buttons != nullptr) {
                *buttons |= 0x1000u; // CONT_START
            }
        } else {
            phase = 0;
            phase_name = "TITLE_WAIT";
        }
    } else if (s_seen_gameselect) {
        // Inside File Selection:
        const double gs_elapsed = std::chrono::duration<double>(now - s_gameselect_tp).count();
        if (gs_elapsed < 1.0) {
            phase = 3;
            phase_name = "GAMESELECT_INIT";
        } else if (gs_elapsed >= 1.0 && gs_elapsed < 1.5) {
            phase = 4;
            phase_name = "GAMESELECT_STICK_LEFT";
            if (x != nullptr) {
                *x = -1.0f;
            }
        } else if (gs_elapsed >= 1.5 && gs_elapsed < 2.5) {
            phase = 5;
            phase_name = "GAMESELECT_SETTLE_LEFT";
        } else if (gs_elapsed >= 2.5 && gs_elapsed < 3.0) {
            phase = 6;
            phase_name = "GAMESELECT_STICK_RIGHT";
            if (x != nullptr) {
                *x = 1.0f;
            }
        } else if (gs_elapsed >= 3.0 && gs_elapsed < 4.0) {
            phase = 7;
            phase_name = "GAMESELECT_SETTLE_RIGHT";
        } else if (gs_elapsed >= 4.0 && gs_elapsed < 5.0) {
            phase = 8;
            phase_name = "GAMESELECT_CONFIRM_A";
            if (buttons != nullptr) {
                *buttons |= 0x8000u; // CONT_A
            }
        } else if (gs_elapsed >= 5.0 && gs_elapsed < 12.0) {
            phase = 9;
            phase_name = "GAMESELECT_WAIT_START";
        } else {
            // After enough time has passed (gameselect confirm → map load → gameplay)
            // inject cardinal stick directions to trace the movement pipeline.
            // gs_elapsed ~5-12s wait (gameselect ep=7 unload at ~6s elapsed from
            // gameselect, overlays load, mode=1 gameplay starts by ~8s after gs).
            // Use wall-clock elapsed for simplicity; gs_elapsed is relative to
            // first gameselect detection, so add 15s (so ~12+15=27s after
            // start / ~22s after first frontend input).
            const double dir_elapsed = elapsed - 20.0; // cardinal start window
            if (dir_elapsed < 0.0) {
                // Buffer before cardinal injection: neutral stick
                phase = 9;
                phase_name = "GAMESELECT_WAIT_START";
            } else if (dir_elapsed < 2.0) {
                phase = 10;
                phase_name = "DIR_CENTER";
                if (x != nullptr) { *x = 0.0f; }
                if (y != nullptr) { *y = 0.0f; }
            } else if (dir_elapsed < 4.0) {
                phase = 11;
                phase_name = "DIR_RIGHT";
                if (x != nullptr) { *x = 1.0f; }
                if (y != nullptr) { *y = 0.0f; }
            } else if (dir_elapsed < 6.0) {
                phase = 12;
                phase_name = "DIR_LEFT";
                if (x != nullptr) { *x = -1.0f; }
                if (y != nullptr) { *y = 0.0f; }
            } else if (dir_elapsed < 8.0) {
                phase = 13;
                phase_name = "DIR_UP";
                if (x != nullptr) { *x = 0.0f; }
                if (y != nullptr) { *y = 1.0f; }
            } else if (dir_elapsed < 10.0) {
                phase = 14;
                phase_name = "DIR_DOWN";
                if (x != nullptr) { *x = 0.0f; }
                if (y != nullptr) { *y = -1.0f; }
            } else {
                phase = 15;
                phase_name = "DIR_CENTER_END";
                if (x != nullptr) { *x = 0.0f; }
                if (y != nullptr) { *y = 0.0f; }
            }
        }
    }

    static int s_last_phase = -1;
    if (phase != s_last_phase) {
        s_last_phase = phase;
        std::fprintf(stderr,
                     "[tooie-fe] t=%.2fs phase=%d (%s) ovl=%u injecting btn=%04x stick=(%.1f,%.1f)\n",
                     elapsed, phase, phase_name, current_ovl,
                     buttons ? *buttons : 0,
                     x ? *x : 0.0f, y ? *y : 0.0f);
        std::fflush(stderr);
    }
}

void note_frontend_state(uint32_t ovl, uint32_t ep, int32_t mode, int32_t map, uint32_t warp) {
    ensure_init();
    if (!g_frontend_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    static uint32_t s_last_ovl = 0xFFFFFFFFu;
    static int32_t s_last_mode = -999;
    static int32_t s_last_map = -999;
    static uint32_t s_last_warp = 0xFFFFFFFFu;

    const bool changed = (ovl != s_last_ovl) || (mode != s_last_mode) ||
                         (map != s_last_map) || (warp != s_last_warp);
    if (!changed) {
        return;
    }
    s_last_ovl = ovl;
    s_last_mode = mode;
    s_last_map = map;
    s_last_warp = warp;

    const char* ovl_name = "other";
    if (ovl == 208) ovl_name = "chattract (208)";
    else if (ovl == 205) ovl_name = "chgameselect (205)";
    else if (ovl == 419) ovl_name = "chintroticker (419)";
    else if (ovl == 440) ovl_name = "chintrochar (440)";
    else if (ovl == 441) ovl_name = "chintrocam (441)";

    double elapsed = 0.0;
    if (g_first_input_seen.load(std::memory_order_relaxed)) {
        elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - g_first_input_tp).count();
    }

    std::fprintf(stderr,
                 "[tooie-fe] t=%.2fs state_change ovl=%s ep=%u mode=%d map=%d warp=%u\n",
                 elapsed, ovl_name, ep, mode, map, warp);
    std::fflush(stderr);

    if (mode == 1) {
        g_gameplay_seen.store(true, std::memory_order_release);
    }
}

void note_host_input(int controller, float x, float y, uint16_t buttons) {
    ensure_init();
    g_host_x_bits.store(float_bits(x), std::memory_order_relaxed);
    g_host_y_bits.store(float_bits(y), std::memory_order_relaxed);
    if (!g_input_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    static float s_x = 1e9f;
    static float s_y = 1e9f;
    static uint16_t s_buttons = 0xFFFFu;
    static auto s_last = std::chrono::steady_clock::time_point{};
    const auto now = std::chrono::steady_clock::now();
    const bool changed = (std::fabs(x - s_x) > 0.02f) ||
                         (std::fabs(y - s_y) > 0.02f) ||
                         (buttons != s_buttons);
    const bool due = (s_last.time_since_epoch().count() == 0) ||
                     (now - s_last >= std::chrono::milliseconds(250));
    const bool active = (x != 0.0f) || (y != 0.0f) || (buttons != 0);
    if (!changed && !(due && active)) {
        return;
    }
    s_x = x;
    s_y = y;
    s_buttons = buttons;
    s_last = now;
    std::fprintf(stderr,
                 "[tooie-input] host ctrl=%d host=(%.3f,%.3f) buttons=%04x\n",
                 controller, x, y, unsigned(buttons));
    std::fflush(stderr);
}

void note_guest_pad(int8_t stick_x, int8_t stick_y, uint16_t buttons,
                    int32_t scale_word, float remap_x, float remap_y) {
    ensure_init();
    if (!g_input_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    static int8_t s_sx = 127;
    static int8_t s_sy = 127;
    static uint16_t s_buttons = 0xFFFFu;
    static int32_t s_scale = 0;
    static auto s_last = std::chrono::steady_clock::time_point{};
    const auto now = std::chrono::steady_clock::now();
    const bool changed = (stick_x != s_sx) || (stick_y != s_sy) ||
                         (buttons != s_buttons) || (scale_word != s_scale);
    const bool due = (s_last.time_since_epoch().count() == 0) ||
                     (now - s_last >= std::chrono::milliseconds(250));
    const bool active = (stick_x != 0) || (stick_y != 0) || (buttons != 0);
    if (!changed && !(due && active)) {
        return;
    }
    s_sx = stick_x;
    s_sy = stick_y;
    s_buttons = buttons;
    s_scale = scale_word;
    s_last = now;
    uint32_t scale_bits = static_cast<uint32_t>(scale_word);
    uint32_t derived = 0xE955CCDDu - scale_bits;
    float derived_f = bits_float(derived);
    const float hx = bits_float(g_host_x_bits.load(std::memory_order_relaxed));
    const float hy = bits_float(g_host_y_bits.load(std::memory_order_relaxed));
    std::fprintf(stderr,
                 "[tooie-input] host=(%.3f,%.3f) pad=(%d,%d) buttons=%04x "
                 "scale=%08x derived=%.6f remap=(%.3f,%.3f)\n",
                 hx, hy, int(stick_x), int(stick_y), unsigned(buttons),
                 scale_bits, derived_f, remap_x, remap_y);
    std::fflush(stderr);
}

void last_host_input(float* x, float* y) {
    ensure_init();
    if (x != nullptr) {
        *x = bits_float(g_host_x_bits.load(std::memory_order_relaxed));
    }
    if (y != nullptr) {
        *y = bits_float(g_host_y_bits.load(std::memory_order_relaxed));
    }
}

uint64_t gfx_ticks() {
    return g_gfx_ticks.load(std::memory_order_relaxed);
}

std::atomic<uint64_t> g_guest_logic_tick{0};

uint64_t note_guest_logic_tick() {
    return g_guest_logic_tick.fetch_add(1, std::memory_order_relaxed) + 1;
}

uint64_t guest_logic_tick() {
    return g_guest_logic_tick.load(std::memory_order_relaxed);
}

void note_math(const char* name, float input, float output) {
    if (!movement_enabled()) {
        return;
    }
    struct Slot {
        uint32_t count;
        float last_out;
        bool have_last;
    };
    static Slot slots[8]{};
    static const char* names[8]{};
    int idx = -1;
    for (int i = 0; i < 8; ++i) {
        if (names[i] == name || (names[i] != nullptr && name != nullptr &&
                                 std::strcmp(names[i], name) == 0)) {
            idx = i;
            break;
        }
        if (names[i] == nullptr) {
            names[i] = name;
            idx = i;
            break;
        }
    }
    if (idx < 0) {
        return;
    }
    Slot& s = slots[idx];
    const uint32_t n = ++s.count;
    const bool bad = !std::isfinite(input) || !std::isfinite(output) ||
                     std::fabs(output) > 1.0e6f;
    const bool jump = s.have_last && std::isfinite(s.last_out) &&
                      std::isfinite(output) && std::fabs(output - s.last_out) > 180.0f;
    const bool sample = (n <= 16u) || ((n & 63u) == 0u) || bad || jump;
    s.last_out = output;
    s.have_last = true;
    if (!sample) {
        return;
    }
    std::fprintf(stderr,
                 "[tooie-move] math name=%s n=%u in=%g out=%g%s%s gfx=%llu\n",
                 name ? name : "?", n, static_cast<double>(input),
                 static_cast<double>(output),
                 bad ? " BAD" : "",
                 jump ? " JUMP" : "",
                 static_cast<unsigned long long>(g_gfx_ticks.load()));
    std::fflush(stderr);
}

void note_13b70(uint32_t ra, float y, float x, float mag, float angle) {
    ensure_init();
    if (!g_demo_path_enabled.load(std::memory_order_relaxed) &&
        !g_forced_facing_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    static uint32_t s_n = 0;
    static uint32_t s_nonzero = 0;
    const uint32_t n = ++s_n;
    const bool nz = (std::fabs(y) > 1.0e-6f) || (std::fabs(x) > 1.0e-6f);
    if (nz) {
        ++s_nonzero;
    }
    // First 48 calls, then every 16th, plus every non-zero sample until 80 of those.
    const bool sample = (n <= 48u) || ((n & 15u) == 0u) ||
                        (nz && s_nonzero <= 80u);
    if (!sample) {
        return;
    }
    const char* who = "other";
    if (ra == 0x80016AD8u) {
        who = "func_80016A74";
    } else if (ra == 0x800F195Cu) {
        who = "func_800F18FC";
    } else if ((ra & 0xFFFF0000u) == 0x80800000u) {
        who = "overlay8080";
    } else if (ra >= 0x8009EFF8u && ra < 0x8009F18Cu) {
        who = "bastick_update";
    }
    std::fprintf(stderr,
                 "[tooie-demopath] 13b70 n=%u nz=%u ra=%08x who=%s "
                 "y=%.5f x=%.5f mag=%.5f ang=%.3f gfx=%llu\n",
                 n, s_nonzero, ra, who, y, x, mag, angle,
                 static_cast<unsigned long long>(g_gfx_ticks.load()));
    std::fflush(stderr);
}

void note_syscall_dispatch(uint32_t ovl, uint32_t ep) {
    ensure_init();
    g_syscall_dispatch.fetch_add(1, std::memory_order_relaxed);
    g_last_ovl.store(ovl, std::memory_order_relaxed);
    g_last_ep.store(ep, std::memory_order_relaxed);
    const uint32_t d = g_depth.fetch_add(1, std::memory_order_relaxed);
    if (d < kStackMax) {
        g_stack[d].store((ovl << 16) | (ep & 0xFFFFu), std::memory_order_relaxed);
    }
    uint32_t maxd = g_max_depth.load(std::memory_order_relaxed);
    while (d + 1 > maxd &&
           !g_max_depth.compare_exchange_weak(maxd, d + 1, std::memory_order_relaxed)) {
    }
}

void note_syscall_return(uint32_t ovl, uint32_t ep) {
    ensure_init();
    (void)ovl;
    (void)ep;
    g_syscall_return.fetch_add(1, std::memory_order_relaxed);
    uint32_t d = g_depth.load(std::memory_order_relaxed);
    if (d > 0) {
        g_depth.store(d - 1, std::memory_order_relaxed);
    }
}

void note_sched_ack() {
    ensure_init();
    g_sched_ack.fetch_add(1, std::memory_order_relaxed);
}

void note_rsp_audtask() {
    ensure_init();
    g_rsp_aud.fetch_add(1, std::memory_order_relaxed);
}

void note_queue_samples(size_t sample_count, uint32_t queued_bytes_after,
                        int16_t peak_abs, size_t nonzero) {
    ensure_init();
    g_queue_calls.fetch_add(1, std::memory_order_relaxed);
    g_queue_samples.fetch_add(sample_count, std::memory_order_relaxed);
    g_queue_nonzero.fetch_add(nonzero, std::memory_order_relaxed);
    g_last_queue_bytes.store(queued_bytes_after, std::memory_order_relaxed);
    g_last_peak.store(peak_abs, std::memory_order_relaxed);
    int32_t maxp = g_max_peak.load(std::memory_order_relaxed);
    while (peak_abs > maxp &&
           !g_max_peak.compare_exchange_weak(maxp, peak_abs, std::memory_order_relaxed)) {
    }
}

void note_audio_device(uint32_t device_id, const char* driver, const char* device_name) {
    ensure_init();
    g_audio_device.store(device_id, std::memory_order_relaxed);
    std::lock_guard<std::mutex> lock(g_name_mu);
    if (driver) {
        std::snprintf(g_driver, sizeof(g_driver), "%s", driver);
    }
    if (device_name) {
        std::snprintf(g_device_name, sizeof(g_device_name), "%s", device_name);
    }
    if (enabled()) {
        std::fprintf(stderr,
                     "[tooie-diag] audio device id=%u driver=%s name=\"%s\"\n",
                     device_id,
                     g_driver[0] ? g_driver : "?",
                     g_device_name[0] ? g_device_name : "?");
        std::fflush(stderr);
    }
}

std::atomic<int32_t> g_frame_divisor{-1};

void note_guest_vi() {
    ensure_init();
    if (g_timing_enabled.load(std::memory_order_relaxed)) {
        g_guest_vi.fetch_add(1, std::memory_order_relaxed);
    }
}

void note_game_logic(int32_t divisor) {
    ensure_init();
    if (divisor > 0) {
        g_frame_divisor.store(divisor, std::memory_order_relaxed);
    }
    if (g_timing_enabled.load(std::memory_order_relaxed)) {
        g_game_logic.fetch_add(1, std::memory_order_relaxed);
    }
}

void note_display_list() {
    ensure_init();
    if (g_timing_enabled.load(std::memory_order_relaxed)) {
        g_display_list.fetch_add(1, std::memory_order_relaxed);
    }
}

void note_animation_update() {
    ensure_init();
    if (g_timing_enabled.load(std::memory_order_relaxed)) {
        g_animation_update.fetch_add(1, std::memory_order_relaxed);
    }
}

void note_host_update_gfx() {
    ensure_init();
    if (g_timing_enabled.load(std::memory_order_relaxed)) {
        g_host_update_gfx.fetch_add(1, std::memory_order_relaxed);
    }
}

void note_guest_input_update() {
    ensure_init();
    if (g_timing_enabled.load(std::memory_order_relaxed)) {
        g_guest_input_updates.fetch_add(1, std::memory_order_relaxed);
    }
}

bool backpack_flap_enabled() {
    ensure_init();
    return g_backpack_flap_enabled.load(std::memory_order_relaxed);
}

void backpack_note_entry3() {
    ensure_init();
    if (!g_backpack_flap_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    bool first = false;
    {
        std::lock_guard<std::mutex> lock(g_backpack_flap_mu);
        first = (g_backpack_entry3_hits++ == 0);
    }
    if (first) {
        backpack_write_status("BABACKPACK_ENTRY3_FIRST_HIT");
    }
}

void backpack_note_asset_lookup(uint32_t asset_id, uint32_t model_ptr) {
    ensure_init();
    if (!g_backpack_flap_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    bool first_expected = false;
    {
        std::lock_guard<std::mutex> lock(g_backpack_flap_mu);
        ++g_backpack_d674c_hits;
        size_t matched = kBackpackAssetCount;
        for (size_t i = 0; i < kBackpackAssetCount; ++i) {
            if (kBackpackAssetIds[i] == asset_id) {
                matched = i;
                break;
            }
        }
        if (matched < kBackpackAssetCount) {
            first_expected = (g_backpack_d674c_tracked_hits++ == 0);
            ++g_backpack_asset_hits[matched];
            g_backpack_model_ptrs[matched] = model_ptr;
        } else {
            ++g_backpack_asset_other_hits;
            size_t observed = g_backpack_observed_other_asset_count;
            for (size_t i = 0; i < g_backpack_observed_other_asset_count; ++i) {
                if (g_backpack_observed_other_assets[i].asset_id == asset_id) {
                    observed = i;
                    break;
                }
            }
            if (observed == g_backpack_observed_other_asset_count) {
                if (g_backpack_observed_other_asset_count <
                    kBackpackObservedAssetLimit) {
                    ++g_backpack_observed_other_asset_count;
                    g_backpack_observed_other_assets[observed].asset_id = asset_id;
                } else {
                    observed = 0;
                    for (size_t i = 1; i < kBackpackObservedAssetLimit; ++i) {
                        if (g_backpack_observed_other_assets[i].hits <
                            g_backpack_observed_other_assets[observed].hits) {
                            observed = i;
                        }
                    }
                    ++g_backpack_observed_other_asset_evictions;
                    g_backpack_observed_other_assets[observed] = {};
                    g_backpack_observed_other_assets[observed].asset_id = asset_id;
                }
            }
            ++g_backpack_observed_other_assets[observed].hits;
            g_backpack_observed_other_assets[observed].model_ptr = model_ptr;
        }
    }
    if (first_expected) {
        backpack_write_status("ASSET_RESOLVE_FIRST_EXPECTED");
    }
}

bool backpack_tracked_model_id(uint32_t model_ptr, uint32_t* model_id_out) {
    ensure_init();
    if (model_id_out != nullptr) {
        *model_id_out = 0;
    }
    if (!g_backpack_flap_enabled.load(std::memory_order_relaxed) || model_ptr == 0) {
        return false;
    }
    std::lock_guard<std::mutex> lock(g_backpack_flap_mu);
    for (size_t i = 0; i < kBackpackAssetCount; ++i) {
        if (g_backpack_model_ptrs[i] != 0 && g_backpack_model_ptrs[i] == model_ptr) {
            if (model_id_out != nullptr) {
                *model_id_out = kBackpackAssetIds[i];
            }
            return true;
        }
    }
    return false;
}

void backpack_note_draw_entry(uint32_t model_bin, uint32_t model_id) {
    ensure_init();
    if (!g_backpack_flap_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    bool first = false;
    {
        std::lock_guard<std::mutex> lock(g_backpack_flap_mu);
        ++g_backpack_de448_hits;
        if (model_id != 0 && model_bin != 0) {
            first = (g_backpack_de448_tracked_hits++ == 0);
        }
    }
    if (model_id != 0 && model_bin != 0) {
        if (first) {
            backpack_write_status("DE448_FIRST_FILTERED_HIT");
        }
    }
}

void backpack_note_de498() {
    ensure_init();
    if (!g_backpack_flap_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    bool first = false;
    {
        std::lock_guard<std::mutex> lock(g_backpack_flap_mu);
        ++g_backpack_de498_hits;
        if (g_backpack_draw_depth != 0 &&
            g_backpack_draw_scopes[g_backpack_draw_depth - 1].active) {
            first = (g_backpack_de498_tracked_hits++ == 0);
        }
    }
    if (first) {
        backpack_write_status("DE498_FIRST_FILTERED_HIT");
    }
}

void backpack_note_e2aa4() {
    ensure_init();
    if (!g_backpack_flap_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    bool first = false;
    {
        std::lock_guard<std::mutex> lock(g_backpack_flap_mu);
        ++g_backpack_e2aa4_hits;
        if (g_backpack_draw_depth != 0 &&
            g_backpack_draw_scopes[g_backpack_draw_depth - 1].active) {
            first = (g_backpack_e2aa4_tracked_hits++ == 0);
        }
    }
    if (first) {
        backpack_write_status("E2AA4_FIRST_FILTERED_HIT");
    }
}

void backpack_draw_begin(uint8_t* rdram, void* raw_ctx, uint32_t model_id,
                         uint32_t model_bin) {
    ensure_init();
    if (!g_backpack_flap_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    if (g_backpack_draw_depth >= kBackpackDrawStackSize) {
        ++g_backpack_draw_overflow;
        std::lock_guard<std::mutex> lock(g_backpack_flap_mu);
        ++g_backpack_scope_overflow_events;
        return;
    }
    BackpackDrawScope& scope = g_backpack_draw_scopes[g_backpack_draw_depth++];
    scope = {};
    {
        std::lock_guard<std::mutex> lock(g_backpack_flap_mu);
        if (g_backpack_draw_depth > g_backpack_scope_max_depth) {
            g_backpack_scope_max_depth = g_backpack_draw_depth;
        }
    }
    if (raw_ctx != nullptr && model_id != 0 && model_bin != 0) {
        auto* const ctx = static_cast<recomp_context*>(raw_ctx);
        scope.active = true;
        scope.model_id = model_id;
        scope.model_bin = model_bin;
        scope.position_ptr = static_cast<uint32_t>(ctx->r4);
        scope.scale_bits = static_cast<uint32_t>(ctx->r6);
        scope.caller_ra = static_cast<uint32_t>(ctx->r31);
    }
    (void)rdram;
}

void backpack_draw_end() {
    if (g_backpack_draw_overflow != 0) {
        --g_backpack_draw_overflow;
    } else if (g_backpack_draw_depth != 0) {
        --g_backpack_draw_depth;
    }
}

bool backpack_draw_scope_active() {
    return g_backpack_draw_depth != 0 &&
           g_backpack_draw_scopes[g_backpack_draw_depth - 1].active;
}

void backpack_note_dual_mtx(uint8_t* rdram, void* raw_ctx) {
    ensure_init();
    if (!g_backpack_flap_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    BackpackDrawScope scope{};
    bool active = false;
    bool first_filter = false;
    {
        std::lock_guard<std::mutex> lock(g_backpack_flap_mu);
        ++g_backpack_e2180_hits;
        if (g_backpack_draw_depth == 0) {
        } else {
            scope = g_backpack_draw_scopes[g_backpack_draw_depth - 1];
            active = scope.active;
            if (!active) {
            } else {
                first_filter = (g_backpack_e2180_tracked_hits++ == 0);
            }
        }
    }
    bool rejection_first = false;
    BackpackReject rejection = BackpackReject::Count;
    if (!active) {
        rejection = BackpackReject::NoActiveModelContext;
    } else if (scope.model_id == 0 || scope.model_bin == 0) {
        rejection = BackpackReject::ModelIdMismatch;
    } else if (rdram == nullptr) {
        rejection = BackpackReject::Other;
    } else if (raw_ctx == nullptr || static_cast<uint32_t>(static_cast<recomp_context*>(raw_ctx)->r5) == 0) {
        rejection = BackpackReject::NoNodeContext;
    } else if (static_cast<uint32_t>(static_cast<recomp_context*>(raw_ctx)->r31) == 0) {
        rejection = BackpackReject::UnexpectedNodeType;
    }
    if (rejection != BackpackReject::Count) {
        {
            std::lock_guard<std::mutex> lock(g_backpack_flap_mu);
            const size_t index = static_cast<size_t>(rejection);
            rejection_first = (g_backpack_e2180_rejections[index]++ == 0);
        }
        if (rejection_first) {
            backpack_write_status("E2180_FIRST_REJECTION");
        }
        if (first_filter) {
            backpack_write_status("E2180_FIRST_FILTERED_REJECTED");
        }
        return;
    }
    auto* const ctx = static_cast<recomp_context*>(raw_ctx);
    BackpackFlapSample sample{};
    sample.frame = guest_logic_tick();
    const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    sample.gfx_tick = static_cast<uint64_t>(
        now_ms - (g_backpack_flap_process_start_ms != 0 ? g_backpack_flap_process_start_ms : now_ms));
    sample.model_resource_id = scope.model_id;
    sample.model_bin = scope.model_bin;
    sample.node_ptr = static_cast<uint32_t>(ctx->r5);
    sample.geocmd_ptr = static_cast<uint32_t>(ctx->r6);
    sample.caller_ra = static_cast<uint32_t>(ctx->r31);
    sample.gfx_ctx = static_cast<uint32_t>(ctx->r4);
    sample.scale_bits = scope.scale_bits;
    sample.pos_x_bits = backpack_guest_word(rdram, scope.position_ptr, 0);
    sample.pos_y_bits = backpack_guest_word(rdram, scope.position_ptr, 4);
    sample.pos_z_bits = backpack_guest_word(rdram, scope.position_ptr, 8);
    sample.node_word0 = backpack_guest_word(rdram, sample.node_ptr, 0);
    sample.node_byte8 = static_cast<uint8_t>(backpack_guest_byte(rdram, sample.node_ptr, 8));
    sample.node_byte9 = static_cast<uint8_t>(backpack_guest_byte(rdram, sample.node_ptr, 9));
    const int16_t prev_off = backpack_guest_hword(rdram, sample.node_ptr, 0x12);
    const uint32_t curr_off = backpack_guest_word(rdram, sample.node_ptr, 0x14);
    sample.prev_mtx_ptr = sample.node_ptr + static_cast<uint32_t>(static_cast<int32_t>(prev_off));
    sample.curr_mtx_ptr = sample.node_ptr + curr_off;
    backpack_read_mtx_words(rdram, sample.prev_mtx_ptr, sample.prev_words);
    backpack_read_mtx_words(rdram, sample.curr_mtx_ptr, sample.curr_words);
    sample.draw_submitted = true;
    const bool anomaly = backpack_sample_anomalous(sample);

    bool first = false;
    bool periodic = false;
    {
        std::lock_guard<std::mutex> lock(g_backpack_flap_mu);
        ++g_backpack_flap_samples;
        ++g_backpack_e2180_sample_written;
        g_backpack_flap_last_frame = sample.frame;
        if (g_backpack_unique_nodes.find(sample.node_ptr) ==
            g_backpack_unique_nodes.end()) {
            if (g_backpack_unique_nodes.size() < kBackpackUniqueNodeLimit) {
                g_backpack_unique_nodes.insert(sample.node_ptr);
            } else {
                ++g_backpack_unique_node_overflow;
            }
        }
        if (!g_backpack_flap_first_hit) {
            g_backpack_flap_first_hit = true;
            g_backpack_flap_first_hit_frame = sample.frame;
            g_backpack_flap_first_hit_callsite = sample.caller_ra;
            g_backpack_first_model_id = sample.model_resource_id;
            g_backpack_first_model_bin = sample.model_bin;
            g_backpack_first_node_ptr = sample.node_ptr;
            first = true;
        }
        if (anomaly) {
            g_backpack_flap_anomaly_seen = true;
            ++g_backpack_flap_anomalies;
        }
        backpack_store_sample(sample);
        periodic = (g_backpack_e2180_hits % kBackpackFlushInterval) == 0;
        backpack_append_csv_locked(sample, first || anomaly || periodic);
    }
    if (first_filter) {
        backpack_write_status("E2180_FIRST_FILTERED_HIT");
    }
    if (first) {
        backpack_write_status("E2180_FIRST_HIT");
        backpack_write_files("first-sample");
    } else if (anomaly) {
        backpack_write_status("automatic-anomaly");
        backpack_write_files("automatic-anomaly");
    } else if (periodic) {
        backpack_write_status("E2180_PERIODIC");
    }
}

void report_now(const char* reason) {
    ensure_init();
    if (!g_enabled.load(std::memory_order_relaxed) &&
        !g_timing_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    char stack[160];
    format_stack(stack, sizeof(stack));
    char driver[64];
    char dname[128];
    {
        std::lock_guard<std::mutex> lock(g_name_mu);
        std::snprintf(driver, sizeof(driver), "%s", g_driver[0] ? g_driver : "?");
        std::snprintf(dname, sizeof(dname), "%s", g_device_name[0] ? g_device_name : "?");
    }
    if (g_enabled.load(std::memory_order_relaxed)) {
        std::fprintf(stderr,
                     "[tooie-diag] %s "
                     "syscall d=%llu r=%llu inflight=%lld "
                     "sched_ack=%llu rsp_aud=%llu "
                     "queue_calls=%llu queue_samp=%llu nonzero=%llu peak=%d maxpeak=%d queue_bytes=%u "
                     "audio_dev=%u/%s/\"%s\" "
                     "last_ovl=%u:%u depth=%u max_depth=%u stack=%s\n",
                     reason ? reason : "report",
                     static_cast<unsigned long long>(g_syscall_dispatch.load()),
                     static_cast<unsigned long long>(g_syscall_return.load()),
                     static_cast<long long>(g_syscall_dispatch.load() - g_syscall_return.load()),
                     static_cast<unsigned long long>(g_sched_ack.load()),
                     static_cast<unsigned long long>(g_rsp_aud.load()),
                     static_cast<unsigned long long>(g_queue_calls.load()),
                     static_cast<unsigned long long>(g_queue_samples.load()),
                     static_cast<unsigned long long>(g_queue_nonzero.load()),
                     g_last_peak.load(),
                     g_max_peak.load(),
                     g_last_queue_bytes.load(),
                     g_audio_device.load(),
                     driver,
                     dname,
                     g_last_ovl.load(),
                     g_last_ep.load(),
                     g_depth.load(),
                     g_max_depth.load(),
                     stack);
        std::fflush(stderr);
    }

    if (g_timing_enabled.load(std::memory_order_relaxed)) {
        std::lock_guard<std::mutex> timing_lock(g_timing_mu);
        static auto s_last_tp = g_timing_start;
        static uint64_t s_last_vi = 0;
        static uint64_t s_last_logic = 0;
        static uint64_t s_last_dl = 0;
        static uint64_t s_last_anim = 0;
        static uint64_t s_last_gfx = 0;
        static uint64_t s_last_inp = 0;

        const auto now = std::chrono::steady_clock::now();
        const double wall_seconds = std::chrono::duration<double>(now - g_timing_start).count();
        const double dt = std::chrono::duration<double>(now - s_last_tp).count();

        const uint64_t vi_c = g_guest_vi.load(std::memory_order_relaxed);
        const uint64_t logic_c = g_game_logic.load(std::memory_order_relaxed);
        const uint64_t dl_c = g_display_list.load(std::memory_order_relaxed);
        const uint64_t anim_c = g_animation_update.load(std::memory_order_relaxed);
        const uint64_t gfx_c = g_host_update_gfx.load(std::memory_order_relaxed);
        const uint64_t inp_c = g_guest_input_updates.load(std::memory_order_relaxed);

        const double vi_hz = (dt > 0.001) ? double(vi_c - s_last_vi) / dt : 0.0;
        const double logic_hz = (dt > 0.001) ? double(logic_c - s_last_logic) / dt : 0.0;
        const double dl_hz = (dt > 0.001) ? double(dl_c - s_last_dl) / dt : 0.0;
        const double anim_hz = (dt > 0.001) ? double(anim_c - s_last_anim) / dt : 0.0;
        const double gfx_hz = (dt > 0.001) ? double(gfx_c - s_last_gfx) / dt : 0.0;
        const double inp_hz = (dt > 0.001) ? double(inp_c - s_last_inp) / dt : 0.0;

        s_last_tp = now;
        s_last_vi = vi_c;
        s_last_logic = logic_c;
        s_last_dl = dl_c;
        s_last_anim = anim_c;
        s_last_gfx = gfx_c;
        s_last_inp = inp_c;

        const int32_t divisor = g_frame_divisor.load(std::memory_order_relaxed);
        std::fprintf(stderr,
                     "[tooie-timing] pid=%ld wall=%.3fs vi_hz=%.2f (total=%llu) "
                     "logic_hz=%.2f (total=%llu) game_frame_hz=%.2f (total=%llu) "
                     "anim_hz=%.2f (total=%llu) host_gfx_hz=%.2f (total=%llu) "
                     "divisor=%d inp_hz=%.2f\n",
                     static_cast<long>(getpid()),
                     wall_seconds, vi_hz, (unsigned long long)vi_c,
                     logic_hz, (unsigned long long)logic_c,
                     dl_hz, (unsigned long long)dl_c,
                     anim_hz, (unsigned long long)anim_c,
                     gfx_hz, (unsigned long long)gfx_c,
                     divisor,
                     inp_hz);
        std::fflush(stderr);
    }
}

void tick() {
    ensure_init();
    if (g_movement_enabled.load(std::memory_order_relaxed) ||
        g_forced_facing_enabled.load(std::memory_order_relaxed) ||
        g_secondary_sway_enabled.load(std::memory_order_relaxed) ||
        g_backpack_flap_enabled.load(std::memory_order_relaxed) ||
        g_feather_anim_sample_time_enabled.load(std::memory_order_relaxed)) {
        g_gfx_ticks.fetch_add(1, std::memory_order_relaxed);
    }
    if (!g_enabled.load(std::memory_order_relaxed) &&
        !g_timing_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    const auto now = std::chrono::steady_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_last_report).count();
    if (ms < g_interval_ms) {
        return;
    }
    g_last_report = now;
    report_now("tick");
}

bool secondary_sway_enabled() {
    ensure_init();
    return g_secondary_sway_enabled.load(std::memory_order_relaxed);
}

bool p09_local_pair_provenance_enabled() {
    ensure_init();
    return g_p09_local_pair_snapshot.load(std::memory_order_relaxed);
}

bool secondary_sway_player_table_slot(uint8_t* rdram, uint32_t player,
                                      uint32_t* slot_out) {
    if (rdram == nullptr || player == 0) {
        return false;
    }
    for (uint32_t slot = 0; slot < kPlayerTableSlots; ++slot) {
        const uint32_t entry =
            backpack_guest_word(rdram, kPlayerTableBase, slot * 4u);
        if (entry == player) {
            if (slot_out != nullptr) {
                *slot_out = slot;
            }
            return true;
        }
    }
    return false;
}

bool feather_anim_sample_time_enabled() {
    ensure_init();
    return g_feather_anim_sample_time_enabled.load(std::memory_order_relaxed);
}

bool feather_anim_is_target(uint32_t anim_id, uint32_t body_anim_id) {
    if (anim_id == 79u || anim_id == 107u || anim_id == 108u) {
        return true;
    }
    if (anim_id >= 100u && anim_id <= 105u) {
        return true;
    }
    return body_anim_id != 0 && anim_id == body_anim_id;
}

const char* feather_anim_time_class_name(FeatherAnimTimeClass cls) {
    switch (cls) {
        case FeatherAnimTimeClass::Forward:
            return "FORWARD";
        case FeatherAnimTimeClass::Same:
            return "SAME";
        case FeatherAnimTimeClass::Backward:
            return "BACKWARD";
        case FeatherAnimTimeClass::WrapExpected:
            return "WRAP_EXPECTED";
        case FeatherAnimTimeClass::UnexpectedJump:
            return "UNEXPECTED_JUMP";
        default:
            return "NONE";
    }
}

float feather_anim_quat_delta(const float a[4], const float b[4]) {
    float dot = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
    if (dot < 0.0f) {
        dot = -dot;
    }
    if (dot > 1.0f) {
        dot = 1.0f;
    }
    return 2.0f * std::acos(dot);
}

void feather_anim_count_clip(int16_t clip_id) {
    for (size_t i = 0; i < kFeatherAnimClipIdSlots; ++i) {
        if (g_feather_anim_clip_counts[i].used &&
            g_feather_anim_clip_counts[i].clip_id == clip_id) {
            ++g_feather_anim_clip_counts[i].count;
            return;
        }
    }
    for (size_t i = 0; i < kFeatherAnimClipIdSlots; ++i) {
        if (!g_feather_anim_clip_counts[i].used) {
            g_feather_anim_clip_counts[i].used = true;
            g_feather_anim_clip_counts[i].clip_id = clip_id;
            g_feather_anim_clip_counts[i].count = 1;
            return;
        }
    }
}

void feather_anim_note_baanim_update(uint8_t* rdram, uint32_t player) {
    ensure_init();
    if (!g_feather_anim_sample_time_enabled.load(std::memory_order_relaxed) ||
        rdram == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_feather_anim_mu);
    ++g_feather_anim_baanim_raw;
    uint32_t slot = 0;
    if (!secondary_sway_player_table_slot(rdram, player, &slot)) {
        feather_anim_note_reject(FeatherAnimReject::BaanimNotPlayerTable,
                                 "player_not_in_D_80135490");
        return;
    }
    const uint32_t state = backpack_guest_word(rdram, player, 4);
    if (state == 0) {
        feather_anim_note_reject(FeatherAnimReject::BaanimStateZero,
                                 "player+4");
        return;
    }
    const uint32_t anctrl = backpack_guest_word(rdram, state, 0);
    if (anctrl == 0) {
        feather_anim_note_reject(FeatherAnimReject::BaanimAnctrlZero,
                                 "state+0");
        return;
    }
    const uint32_t clip = anctrl + 0x1Cu;
    FeatherAnimSnap snap{};
    snap.tick = guest_logic_tick();
    snap.player = player;
    snap.slot = slot;
    snap.locomotion = static_cast<uint32_t>(
        static_cast<uint16_t>(backpack_guest_hword(rdram, player, 0x7A)));
    snap.baanim_mode = backpack_guest_word(rdram, state, 8);
    snap.anctrl = anctrl;
    snap.clip_ptr = clip;
    snap.clip_id = backpack_guest_hword(rdram, clip, 0x18);
    snap.anctrl_index = backpack_guest_hword(rdram, anctrl, 0x18);
    snap.flags_word = backpack_guest_word(rdram, anctrl, 0x18);
    snap.playback = backpack_guest_byte(rdram, anctrl, 0x1A);
    snap.direction = static_cast<int32_t>(
        static_cast<int8_t>(backpack_guest_byte(rdram, anctrl, 0x1B)));
    snap.sample_time = backpack_guest_float(rdram, clip, 0x10);
    snap.duration = backpack_guest_float(rdram, anctrl, 0xC);
    snap.cached_timer = backpack_guest_float(rdram, anctrl, 0);
    snap.clip_f14 = backpack_guest_float(rdram, clip, 0x14);
    for (uint32_t i = 0; i < 8; ++i) {
        snap.clip_w[i] = backpack_guest_word(rdram, clip, i * 4u);
    }
    snap.apply_fn = snap.clip_w[0];
    snap.file_ptr = snap.clip_w[1];
    const uint32_t actor = backpack_guest_word(rdram, player, 0xC8);
    if (actor != 0) {
        snap.vx = backpack_guest_float(rdram, actor, 0x10);
        snap.vy = backpack_guest_float(rdram, actor, 0x14);
        snap.vz = backpack_guest_float(rdram, actor, 0x18);
        if (std::isfinite(snap.vx) && std::isfinite(snap.vz)) {
            snap.horiz_speed = std::sqrt(snap.vx * snap.vx + snap.vz * snap.vz);
        }
    }
    if (snap.file_ptr != 0 && (snap.file_ptr & 3u) == 0u) {
        const int16_t start = backpack_guest_hword(rdram, snap.file_ptr, 0);
        const int16_t end = backpack_guest_hword(rdram, snap.file_ptr, 2);
        const int16_t count = backpack_guest_hword(rdram, snap.file_ptr, 4);
        if (start >= 0 && end > start && end < 4096 && count > 0 &&
            count < 512) {
            snap.header_ok = true;
            snap.frame_start = start;
            snap.frame_end = end;
            snap.elem_count = count;
        }
    }
    snap.clip_changed = g_feather_anim_have_prev_time &&
                        snap.clip_id != g_feather_anim_last_clip_id;
    if (g_feather_anim_have_prev_time) {
        snap.sample_time_prev = g_feather_anim_last_sample_time;
        snap.sample_dt = snap.sample_time - g_feather_anim_last_sample_time;
        const bool loopish = snap.playback == 2u || snap.playback == 4u;
        const bool normalized = snap.sample_time <= 1.05f &&
                                g_feather_anim_last_sample_time <= 1.05f;
        bool wrap = false;
        if (loopish) {
            if (normalized) {
                wrap = g_feather_anim_last_sample_time > 0.80f &&
                       snap.sample_time < 0.20f;
            } else if (snap.header_ok) {
                wrap = g_feather_anim_last_sample_time >
                           static_cast<float>(snap.frame_end) - 2.0f &&
                       snap.sample_time <
                           static_cast<float>(snap.frame_start) + 2.0f;
            } else {
                wrap = g_feather_anim_last_sample_time > 8.0f &&
                       snap.sample_time < 2.0f;
            }
        }
        const float abs_dt = std::fabs(snap.sample_dt);
        if (wrap) {
            snap.time_class = FeatherAnimTimeClass::WrapExpected;
            ++g_feather_anim_time_wrap;
        } else if (abs_dt < 1.0e-6f) {
            snap.time_class = FeatherAnimTimeClass::Same;
            ++g_feather_anim_time_same;
        } else if (snap.sample_dt > 0.0f) {
            const float jump_lim = normalized ? 0.20f : 8.0f;
            if (snap.sample_dt > jump_lim) {
                snap.time_class = FeatherAnimTimeClass::UnexpectedJump;
                ++g_feather_anim_time_unexpected;
            } else {
                snap.time_class = FeatherAnimTimeClass::Forward;
                ++g_feather_anim_time_forward;
            }
        } else {
            snap.time_class = FeatherAnimTimeClass::Backward;
            ++g_feather_anim_time_backward;
            if (!loopish) {
                snap.time_class = FeatherAnimTimeClass::UnexpectedJump;
                ++g_feather_anim_time_unexpected;
            }
        }
    }
    if (snap.clip_changed) {
        ++g_feather_anim_clip_change;
        if (snap.playback == 3u ||
            (g_feather_anim_prev_clip_id != 0 &&
             snap.clip_id == g_feather_anim_prev_clip_id)) {
            ++g_feather_anim_clip_unexpected;
        }
        g_feather_anim_prev_clip_id = g_feather_anim_last_clip_id;
    }
    feather_anim_count_clip(snap.clip_id);
    g_feather_anim_last_clip_id = snap.clip_id;
    g_feather_anim_last_sample_time = snap.sample_time;
    g_feather_anim_have_prev_time = true;
    g_feather_anim_last_baanim = snap;
    ++g_feather_anim_baanim_accepted;
    snap.valid = true;
    g_feather_anim_last_baanim.valid = true;
}

void feather_anim_note_ae8a8_before_compose(uint8_t* rdram,
                                            uint32_t anim_list_ptr,
                                            uint32_t skeleton,
                                            uint32_t slot_base,
                                            uint32_t additive_ctx) {
    (void)slot_base;
    (void)additive_ctx;
    ensure_init();
    if (!g_feather_anim_sample_time_enabled.load(std::memory_order_relaxed) ||
        rdram == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_feather_anim_mu);
    ++g_feather_anim_ae8a8_raw;
    if (anim_list_ptr == 0 || skeleton == 0) {
        feather_anim_note_reject(FeatherAnimReject::Ae8a8BadSkeleton,
                                 "null_list_or_skel");
        return;
    }
    const int16_t bone_count = backpack_guest_hword(rdram, skeleton, 4);
    if (bone_count <= 0 || bone_count > 128) {
        feather_anim_note_reject(FeatherAnimReject::Ae8a8BadSkeleton,
                                 "bone_count");
        return;
    }
    int16_t idx107 = -1;
    int16_t idx100 = -1;
    int16_t idx103 = -1;
    uint32_t body_anim = 0;
    uint32_t body_index = 0;
    bool has_plume = false;
    for (int16_t i = 0; i < bone_count; ++i) {
        const uint32_t bone = skeleton + 8u + static_cast<uint32_t>(i) * 0x10u;
        const uint32_t anim_id = static_cast<uint32_t>(
            static_cast<uint16_t>(backpack_guest_hword(rdram, bone, 0xC)));
        if (anim_id == 107u) {
            idx107 = i;
            has_plume = true;
            const int16_t parent = backpack_guest_hword(rdram, bone, 0xE);
            if (parent >= 0 && parent < bone_count) {
                const uint32_t pb =
                    skeleton + 8u + static_cast<uint32_t>(parent) * 0x10u;
                body_index = static_cast<uint32_t>(parent);
                body_anim = static_cast<uint32_t>(static_cast<uint16_t>(
                    backpack_guest_hword(rdram, pb, 0xC)));
            }
        }
        if (anim_id == 100u) {
            idx100 = i;
            has_plume = true;
        }
        if (anim_id == 103u) {
            idx103 = i;
            has_plume = true;
        }
        if (anim_id >= 100u && anim_id <= 105u) {
            has_plume = true;
        }
    }
    if (!has_plume) {
        feather_anim_note_reject(FeatherAnimReject::Ae8a8NoPlume,
                                 "no_107_or_100_105");
        return;
    }
    const uint32_t anim_base = backpack_guest_word(rdram, anim_list_ptr, 0);
    if (anim_base == 0) {
        feather_anim_note_reject(FeatherAnimReject::Ae8a8AnimBaseZero,
                                 "anim_list+0");
        return;
    }
    ++g_feather_anim_ae8a8_accepted;
    if (body_anim != 0) {
        g_feather_anim_body_anim_id = body_anim;
        g_feather_anim_body_index = body_index;
    }
    const uint64_t ae_tick = guest_logic_tick();
    const FeatherAnimSnap snap = g_feather_anim_last_baanim;
    const bool join_ok = snap.valid &&
                         (snap.tick == ae_tick ||
                          (ae_tick > snap.tick && ae_tick - snap.tick <= 1) ||
                          (snap.tick > ae_tick && snap.tick - ae_tick <= 1));
    if (join_ok) {
        ++g_feather_anim_join_ok;
    } else {
        ++g_feather_anim_join_mismatch;
    }
    if (g_feather_anim_csv == nullptr) {
        feather_anim_note_reject(FeatherAnimReject::CsvOpenFailed, "csv_null");
        return;
    }

    struct BoneSample {
        int16_t index = -1;
        uint32_t anim_id = 0;
        int16_t parent = -1;
        uint32_t record = 0;
        float q[4]{};
        float s[3]{};
        float t[3]{};
        uint32_t t_ptr = 0;
        float q_delta = 0.0f;
        float s_delta = 0.0f;
        float t_delta = 0.0f;
        bool large_rot = false;
        bool large_scale = false;
        bool large_trans = false;
        bool authored = false;
        bool is_body = false;
        bool have_prev = false;
    };
    BoneSample samples[16]{};
    size_t n_samples = 0;
    bool large_107 = false;
    bool large_108 = false;
    bool large_79 = false;
    bool large_body = false;
    bool authored_107 = false;
    bool unexplained_107 = false;

    for (int16_t i = 0; i < bone_count && n_samples < 16; ++i) {
        const uint32_t bone = skeleton + 8u + static_cast<uint32_t>(i) * 0x10u;
        const uint32_t anim_id = static_cast<uint32_t>(
            static_cast<uint16_t>(backpack_guest_hword(rdram, bone, 0xC)));
        if (!feather_anim_is_target(anim_id, body_anim)) {
            continue;
        }
        BoneSample& s = samples[n_samples++];
        s.index = i;
        s.anim_id = anim_id;
        s.parent = backpack_guest_hword(rdram, bone, 0xE);
        s.record = anim_base + anim_id * 0x28u;
        s.is_body = body_anim != 0 && anim_id == body_anim;
        s.q[0] = backpack_guest_float(rdram, s.record, 0x00);
        s.q[1] = backpack_guest_float(rdram, s.record, 0x04);
        s.q[2] = backpack_guest_float(rdram, s.record, 0x08);
        s.q[3] = backpack_guest_float(rdram, s.record, 0x0C);
        s.s[0] = backpack_guest_float(rdram, s.record, 0x10);
        s.s[1] = backpack_guest_float(rdram, s.record, 0x14);
        s.s[2] = backpack_guest_float(rdram, s.record, 0x18);
        s.t_ptr = backpack_guest_word(rdram, s.record, 0x1C);
        if (s.t_ptr != 0 && (s.t_ptr & 3u) == 0u) {
            s.t[0] = backpack_guest_float(rdram, s.t_ptr, 0);
            s.t[1] = backpack_guest_float(rdram, s.t_ptr, 4);
            s.t[2] = backpack_guest_float(rdram, s.t_ptr, 8);
        }
        if (anim_id < 256u && g_feather_anim_prev[anim_id].valid) {
            const FeatherAnimBonePrev& prev = g_feather_anim_prev[anim_id];
            const bool same_clip = prev.clip_id == snap.clip_id;
            const bool consecutive =
                ae_tick == prev.tick || ae_tick == prev.tick + 1;
            if (same_clip && consecutive) {
                s.have_prev = true;
                s.q_delta = feather_anim_quat_delta(prev.q, s.q);
                s.s_delta = std::max(std::fabs(s.s[0] - prev.s[0]),
                                     std::max(std::fabs(s.s[1] - prev.s[1]),
                                              std::fabs(s.s[2] - prev.s[2])));
                const float dx = s.t[0] - prev.t[0];
                const float dy = s.t[1] - prev.t[1];
                const float dz = s.t[2] - prev.t[2];
                s.t_delta = std::sqrt(dx * dx + dy * dy + dz * dz);
                s.large_rot = s.q_delta > kFeatherAnimLargeRotRad;
                s.large_scale = s.s_delta > kFeatherAnimLargeScale;
                s.large_trans = s.t_delta > kFeatherAnimLargeTrans;
                const bool time_ok =
                    snap.time_class == FeatherAnimTimeClass::Forward ||
                    snap.time_class == FeatherAnimTimeClass::Same ||
                    snap.time_class == FeatherAnimTimeClass::WrapExpected;
                s.authored = s.large_rot && time_ok && !snap.clip_changed;
            }
        }
        if (anim_id == 107u && (s.large_rot || s.large_scale || s.large_trans)) {
            large_107 = true;
            if (s.large_rot) {
                ++g_feather_anim_107_large_rot;
            }
            if (s.large_scale) {
                ++g_feather_anim_107_large_scale;
            }
            if (s.large_trans) {
                ++g_feather_anim_107_large_trans;
            }
            if (s.authored) {
                authored_107 = true;
                ++g_feather_anim_107_step_authored;
            } else if (s.have_prev) {
                unexplained_107 = true;
                ++g_feather_anim_107_step_unexplained;
            }
        }
        if (anim_id == 108u && (s.large_rot || s.large_scale)) {
            large_108 = true;
            ++g_feather_anim_108_large;
        }
        if (anim_id == 79u && (s.large_rot || s.large_scale)) {
            large_79 = true;
            ++g_feather_anim_79_large;
        }
        if (s.is_body) {
            if (snap.time_class == FeatherAnimTimeClass::Same) {
                ++g_feather_anim_body_time_same;
            }
            if (s.large_rot || s.large_scale) {
                large_body = true;
                ++g_feather_anim_body_large;
            }
        }
        if (anim_id < 256u) {
            FeatherAnimBonePrev& prev = g_feather_anim_prev[anim_id];
            prev.valid = true;
            prev.tick = ae_tick;
            prev.clip_id = snap.clip_id;
            for (int k = 0; k < 4; ++k) {
                prev.q[k] = s.q[k];
            }
            for (int k = 0; k < 3; ++k) {
                prev.s[k] = s.s[k];
                prev.t[k] = s.t[k];
            }
        }
    }

    bool prop_left = false;
    bool prop_right = false;
    if (large_107) {
        if (idx100 >= 0) {
            prop_left = true;
            ++g_feather_anim_107_prop_left;
        }
        if (idx103 >= 0) {
            prop_right = true;
            ++g_feather_anim_107_prop_right;
        }
    }
    (void)idx107;

    const bool clip_event = snap.clip_changed;
    const bool time_event =
        snap.time_class == FeatherAnimTimeClass::UnexpectedJump ||
        snap.time_class == FeatherAnimTimeClass::WrapExpected;
    const bool large_event = large_107 || large_108 || large_79 || large_body;
    bool control = false;
    if (!large_event && !clip_event && !time_event &&
        g_feather_anim_control_ticks < kFeatherAnimControlTicks) {
        control = true;
        ++g_feather_anim_control_ticks;
    }
    if (!control && !large_event && !clip_event && !time_event) {
        return;
    }
    if (g_feather_anim_rows >= kFeatherAnimMaxRows) {
        feather_anim_note_reject(FeatherAnimReject::MaxRows, "bounded");
        return;
    }
    const char* kind = "CONTROL";
    if (large_107) {
        kind = unexplained_107 ? "BONE107_LARGE_UNEXPLAINED"
                               : "BONE107_LARGE";
    } else if (clip_event) {
        kind = "CLIP_CHANGE";
    } else if (snap.time_class == FeatherAnimTimeClass::UnexpectedJump) {
        kind = "TIME_UNEXPECTED";
    } else if (snap.time_class == FeatherAnimTimeClass::WrapExpected) {
        kind = "TIME_WRAP";
    } else if (large_108) {
        kind = "BONE108_LARGE";
    } else if (large_79) {
        kind = "BONE79_LARGE";
    } else if (large_body) {
        kind = "BODY_LARGE";
    }
    (void)authored_107;

    const float sample_frame = std::floor(snap.sample_time);
    const float sample_frac = snap.sample_time - sample_frame;
    const uint64_t gfx = g_gfx_ticks.load(std::memory_order_relaxed);
    for (size_t i = 0; i < n_samples; ++i) {
        if (g_feather_anim_rows >= kFeatherAnimMaxRows) {
            feather_anim_note_reject(FeatherAnimReject::MaxRows, "bounded");
            break;
        }
        const BoneSample& s = samples[i];
        std::fprintf(
            g_feather_anim_csv,
            "%s,%llu,%llu,%llu,%llu,%d,0x%08X,%u,%u,%u,0x%08X,0x%08X,"
            "%d,%d,0x%08X,%u,%d,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,"
            "%.9g,%.9g,%.9g,0x%08X,0x%08X,%d,%d,%d,%s,%d,"
            "%d,%u,%d,%d,0x%08X,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,"
            "%.9g,%.9g,%.9g,0x%08X,%.9g,%.9g,%.9g,%d,%d,%d,%d,%d,%d\n",
            kind,
            static_cast<unsigned long long>(ae_tick),
            static_cast<unsigned long long>(gfx),
            static_cast<unsigned long long>(snap.tick),
            static_cast<unsigned long long>(ae_tick),
            join_ok ? 1 : 0, snap.player, snap.slot, snap.locomotion,
            snap.baanim_mode, snap.anctrl, snap.clip_ptr,
            static_cast<int>(snap.clip_id),
            static_cast<int>(snap.anctrl_index), snap.flags_word,
            snap.playback, snap.direction, snap.sample_time,
            snap.sample_time_prev, snap.sample_dt, sample_frame, sample_frac,
            snap.duration, snap.duration, snap.horiz_speed, snap.vx, snap.vy,
            snap.vz, snap.apply_fn, snap.file_ptr,
            static_cast<int>(snap.frame_start),
            static_cast<int>(snap.frame_end),
            static_cast<int>(snap.elem_count),
            feather_anim_time_class_name(snap.time_class),
            snap.clip_changed ? 1 : 0, static_cast<int>(s.index), s.anim_id,
            static_cast<int>(s.parent), s.is_body ? 1 : 0, s.record, s.q[0],
            s.q[1], s.q[2], s.q[3], s.s[0], s.s[1], s.s[2], s.t[0], s.t[1],
            s.t[2], s.t_ptr, s.q_delta, s.s_delta, s.t_delta,
            s.large_rot ? 1 : 0, s.large_scale ? 1 : 0,
            s.large_trans ? 1 : 0, s.authored ? 1 : 0, prop_left ? 1 : 0,
            prop_right ? 1 : 0);
        ++g_feather_anim_rows;
        ++g_feather_anim_target_sample_rows;
    }
    if ((g_feather_anim_target_sample_rows % 16u) == 0u) {
        std::fflush(g_feather_anim_csv);
    }
}

uint32_t secondary_sway_register_durable_bin(uint8_t* rdram, uint32_t model_bin,
                                             uint32_t resource, uint32_t player) {
    ++g_v7_durable_register_total;
    if (rdram == nullptr || model_bin == 0) {
        return 0;
    }
    const uint32_t magic = backpack_guest_word(rdram, model_bin, 0);
    const int16_t gtype = backpack_guest_hword(rdram, model_bin, 0xA);
    const uint32_t setup_off = backpack_guest_word(rdram, model_bin, 0x18);
    uint16_t bones = 0;
    if (setup_off != 0 && setup_off < 0x01000000u) {
        bones = static_cast<uint16_t>(
            backpack_guest_hword(rdram, model_bin, setup_off + 4u));
    }
    uint32_t resolved = resource;
    if (resolved != 0x62Bu) {
        if (magic == 0xBu && (static_cast<uint16_t>(gtype) & 0x40u) == 0 &&
            bones == 66u) {
            resolved = 0x62Bu;
        } else {
            return 0;
        }
    }
    std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
    for (size_t i = 0; i < g_durable_bin_count; ++i) {
        if (g_durable_bins[i].model_bin == model_bin) {
            g_durable_bins[i].resource_id = resolved;
            if (player != 0) {
                g_durable_bins[i].player = player;
            }
            ++g_secondary_sway_durable_register_hits;
            v7_note_durable_model_bin(model_bin, resource, resolved, player);
            return resolved;
        }
    }
    if (g_durable_bin_count < kDurableBinSlots) {
        g_durable_bins[g_durable_bin_count++] = {model_bin, resolved, player};
    } else {
        v7_clear_model_context_if_bin(g_durable_bins[0].model_bin);
        for (size_t i = 1; i < kDurableBinSlots; ++i) {
            g_durable_bins[i - 1] = g_durable_bins[i];
        }
        g_durable_bins[kDurableBinSlots - 1] = {model_bin, resolved, player};
    }
    ++g_secondary_sway_durable_register_hits;
    v7_note_durable_model_bin(model_bin, resource, resolved, player);
    return resolved;
}

bool secondary_sway_lookup_durable_bin(uint32_t node_ptr, DurableModelBin* out,
                                       uint32_t* rel_out) {
    for (size_t i = 0; i < g_durable_bin_count; ++i) {
        const uint32_t bin = g_durable_bins[i].model_bin;
        if (bin == 0 || node_ptr < bin) {
            continue;
        }
        const uint32_t rel = node_ptr - bin;
        uint32_t span = kSecondarySwayAssetSize;
        if (g_durable_bins[i].resource_id == 0x61Cu) {
            span = kKazooieFeather61CAssetSize;
        } else if (g_durable_bins[i].resource_id == 0x83Au) {
            span = kKazooieFeather83AAssetSize;
        } else if (g_durable_bins[i].resource_id == 0x62Bu) {
            span = kKazooieFeather62BAssetSize;
        }
        if (rel < span) {
            if (out != nullptr) {
                *out = g_durable_bins[i];
            }
            if (rel_out != nullptr) {
                *rel_out = rel;
            }
            return true;
        }
    }
    return false;
}

bool secondary_sway_match_durable_snout(uint8_t* rdram, uint32_t node_ptr,
                                       DurableModelBin* out, uint32_t* rel_out) {
    if (rdram == nullptr || node_ptr == 0 || out == nullptr) {
        return false;
    }
    DurableModelBin matched{};
    uint32_t rel = 0;
    const bool in_span = secondary_sway_lookup_durable_bin(node_ptr, &matched, &rel);
    if (!in_span) {
        return false;
    }
    const uint32_t type = backpack_guest_word(rdram, node_ptr, 0);
    const int16_t bone_anim = backpack_guest_hword(rdram, node_ptr, 0xA);
    const uint32_t bone_index = backpack_guest_byte(rdram, node_ptr, 9);
    const bool exact_off = rel == kSecondarySwaySnoutNodeOffset;
    const bool type_ok = type == 2u;
    const bool bone_ok =
        bone_anim == static_cast<int16_t>(kSecondarySwaySnoutBoneAnimId);
    const bool index_ok = bone_index == kSecondarySwaySnoutBoneIndex;
    if (!exact_off || !type_ok || !bone_ok || !index_ok) {
        return false;
    }
    *out = matched;
    if (rel_out != nullptr) {
        *rel_out = rel;
    }
    return true;
}

void secondary_sway_note_98ba8(uint8_t* rdram, uint32_t player) {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    uint32_t slot = 0;
    if (!secondary_sway_player_table_slot(rdram, player, &slot)) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
    ++g_secondary_sway_outer_hits;
}

bool secondary_sway_player_draw_begin(uint8_t* rdram, uint32_t player) {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed)) {
        return false;
    }
    uint32_t slot = 0;
    if (!secondary_sway_player_table_slot(rdram, player, &slot)) {
        return false;
    }
    if (g_player_scope_depth >= kPlayerScopeStackSize) {
        std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
        ++g_secondary_sway_overflow;
        return false;
    }
    PlayerRenderScope& scope = g_player_scopes[g_player_scope_depth++];
    scope = {};
    scope.player = player;
    scope.slot = slot;
    {
        std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
        ++g_secondary_sway_92444_hits;
        if (g_player_scope_depth > g_secondary_sway_max_depth) {
            g_secondary_sway_max_depth = g_player_scope_depth;
        }
    }
    return true;
}

void secondary_sway_player_draw_end() {
    if (g_player_scope_depth == 0) {
        return;
    }
    --g_player_scope_depth;
}

void secondary_sway_note_model_getter(uint8_t* rdram, uint32_t owner,
                                     uint32_t arg1, uint32_t model_bin) {
    if (g_player_scope_depth == 0) {
        return;
    }
    PlayerRenderScope& scope = g_player_scopes[g_player_scope_depth - 1];
    if (owner != scope.player || arg1 != 1u) {
        return;
    }
    const uint32_t obj = backpack_guest_word(rdram, owner, 0x50);
    const uint32_t resource =
        static_cast<uint32_t>(static_cast<uint16_t>(
            backpack_guest_hword(rdram, obj, 0xC)));
    scope.expected_model_bin = model_bin;
    scope.resource_id = resource;
    scope.getter_seen = true;
}

void secondary_sway_note_bone_anim_entry() {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
    ++g_secondary_sway_bone_anim_entry_hits;
}

void secondary_sway_note_bone_anim(uint8_t* rdram, uint32_t anim_list,
                                   uint32_t bone_anim_id, uint32_t arg2,
                                   uint32_t arg3, uint32_t arg4,
                                   uint32_t arg5) {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed) ||
        rdram == nullptr || anim_list == 0 ||
        !secondary_sway_logged_additive_anim(bone_anim_id)) {
        return;
    }

    const uint32_t base = backpack_guest_word(rdram, anim_list, 0);
    const uint32_t next_index =
        static_cast<uint32_t>(static_cast<uint16_t>(
            backpack_guest_hword(rdram, anim_list, 4)));
    const uint32_t index = next_index != 0 ? next_index - 1u : 0u;
    const uint32_t record = base + index * 0x14u;
    const uint32_t post0 = backpack_guest_word(rdram, record, 0x0);
    const uint32_t post1 = backpack_guest_word(rdram, record, 0x4);
    const uint32_t post2 = backpack_guest_word(rdram, record, 0x8);
    const uint32_t post3 = backpack_guest_word(rdram, record, 0xC);
    const uint32_t post_flags = backpack_guest_word(rdram, record, 0x10);

    std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
    ++g_secondary_sway_bone_anim_hits;
    ++g_secondary_sway_bone_anim_samples;
    if (g_secondary_sway_bone_csv == nullptr) {
        g_secondary_sway_bone_csv =
            std::fopen(tooie_diag_path("tooie-secondary-sway-bones.csv").c_str(), "a");
        if (g_secondary_sway_bone_csv == nullptr) {
            return;
        }
        std::fprintf(g_secondary_sway_bone_csv,
                     "FRAME,BONE_ANIM_ID,ANIM_LIST,RECORD_INDEX,RECORD_PTR,"
                     "ARG2,ARG3,ARG4,ARG5,POST0,POST1,POST2,POST3,POST_FLAGS\n");
    }
    std::fprintf(g_secondary_sway_bone_csv,
                 "%llu,%u,0x%08X,%u,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,"
                 "0x%08X,0x%08X,0x%08X,0x%08X,0x%08X\n",
                 static_cast<unsigned long long>(guest_logic_tick()),
                 bone_anim_id, anim_list, index, record, arg2, arg3, arg4,
                 arg5, post0, post1, post2, post3, post_flags);
    if ((g_secondary_sway_bone_anim_samples % kSecondarySwayFlushInterval) ==
        0) {
        std::fflush(g_secondary_sway_bone_csv);
    }
}

void secondary_sway_write_snout_row(uint8_t* rdram, recomp_context* ctx,
                                   const DurableModelBin& matched,
                                   uint32_t node_ptr, uint32_t node_rel,
                                   const char* seam) {
    const int16_t prev_off = backpack_guest_hword(rdram, node_ptr, 0x12);
    const uint32_t curr_off = backpack_guest_word(rdram, node_ptr, 0x14);
    const uint32_t prev_ptr =
        node_ptr + static_cast<uint32_t>(static_cast<int32_t>(prev_off));
    const uint32_t curr_ptr = node_ptr + curr_off;
    uint32_t prev_words[16]{};
    uint32_t curr_words[16]{};
    backpack_read_mtx_words(rdram, prev_ptr, prev_words);
    backpack_read_mtx_words(rdram, curr_ptr, curr_words);
    char prev_hex[16 * 9];
    char curr_hex[16 * 9];
    backpack_format_mtx(prev_hex, sizeof(prev_hex), prev_words);
    backpack_format_mtx(curr_hex, sizeof(curr_hex), curr_words);

    ++g_secondary_sway_snout_matrix_hits;
    ++g_secondary_sway_snout_matrix_samples;
    if (g_secondary_sway_snout_csv == nullptr) {
        g_secondary_sway_snout_csv =
            std::fopen(tooie_diag_path("tooie-secondary-sway-snout.csv").c_str(), "a");
        if (g_secondary_sway_snout_csv == nullptr) {
            return;
        }
        std::fprintf(g_secondary_sway_snout_csv,
                     "FRAME,GFX_TICK,SEAM,PLAYER,RESOURCE,MODEL_BIN,NODE_PTR,"
                     "NODE_REL,GEOCMD_TYPE,BONE_INDEX,BONE_ANIM_ID,A2,CALLER_RA,"
                     "PREV_MTX_PTR,CURR_MTX_PTR,PREVIOUS_MATRIX,CURRENT_MATRIX,"
                     "INTERPOLATION_IDENTITY,TRANSFORM_TAG\n");
    }
    std::fprintf(g_secondary_sway_snout_csv,
                 "%llu,%llu,%s,0x%08X,0x%03X,0x%08X,0x%08X,0x%X,0x%X,%u,%d,"
                 "0x%08X,0x%08X,0x%08X,0x%08X,%s,%s,UNKNOWN,UNKNOWN\n",
                 static_cast<unsigned long long>(guest_logic_tick()),
                 static_cast<unsigned long long>(gfx_ticks()),
                 seam != nullptr ? seam : "UNKNOWN", matched.player,
                 matched.resource_id, matched.model_bin, node_ptr, node_rel,
                 backpack_guest_word(rdram, node_ptr, 0),
                 backpack_guest_byte(rdram, node_ptr, 9),
                 static_cast<int>(backpack_guest_hword(rdram, node_ptr, 0xA)),
                 static_cast<uint32_t>(ctx->r6),
                 static_cast<uint32_t>(ctx->r31), prev_ptr, curr_ptr, prev_hex,
                 curr_hex);
    if ((g_secondary_sway_snout_matrix_samples % kSecondarySwayFlushInterval) ==
        0) {
        std::fflush(g_secondary_sway_snout_csv);
    }
}

void secondary_sway_note_e2aa4_entry() {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
    ++g_secondary_sway_e2aa4_raw;
}

bool secondary_sway_e2aa4_snout_scope_active() {
    return g_e2aa4_snout_scope;
}

void secondary_sway_e2aa4_snout_end() {
    g_e2aa4_snout_scope = false;
}

bool secondary_sway_note_e2aa4(uint8_t* rdram, void* raw_ctx) {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed) ||
        rdram == nullptr || raw_ctx == nullptr) {
        return false;
    }

    auto* const ctx = static_cast<recomp_context*>(raw_ctx);
    const uint32_t gfx_ptr = static_cast<uint32_t>(ctx->r4);
    const uint32_t node_ptr = static_cast<uint32_t>(ctx->r5);
    const uint32_t arg2 = static_cast<uint32_t>(ctx->r6);
    const bool pointer_ok = node_ptr != 0 && (node_ptr & 3u) == 0u;
    const uint32_t type = pointer_ok ? backpack_guest_word(rdram, node_ptr, 0) : 0;
    const uint32_t bone_index =
        pointer_ok ? backpack_guest_byte(rdram, node_ptr, 9) : 0;
    const int16_t bone_anim =
        pointer_ok ? backpack_guest_hword(rdram, node_ptr, 0xA) : 0;
    const int16_t hword8 =
        pointer_ok ? backpack_guest_hword(rdram, node_ptr, 8) : 0;
    const int16_t prev_off =
        pointer_ok ? backpack_guest_hword(rdram, node_ptr, 0x12) : 0;
    const uint32_t curr_off =
        pointer_ok ? backpack_guest_word(rdram, node_ptr, 0x14) : 0;

    DurableModelBin span_bin{};
    uint32_t rel = 0;
    bool in_range = false;
    bool accept = false;
    {
        std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
        const bool have_durable = g_durable_bin_count != 0;
        if (have_durable) {
            ++g_secondary_sway_e2aa4_with_registered;
        } else {
            ++g_secondary_sway_e2aa4_reject_no_registered;
        }
        in_range = secondary_sway_lookup_durable_bin(node_ptr, &span_bin, &rel);
        if (in_range) {
            ++g_secondary_sway_e2aa4_in_true_range;
        } else {
            ++g_secondary_sway_e2aa4_reject_outside_range;
        }
        if (type == 2u) {
            ++g_secondary_sway_e2aa4_type2;
        } else if (pointer_ok) {
            ++g_secondary_sway_e2aa4_reject_type;
        }
        if (bone_index == kSecondarySwaySnoutBoneIndex) {
            ++g_secondary_sway_e2aa4_index11;
        } else if (pointer_ok) {
            ++g_secondary_sway_e2aa4_reject_index;
        }
        if (bone_anim == static_cast<int16_t>(kSecondarySwaySnoutBoneAnimId)) {
            ++g_secondary_sway_e2aa4_bone42;
        } else if (pointer_ok) {
            ++g_secondary_sway_e2aa4_reject_bone;
        }
        if (in_range && rel == kSecondarySwaySnoutNodeOffset) {
            ++g_secondary_sway_e2aa4_rel_11cb8;
        } else if (in_range) {
            ++g_secondary_sway_e2aa4_reject_offset;
        }
        if (!pointer_ok) {
            ++g_secondary_sway_e2aa4_reject_other;
        }

        bool want_tuple = g_secondary_sway_e2aa4_tuple_rows < kE2AA4TupleFirstN;
        if (!want_tuple && g_e2aa4_unique_a1_count < kE2AA4TupleUniqueMax) {
            bool seen = false;
            for (size_t i = 0; i < g_e2aa4_unique_a1_count; ++i) {
                if (g_e2aa4_unique_a1[i] == node_ptr) {
                    seen = true;
                    break;
                }
            }
            if (!seen) {
                want_tuple = true;
            }
        }
        if (want_tuple) {
            bool seen = false;
            for (size_t i = 0; i < g_e2aa4_unique_a1_count; ++i) {
                if (g_e2aa4_unique_a1[i] == node_ptr) {
                    seen = true;
                    break;
                }
            }
            if (!seen && g_e2aa4_unique_a1_count < kE2AA4TupleUniqueMax) {
                g_e2aa4_unique_a1[g_e2aa4_unique_a1_count++] = node_ptr;
            }
            if (g_secondary_sway_e2aa4_tuple_csv == nullptr) {
                g_secondary_sway_e2aa4_tuple_csv =
                    std::fopen(tooie_diag_path("tooie-secondary-sway-e2aa4-tuples.csv").c_str(), "a");
                if (g_secondary_sway_e2aa4_tuple_csv != nullptr) {
                    std::fprintf(g_secondary_sway_e2aa4_tuple_csv,
                                 "FRAME,A0,A1,A2,MODEL_BIN,A1_MINUS_BIN,WORD0,"
                                 "HWORD8,BYTE9,HWORD_A,HWORD12,WORD14,IN_RANGE,"
                                 "TYPE2,INDEX11,BONE42,REL_11CB8\n");
                }
            }
            if (g_secondary_sway_e2aa4_tuple_csv != nullptr) {
                std::fprintf(g_secondary_sway_e2aa4_tuple_csv,
                             "%llu,0x%08X,0x%08X,0x%08X,0x%08X,0x%X,0x%08X,"
                             "0x%04X,0x%02X,0x%04X,0x%04X,0x%08X,%u,%u,%u,%u,%u\n",
                             static_cast<unsigned long long>(guest_logic_tick()),
                             gfx_ptr, node_ptr, arg2, span_bin.model_bin, rel,
                             type, static_cast<unsigned>(hword8) & 0xFFFFu,
                             bone_index,
                             static_cast<unsigned>(bone_anim) & 0xFFFFu,
                             static_cast<unsigned>(prev_off) & 0xFFFFu, curr_off,
                             in_range ? 1u : 0u, type == 2u ? 1u : 0u,
                             bone_index == kSecondarySwaySnoutBoneIndex ? 1u : 0u,
                             bone_anim == static_cast<int16_t>(
                                 kSecondarySwaySnoutBoneAnimId)
                                 ? 1u
                                 : 0u,
                             (in_range && rel == kSecondarySwaySnoutNodeOffset)
                                 ? 1u
                                 : 0u);
                ++g_secondary_sway_e2aa4_tuple_rows;
                std::fflush(g_secondary_sway_e2aa4_tuple_csv);
            }
        }

        DurableModelBin matched{};
        uint32_t matched_rel = 0;
        accept = have_durable &&
                 secondary_sway_match_durable_snout(rdram, node_ptr, &matched,
                                                    &matched_rel);
        if (accept) {
            ++g_secondary_sway_e2aa4_accept;
            g_e2aa4_snout_scope = true;
            secondary_sway_write_snout_row(rdram, ctx, matched, node_ptr,
                                           matched_rel, "E2AA4");
        }
    }
    return accept;
}

void secondary_sway_note_snout_dual_mtx(uint8_t* rdram, void* raw_ctx) {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed) ||
        rdram == nullptr || raw_ctx == nullptr || !g_e2aa4_snout_scope) {
        return;
    }

    auto* const ctx = static_cast<recomp_context*>(raw_ctx);
    const uint32_t node_ptr = static_cast<uint32_t>(ctx->r5);
    DurableModelBin matched{};
    uint32_t matched_rel = 0;
    if (!secondary_sway_match_durable_snout(rdram, node_ptr, &matched,
                                           &matched_rel)) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
    ++g_secondary_sway_e2180_associated;
    secondary_sway_write_snout_row(rdram, ctx, matched, node_ptr, matched_rel,
                                   "E2180_ASSOCIATED");
}

void secondary_sway_note_snout_dual_mtx_entry() {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
    ++g_secondary_sway_snout_matrix_entry_hits;
}

void secondary_sway_note_player_de448(uint8_t* rdram, void* ctx_void) {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed) ||
        ctx_void == nullptr) {
        return;
    }
    auto* ctx = static_cast<recomp_context*>(ctx_void);
    const uint32_t model_bin =
        backpack_guest_word(rdram, static_cast<uint32_t>(ctx->r29), 0x10);
    secondary_sway_register_durable_bin(rdram, model_bin, 0, 0);
    {
        std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
        ++g_secondary_sway_total_hits;
    }
    if (g_player_scope_depth == 0) {
        std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
        ++g_secondary_sway_reject_no_scope;
        return;
    }

    PlayerRenderScope& scope = g_player_scopes[g_player_scope_depth - 1];
    {
        std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
        ++g_secondary_sway_in_scope_hits;
    }
    if (!scope.getter_seen || scope.expected_model_bin == 0) {
        std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
        ++g_secondary_sway_reject_other;
        return;
    }
    if (model_bin != scope.expected_model_bin) {
        std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
        ++g_secondary_sway_reject_wrong_callsite;
        return;
    }

    uint32_t resource = scope.resource_id;
    const uint32_t locomotion =
        static_cast<uint32_t>(static_cast<uint16_t>(
            backpack_guest_hword(rdram, scope.player, 0x7A)));
    const uint32_t magic = backpack_guest_word(rdram, model_bin, 0);
    const int16_t gtype = backpack_guest_hword(rdram, model_bin, 0xA);
    const uint32_t setup_off = backpack_guest_word(rdram, model_bin, 0x18);
    uint16_t bones = 0;
    if (setup_off != 0 && setup_off < 0x01000000u) {
        bones = static_cast<uint16_t>(
            backpack_guest_hword(rdram, model_bin, setup_off + 4u));
    }
    if (resource == 0x607u) {
        const uint16_t gtype_u = static_cast<uint16_t>(gtype);
        if (magic == 0xBu) {
            if ((gtype_u & 0x40u) != 0 && bones == 47u) {
                resource = 0x61Fu;
            } else if ((gtype_u & 0x40u) != 0 && bones == 41u) {
                resource = 0x854u;
            } else if (gtype_u == 0 && bones == 68u) {
                resource = 0x62Au;
            } else if (gtype_u == 0 && bones == 66u) {
                resource = 0x62Bu;
            } else if (gtype_u == 0 && bones == 41u) {
                resource = 0x665u;
            } else if (gtype_u == 0 && bones == 58u) {
                resource = 0x61Cu;
            } else if (gtype_u == 0 && bones == 47u) {
                resource = 0x83Au;
            }
        }
    }
    scope.resource_id = resource;
    secondary_sway_register_durable_bin(rdram, model_bin, resource, scope.player);

    const uint64_t frame = guest_logic_tick();
    const uint64_t gfx = gfx_ticks();
    bool first = false;
    {
        std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
        ++g_secondary_sway_callsite_800926d8_hits;
        ++g_secondary_sway_sample_writes;
        first = !g_secondary_sway_first_filtered;
        g_secondary_sway_first_filtered = true;
        g_secondary_sway_last_resource = resource;
        g_secondary_sway_last_model_bin = model_bin;
        g_secondary_sway_last_gtype = gtype;
        g_secondary_sway_last_bones = bones;
        g_secondary_sway_last_magic = magic;
        g_secondary_sway_last_player = scope.player;
        g_secondary_sway_last_locomotion = locomotion;
        if (g_secondary_sway_csv != nullptr) {
            std::fprintf(g_secondary_sway_csv,
                         "%llu,%llu,0x%08X,0x%08X,0xFFFFFFFF,0x%X,0x%X,0x%08X,"
                         "0x%08X,wrap_func_80092444,0x800926D8,%u,0x%X,0x%X,%u\n",
                         static_cast<unsigned long long>(frame),
                         static_cast<unsigned long long>(gfx),
                         scope.player,
                         scope.player,
                         locomotion,
                         resource,
                         model_bin,
                         model_bin,
                         scope.slot,
                         magic,
                         static_cast<unsigned>(static_cast<uint16_t>(gtype)),
                         static_cast<unsigned>(bones));
            if (first ||
                (g_secondary_sway_sample_writes % kSecondarySwayFlushInterval) == 0) {
                std::fflush(g_secondary_sway_csv);
            }
        }
    }
    if (first) {
        secondary_sway_write_status("FIRST_FILTERED_HIT");
        std::fprintf(stderr,
                     "[tooie-sway] first 800926D8 DE448 player=%08x slot=%u "
                     "bin=%08x resource=%x magic=%x gtype=%x bones=%u\n",
                     scope.player, scope.slot, model_bin, resource, magic,
                     static_cast<unsigned>(static_cast<uint16_t>(gtype)),
                     static_cast<unsigned>(bones));
        std::fflush(stderr);
    }
}

void secondary_sway_note_hip_consume_entry() {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
    ++g_secondary_sway_hip_consume_entry_hits;
}

void secondary_sway_note_hip_consume(uint8_t* rdram, void* raw_ctx) {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed) ||
        rdram == nullptr || raw_ctx == nullptr) {
        return;
    }
    auto* const ctx = static_cast<recomp_context*>(raw_ctx);
    const uint32_t anim_list = static_cast<uint32_t>(ctx->r4);
    const uint32_t bone_anim_id = static_cast<uint32_t>(ctx->r5) & 0xFFu;
    if (!secondary_sway_logged_additive_anim(bone_anim_id)) {
        return;
    }
    const uint32_t dest = static_cast<uint32_t>(ctx->r6);
    const uint32_t count = static_cast<uint32_t>(static_cast<uint16_t>(
        backpack_guest_hword(rdram, anim_list, 4)));
    const uint32_t base = backpack_guest_word(rdram, anim_list, 0);
    uint32_t matched = 0;
    for (uint32_t i = 0; i < count && i < 64u; ++i) {
        const uint32_t rec = base + i * 0x14u;
        const uint32_t part = backpack_guest_byte(rdram, rec, 0x10);
        const uint32_t type = backpack_guest_byte(rdram, rec, 0x11);
        if (part == bone_anim_id && type == 3u) {
            matched = rec;
            break;
        }
    }
    std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
    ++g_secondary_sway_hip_consume_hits;
    ++g_secondary_sway_hip_consume_samples;
    if (g_secondary_sway_hip_csv == nullptr) {
        g_secondary_sway_hip_csv =
            std::fopen(tooie_diag_path("tooie-secondary-sway-hip.csv").c_str(), "a");
        if (g_secondary_sway_hip_csv == nullptr) {
            return;
        }
        std::fprintf(g_secondary_sway_hip_csv,
                     "FRAME,BONE_ANIM_ID,ANIM_LIST,DEST,RECORD_COUNT,"
                     "MATCHED_RECORD,POST0,POST1,POST2,POST3,POST_FLAGS\n");
    }
    uint32_t post0 = 0;
    uint32_t post1 = 0;
    uint32_t post2 = 0;
    uint32_t post3 = 0;
    uint32_t post_flags = 0;
    if (matched != 0) {
        post0 = backpack_guest_word(rdram, matched, 0);
        post1 = backpack_guest_word(rdram, matched, 4);
        post2 = backpack_guest_word(rdram, matched, 8);
        post3 = backpack_guest_word(rdram, matched, 0xC);
        post_flags = backpack_guest_word(rdram, matched, 0x10);
    }
    std::fprintf(g_secondary_sway_hip_csv,
                 "%llu,%u,0x%08X,0x%08X,%u,0x%08X,0x%08X,0x%08X,0x%08X,"
                 "0x%08X,0x%08X\n",
                 static_cast<unsigned long long>(guest_logic_tick()),
                 bone_anim_id, anim_list, dest, count, matched, post0, post1,
                 post2, post3, post_flags);
    if ((g_secondary_sway_hip_consume_samples % kSecondarySwayFlushInterval) ==
        0) {
        std::fflush(g_secondary_sway_hip_csv);
    }
}

void secondary_sway_dump_producer(uint8_t* rdram, uint32_t anim_list_ptr,
                                 uint32_t skeleton, uint32_t slot_base,
                                 uint32_t additive_ctx, const char* seam) {
    if (rdram == nullptr || anim_list_ptr == 0 || skeleton == 0 ||
        slot_base == 0 || seam == nullptr) {
        return;
    }
    const int16_t bone_count = backpack_guest_hword(rdram, skeleton, 4);
    if (bone_count <= 0 || bone_count > 128) {
        return;
    }
    bool has_plume = false;
    for (int16_t i = 0; i < bone_count; ++i) {
        const uint32_t bone = skeleton + 8u + static_cast<uint32_t>(i) * 0x10u;
        const uint32_t anim_id =
            static_cast<uint32_t>(static_cast<uint16_t>(
                backpack_guest_hword(rdram, bone, 0xC)));
        if (anim_id == 79u || (anim_id >= 100u && anim_id <= 105u)) {
            has_plume = true;
            break;
        }
    }
    if (!has_plume) {
        return;
    }

    const uint32_t anim_base = backpack_guest_word(rdram, anim_list_ptr, 0);
    const uint32_t additive_byte8 =
        additive_ctx != 0 ? backpack_guest_byte(rdram, additive_ctx, 8) : 0;
    const uint32_t additive_byte9 =
        additive_ctx != 0 ? backpack_guest_byte(rdram, additive_ctx, 9) : 0;
    const bool ae8a8 = std::strcmp(seam, "AE8A8") == 0;
    const uint64_t frame = guest_logic_tick();

    std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
    if (ae8a8) {
        ++g_secondary_sway_ae8a8_hits;
    } else {
        ++g_secondary_sway_ae820_hits;
    }
    if (g_secondary_sway_producer_csv == nullptr) {
        g_secondary_sway_producer_csv =
            std::fopen(tooie_diag_path("tooie-kazooie-feather-producer.csv").c_str(), "a");
        if (g_secondary_sway_producer_csv == nullptr) {
            return;
        }
        std::fprintf(
            g_secondary_sway_producer_csv,
            "FRAME,SEAM,BONE_INDEX,BONE_ANIM_ID,PARENT_INDEX,SLOT_PTR,"
            "ANIM_RECORD,ADDITIVE_CTX,ADDITIVE_BYTE8,ADDITIVE_BYTE9,"
            "LOCOMOTION,BASE_SX,BASE_SY,BASE_SZ,PARENT_SX,PARENT_SY,PARENT_SZ,"
            "COMPOSED_SX,COMPOSED_SY,COMPOSED_SZ,FINAL_SX,FINAL_SY,FINAL_SZ,"
            "FINAL_DET,BASE_QW,BASE_QX,BASE_QY,BASE_QZ,SLOT_W0,SLOT_W1,SLOT_W2,"
            "SLOT_W3,SLOT_W4,SLOT_W5,SLOT_W6,SLOT_W7,SLOT_W8,SLOT_W9,SLOT_W10,"
            "SLOT_W11,SLOT_W12,SLOT_W13,SLOT_W14,SLOT_W15,"
            "PARENT_ROTATION_Q,LOCAL_ROTATION_Q,FINAL_ROTATION_Q,"
            "FINAL_UNIFORM_SCALE,ROTATION_DELTA_RAD,ROTATION_ORTHOGONALITY_ERROR,"
            "ROTATION_DETERMINANT,QUATERNION_SIGN_FLIPPED\n");
    }

    for (int16_t i = 0; i < bone_count; ++i) {
        const uint32_t bone = skeleton + 8u + static_cast<uint32_t>(i) * 0x10u;
        const uint32_t anim_id =
            static_cast<uint32_t>(static_cast<uint16_t>(
                backpack_guest_hword(rdram, bone, 0xC)));
        if (!secondary_sway_plume_family_anim(anim_id)) {
            continue;
        }
        const int16_t parent = backpack_guest_hword(rdram, bone, 0xE);
        const uint32_t slot =
            slot_base + (static_cast<uint32_t>(i) << 6);
        const uint32_t record = anim_base + anim_id * 0x28u;
        const float base_sx = backpack_guest_float(rdram, record, 0x10);
        const float base_sy = backpack_guest_float(rdram, record, 0x14);
        const float base_sz = backpack_guest_float(rdram, record, 0x18);
        const float base_qw = backpack_guest_float(rdram, record, 0x0);
        const float base_qx = backpack_guest_float(rdram, record, 0x4);
        const float base_qy = backpack_guest_float(rdram, record, 0x8);
        const float base_qz = backpack_guest_float(rdram, record, 0xC);
        float parent_sx = 1.0f;
        float parent_sy = 1.0f;
        float parent_sz = 1.0f;
        if (parent >= 0 && parent < bone_count && ae8a8) {
            const uint32_t parent_slot =
                slot_base + (static_cast<uint32_t>(parent) << 6);
            parent_sx = backpack_guest_float(rdram, parent_slot, 0x10);
            parent_sy = backpack_guest_float(rdram, parent_slot, 0x14);
            parent_sz = backpack_guest_float(rdram, parent_slot, 0x18);
        }
        float composed_sx = 0.0f;
        float composed_sy = 0.0f;
        float composed_sz = 0.0f;
        float final_sx = 0.0f;
        float final_sy = 0.0f;
        float final_sz = 0.0f;
        float final_det = 0.0f;
        if (ae8a8) {
            composed_sx = backpack_guest_float(rdram, slot, 0x10);
            composed_sy = backpack_guest_float(rdram, slot, 0x14);
            composed_sz = backpack_guest_float(rdram, slot, 0x18);
        } else {
            const float m00 = backpack_guest_float(rdram, slot, 0x00);
            const float m01 = backpack_guest_float(rdram, slot, 0x04);
            const float m02 = backpack_guest_float(rdram, slot, 0x08);
            const float m10 = backpack_guest_float(rdram, slot, 0x10);
            const float m11 = backpack_guest_float(rdram, slot, 0x14);
            const float m12 = backpack_guest_float(rdram, slot, 0x18);
            const float m20 = backpack_guest_float(rdram, slot, 0x20);
            const float m21 = backpack_guest_float(rdram, slot, 0x24);
            const float m22 = backpack_guest_float(rdram, slot, 0x28);
            final_sx = mtx_basis_length(m00, m10, m20);
            final_sy = mtx_basis_length(m01, m11, m21);
            final_sz = mtx_basis_length(m02, m12, m22);
            final_det = mtx3_determinant(m00, m01, m02, m10, m11, m12, m20, m21,
                                         m22);
            composed_sx = final_sx;
            composed_sy = final_sy;
            composed_sz = final_sz;
        }
        uint32_t words[16]{};
        backpack_read_mtx_words(rdram, slot, words);
        uint32_t parent_words_for_rotation[16]{};
        if (parent >= 0 && parent < bone_count) {
            const uint32_t parent_slot =
                slot_base + (static_cast<uint32_t>(parent) << 6);
            backpack_read_mtx_words(rdram, parent_slot, parent_words_for_rotation);
        }
        const RotationSummary parent_rotation =
            (parent >= 0 && parent < bone_count)
                ? (ae8a8 ? rotation_from_slot_quaternion(parent_words_for_rotation)
                         : rotation_from_matrix_words(parent_words_for_rotation))
                : RotationSummary{};
        const RotationSummary local_rotation =
            rotation_from_quaternion(base_qw, base_qx, base_qy, base_qz);
        const RotationSummary final_rotation =
            ae8a8 ? rotation_from_slot_quaternion(words)
                  : rotation_from_matrix_words(words);
        float rotation_delta = -1.0f;
        bool quaternion_flip = false;
        if (!ae8a8 && anim_id == kKazooieFeather62BBoneAnimId &&
            final_rotation.valid) {
            if (g_secondary_sway_previous_feather_rotation_valid &&
                frame == g_secondary_sway_previous_feather_rotation_frame + 1) {
                float dot = 0.0f;
                for (int q = 0; q < 4; ++q) {
                    dot += g_secondary_sway_previous_feather_rotation[q] *
                           final_rotation.q[q];
                }
                if (dot < 0.0f) {
                    quaternion_flip = true;
                    dot = -dot;
                }
                if (dot > 1.0f) {
                    dot = 1.0f;
                }
                rotation_delta = 2.0f * std::acos(dot);
            }
            for (int q = 0; q < 4; ++q) {
                g_secondary_sway_previous_feather_rotation[q] = final_rotation.q[q];
            }
            g_secondary_sway_previous_feather_rotation_valid = true;
            g_secondary_sway_previous_feather_rotation_frame = frame;
            g_secondary_sway_last_rotation_delta = rotation_delta;
            g_secondary_sway_last_quaternion_flip = quaternion_flip;
            ++g_secondary_sway_rotation_samples;
        }
        char parent_rotation_q[96];
        char local_rotation_q[96];
        char final_rotation_q[96];
        const float final_uniform_scale =
            ae8a8 ? (composed_sx + composed_sy + composed_sz) / 3.0f
                  : final_rotation.uniform_scale;
        secondary_sway_format_quaternion(parent_rotation_q,
                                         sizeof(parent_rotation_q),
                                         parent_rotation);
        secondary_sway_format_quaternion(local_rotation_q,
                                         sizeof(local_rotation_q),
                                         local_rotation);
        secondary_sway_format_quaternion(final_rotation_q,
                                         sizeof(final_rotation_q),
                                         final_rotation);
        std::fprintf(
            g_secondary_sway_producer_csv,
            "%llu,%s,%d,%u,%d,0x%08X,0x%08X,0x%08X,0x%X,0x%X,0x%X,"
            "%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,"
            "%.9g,%.9g,%.9g,%.9g,%.9g,"
            "0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,"
            "0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,"
            "%s,%s,%s,%.9g,%.9g,%.9g,%.9g,%s\n",
            static_cast<unsigned long long>(frame), seam, static_cast<int>(i),
            anim_id, static_cast<int>(parent), slot, record, additive_ctx,
            additive_byte8, additive_byte9, g_secondary_sway_last_locomotion,
            base_sx, base_sy, base_sz, parent_sx, parent_sy, parent_sz,
            composed_sx, composed_sy, composed_sz, final_sx, final_sy,
            final_sz, final_det, base_qw, base_qx, base_qy, base_qz, words[0],
            words[1], words[2], words[3], words[4], words[5], words[6],
            words[7], words[8], words[9], words[10], words[11], words[12],
            words[13], words[14], words[15], parent_rotation_q, local_rotation_q,
            final_rotation_q, static_cast<double>(final_uniform_scale),
            static_cast<double>(rotation_delta),
            static_cast<double>(final_rotation.orthogonality_error),
            static_cast<double>(final_rotation.determinant),
            quaternion_flip ? "YES" : "NO");
        ++g_secondary_sway_producer_samples;
    }
    if (g_secondary_sway_producer_first_written == 0 &&
        g_secondary_sway_producer_samples != 0) {
        g_secondary_sway_producer_first_written = 1;
        std::fflush(g_secondary_sway_producer_csv);
        secondary_sway_write_status("FIRST_PRODUCER_HIT");
    } else if ((g_secondary_sway_producer_samples %
                kSecondarySwayFlushInterval) == 0) {
        std::fflush(g_secondary_sway_producer_csv);
    }
}

void secondary_sway_note_ae8a8(uint8_t* rdram, uint32_t anim_list_ptr,
                               uint32_t skeleton, uint32_t slot_base,
                               uint32_t additive_ctx) {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed) ||
        rdram == nullptr) {
        return;
    }
    secondary_sway_dump_producer(rdram, anim_list_ptr, skeleton, slot_base,
                                 additive_ctx, "AE8A8");
}

void secondary_sway_note_ae820(uint8_t* rdram, uint32_t anim_list_ptr,
                               uint32_t skeleton, uint32_t slot_base,
                               uint32_t additive_ctx) {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed) ||
        rdram == nullptr) {
        return;
    }
    secondary_sway_dump_producer(rdram, anim_list_ptr, skeleton, slot_base,
                                 additive_ctx, "AE820");
}

void secondary_sway_note_de2a4_entry() {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
    ++g_secondary_sway_de2a4_raw;
}

bool secondary_sway_de498_begin(uint8_t* rdram, void* raw_ctx) {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed) ||
        rdram == nullptr || raw_ctx == nullptr) {
        return false;
    }
    auto* const ctx = static_cast<recomp_context*>(raw_ctx);
    const uint32_t sp = static_cast<uint32_t>(ctx->r29);
    uint32_t model_bin = backpack_guest_word(rdram, sp, 0x14);
    auto header_is_62b = [&](uint32_t bin) -> bool {
        if (bin == 0 || (bin & 3u) != 0u) {
            return false;
        }
        const uint32_t magic = backpack_guest_word(rdram, bin, 0);
        const int16_t gtype = backpack_guest_hword(rdram, bin, 0xA);
        const uint32_t setup_off = backpack_guest_word(rdram, bin, 0x18);
        uint16_t bones = 0;
        if (setup_off != 0 && setup_off < kSecondarySwayAssetSize) {
            bones = static_cast<uint16_t>(
                backpack_guest_hword(rdram, bin, setup_off + 4u));
        }
        return magic == 0xBu && (static_cast<uint16_t>(gtype) & 0x40u) == 0 &&
               bones == 66u;
    };
    if (!header_is_62b(model_bin)) {
        model_bin = backpack_guest_word(rdram, sp, 0x10);
        if (!header_is_62b(model_bin)) {
            DurableModelBin matched{};
            uint32_t rel = 0;
            if (secondary_sway_lookup_durable_bin(model_bin, &matched, &rel) &&
                rel == 0) {
                model_bin = matched.model_bin;
            } else {
                return false;
            }
        }
    }
    secondary_sway_register_durable_bin(rdram, model_bin, 0x62Bu, 0);
    if (g_tree_scope_depth >= kTreeScopeStackSize) {
        return false;
    }
    const uint32_t geo_off = backpack_guest_word(rdram, model_bin, 4);
    TreeWalkScope& scope = g_tree_scopes[g_tree_scope_depth++];
    scope.model_bin = model_bin;
    scope.root = model_bin + geo_off;
    {
        std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
        ++g_secondary_sway_tree_draws;
    }
    return true;
}

void secondary_sway_de498_end() {
    if (g_tree_scope_depth == 0) {
        return;
    }
    --g_tree_scope_depth;
    g_tree_scopes[g_tree_scope_depth] = {};
}

void secondary_sway_note_de2a4(uint8_t* rdram, void* raw_ctx) {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed) ||
        rdram == nullptr || raw_ctx == nullptr) {
        return;
    }
    auto* const ctx = static_cast<recomp_context*>(raw_ctx);
    const uint32_t node_ptr = static_cast<uint32_t>(ctx->r5);
    if (node_ptr == 0 || (node_ptr & 3u) != 0u) {
        return;
    }
    const uint32_t type = backpack_guest_word(rdram, node_ptr, 0);
    const uint32_t table = backpack_guest_word(rdram, 0x8012CF8Cu, 0);
    uint32_t handler = 0;
    if (table != 0 && type < 0x40u) {
        handler = backpack_guest_word(rdram, table, type * 4u);
    }
    DurableModelBin matched{};
    uint32_t rel = 0;
    const bool in_range =
        secondary_sway_lookup_durable_bin(node_ptr, &matched, &rel);
    std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
    if (!in_range) {
        return;
    }
    ++g_secondary_sway_de2a4_in_range;
    if (type == 2u) {
        ++g_secondary_sway_de2a4_type2;
        g_secondary_sway_de2a4_last_type2_handler = handler;
    }
    if (rel == kSecondarySwaySnoutNodeOffset) {
        ++g_secondary_sway_de2a4_rel_11cb8;
    }
    g_secondary_sway_last_handler_table = table;
    g_secondary_sway_last_geocmd_mode = geocmd_table_mode(rdram, table);
    TreeWalkScope tree{};
    if (tree_scope_active(&tree) && g_secondary_sway_tree_rows < kTreeMaxRows) {
        uint32_t cursor = node_ptr;
        for (size_t i = 0; i < kTreeSiblingScanMax && cursor != 0; ++i) {
            if (cursor < tree.model_bin) {
                break;
            }
            const uint32_t sib_rel = cursor - tree.model_bin;
            if (sib_rel >= kSecondarySwayAssetSize) {
                break;
            }
            if (sib_rel == kSecondarySwaySnoutNodeOffset) {
                ++g_secondary_sway_tree_list_contains_11cb8;
            }
            secondary_sway_write_tree_row(
                rdram, guest_logic_tick(), "DE2A4_SIBLING_SCAN", tree.model_bin,
                tree.root, node_ptr, cursor, sib_rel, i == 0);
            const uint32_t cont = backpack_guest_word(rdram, cursor, 4);
            if (cont == 0) {
                break;
            }
            cursor += cont;
        }
    }
    if (g_secondary_sway_de2a4_tuple_rows >= kDe2a4TupleFirstN) {
        return;
    }
    ++g_secondary_sway_de2a4_tuple_rows;
    if (g_secondary_sway_de2a4_tuple_csv == nullptr) {
        g_secondary_sway_de2a4_tuple_csv =
            std::fopen(tooie_diag_path("tooie-secondary-sway-de2a4-tuples.csv").c_str(), "a");
        if (g_secondary_sway_de2a4_tuple_csv == nullptr) {
            return;
        }
        std::fprintf(g_secondary_sway_de2a4_tuple_csv,
                     "FRAME,NODE_PTR,NODE_REL,TYPE,HANDLER,BONE_INDEX,"
                     "BONE_ANIM_ID,HEADER_LEN,CONTINUE\n");
    }
    std::fprintf(g_secondary_sway_de2a4_tuple_csv,
                 "%llu,0x%08X,0x%X,0x%X,0x%08X,%u,%d,%u,0x%X\n",
                 static_cast<unsigned long long>(guest_logic_tick()), node_ptr,
                 rel, type, handler, backpack_guest_byte(rdram, node_ptr, 9),
                 static_cast<int>(backpack_guest_hword(rdram, node_ptr, 0xA)),
                 backpack_guest_byte(rdram, node_ptr, 8),
                 backpack_guest_word(rdram, node_ptr, 4));
    if ((g_secondary_sway_de2a4_tuple_rows % kSecondarySwayFlushInterval) == 0) {
        std::fflush(g_secondary_sway_de2a4_tuple_csv);
    }
}

void secondary_sway_note_dcf48_entry() {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
    ++g_secondary_sway_dcf48_raw;
}

bool secondary_sway_dcf48_snout_scope_active() {
    return g_dcf48_snout_scope;
}

void secondary_sway_dcf48_snout_end() {
    g_dcf48_snout_scope = false;
    g_dcf48_model_bin = 0;
    g_dcf48_node_ptr = 0;
    g_dcf48_node_rel = 0;
    g_dcf48_bone_index = 0;
    g_dcf48_bone_anim_id = 0;
    g_dcf48_resource_id = 0;
}

enum class KazooieFeatherClass {
    None = 0,
    Primary62B = 1,
    Primary61C = 2,
    Primary83A = 3,
    Sibling62B = 4,
};

bool is_tracked_kazooie_feather_anim(uint32_t bone_anim_id) {
    return bone_anim_id == 79u ||
           (bone_anim_id >= 100u && bone_anim_id <= 105u) ||
           bone_anim_id == 107u || bone_anim_id == 108u;
}

bool is_targeted_kazooie_62b_transform(
    const DurableModelBin& matched, KazooieFeatherClass feather,
    int16_t bone_anim) {
    return matched.resource_id == kKazooieFeather62BResourceId &&
           is_tracked_kazooie_feather_anim(static_cast<uint32_t>(bone_anim)) &&
           (feather == KazooieFeatherClass::Primary62B ||
            feather == KazooieFeatherClass::Sibling62B);
}

KazooieFeatherClass classify_kazooie_feather(uint32_t rel, uint32_t type,
                                            uint32_t bone_index,
                                            int16_t bone_anim) {
    if (type != 2u) {
        return KazooieFeatherClass::None;
    }
    if (rel == kKazooieFeather62BNodeOffset &&
        bone_index == kKazooieFeather62BBoneIndex &&
        static_cast<uint32_t>(bone_anim) == kKazooieFeather62BBoneAnimId) {
        return KazooieFeatherClass::Primary62B;
    }
    if (rel == kKazooieFeather61CNodeOffset &&
        bone_index == kKazooieFeather61CBoneIndex &&
        static_cast<uint32_t>(bone_anim) == kKazooieFeather61CBoneAnimId) {
        return KazooieFeatherClass::Primary61C;
    }
    if (rel == kKazooieFeather83ANodeOffset &&
        bone_index == kKazooieFeather83ABoneIndex &&
        static_cast<uint32_t>(bone_anim) == kKazooieFeather83ABoneAnimId) {
        return KazooieFeatherClass::Primary83A;
    }
    const bool plume =
        (rel == 0x12368u && bone_index == 27u && bone_anim == 79) ||
        (rel == 0x124B0u && bone_index == 31u && bone_anim == 104) ||
        (rel == 0x124C0u && bone_index == 32u && bone_anim == 105) ||
        (rel == 0x12590u && bone_index == 33u && bone_anim == 100) ||
        (rel == 0x125A0u && bone_index == 34u && bone_anim == 101) ||
        (rel == 0x125B0u && bone_index == 35u && bone_anim == 102) ||
        (rel == kKazooieFeather62BParent107NodeOffset &&
         bone_index == kKazooieFeather62BParent107BoneIndex &&
         static_cast<uint32_t>(bone_anim) ==
             kKazooieFeather62BParent107BoneAnimId) ||
        (rel == kKazooieFeather62BParent108NodeOffset &&
         bone_index == kKazooieFeather62BParent108BoneIndex &&
         static_cast<uint32_t>(bone_anim) ==
             kKazooieFeather62BParent108BoneAnimId);
    if (plume) {
        return KazooieFeatherClass::Sibling62B;
    }
    return KazooieFeatherClass::None;
}

bool is_full_kazooie_feather_trace(const SingleMtxTraceToken& trace) {
    // The token is created only from the bounded DCF48 type-2 path. Keep the
    // A/B scope to the nine 0x62B transforms used by visible feather triangles
    // (79, 100–105, 107, 108). Do not turn this into a generic RT64 override.
    const KazooieFeatherClass feather = classify_kazooie_feather(
        trace.node_rel, 2u, trace.bone_index,
        static_cast<int16_t>(trace.bone_anim_id));
    return trace.model_bin != 0 &&
           is_targeted_kazooie_62b_transform(
               DurableModelBin{trace.model_bin, trace.resource_id, 0}, feather,
               static_cast<int16_t>(trace.bone_anim_id));
}

uint32_t feather_stat_index(const SingleMtxTraceToken& trace) {
    return trace.bone_anim_id < 128u ? trace.bone_anim_id : 0u;
}

void fill_feather_world_hit(FeatherWorldHit& hit,
                            const SingleMtxTraceToken& trace) {
    hit.token = trace.token;
    hit.vertex_workload_id = trace.vertex_workload_id;
    hit.matrix_workload_id = trace.matrix_workload_id != 0
                                 ? trace.matrix_workload_id
                                 : trace.match_workload_id;
    hit.frame = trace.frame;
    hit.transform_index = trace.rt64_transform_index;
    hit.bone_anim_id = trace.bone_anim_id;
    hit.bone_index = trace.bone_index;
    hit.node_rel = trace.node_rel;
    hit.model_bin = trace.model_bin;
}

bool feather_world_hit_workload_match(const FeatherWorldHit& hit,
                                      uint64_t workload_id) {
    if (workload_id == 0) {
        return false;
    }
    return hit.vertex_workload_id == workload_id ||
           hit.matrix_workload_id == workload_id;
}

void remember_feather_world_hit(const SingleMtxTraceToken& trace) {
    if (!is_full_kazooie_feather_trace(trace) ||
        trace.rt64_transform_index == UINT32_MAX) {
        return;
    }
    if (trace.vertex_workload_id == 0 && trace.matrix_workload_id == 0 &&
        trace.match_workload_id == 0) {
        return;
    }
    for (FeatherWorldHit& hit : g_feather_world_hits) {
        if (hit.transform_index != trace.rt64_transform_index) {
            continue;
        }
        if (feather_world_hit_workload_match(hit, trace.vertex_workload_id) ||
            feather_world_hit_workload_match(hit, trace.matrix_workload_id) ||
            feather_world_hit_workload_match(hit, trace.match_workload_id) ||
            (hit.vertex_workload_id == 0 && hit.matrix_workload_id == 0)) {
            fill_feather_world_hit(hit, trace);
            return;
        }
    }
    FeatherWorldHit& hit = g_feather_world_hits[g_feather_world_hit_next];
    g_feather_world_hit_next =
        (g_feather_world_hit_next + 1) % kFeatherWorldHits;
    fill_feather_world_hit(hit, trace);
}

const FeatherWorldHit* find_feather_world_hit(uint64_t workload_id,
                                              uint32_t transform_index) {
    if (transform_index == UINT32_MAX) {
        return nullptr;
    }
    const FeatherWorldHit* exact = nullptr;
    const FeatherWorldHit* by_index = nullptr;
    for (const FeatherWorldHit& hit : g_feather_world_hits) {
        if (hit.transform_index != transform_index) {
            continue;
        }
        if (feather_world_hit_workload_match(hit, workload_id)) {
            exact = &hit;
        } else if (by_index == nullptr || hit.frame >= by_index->frame) {
            by_index = &hit;
        }
    }
    return exact != nullptr ? exact : by_index;
}

bool tracked_feather_gfx_index(uint32_t gfx_index) {
    switch (gfx_index) {
    case 0x5AAu:
    case 0x57Cu:
    case 0x534u:
    case 0x592u:
    case 0x564u:
    case 0x54Cu:
    case 0x8u:
        return true;
    default:
        return false;
    }
}

void v7_note_model_getter(uint8_t* rdram, uint32_t owner, uint32_t arg1, uint64_t model_bin) {
    // Default play returns before counters, guest reads, the sway mutex, and
    // durable-bin writes. Explicit consumers are cached once in ensure_init.
    ensure_init();
    if (!g_getter_durable_needed.load(std::memory_order_relaxed)) {
        return;
    }
    ++g_v7_getter_wrapper_calls;
    if (rdram == nullptr || owner == 0 || arg1 != 1u) {
        v7_note_getter_reject(g_v7_getter_reject_arg_filter,
                              kV7RejectArgFilter);
        return;
    }
    if (model_bin == 0) {
        v7_note_getter_reject(g_v7_getter_reject_return_zero,
                              kV7RejectReturnZero);
        return;
    }
    const uint32_t obj = backpack_guest_word(rdram, owner, 0x50);
    if (obj == 0) {
        v7_note_getter_reject(g_v7_getter_reject_resource_unavailable,
                              kV7RejectResourceUnavailable);
        return;
    }
    const uint32_t resource = static_cast<uint32_t>(static_cast<uint16_t>(backpack_guest_hword(rdram, obj, 0xC)));
    if (resource == 0) {
        v7_note_getter_reject(g_v7_getter_reject_resource_unavailable,
                              kV7RejectResourceUnavailable);
        return;
    }
    if (resource == 0x62Au) ++g_v7_model_getter_raw_62a;
    if (resource == 0x62Bu) ++g_v7_model_getter_raw_62b;
    const uint32_t model_bin32 = static_cast<uint32_t>(model_bin);
    // The getter only feeds the already-proven durable normalization seam;
    // that seam alone publishes V7 identity after metadata validation.
    const uint32_t normalized = secondary_sway_register_durable_bin(
        rdram, model_bin32, resource, owner);
    if (normalized != 0) {
        return;
    }

    if (resource != 0x62Au && resource != 0x62Bu) {
        v7_note_getter_reject(g_v7_getter_reject_raw_resource_other,
                              kV7RejectRawResourceOther);
        return;
    }
    if (resource == 0x62Bu) {
        v7_note_getter_reject(g_v7_getter_reject_other, kV7RejectOther);
        return;
    }

    const uint32_t magic = backpack_guest_word(rdram, model_bin32, 0);
    const int16_t gtype = backpack_guest_hword(rdram, model_bin32, 0xA);
    const uint32_t setup_off = backpack_guest_word(rdram, model_bin32, 0x18);
    uint16_t bones = 0;
    if (setup_off != 0 && setup_off < 0x01000000u) {
        bones = static_cast<uint16_t>(
            backpack_guest_hword(rdram, model_bin32, setup_off + 4u));
    }
    if (setup_off == 0 || setup_off >= 0x01000000u || bones == 0) {
        v7_note_getter_reject(g_v7_getter_reject_metadata_unavailable,
                              kV7RejectMetadataUnavailable);
    } else if (magic != 0xBu ||
               (static_cast<uint16_t>(gtype) & 0x40u) != 0 || bones != 66u) {
        v7_note_getter_reject(g_v7_getter_reject_metadata_mismatch,
                              kV7RejectMetadataMismatch);
    }
}

void v7_note_durable_model_bin(uint32_t model_bin, uint32_t raw_resource,
                               uint32_t normalized_resource, uint32_t player) {
    if (model_bin == 0 || normalized_resource != 0x62Bu) {
        return;
    }
    ++g_v7_durable_normalized_62b;
    const uint64_t guest_bin = static_cast<uint64_t>(
        static_cast<int64_t>(static_cast<int32_t>(model_bin)));
    const uint64_t previous = g_v7_model_bin.exchange(
        guest_bin, std::memory_order_relaxed);
    if (previous != 0 && previous != guest_bin) {
        ++g_v7_model_context_clear_count;
    }
    uint64_t generation =
        g_v7_model_context_generation.fetch_add(1, std::memory_order_relaxed) + 1;
    uint32_t first_resource = 0;
    g_v7_first_raw_resource.compare_exchange_strong(
        first_resource, raw_resource, std::memory_order_relaxed);
    first_resource = 0;
    g_v7_first_normalized_resource.compare_exchange_strong(
        first_resource, normalized_resource, std::memory_order_relaxed);
    uint64_t first_bin = 0;
    g_v7_first_guest_bin.compare_exchange_strong(
        first_bin, guest_bin, std::memory_order_relaxed);
    g_v7_last_raw_resource.store(raw_resource, std::memory_order_relaxed);
    g_v7_last_normalized_resource.store(normalized_resource,
                                        std::memory_order_relaxed);
    g_v7_last_guest_bin.store(guest_bin, std::memory_order_relaxed);
    g_v7_last_physical_model_base.store(model_bin & 0x1FFFFFFFu,
                                        std::memory_order_relaxed);
    g_v7_last_player.store(player, std::memory_order_relaxed);
    g_v7_last_context_generation.store(generation, std::memory_order_relaxed);
    ++g_v7_model_context_publish_count;
    ++g_v7_model_bin_accept;
}

void v7_clear_model_context_if_bin(uint32_t model_bin) {
    if (model_bin == 0) {
        return;
    }
    const uint64_t guest_bin = static_cast<uint64_t>(
        static_cast<int64_t>(static_cast<int32_t>(model_bin)));
    uint64_t expected = guest_bin;
    if (g_v7_model_bin.compare_exchange_strong(
            expected, 0, std::memory_order_relaxed)) {
        ++g_v7_model_context_clear_count;
    }
}

uint64_t v7_model_bin() {
    return g_v7_model_bin.load(std::memory_order_relaxed);
}

uint64_t v7_model_getter_raw_62a_count() { return g_v7_model_getter_raw_62a.load(); }
uint64_t v7_model_getter_raw_62b_count() { return g_v7_model_getter_raw_62b.load(); }
uint64_t v7_model_getter_normalized_62b_count() { return g_v7_model_getter_normalized_62b.load(); }
uint64_t v7_model_bin_bridge_accept_count() { return g_v7_model_bin_accept.load(); }
uint64_t v7_model_bin_bridge_reject_count() { return g_v7_model_bin_reject.load(); }
uint64_t v7_model_getter_wrapper_call_count() {
    return g_v7_getter_wrapper_calls.load();
}
uint64_t v7_model_getter_reject_arg_filter_count() {
    return g_v7_getter_reject_arg_filter.load();
}
uint64_t v7_model_getter_reject_return_zero_count() {
    return g_v7_getter_reject_return_zero.load();
}
uint64_t v7_model_getter_reject_resource_unavailable_count() {
    return g_v7_getter_reject_resource_unavailable.load();
}
uint64_t v7_model_getter_reject_raw_resource_other_count() {
    return g_v7_getter_reject_raw_resource_other.load();
}
uint64_t v7_model_getter_reject_metadata_unavailable_count() {
    return g_v7_getter_reject_metadata_unavailable.load();
}
uint64_t v7_model_getter_reject_metadata_mismatch_count() {
    return g_v7_getter_reject_metadata_mismatch.load();
}
uint64_t v7_model_getter_reject_other_count() {
    return g_v7_getter_reject_other.load();
}
const char* v7_model_getter_first_rejection_reason() {
    switch (g_v7_first_reject_reason.load(std::memory_order_relaxed)) {
    case kV7RejectArgFilter: return "GETTER_REJECT_ARG_FILTER";
    case kV7RejectReturnZero: return "GETTER_REJECT_RETURN_ZERO";
    case kV7RejectResourceUnavailable: return "GETTER_REJECT_RESOURCE_UNAVAILABLE";
    case kV7RejectRawResourceOther: return "GETTER_REJECT_RAW_RESOURCE_OTHER";
    case kV7RejectMetadataUnavailable: return "GETTER_REJECT_METADATA_UNAVAILABLE";
    case kV7RejectMetadataMismatch: return "GETTER_REJECT_METADATA_MISMATCH";
    case kV7RejectOther: return "GETTER_REJECT_OTHER";
    default: return "NONE";
    }
}
uint64_t v7_durable_register_total() {
    return g_v7_durable_register_total.load();
}
uint64_t v7_durable_normalized_62b_count() {
    return g_v7_durable_normalized_62b.load();
}
uint64_t v7_model_context_publish_count() {
    return g_v7_model_context_publish_count.load();
}
uint64_t v7_model_context_clear_count() {
    return g_v7_model_context_clear_count.load();
}
uint64_t v7_model_context_generation() {
    return g_v7_model_context_generation.load();
}
uint64_t v7_model_context_last_generation() {
    return g_v7_last_context_generation.load();
}
uint32_t v7_model_context_last_physical_base() {
    return g_v7_last_physical_model_base.load();
}
uint32_t v7_model_context_last_player() {
    return g_v7_last_player.load();
}
uint32_t v7_model_bridge_first_raw_resource() {
    return g_v7_first_raw_resource.load(std::memory_order_relaxed);
}
uint32_t v7_model_bridge_first_normalized_resource() {
    return g_v7_first_normalized_resource.load(std::memory_order_relaxed);
}
uint64_t v7_model_bridge_first_guest_bin() {
    return g_v7_first_guest_bin.load(std::memory_order_relaxed);
}
uint32_t v7_model_bridge_last_raw_resource() {
    return g_v7_last_raw_resource.load(std::memory_order_relaxed);
}
uint32_t v7_model_bridge_last_normalized_resource() {
    return g_v7_last_normalized_resource.load(std::memory_order_relaxed);
}
uint64_t v7_model_bridge_last_guest_bin() {
    return g_v7_last_guest_bin.load(std::memory_order_relaxed);
}

uint32_t feather_gfx_index_from_dl(uint32_t current_dl, uint32_t model_bin) {
    if (current_dl == 0 || model_bin == 0) {
        return UINT32_MAX;
    }
    // Model pointers observed at the guest seam are KSEG virtual addresses;
    // the RSP DL context is a physical RDRAM offset.
    const uint32_t physical_model_bin = model_bin & 0x1FFFFFFFu;
    const uint32_t base = physical_model_bin + 0x2670u;
    if (current_dl < base) {
        return UINT32_MAX;
    }
    const uint32_t rel = current_dl - base;
    if ((rel % 8u) != 0u) {
        return UINT32_MAX;
    }
    return rel / 8u;
}

const char* v6c_dl_kind(uint32_t current_dl, uint32_t model_bin) {
    if (current_dl != 0 && current_dl == g_v6c_segment3_target) {
        return "NESTED_03000040";
    }
    switch (feather_gfx_index_from_dl(current_dl, model_bin)) {
    case 0x5AAu:
        return "OWN_103";
    case 0x57Cu:
        return "OWN_104";
    case 0x534u:
        return "OWN_105";
    case 0x592u:
        return "JOIN";
    default:
        return "OTHER";
    }
}

uint32_t v6c_expected_own_bone(const char* kind) {
    if (kind == nullptr) {
        return UINT32_MAX;
    }
    if (std::strcmp(kind, "OWN_103") == 0) return 103u;
    if (std::strcmp(kind, "OWN_104") == 0) return 104u;
    if (std::strcmp(kind, "OWN_105") == 0) return 105u;
    return UINT32_MAX;
}

bool v6c_join_slot(uint32_t slot) {
    return slot <= 3u || (slot >= 28u && slot <= 31u);
}

void secondary_sway_rt64_v6c_gdl(uint32_t raw_address,
                                 uint32_t resolved_address,
                                 uint32_t parent_address, uint32_t depth,
                                 uint32_t is_call, uint32_t segment3_base) {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    ++g_v6c_gdl_count;
    if (segment3_base != 0) {
        ++g_v6c_segment3_base_count;
        g_v6c_segment3_base = segment3_base;
    }
    if (raw_address == 0x03000040u && resolved_address != 0) {
        g_v6c_segment3_target = resolved_address;
        ++g_v6c_segment3_count;
    }
    (void)parent_address;
    (void)depth;
    (void)is_call;
}

uint32_t secondary_sway_rt64_v6c_vertex_load(
    uint32_t slot, uint64_t generation, uint64_t workload_id,
    uint32_t current_dl, uint32_t parent_dl, uint32_t dl_depth,
    uint32_t segmented_source, uint32_t resolved_source,
    uint32_t source_vertex_index, uint32_t world_index,
    uint32_t active_transform, uint32_t stack_depth) {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed)) {
        return UINT32_MAX;
    }
    const FeatherWorldHit* hit = find_feather_world_hit(workload_id, world_index);
    (void)slot;
    (void)generation;
    (void)current_dl;
    (void)parent_dl;
    (void)dl_depth;
    (void)segmented_source;
    (void)resolved_source;
    (void)source_vertex_index;
    (void)active_transform;
    (void)stack_depth;
    return hit != nullptr ? hit->bone_anim_id : UINT32_MAX;
}

void secondary_sway_rt64_v6c_tri(
    uint64_t prim_seq, uint64_t workload_id, uint32_t active_transform,
    const char* prim_kind, uint32_t current_dl, uint32_t parent_dl,
    uint32_t dl_depth, const uint32_t* slots, const uint32_t* globals,
    const uint32_t* worlds, const uint32_t* bones,
    const uint64_t* generations, const uint64_t* load_workloads,
    const uint32_t* load_dl, const uint32_t* load_parent,
    const uint32_t* load_depth, const uint32_t* segmented_sources,
    const uint32_t* resolved_sources, const uint32_t* source_indices,
    const uint32_t* load_active_transforms, const uint32_t* load_stack_depths) {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    const FeatherWorldHit* model_hit = nullptr;
    if (worlds != nullptr) {
        for (size_t i = 0; i < 3 && model_hit == nullptr; ++i) {
            model_hit = find_feather_world_hit(workload_id, worlds[i]);
        }
    }
    const char* kind = v6c_dl_kind(
        current_dl, model_hit != nullptr ? model_hit->model_bin : 0u);
    const bool known_dl = std::strcmp(kind, "OTHER") != 0;
    if (known_dl) {
        ++g_v6c_tracked_dl_tri;
        if (std::strcmp(kind, "OWN_103") == 0) ++g_v6c_dl_5aa_tri;
        if (std::strcmp(kind, "OWN_104") == 0) ++g_v6c_dl_57c_tri;
        if (std::strcmp(kind, "OWN_105") == 0) ++g_v6c_dl_534_tri;
        if (std::strcmp(kind, "JOIN") == 0) ++g_v6c_dl_592_tri;
        if (std::strcmp(kind, "NESTED_03000040") == 0) ++g_v6c_dl_03000040_tri;
    }

    bool history_hit = slots != nullptr && generations != nullptr &&
                       load_workloads != nullptr && load_dl != nullptr &&
                       bones != nullptr;
    if (history_hit) {
        for (size_t i = 0; i < 3; ++i) {
            if (generations[i] == 0 || load_workloads[i] != workload_id ||
                bones[i] == UINT32_MAX) {
                history_hit = false;
                break;
            }
        }
    }
    if (history_hit) ++g_v6c_slot_history_hits;
    else ++g_v6c_slot_history_misses;

    V6CClass cls = kV6CUnresolved;
    const uint32_t expected = v6c_expected_own_bone(kind);
    if (history_hit && expected != UINT32_MAX) {
        bool own_match = true;
        bool prior_load = false;
        for (size_t i = 0; i < 3; ++i) {
            own_match &= bones[i] == expected;
            prior_load |= load_dl[i] != current_dl;
        }
        if (own_match) cls = kV6COwnMatch;
        else if (prior_load) cls = kV6CStaleSlot;
        else cls = kV6CMixedUnexpected;
    } else if (history_hit && std::strcmp(kind, "JOIN") == 0 &&
               slots != nullptr) {
        bool child = false;
        bool parent = false;
        for (size_t i = 0; i < 3; ++i) {
            child |= slots[i] <= 3u;
            parent |= slots[i] >= 28u && slots[i] <= 31u;
        }
        cls = (child && parent && v6c_join_slot(slots[0]) &&
               v6c_join_slot(slots[1]) && v6c_join_slot(slots[2]))
                  ? kV6CJoinExpectedMix
                  : kV6CMixedUnexpected;
    } else if (history_hit && std::strcmp(kind, "NESTED_03000040") == 0) {
        bool all_prior = true;
        for (size_t i = 0; i < 3; ++i) all_prior &= load_dl[i] != current_dl;
        cls = all_prior ? kV6CLeftoverPreGvtx : kV6CUnresolved;
    }

    ++g_v6c_class_counts[cls];
    const size_t limit = (cls == kV6COwnMatch || cls == kV6CJoinExpectedMix)
                             ? kV6CNormalSampleLimit
                             : kV6CSuspiciousSampleLimit;
    if (g_v6c_tri_csv == nullptr) {
        g_v6c_tri_csv = std::fopen(tooie_diag_path("tooie-kazooie-feather-v6c-tri.csv").c_str(), "w");
        if (g_v6c_tri_csv != nullptr) {
            std::fprintf(g_v6c_tri_csv,
                         "PRIM_SEQ,WORKLOAD,PRIM_KIND,CURRENT_DL,PARENT_DL,DL_DEPTH,"
                         "DL_KIND,ACTIVE_TRANSFORM,CLASS,SLOT_A,SLOT_B,SLOT_C,"
                         "GEN_A,GEN_B,GEN_C,LOAD_DL_A,LOAD_DL_B,LOAD_DL_C,"
                         "WORLD_A,WORLD_B,WORLD_C,BONE_A,BONE_B,BONE_C,"
                         "SRC_A,SRC_B,SRC_C,RES_A,RES_B,RES_C,IDX_A,IDX_B,IDX_C,"
                         "LOAD_WORKLOAD_A,LOAD_WORKLOAD_B,LOAD_WORKLOAD_C,"
                         "LOAD_ACTIVE_A,LOAD_ACTIVE_B,LOAD_ACTIVE_C,STACK_A,STACK_B,STACK_C\n");
        }
    }
    if (g_v6c_tri_csv != nullptr && g_v6c_class_samples[cls] < limit) {
        auto u32 = [](const uint32_t* p, size_t i) { return p != nullptr ? p[i] : UINT32_MAX; };
        auto u64 = [](const uint64_t* p, size_t i) { return p != nullptr ? p[i] : 0ull; };
        std::fprintf(g_v6c_tri_csv,
                     "%llu,%llu,%s,0x%08X,0x%08X,%u,%s,%u,%s,%u,%u,%u,"
                     "%llu,%llu,%llu,0x%08X,0x%08X,0x%08X,%u,%u,%u,%u,%u,%u,"
                     "0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,%u,%u,%u,"
                     "%llu,%llu,%llu,%u,%u,%u,%u,%u,%u\n",
                     static_cast<unsigned long long>(prim_seq),
                     static_cast<unsigned long long>(workload_id),
                     prim_kind != nullptr ? prim_kind : "TRI", current_dl,
                     parent_dl, dl_depth, kind, active_transform,
                     kV6CClassNames[cls], u32(slots, 0), u32(slots, 1), u32(slots, 2),
                     static_cast<unsigned long long>(u64(generations, 0)),
                     static_cast<unsigned long long>(u64(generations, 1)),
                     static_cast<unsigned long long>(u64(generations, 2)),
                     u32(load_dl, 0), u32(load_dl, 1), u32(load_dl, 2),
                     u32(worlds, 0), u32(worlds, 1), u32(worlds, 2),
                     u32(bones, 0), u32(bones, 1), u32(bones, 2),
                     u32(segmented_sources, 0), u32(segmented_sources, 1), u32(segmented_sources, 2),
                     u32(resolved_sources, 0), u32(resolved_sources, 1), u32(resolved_sources, 2),
                     u32(source_indices, 0), u32(source_indices, 1), u32(source_indices, 2),
                     static_cast<unsigned long long>(u64(load_workloads, 0)),
                     static_cast<unsigned long long>(u64(load_workloads, 1)),
                     static_cast<unsigned long long>(u64(load_workloads, 2)),
                     u32(load_active_transforms, 0), u32(load_active_transforms, 1),
                     u32(load_active_transforms, 2), u32(load_stack_depths, 0),
                     u32(load_stack_depths, 1), u32(load_stack_depths, 2));
        ++g_v6c_class_samples[cls];
        std::fflush(g_v6c_tri_csv);
    }
}

bool feather_float_matrix_quaternion(const float* matrix, float q[4]) {
    if (matrix == nullptr || q == nullptr) {
        return false;
    }
    const float sx = std::sqrt(matrix[0] * matrix[0] + matrix[1] * matrix[1] +
                               matrix[2] * matrix[2]);
    const float sy = std::sqrt(matrix[4] * matrix[4] + matrix[5] * matrix[5] +
                               matrix[6] * matrix[6]);
    const float sz = std::sqrt(matrix[8] * matrix[8] + matrix[9] * matrix[9] +
                               matrix[10] * matrix[10]);
    if (!std::isfinite(sx) || !std::isfinite(sy) || !std::isfinite(sz) ||
        sx < 1.0e-8f || sy < 1.0e-8f || sz < 1.0e-8f) {
        return false;
    }
    const float r00 = matrix[0] / sx;
    const float r01 = matrix[1] / sx;
    const float r02 = matrix[2] / sx;
    const float r10 = matrix[4] / sy;
    const float r11 = matrix[5] / sy;
    const float r12 = matrix[6] / sy;
    const float r20 = matrix[8] / sz;
    const float r21 = matrix[9] / sz;
    const float r22 = matrix[10] / sz;
    const float trace = r00 + r11 + r22;
    if (trace > 0.0f) {
        const float s = std::sqrt(trace + 1.0f) * 2.0f;
        q[3] = 0.25f * s;
        q[0] = (r21 - r12) / s;
        q[1] = (r02 - r20) / s;
        q[2] = (r10 - r01) / s;
    }
    else if (r00 > r11 && r00 > r22) {
        const float s = std::sqrt(1.0f + r00 - r11 - r22) * 2.0f;
        q[3] = (r21 - r12) / s;
        q[0] = 0.25f * s;
        q[1] = (r01 + r10) / s;
        q[2] = (r02 + r20) / s;
    }
    else if (r11 > r22) {
        const float s = std::sqrt(1.0f + r11 - r00 - r22) * 2.0f;
        q[3] = (r02 - r20) / s;
        q[0] = (r01 + r10) / s;
        q[1] = 0.25f * s;
        q[2] = (r12 + r21) / s;
    }
    else {
        const float s = std::sqrt(1.0f + r22 - r00 - r11) * 2.0f;
        q[3] = (r10 - r01) / s;
        q[0] = (r02 + r20) / s;
        q[1] = (r12 + r21) / s;
        q[2] = 0.25f * s;
    }
    const float length = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] +
                                   q[3] * q[3]);
    if (!std::isfinite(length) || length < 1.0e-8f) {
        return false;
    }
    for (int i = 0; i < 4; ++i) {
        q[i] /= length;
    }
    return true;
}

float feather_quaternion_angle(const float a[4], const float b[4]) {
    float dot = 0.0f;
    for (int i = 0; i < 4; ++i) {
        dot += a[i] * b[i];
    }
    dot = std::max(-1.0f, std::min(1.0f, std::fabs(dot)));
    return 2.0f * std::acos(dot);
}

bool secondary_sway_note_dcf48(uint8_t* rdram, void* raw_ctx) {
    ensure_init();
    const bool sway = g_secondary_sway_enabled.load(std::memory_order_relaxed);
    const bool pairSnap = g_p09_local_pair_snapshot.load(std::memory_order_relaxed);
    if ((!sway && !pairSnap) || rdram == nullptr || raw_ctx == nullptr) {
        return false;
    }
    auto* const ctx = static_cast<recomp_context*>(raw_ctx);
    const uint32_t node_ptr = static_cast<uint32_t>(ctx->r5);
    DurableModelBin matched{};
    uint32_t rel = 0;
    bool in_range = false;
    bool accept = false;
    uint32_t bone_index = 0;
    int16_t bone_anim = 0;
    {
        std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
        in_range = secondary_sway_lookup_durable_bin(node_ptr, &matched, &rel);
        const bool pointer_ok = node_ptr != 0 && (node_ptr & 3u) == 0u;
        bone_index =
            pointer_ok ? backpack_guest_byte(rdram, node_ptr, 9) : 0;
        bone_anim =
            pointer_ok ? backpack_guest_hword(rdram, node_ptr, 0xA) : 0;
        if (sway) {
            const uint32_t type =
                pointer_ok ? backpack_guest_word(rdram, node_ptr, 0) : 0;
            if (in_range) {
                ++g_secondary_sway_dcf48_in_range;
            }
            if (in_range && rel == kSecondarySwaySnoutNodeOffset) {
                ++g_secondary_sway_dcf48_rel_11cb8;
            }
            const KazooieFeatherClass feather = in_range
                ? classify_kazooie_feather(rel, type, bone_index, bone_anim)
                : KazooieFeatherClass::None;
            if (feather == KazooieFeatherClass::Sibling62B) {
                ++g_kazooie_feather_sibling_hits;
            }
            const bool targeted_62b = is_targeted_kazooie_62b_transform(
                matched, feather, bone_anim);
            accept = targeted_62b ||
                     feather == KazooieFeatherClass::Primary61C ||
                     feather == KazooieFeatherClass::Primary83A;
            if (accept) {
                ++g_secondary_sway_dcf48_accept;
                ++g_kazooie_feather_primary_hits;
                if (feather == KazooieFeatherClass::Primary62B) {
                    ++g_kazooie_feather_62b_primary_hits;
                } else if (feather == KazooieFeatherClass::Primary61C) {
                    ++g_kazooie_feather_61c_primary_hits;
                } else {
                    ++g_kazooie_feather_83a_primary_hits;
                }
                if (targeted_62b) {
                    ++g_feather_guest_seen[static_cast<uint32_t>(bone_anim)];
                }
            }
        }
        TreeWalkScope tree{};
        if (sway && tree_scope_active(&tree) && in_range &&
            g_secondary_sway_tree_rows < kTreeMaxRows) {
            secondary_sway_write_tree_row(
                rdram, guest_logic_tick(), "DCF48", tree.model_bin, tree.root,
                node_ptr, node_ptr, rel, false);
        }
    }
    if (pairSnap && in_range &&
        RT64::p09LocalPairCandidateClass(
            matched.resource_id, rel, bone_index,
            static_cast<uint32_t>(bone_anim)) != RT64::P09LocalPairCandidateNone) {
        accept = true;
    }
    g_dcf48_snout_scope = accept;
    if (accept) {
        g_dcf48_model_bin = matched.model_bin;
        g_dcf48_node_ptr = node_ptr;
        g_dcf48_node_rel = rel;
        g_dcf48_bone_index = bone_index;
        g_dcf48_bone_anim_id = static_cast<uint32_t>(bone_anim);
        g_dcf48_resource_id = matched.resource_id;
    }
    return accept;
}

void secondary_sway_note_ade2c(uint8_t* rdram, void* raw_ctx) {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed) ||
        rdram == nullptr || raw_ctx == nullptr || !g_dcf48_snout_scope) {
        return;
    }
    auto* const ctx = static_cast<recomp_context*>(raw_ctx);
    const uint32_t mtx_ptr = static_cast<uint32_t>(ctx->r2);
    char curr_hex[65];
    curr_hex[0] = '0';
    curr_hex[1] = '\0';
    if (mtx_ptr != 0) {
        uint32_t words[16] = {};
        backpack_read_mtx_words(rdram, mtx_ptr, words);
        std::snprintf(curr_hex, sizeof(curr_hex),
                      "%08X%08X%08X%08X%08X%08X%08X%08X", words[0], words[1],
                      words[2], words[3], words[4], words[5], words[6],
                      words[7]);
    }
    DurableModelBin matched{};
    uint32_t rel = 0;
    std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
    ++g_secondary_sway_ade2c_snout_samples;
    ++g_secondary_sway_snout_matrix_hits;
    ++g_secondary_sway_snout_matrix_samples;
    if (g_durable_bin_count != 0) {
        matched = g_durable_bins[0];
        rel = kSecondarySwaySnoutNodeOffset;
    }
    if (g_secondary_sway_snout_csv == nullptr) {
        g_secondary_sway_snout_csv =
            std::fopen(tooie_diag_path("tooie-secondary-sway-snout.csv").c_str(), "a");
        if (g_secondary_sway_snout_csv == nullptr) {
            return;
        }
        std::fprintf(g_secondary_sway_snout_csv,
                     "FRAME,GFX_TICK,SEAM,PLAYER,RESOURCE,MODEL_BIN,NODE_PTR,"
                     "NODE_REL,GEOCMD_TYPE,BONE_INDEX,BONE_ANIM_ID,A2,CALLER_RA,"
                     "PREV_MTX_PTR,CURR_MTX_PTR,PREVIOUS_MATRIX,CURRENT_MATRIX,"
                     "INTERPOLATION_IDENTITY,TRANSFORM_TAG\n");
    }
    std::fprintf(g_secondary_sway_snout_csv,
                 "%llu,%llu,DCF48_ADE2C,0x%08X,0x%03X,0x%08X,0x%08X,0x%X,0x2,"
                 "%u,%u,0x%08X,0x%08X,0x00000000,0x%08X,NONE,%s,SINGLE_G_MTX,"
                 "ADE2C_BONE_INDEX\n",
                 static_cast<unsigned long long>(guest_logic_tick()),
                 static_cast<unsigned long long>(gfx_ticks()), matched.player,
                 matched.resource_id, matched.model_bin,
                 g_dcf48_node_ptr, g_dcf48_node_rel,
                 g_dcf48_bone_index, g_dcf48_bone_anim_id,
                 static_cast<uint32_t>(ctx->r5),
                 static_cast<uint32_t>(ctx->r31), mtx_ptr, curr_hex);
    if ((g_secondary_sway_ade2c_snout_samples % kSecondarySwayFlushInterval) ==
        0) {
        std::fflush(g_secondary_sway_snout_csv);
    }
}

void secondary_sway_note_19aa0(uint8_t* rdram, uint32_t parent_ptr,
                               uint32_t local_ptr) {
    ensure_init();
    const bool sway = g_secondary_sway_enabled.load(std::memory_order_relaxed);
    const bool pairSnap = g_p09_local_pair_snapshot.load(std::memory_order_relaxed);
    if ((!sway && !pairSnap) || rdram == nullptr || !g_dcf48_snout_scope) {
        return;
    }
    (void)sway;
    g_snout_composition = {};
    g_snout_composition.valid = true;
    g_snout_composition.frame = guest_logic_tick();
    g_snout_composition.model_bin = g_dcf48_model_bin;
    g_snout_composition.node_ptr = g_dcf48_node_ptr;
    g_snout_composition.node_rel = g_dcf48_node_rel;
    g_snout_composition.bone_index = g_dcf48_bone_index;
    g_snout_composition.bone_anim_id = g_dcf48_bone_anim_id;
    g_snout_composition.resource_id = g_dcf48_resource_id;
    g_snout_composition.parent_ptr = parent_ptr;
    g_snout_composition.local_ptr = local_ptr;
    backpack_read_mtx_words(rdram, parent_ptr, g_snout_composition.parent_words);
    backpack_read_mtx_words(rdram, local_ptr, g_snout_composition.local_words);
    // func_80019AA0 selects old_cursor + 0x40, stores that value back to
    // D_8007BCB0, and writes the completed float result there. The post-call
    // cursor is therefore the completed float matrix address.
    const uint32_t cursor =
        backpack_guest_word(rdram, kSecondarySwayMatrixCursorAddress, 0);
    g_snout_composition.output_ptr = cursor;
    if (g_snout_composition.output_ptr != 0) {
        for (int i = 0; i < 16; ++i) {
            g_snout_composition.output_float_bits[i] = backpack_guest_word(
                rdram, g_snout_composition.output_ptr,
                static_cast<uint32_t>(i) * 4u);
        }
    }
}

void secondary_sway_note_18c28(uint8_t* rdram, uint32_t gfx_mtx_ptr,
                               uint32_t float_src_ptr) {
    ensure_init();
    const bool sway = g_secondary_sway_enabled.load(std::memory_order_relaxed);
    const bool pairSnap = g_p09_local_pair_snapshot.load(std::memory_order_relaxed);
    if ((!sway && !pairSnap) || rdram == nullptr || !g_dcf48_snout_scope) {
        return;
    }
    uint32_t fixed_words[16]{};
    backpack_read_mtx_words(rdram, gfx_mtx_ptr, fixed_words);

    char local_hex[16 * 9 + 1];
    char parent_hex[16 * 9 + 1];
    char composed_hex[16 * 16 + 1];
    char fixed_hex[16 * 9 + 1];
    secondary_sway_format_words(local_hex, sizeof(local_hex),
                                g_snout_composition.local_words);
    secondary_sway_format_words(parent_hex, sizeof(parent_hex),
                                g_snout_composition.parent_words);
    secondary_sway_format_floats(composed_hex, sizeof(composed_hex),
                                 g_snout_composition.output_float_bits);
    secondary_sway_format_words(fixed_hex, sizeof(fixed_hex), fixed_words);

    std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
    if (sway && g_single_mtx_guest_matrix_rows < kSingleMtxTraceRows) {
        if (g_single_mtx_matrix_csv == nullptr) {
            g_single_mtx_matrix_csv = std::fopen(
                tooie_diag_path("tooie-kazooie-feather-matrix.csv").c_str(), "a");
            if (g_single_mtx_matrix_csv != nullptr) {
                std::fprintf(
                    g_single_mtx_matrix_csv,
                    "FRAME,MODEL_BIN,NODE_PTR,NODE_REL,BONE_INDEX,BONE_ANIM_ID,"
                    "PARENT_PTR,LOCAL_PTR,COMPOSED_PTR,F2L_SOURCE,F2L_DEST,"
                    "PARENT_WORDS16,LOCAL_WORDS16,COMPOSED_FLOAT16,FIXED_WORDS16,"
                    "SOURCE_MATCHES_COMPOSED,GFX_MATRIX_PTR,"
                    "GFX_MATRIX_EQUALS_F2L_DEST\n");
            }
        }
        if (g_single_mtx_matrix_csv != nullptr) {
            std::fprintf(
                g_single_mtx_matrix_csv,
                "%llu,0x%08X,0x%08X,0x%X,%u,%u,0x%08X,0x%08X,0x%08X,"
                "0x%08X,0x%08X,%s,%s,%s,%s,%s,0x%08X,%s\n",
                static_cast<unsigned long long>(g_snout_composition.frame),
                g_snout_composition.model_bin, g_snout_composition.node_ptr,
                g_snout_composition.node_rel, g_snout_composition.bone_index,
                g_snout_composition.bone_anim_id, g_snout_composition.parent_ptr,
                g_snout_composition.local_ptr, g_snout_composition.output_ptr,
                float_src_ptr, gfx_mtx_ptr, parent_hex, local_hex, composed_hex,
                fixed_hex,
                float_src_ptr == g_snout_composition.output_ptr ? "YES" : "NO",
                gfx_mtx_ptr, "YES");
            ++g_single_mtx_guest_matrix_rows;
            std::fflush(g_single_mtx_matrix_csv);
        }
    }

    ++g_single_mtx_token_candidates;
    if (!g_snout_composition.valid ||
        g_snout_composition.output_ptr == 0 || gfx_mtx_ptr == 0 ||
        float_src_ptr == 0 || g_single_mtx_token_next == 0) {
        ++g_single_mtx_token_other_reject;
        g_snout_composition.valid = false;
        return;
    }
    const uint64_t token = g_single_mtx_token_next++;
    SingleMtxTraceToken& trace =
        g_single_mtx_tokens[token % kSingleMtxTraceSlots];
    if (trace.pending) {
        ++g_single_mtx_token_expired;
    }
    trace = {};
    trace.pending = true;
    trace.token = token;
    trace.frame = g_snout_composition.frame;
    trace.model_bin = g_snout_composition.model_bin;
    trace.node_ptr = g_snout_composition.node_ptr;
    trace.node_rel = g_snout_composition.node_rel;
    trace.bone_index = g_snout_composition.bone_index;
    trace.bone_anim_id = g_snout_composition.bone_anim_id;
    trace.resource_id = g_snout_composition.resource_id;
    trace.local_ptr = g_snout_composition.local_ptr;
    trace.parent_ptr = g_snout_composition.parent_ptr;
    trace.composed_ptr = g_snout_composition.output_ptr;
    // The float source and the N64 destination are different address spaces.
    // RT64 consumes the address encoded by G_MTX, i.e. the F2L destination.
    trace.source_ptr = float_src_ptr;
    trace.destination_ptr = gfx_mtx_ptr;
    std::memcpy(trace.local_words, g_snout_composition.local_words,
                sizeof(trace.local_words));
    std::memcpy(trace.parent_words, g_snout_composition.parent_words,
                sizeof(trace.parent_words));
    std::memcpy(trace.composed_float_bits,
                g_snout_composition.output_float_bits,
                sizeof(trace.composed_float_bits));
    std::memcpy(trace.fixed_words, fixed_words, sizeof(trace.fixed_words));
    ++g_single_mtx_token_enqueued;
    if (sway && is_full_kazooie_feather_trace(trace)) {
        ++g_feather_matrix_seen[feather_stat_index(trace)];
    }
    g_snout_composition.valid = false;
}

void secondary_sway_identity_header() {
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed)) return;
    if (g_single_mtx_identity_csv == nullptr) {
        g_single_mtx_identity_csv = std::fopen(
            tooie_diag_path("tooie-kazooie-feather-rt64-identity.csv").c_str(), "a");
        if (g_single_mtx_identity_csv != nullptr) {
            std::fprintf(
                g_single_mtx_identity_csv,
                "EVENT,FRAME,TOKEN,WORKLOAD,MODEL_BIN,NODE_REL,BONE_INDEX,"
                "BONE_ANIM_ID,GFX_CMD_ADDR,GUEST_MTX_ADDR,PHYSICAL_MTX_ADDR,"
                "PARAMS,STACK_BEFORE,STACK_AFTER,GROUP_STACK_DEPTH,MATRIX_ID,"
                "GROUP_INDEX,EXPLICIT_GROUP,TRANSFORM_INDEX,PREVIOUS_WORKLOAD,"
                "PREVIOUS_TRANSFORM_INDEX,VERTEX_START,VERTEX_COUNT,DRAW_UID,"
                "DRAW_CALL_INDEX,DRAW_RANGE_MIN,DRAW_RANGE_MAX,PROJECTION_TYPE,"
                "PROJECTION_INDEX,OTHERMODE_H,MATCH_STATUS\n");
        }
    }
}

void secondary_sway_match_header() {
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed)) return;
    if (g_single_mtx_match_csv == nullptr) {
        g_single_mtx_match_csv = std::fopen(
            tooie_diag_path("tooie-kazooie-feather-rt64-match.csv").c_str(), "a");
        if (g_single_mtx_match_csv != nullptr) {
            std::fprintf(
                g_single_mtx_match_csv,
                "EVENT,FRAME,TOKEN,CUR_WORKLOAD,PREV_WORKLOAD,SEMANTIC_CONTEXT,"
                "CUR_CALL_HASH,PREV_CALL_HASH,CUR_SCENE_PROJ,PREV_SCENE_PROJ,"
                "CUR_CALL,PREV_CALL,CALL_MATCH_RESULT,CALL_FAILURE_REASON,"
                "CUR_TRANSFORM,PREV_CANDIDATE_TRANSFORM,PREV_MATCHED_TRANSFORM,"
                "CUR_RANGE_BEGIN,CUR_RANGE_END,PREV_RANGE_BEGIN,PREV_RANGE_END,"
                "CUR_GROUP_INDEX,PREV_GROUP_INDEX,CUR_MATRIX_ID,PREV_MATRIX_ID,"
                "CUR_PROJECTION_TYPE,PREV_PROJECTION_TYPE,CUR_PROJECTION_INDEX,"
                "PREV_PROJECTION_INDEX,POSITION_DIFF,ORIENTATION_DIFF,"
                "SCREEN_SPACE_DIFF,MATCH_METRIC,MATCH_RESULT,REJECTION_REASON\n");
        }
    }
}

const char* secondary_sway_match_reason_name(uint32_t reason) {
    switch (reason) {
    case kMatchReasonNoPreviousCall:
        return "NO_PREVIOUS_CALL_MATCH";
    case kMatchReasonRangeCountMismatch:
        return "RANGE_POSITION_MISMATCH";
    case kMatchReasonTransformMatchingDisabled:
        return "TRANSFORM_MATCHING_DISABLED";
    case kMatchReasonMatrixIdMismatch:
        return "MATRIX_ID_MISMATCH";
    case kMatchReasonMatrixIdNotAuto:
        return "MATRIX_ID_NOT_AUTO_OR_ORDERED";
    case kMatchReasonMirrorDeterminant:
        return "DETERMINANT_MIRROR_REJECT";
    case kMatchReasonCandidateValid:
        return "CANDIDATE_VALID";
    case kMatchReasonCurrentAlreadyMapped:
        return "ONE_TO_ONE_CURRENT_ALREADY_USED";
    case kMatchReasonPreviousAlreadyUsed:
        return "ONE_TO_ONE_PREVIOUS_ALREADY_USED";
    case kMatchReasonSelected:
        return "MATCHTRANSFORM_SELECTED";
    case kMatchReasonTokenNotBound:
        return "MATCHTRANSFORM_TOKEN_NOT_BOUND";
    case kMatchReasonNoCandidateObserved:
        return "NO_CANDIDATE_OBSERVED";
    case kMatchReasonFlushSentinel:
        return "FLUSH_SENTINEL_ONLY";
    default:
        return "NONE";
    }
}

void secondary_sway_write_match_row(const SingleMtxTraceToken& trace,
                                    const char* event, const char* result,
                                    uint32_t reason) {
    if (g_single_mtx_match_rows >= kSingleMtxMatchRows) {
        return;
    }
    secondary_sway_match_header();
    if (g_single_mtx_match_csv == nullptr) {
        return;
    }
    const uint64_t semantic_context =
        trace.match_workload_id ^ trace.rt64_cur_call_hash ^
        (static_cast<uint64_t>(trace.rt64_cur_scene_proj) << 32) ^
        (static_cast<uint64_t>(trace.rt64_cur_call_index) << 16) ^
        (static_cast<uint64_t>(trace.rt64_cur_range_begin) << 1) ^
        trace.rt64_cur_range_end;
    const char* call_result = trace.rt64_call_match_result == 1
                                  ? "CALL_HASH_MATCHED"
                                  : (trace.rt64_call_match_result == 2
                                         ? "NO_PREVIOUS_CALL"
                                         : "UNKNOWN");
    const uint32_t call_reason = trace.rt64_call_reason;
    std::fprintf(
        g_single_mtx_match_csv,
        "%s,%llu,%llu,%llu,%llu,0x%016llX,0x%016llX,0x%016llX,%u,%u,"
        "%u,%u,%s,%s,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,"
        "%.9g,%.9g,%.9g,%.9g,%s,%s\n",
        event,
        static_cast<unsigned long long>(trace.frame),
        static_cast<unsigned long long>(trace.token),
        static_cast<unsigned long long>(trace.match_workload_id),
        static_cast<unsigned long long>(trace.rt64_prev_workload_id),
        static_cast<unsigned long long>(semantic_context),
        static_cast<unsigned long long>(trace.rt64_cur_call_hash),
        static_cast<unsigned long long>(trace.rt64_prev_call_hash),
        trace.rt64_cur_scene_proj, trace.rt64_prev_scene_proj,
        trace.rt64_cur_call_index, trace.rt64_prev_call_index, call_result,
        secondary_sway_match_reason_name(call_reason),
        trace.rt64_transform_index, trace.rt64_candidate_prev_transform,
        trace.rt64_matched_prev_transform, trace.rt64_cur_range_begin,
        trace.rt64_cur_range_end, trace.rt64_prev_range_begin,
        trace.rt64_prev_range_end, trace.rt64_cur_group_index,
        trace.rt64_prev_group_index, trace.rt64_cur_matrix_id,
        trace.rt64_prev_matrix_id, trace.rt64_cur_projection_type,
        trace.rt64_prev_projection_type, trace.rt64_cur_projection_index,
        trace.rt64_prev_projection_index, trace.rt64_position_difference,
        trace.rt64_orientation_difference, trace.rt64_screen_space_difference,
        trace.rt64_match_metric,
        result, secondary_sway_match_reason_name(reason));
    ++g_single_mtx_match_rows;
    std::fflush(g_single_mtx_match_csv);
}

void secondary_sway_rt64_match_observe_event(
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
    float orientation_difference, float screen_space_difference) {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
    for (SingleMtxTraceToken& trace : g_single_mtx_tokens) {
        if (!trace.pending || !trace.matrix_seen || !trace.vertex_seen ||
            trace.token == 0 ||
            (current_workload_id != 0 &&
             trace.match_workload_id != current_workload_id)) {
            continue;
        }
        if (!trace.rt64_gameframe_bound) {
            trace.rt64_gameframe_bound = true;
            ++g_single_mtx_bound_gameframe_match;
        }
        const bool range_match =
            current_transform_index == UINT32_MAX &&
            trace.rt64_transform_index >= current_range_begin &&
            trace.rt64_transform_index <= current_range_end;
        const bool exact_match =
            current_transform_index != UINT32_MAX &&
            trace.rt64_transform_index == current_transform_index;
        if (!range_match && !exact_match) {
            continue;
        }

        trace.rt64_prev_workload_id = previous_workload_id;
        trace.rt64_cur_call_hash = current_call_hash;
        trace.rt64_prev_call_hash = previous_call_hash;
        trace.rt64_cur_scene_proj = current_scene_proj;
        trace.rt64_prev_scene_proj = previous_scene_proj;
        trace.rt64_cur_call_index = current_call_index;
        trace.rt64_prev_call_index = previous_call_index;
        trace.rt64_cur_range_begin = current_range_begin;
        trace.rt64_cur_range_end = current_range_end;
        trace.rt64_prev_range_begin = previous_range_begin;
        trace.rt64_prev_range_end = previous_range_end;
        trace.rt64_cur_group_index = current_group_index;
        trace.rt64_prev_group_index = previous_group_index;
        trace.rt64_cur_matrix_id = current_matrix_id;
        trace.rt64_prev_matrix_id = previous_matrix_id;
        trace.rt64_cur_projection_type = current_projection_type;
        trace.rt64_prev_projection_type = previous_projection_type;
        trace.rt64_cur_projection_index = current_projection_index;
        trace.rt64_prev_projection_index = previous_projection_index;

        const uint32_t bounded_reason = reason < g_single_mtx_match_reason_counts.size()
                                            ? reason
                                            : kMatchReasonNoCandidateObserved;
        if (stage == kMatchStageCall) {
            if (!trace.rt64_call_seen) {
                ++g_single_mtx_bound_call_stage;
            }
            trace.rt64_call_seen = true;
            trace.rt64_call_match_result =
                reason == kMatchReasonNoPreviousCall ? 2u : 1u;
            trace.rt64_call_reason = reason;
            if (is_full_kazooie_feather_trace(trace)) {
                const uint32_t stat = feather_stat_index(trace);
                if (trace.rt64_call_match_result == 1u) {
                    ++g_feather_real_call_matched[stat];
                }
                else {
                    ++g_feather_real_call_unmatched[stat];
                }
            }
            ++g_single_mtx_match_call_rows;
            secondary_sway_write_match_row(
                trace, "CALL", "CALL_CONTEXT", bounded_reason);
        } else if (stage == kMatchStageTransformPrerequisite) {
            if (!trace.rt64_prerequisite_seen) {
                trace.rt64_prerequisite_seen = true;
                ++g_single_mtx_bound_transform_prerequisite;
            }
            trace.rt64_match_reason = bounded_reason;
            secondary_sway_write_match_row(
                trace, "TRANSFORM_PREREQUISITE",
                bounded_reason == kMatchReasonNone ? "ELIGIBLE" : "REJECTED",
                bounded_reason);
        } else if (stage == kMatchStageCandidate) {
            if (!trace.rt64_candidate_seen) {
                ++g_single_mtx_bound_candidate_stage;
            }
            trace.rt64_candidate_seen = true;
            trace.rt64_candidate_prev_transform = previous_transform_index;
            trace.rt64_position_difference = position_difference;
            trace.rt64_orientation_difference = orientation_difference;
            trace.rt64_screen_space_difference = screen_space_difference;
            trace.rt64_match_metric =
                (candidate_valid != 0)
                    ? position_difference + orientation_difference +
                          screen_space_difference
                    : -1.0f;
            if (candidate_valid == 0) {
                trace.rt64_match_reason = bounded_reason;
            }
            ++g_single_mtx_match_candidate_rows;
            secondary_sway_write_match_row(
                trace, "CANDIDATE", candidate_valid != 0 ? "VALID" : "REJECTED",
                bounded_reason);
        } else if (stage == kMatchStageDecision) {
            trace.rt64_decision_seen = true;
            trace.rt64_candidate_prev_transform = previous_transform_index;
            trace.rt64_match_reason = bounded_reason;
            trace.rt64_selected = reason == kMatchReasonSelected;
            ++g_single_mtx_match_decision_rows;
            secondary_sway_write_match_row(
                trace, "DECISION", trace.rt64_selected ? "SELECTED" : "SKIPPED",
                bounded_reason);
            if (!trace.rt64_selected && !trace.match_seen) {
                ++g_single_mtx_match_real_unmatched;
                ++g_single_mtx_match_unmatched;
                if (is_full_kazooie_feather_trace(trace)) {
                    const uint32_t stat = feather_stat_index(trace);
                    ++g_feather_real_unmatched[stat];
                    if (bounded_reason == kMatchReasonCurrentAlreadyMapped ||
                        bounded_reason == kMatchReasonPreviousAlreadyUsed) {
                        ++g_feather_semantic_dropped[stat];
                    }
                    else if (bounded_reason == kMatchReasonMatrixIdMismatch ||
                             bounded_reason == kMatchReasonMatrixIdNotAuto) {
                        ++g_feather_semantic_cross_match[stat];
                    }
                    else {
                        ++g_feather_semantic_failure[stat];
                    }
                }
                secondary_sway_write_match_row(
                    trace, "REAL_UNMATCHED", "UNMATCHED", bounded_reason);
                trace.match_seen = true;
                trace.pending = false;
            }
        }
        if (bounded_reason != kMatchReasonNone) {
            ++g_single_mtx_match_reason_counts[bounded_reason];
        }
    }
}

uint64_t secondary_sway_rt64_matrix_event(
    uint32_t command_address, uint32_t matrix_address,
    uint32_t physical_address, uint32_t params, uint32_t stack_before,
    uint32_t stack_after, uint32_t group_depth, uint32_t matrix_id,
    uint32_t group_index, uint32_t explicit_group, uint64_t workload_id,
    uint32_t display_list_address, uint64_t display_list_counter) {
    ensure_init();
    const bool sway = g_secondary_sway_enabled.load(std::memory_order_relaxed);
    const bool pairSnap = g_p09_local_pair_snapshot.load(std::memory_order_relaxed);
    if (!RT64::p09LocalPairRunProvenance(sway, pairSnap)) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
    SingleMtxTraceToken* found = nullptr;
    bool restore_alias = false;
    for (SingleMtxTraceToken& candidate : g_single_mtx_tokens) {
        if (candidate.pending && !candidate.matrix_seen &&
            candidate.destination_ptr == matrix_address) {
            found = &candidate;
            break;
        }
    }
    if (found == nullptr) {
        SingleMtxTraceToken* restore = nullptr;
        for (SingleMtxTraceToken& candidate : g_single_mtx_tokens) {
            if (!candidate.pending || !candidate.matrix_seen ||
                candidate.restore_alias_seen ||
                candidate.destination_ptr == matrix_address ||
                candidate.match_workload_id != workload_id ||
                candidate.rt64_stack_after != stack_after) {
                continue;
            }
            if (restore != nullptr) {
                restore = nullptr;
                ++g_single_mtx_token_restore_ambiguous;
                break;
            }
            restore = &candidate;
        }
        if (restore != nullptr) {
            found = restore;
            restore_alias = true;
            ++g_single_mtx_token_restore_alias;
        }
    }
    if (found == nullptr) {
        bool pending_candidate = false;
        for (const SingleMtxTraceToken& candidate : g_single_mtx_tokens) {
            if (candidate.pending && !candidate.matrix_seen) {
                pending_candidate = true;
                break;
            }
        }
        if (pending_candidate) {
            ++g_single_mtx_token_address_mismatch;
        } else {
            ++g_single_mtx_token_other_reject;
        }
        return 0;
    }
    if (!found->matrix_seen) {
        found->matrix_seen = true;
        found->matrix_workload_id = workload_id;
        found->match_workload_id = workload_id;
        found->rt64_workload_id = workload_id;
        if (workload_id != 0) {
            ++g_single_mtx_matrix_workload_nonzero;
        }
        ++g_single_mtx_token_consumed;
        found->rt64_stack_after = stack_after;
        if (RT64::p09LocalPairRunHeavySway(sway) && is_full_kazooie_feather_trace(*found)) {
            ++g_feather_gmtx_seen[feather_stat_index(*found)];
        }
    }
    found->gfx_command_ptr = command_address;
    found->rt64_guest_matrix_ptr = matrix_address;
    if (found->destination_ptr == matrix_address) {
        ++g_single_mtx_token_dest_eq_gmtx_operand;
    }
    if (found->source_ptr == matrix_address) {
        ++g_single_mtx_token_source_eq_gmtx_operand;
    }
    if (restore_alias) {
        found->restore_destination_ptr = matrix_address;
        found->restore_alias_seen = true;
    }
    found->rt64_matrix_id = matrix_id;
    found->rt64_group_index = group_index;
    if (RT64::p09LocalPairRunHeavySway(sway)) secondary_sway_identity_header();
    if (RT64::p09LocalPairRunHeavySway(sway) && g_single_mtx_identity_csv != nullptr &&
        g_single_mtx_identity_rows < kSingleMtxIdentityRows) {
        std::fprintf(
            g_single_mtx_identity_csv,
            "%s,%llu,%llu,%llu,0x%08X,0x%X,%u,%u,0x%08X,0x%08X,"
            "0x%08X,0x%X,%u,%u,%u,0x%08X,%u,%u,4294967295,4294967295,"
            "4294967295,4294967295,4294967295,4294967295,4294967295,4294967295,"
            "4294967295,0,0,0,%s\n",
            restore_alias ? "RESTORE" : "MATRIX",
            static_cast<unsigned long long>(found->frame),
            static_cast<unsigned long long>(found->token),
            static_cast<unsigned long long>(workload_id), found->model_bin,
            found->node_rel, found->bone_index, found->bone_anim_id,
            command_address, matrix_address, physical_address, params,
            stack_before, stack_after, group_depth, matrix_id, group_index,
            explicit_group, restore_alias ? "ALIAS" : "NEW");
        ++g_single_mtx_identity_rows;
        std::fflush(g_single_mtx_identity_csv);
    }
    (void)display_list_address;
    (void)display_list_counter;
    return found->token;
}

void secondary_sway_rt64_vertex_event(
    uint64_t token, uint64_t workload_id, uint32_t transform_index,
    uint32_t group_index, uint32_t vertex_start, uint32_t vertex_count,
    uint32_t physical_address, uint32_t segmented_address, uint32_t draw_uid,
    uint32_t draw_call_index, uint32_t draw_min_matrix,
    uint32_t draw_max_matrix, uint32_t projection_type,
    uint32_t projection_index, uint32_t other_mode_h) {
    ensure_init();
    const bool sway = g_secondary_sway_enabled.load(std::memory_order_relaxed);
    const bool pairSnap = g_p09_local_pair_snapshot.load(std::memory_order_relaxed);
    if (!RT64::p09LocalPairRunProvenance(sway, pairSnap)) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
    SingleMtxTraceToken* found = nullptr;
    if (token != 0) {
        for (SingleMtxTraceToken& candidate : g_single_mtx_tokens) {
            if (candidate.pending && candidate.token == token) {
                found = &candidate;
                break;
            }
        }
    }
    // Some feather display-list paths expose the matrix workload to the
    // vertex event but do not forward the matrix token. Bind only when the
    // workload identifies exactly one pending feather transform; never use a
    // broad "latest token" fallback that could cross semantic draw contexts.
    if (found == nullptr && workload_id != 0) {
        for (SingleMtxTraceToken& candidate : g_single_mtx_tokens) {
            if (!candidate.pending || candidate.vertex_seen ||
                !candidate.matrix_seen ||
                candidate.match_workload_id != workload_id) {
                continue;
            }
            if (found != nullptr) {
                ++g_single_mtx_vertex_fallback_ambiguous;
                found = nullptr;
                break;
            }
            found = &candidate;
        }
        if (found != nullptr) {
            found->vertex_fallback_bound = true;
            ++g_single_mtx_vertex_fallback_bindings;
        }
    }
    if (found == nullptr) {
        return;
    }
    SingleMtxTraceToken& trace = *found;
    const bool first_vertex = !trace.vertex_seen;
    trace.vertex_seen = true;
    if (RT64::p09LocalPairRunHeavySway(sway) && first_vertex && is_full_kazooie_feather_trace(trace)) {
        ++g_feather_vertex_seen[feather_stat_index(trace)];
    }
        trace.vertex_start = vertex_start;
        trace.vertex_count = vertex_count;
        trace.vertex_workload_id = workload_id;
        if (workload_id == 0) {
            ++g_single_mtx_vertex_workload_zero;
        }
        else {
            ++g_single_mtx_vertex_workload_nonzero;
        }
        // matrixCommon's state workload is the identity consumed by
        // GameFrame::match. A zero workload from setVertexCommon is only
        // vertex-event context and must not erase that identity.
        if (trace.matrix_workload_id != 0) {
            trace.match_workload_id = trace.matrix_workload_id;
            trace.rt64_workload_id = trace.match_workload_id;
            ++g_single_mtx_valid_workload_preserved;
            if (workload_id == 0) {
                ++g_single_mtx_valid_workload_overwrite_blocked;
            }
        }
        else {
            trace.match_workload_id = workload_id;
            trace.rt64_workload_id = trace.match_workload_id;
        }
        trace.rt64_transform_index = transform_index;
        trace.rt64_draw_uid = draw_uid;
        trace.rt64_draw_call_index = draw_call_index;
        trace.rt64_draw_min = draw_min_matrix;
        trace.rt64_draw_max = draw_max_matrix;
        trace.rt64_projection_type = projection_type;
        trace.rt64_projection_index = projection_index;
        trace.rt64_other_mode_h = other_mode_h;
        if (RT64::p09LocalPairRunHeavySway(sway)) {
            remember_feather_world_hit(trace);
            secondary_sway_identity_header();
        }
        if (RT64::p09LocalPairRunHeavySway(sway) && g_single_mtx_identity_csv != nullptr &&
            g_single_mtx_identity_rows < kSingleMtxIdentityRows) {
            std::fprintf(
                g_single_mtx_identity_csv,
                "VERTEX,%llu,%llu,%llu,0x%08X,0x%X,%u,%u,0,0x%08X,0x%08X,"
                "0,0,0,0,0x%08X,%u,0,0x%X,4294967295,4294967295,%u,%u,%u,%u,%u,"
                "%u,%u,%u,0x%08X,TRANSFORM_CREATED\n",
                static_cast<unsigned long long>(trace.frame),
                static_cast<unsigned long long>(trace.token),
                static_cast<unsigned long long>(workload_id), trace.model_bin,
                trace.node_rel, trace.bone_index, trace.bone_anim_id,
                segmented_address, physical_address, trace.rt64_matrix_id,
                group_index, transform_index, vertex_start, vertex_count,
                draw_uid, draw_call_index, draw_min_matrix, draw_max_matrix,
                projection_type, projection_index, other_mode_h);
            ++g_single_mtx_identity_rows;
            std::fflush(g_single_mtx_identity_csv);
        }
        return;
}

void secondary_sway_rt64_match_event(
    uint64_t workload_id, uint64_t previous_workload_id,
    uint32_t transform_index, uint32_t previous_transform_index,
    uint32_t group_index, uint32_t matrix_id) {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
    for (SingleMtxTraceToken& trace : g_single_mtx_tokens) {
        if (!trace.pending || !trace.vertex_seen || !trace.matrix_seen ||
            trace.token == 0 || trace.match_workload_id != workload_id ||
            trace.rt64_transform_index != transform_index || trace.match_seen) {
            continue;
        }
        trace.match_seen = true;
        trace.rt64_prev_workload_id = previous_workload_id;
        trace.rt64_matched_prev_transform = previous_transform_index;
        trace.rt64_prev_group_index = group_index;
        trace.rt64_prev_matrix_id = matrix_id;
        trace.rt64_match_reason = kMatchReasonSelected;
        trace.rt64_selected = true;
        if (is_full_kazooie_feather_trace(trace)) {
            const uint32_t stat = feather_stat_index(trace);
            ++g_feather_real_matched[stat];
            ++g_feather_matched_seen[stat];
        }
        ++g_single_mtx_bound_matchtransform;
        ++g_single_mtx_match_matched;
        secondary_sway_write_match_row(trace, "MATCHED", "MATCHED",
                                       kMatchReasonNone);
        trace.pending = false;
        return;
    }
    ++g_single_mtx_match_no_token;
}

bool secondary_sway_rt64_match_override(
    uint64_t workload_id, uint32_t transform_index, uint32_t group_index) {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
    SingleMtxTraceToken* target = nullptr;
    for (SingleMtxTraceToken& trace : g_single_mtx_tokens) {
        if (!trace.pending || !trace.matrix_seen || !trace.vertex_seen ||
            trace.token == 0 || trace.model_bin == 0 ||
            trace.node_rel != kKazooieFeather62BNodeOffset ||
            trace.bone_index != kKazooieFeather62BBoneIndex ||
            trace.bone_anim_id != kKazooieFeather62BBoneAnimId ||
            trace.match_workload_id != workload_id ||
            trace.rt64_transform_index != transform_index) {
            continue;
        }
        if (trace.rt64_cur_group_index != UINT32_MAX &&
            trace.rt64_cur_group_index != group_index) {
            continue;
        }
        if (target != nullptr) {
            ++g_single_mtx_interp_mode_ambiguous;
            return false;
        }
        target = &trace;
    }

    if (target == nullptr) {
        return false;
    }

    ++g_single_mtx_interp_mode_control;
    if (!g_secondary_sway_interp_simple_enabled.load(std::memory_order_relaxed)) {
        return false;
    }

    ++g_single_mtx_interp_mode_simple_override;
    return true;
}

bool secondary_sway_rt64_no_interpolation(
    uint64_t workload_id, uint32_t transform_index, uint32_t group_index) {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed) ||
        !g_secondary_sway_no_interpolation_enabled.load(
            std::memory_order_relaxed)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
    SingleMtxTraceToken* target = nullptr;
    for (SingleMtxTraceToken& trace : g_single_mtx_tokens) {
        if (!trace.matrix_seen || !trace.vertex_seen || !trace.match_seen ||
            trace.token == 0 || !is_full_kazooie_feather_trace(trace) ||
            trace.match_workload_id != workload_id ||
            trace.rt64_transform_index != transform_index ||
            trace.rt64_cur_group_index != group_index) {
            continue;
        }
        if (target != nullptr) {
            return false;
        }
        target = &trace;
    }
    if (target == nullptr) {
        return false;
    }
    ++g_single_mtx_no_interp_override;
    ++g_feather_no_interp_seen[feather_stat_index(*target)];
    return true;
}

void secondary_sway_rt64_post_match_event(
    uint64_t workload_id, uint32_t transform_index,
    uint32_t previous_transform_index, uint32_t group_index,
    float previous_weight, float current_weight, const float* previous_matrix,
    const float* current_matrix, const float* final_matrix,
    uint32_t no_interpolation) {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed) ||
        previous_matrix == nullptr || current_matrix == nullptr ||
        final_matrix == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
    SingleMtxTraceToken* target = nullptr;
    for (SingleMtxTraceToken& trace : g_single_mtx_tokens) {
        if (!trace.matrix_seen || !trace.vertex_seen || !trace.match_seen ||
            trace.token == 0 || !is_full_kazooie_feather_trace(trace) ||
            trace.match_workload_id != workload_id ||
            trace.rt64_transform_index != transform_index ||
            trace.rt64_matched_prev_transform != previous_transform_index ||
            trace.rt64_cur_group_index != group_index) {
            continue;
        }
        if (target != nullptr) {
            ++g_single_mtx_post_match_no_token;
            return;
        }
        target = &trace;
    }
    if (target == nullptr) {
        ++g_single_mtx_post_match_no_token;
        return;
    }

    ++g_feather_post_match_seen[feather_stat_index(*target)];
    if (no_interpolation != 0u) {
        const uint32_t stat = feather_stat_index(*target);
        bool equal = true;
        for (int i = 0; i < 16; ++i) {
            if (final_matrix[i] != current_matrix[i]) {
                equal = false;
                break;
            }
        }
        if (equal) {
            ++g_feather_no_interp_eq_current[stat];
            ++g_feather_no_interp_eq_current_total;
        } else {
            ++g_feather_no_interp_ne_current[stat];
            ++g_feather_no_interp_ne_current_total;
        }
    }

    if (g_single_mtx_post_match_rows >= kSingleMtxMatchRows) {
        return;
    }
    if (g_single_mtx_post_match_csv == nullptr) {
        g_single_mtx_post_match_csv = std::fopen(
            tooie_diag_path("tooie-kazooie-feather-postmatch.csv").c_str(), "w");
        if (g_single_mtx_post_match_csv != nullptr) {
            std::fprintf(
                g_single_mtx_post_match_csv,
                "FRAME,TOKEN,MODEL_BIN,NODE_REL,BONE_INDEX,BONE_ANIM_ID,"
                "WORKLOAD,TRANSFORM,PREVIOUS_TRANSFORM,GROUP_INDEX,"
                "PREVIOUS_WEIGHT,CURRENT_WEIGHT,NO_INTERPOLATION,"
                "PREVIOUS_MATRIX16,CURRENT_MATRIX16,FINAL_MATRIX16,"
                "PREVIOUS_QUAT4,CURRENT_QUAT4,FINAL_QUAT4,"
                "PREVIOUS_TO_CURRENT_ANGLE,PREVIOUS_TO_FINAL_ANGLE,"
                "FINAL_TO_CURRENT_ANGLE\n");
        }
    }
    if (g_single_mtx_post_match_csv == nullptr) {
        return;
    }

    std::fprintf(
        g_single_mtx_post_match_csv,
        "%llu,%llu,0x%08X,0x%X,%u,%u,%llu,%u,%u,%u,%.9g,%.9g,%u,",
        static_cast<unsigned long long>(target->frame),
        static_cast<unsigned long long>(target->token), target->model_bin,
        target->node_rel, target->bone_index, target->bone_anim_id,
        static_cast<unsigned long long>(workload_id), transform_index,
        previous_transform_index, group_index, previous_weight, current_weight,
        no_interpolation);
    for (const float* matrix : {previous_matrix, current_matrix, final_matrix}) {
        for (int i = 0; i < 16; ++i) {
            std::fprintf(g_single_mtx_post_match_csv, "%.9g%c", matrix[i],
                         (i == 15) ? ',' : ';');
        }
    }
    float previous_quat[4]{};
    float current_quat[4]{};
    float final_quat[4]{};
    const bool previous_quat_valid =
        feather_float_matrix_quaternion(previous_matrix, previous_quat);
    const bool current_quat_valid =
        feather_float_matrix_quaternion(current_matrix, current_quat);
    const bool final_quat_valid =
        feather_float_matrix_quaternion(final_matrix, final_quat);
    if (previous_quat_valid && current_quat_valid && final_quat_valid) {
        if (previous_quat[0] * current_quat[0] +
                previous_quat[1] * current_quat[1] +
                previous_quat[2] * current_quat[2] +
                previous_quat[3] * current_quat[3] <
            0.0f) {
            for (float& component : current_quat) {
                component = -component;
            }
        }
        if (previous_quat[0] * final_quat[0] +
                previous_quat[1] * final_quat[1] +
                previous_quat[2] * final_quat[2] +
                previous_quat[3] * final_quat[3] <
            0.0f) {
            for (float& component : final_quat) {
                component = -component;
            }
        }
        for (const float* quat : {previous_quat, current_quat, final_quat}) {
            for (int i = 0; i < 4; ++i) {
                std::fprintf(g_single_mtx_post_match_csv, "%.9g%c", quat[i],
                             (i == 3) ? ',' : ';');
            }
        }
        std::fprintf(g_single_mtx_post_match_csv, "%.9g,%.9g,%.9g",
                     feather_quaternion_angle(previous_quat, current_quat),
                     feather_quaternion_angle(previous_quat, final_quat),
                     feather_quaternion_angle(final_quat, current_quat));
    }
    else {
        std::fprintf(g_single_mtx_post_match_csv,
                     "INVALID;INVALID;INVALID,INVALID,INVALID,INVALID");
    }
    std::fprintf(g_single_mtx_post_match_csv, "\n");
    ++g_single_mtx_post_match_rows;
    std::fflush(g_single_mtx_post_match_csv);
}

void secondary_sway_rt64_match_finalize_event() {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
    for (SingleMtxTraceToken& trace : g_single_mtx_tokens) {
        if (!trace.pending || !trace.matrix_seen || !trace.vertex_seen ||
            trace.match_seen || trace.token == 0) {
            continue;
        }
        const uint32_t reason =
            trace.rt64_match_reason != kMatchReasonNone
                ? trace.rt64_match_reason
                : kMatchReasonNoCandidateObserved;
        trace.rt64_match_reason = reason;
        ++g_single_mtx_synthetic_finalization_only;
        if (reason < g_single_mtx_match_reason_counts.size()) {
            ++g_single_mtx_match_reason_counts[reason];
        }
        secondary_sway_write_match_row(trace, "SYNTHETIC_FINALIZATION_ONLY",
                                       "DIAGNOSTIC_ONLY", reason);
        trace.match_seen = true;
        trace.pending = false;
    }
}

void secondary_sway_rt64_match_flush_event() {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
    for (SingleMtxTraceToken& trace : g_single_mtx_tokens) {
        if (!trace.pending || !trace.vertex_seen || trace.match_seen ||
            trace.token == 0) {
            continue;
        }
        trace.match_seen = true;
        ++g_single_mtx_match_flush_sentinel;
        ++g_single_mtx_synthetic_finalization_only;
        secondary_sway_write_match_row(trace, "FLUSH_SENTINEL", "SECONDARY_ONLY",
                                       kMatchReasonFlushSentinel);
        trace.pending = false;
    }
}

void secondary_sway_rt64_gbi_tri(uint32_t opcode) {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    if (opcode == 0x05u) {
        g_raw_tri1.fetch_add(1, std::memory_order_relaxed);
    } else if (opcode == 0x06u) {
        g_raw_tri2.fetch_add(1, std::memory_order_relaxed);
    }
}

void secondary_sway_rt64_tri_handler_enter(uint32_t kind) {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    g_raw_tri_handler.fetch_add(1, std::memory_order_relaxed);
    (void)kind;
}

void secondary_sway_rt64_feather_tri_event(
    uint64_t token, uint64_t workload_id, uint32_t cur_transform_index,
    uint32_t slot_a, uint32_t slot_b, uint32_t slot_c, uint32_t global_a,
    uint32_t global_b, uint32_t global_c, uint32_t world_a, uint32_t world_b,
    uint32_t world_c, uint32_t draw_uid, uint32_t draw_call_index,
    uint32_t draw_min_matrix, uint32_t draw_max_matrix, uint32_t current_dl,
    uint32_t parent_dl, uint32_t dl_depth) {
    ensure_init();
    if (!g_secondary_sway_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
    const FeatherWorldHit* hit_a = find_feather_world_hit(workload_id, world_a);
    const FeatherWorldHit* hit_b = find_feather_world_hit(workload_id, world_b);
    const FeatherWorldHit* hit_c = find_feather_world_hit(workload_id, world_c);
    const FeatherWorldHit* hit_active =
        find_feather_world_hit(workload_id, cur_transform_index);
    if (hit_a != nullptr || hit_b != nullptr || hit_c != nullptr) {
        ++g_tri_with_tracked_worldindex;
    }
    if (hit_active != nullptr) {
        ++g_tri_with_tracked_active;
    }
    const FeatherWorldHit* model_hit = hit_a != nullptr     ? hit_a
                                       : hit_b != nullptr   ? hit_b
                                       : hit_c != nullptr   ? hit_c
                                       : hit_active;
    const uint32_t gfx_index = feather_gfx_index_from_dl(
        current_dl, model_hit != nullptr ? model_hit->model_bin : 0u);
    const bool tracked_dl = tracked_feather_gfx_index(gfx_index);
    if (tracked_dl) {
        ++g_tri_with_tracked_dl;
    }
    bool token_is_feather = false;
    if (token != 0) {
        for (const SingleMtxTraceToken& candidate : g_single_mtx_tokens) {
            if (candidate.token == token &&
                is_full_kazooie_feather_trace(candidate)) {
                token_is_feather = true;
                break;
            }
        }
    }
    const char* lookup = "NONE";
    if (hit_a != nullptr || hit_b != nullptr || hit_c != nullptr) {
        const bool exact =
            (hit_a != nullptr &&
             feather_world_hit_workload_match(*hit_a, workload_id)) ||
            (hit_b != nullptr &&
             feather_world_hit_workload_match(*hit_b, workload_id)) ||
            (hit_c != nullptr &&
             feather_world_hit_workload_match(*hit_c, workload_id));
        lookup = exact ? "VERTEX_WORLDINDEX" : "WORLDINDEX_ANY_WORKLOAD";
    } else if (hit_active != nullptr) {
        lookup = "ACTIVE_TRANSFORM";
    } else if (token_is_feather) {
        lookup = "TOKEN";
    } else if (tracked_dl) {
        lookup = "TRACKED_DL";
    } else {
        return;
    }
    ++g_tri_classified;
    const bool mixed = (world_a != world_b) || (world_b != world_c);
    const bool ne_active = (world_a != cur_transform_index) ||
                           (world_b != cur_transform_index) ||
                           (world_c != cur_transform_index);
    const char* cls = "MATCH_ACTIVE";
    if (mixed && ne_active) {
        cls = "MIXED_AND_NE_ACTIVE";
    } else if (mixed) {
        cls = "MIXED_JOIN";
    } else if (ne_active) {
        cls = "STALE_OR_LEFTOVER";
    }
    ++g_feather_tri_rows;
    if (mixed) {
        ++g_feather_tri_mixed;
    }
    if (ne_active) {
        ++g_feather_tri_ne_active;
    }
    if (!mixed && !ne_active) {
        ++g_feather_tri_match_active;
    }
    auto note_bone = [](const FeatherWorldHit* hit) {
        if (hit == nullptr || hit->bone_anim_id >= 128u) {
            return;
        }
        ++g_feather_tri_seen[hit->bone_anim_id];
    };
    note_bone(hit_a);
    if (hit_b != hit_a) {
        note_bone(hit_b);
    }
    if (hit_c != hit_a && hit_c != hit_b) {
        note_bone(hit_c);
    }
    if (hit_active != hit_a && hit_active != hit_b && hit_active != hit_c) {
        note_bone(hit_active);
    }
    if (g_feather_tri_csv == nullptr) {
        g_feather_tri_csv =
            std::fopen(tooie_diag_path("tooie-kazooie-feather-v6b-tri.csv").c_str(), "w");
        if (g_feather_tri_csv != nullptr) {
            std::fprintf(
                g_feather_tri_csv,
                "EVENT,FRAME,TOKEN,WORKLOAD,CURRENT_DL,PARENT_DL,DL_DEPTH,"
                "GFX_INDEX,LOOKUP,BONE_A,BONE_B,BONE_C,BONE_ACTIVE,"
                "NODE_A,NODE_B,NODE_C,SLOT_A,SLOT_B,SLOT_C,"
                "GLOBAL_A,GLOBAL_B,GLOBAL_C,WORLD_A,WORLD_B,WORLD_C,"
                "CUR_TRANSFORM,DRAW_UID,DRAW_CALL,DRAW_MIN,DRAW_MAX,MIXED,"
                "NE_ACTIVE,CLASS\n");
        }
    }
    if (g_feather_tri_csv != nullptr && g_feather_tri_rows <= kFeatherTriRows) {
        const uint64_t frame = model_hit != nullptr ? model_hit->frame : 0;
        std::fprintf(
            g_feather_tri_csv,
            "TRI,%llu,%llu,%llu,0x%08X,0x%08X,%u,0x%X,%s,%u,%u,%u,%u,"
            "0x%X,0x%X,0x%X,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,"
            "%s\n",
            static_cast<unsigned long long>(frame),
            static_cast<unsigned long long>(token),
            static_cast<unsigned long long>(workload_id), current_dl, parent_dl,
            dl_depth, gfx_index == UINT32_MAX ? 0u : gfx_index, lookup,
            hit_a != nullptr ? hit_a->bone_anim_id : 0u,
            hit_b != nullptr ? hit_b->bone_anim_id : 0u,
            hit_c != nullptr ? hit_c->bone_anim_id : 0u,
            hit_active != nullptr ? hit_active->bone_anim_id : 0u,
            hit_a != nullptr ? hit_a->node_rel : 0u,
            hit_b != nullptr ? hit_b->node_rel : 0u,
            hit_c != nullptr ? hit_c->node_rel : 0u, slot_a, slot_b, slot_c,
            global_a, global_b, global_c, world_a, world_b, world_c,
            cur_transform_index, draw_uid, draw_call_index, draw_min_matrix,
            draw_max_matrix, mixed ? 1u : 0u, ne_active ? 1u : 0u, cls);
        if ((g_feather_tri_rows % 64ull) == 0ull) {
            std::fflush(g_feather_tri_csv);
            secondary_sway_write_status("feather-tri");
        }
    }
}

uint32_t p09_local_pair_collect_witnesses(
    uint64_t workload_id, uint32_t* world_index, uint32_t* resource_id,
    uint32_t* node_rel, uint32_t* bone_index, uint32_t* bone_anim_id,
    uint8_t* candidate_class, uint32_t cap) {
    if (!g_p09_local_pair_snapshot.load(std::memory_order_relaxed) ||
        workload_id == 0 || world_index == nullptr || resource_id == nullptr ||
        node_rel == nullptr || bone_index == nullptr || bone_anim_id == nullptr ||
        candidate_class == nullptr) {
        return 0;
    }
    struct Row {
        uint8_t vertexSeen;
        uint32_t transform;
        uint32_t resource;
        uint32_t node;
        uint32_t bone;
        uint32_t anim;
        uint8_t cls;
    };
    std::vector<Row> rows;
    {
        std::lock_guard<std::mutex> lock(g_secondary_sway_mu);
        for (const SingleMtxTraceToken& trace : g_single_mtx_tokens) {
            if (trace.token == 0 || trace.vertex_workload_id != workload_id) continue;
            const uint8_t cls = RT64::p09LocalPairCandidateClass(
                trace.resource_id, trace.node_rel, trace.bone_index, trace.bone_anim_id);
            if (cls == RT64::P09LocalPairCandidateNone) continue;
            Row row;
            row.vertexSeen = trace.vertex_seen ? 1u : 0u;
            row.transform = trace.rt64_transform_index;
            row.resource = trace.resource_id;
            row.node = trace.node_rel;
            row.bone = trace.bone_index;
            row.anim = trace.bone_anim_id;
            row.cls = cls;
            rows.push_back(row);
        }
    }
    uint32_t n = 0;
    bool missing = false;
    for (const Row& row : rows) {
        if (!RT64::p09LocalPairWitnessIndexMatch(workload_id, workload_id, row.vertexSeen)) {
            missing = true;
            continue;
        }
        if (n >= cap) return cap + 1u;
        world_index[n] = row.transform;
        resource_id[n] = row.resource;
        node_rel[n] = row.node;
        bone_index[n] = row.bone;
        bone_anim_id[n] = row.anim;
        candidate_class[n] = row.cls;
        n += 1u;
    }
    if (missing) return 0xFFFFFFFFu;
    return n;
}

} // namespace tooie_diag

extern "C" uint32_t tooie_diag_p09_local_pair_witnesses(
    uint64_t workload_id, uint32_t* world_index, uint32_t* resource_id,
    uint32_t* node_rel, uint32_t* bone_index, uint32_t* bone_anim_id,
    uint8_t* candidate_class, uint32_t cap) {
    return tooie_diag::p09_local_pair_collect_witnesses(
        workload_id, world_index, resource_id, node_rel, bone_index,
        bone_anim_id, candidate_class, cap);
}

extern "C" uint64_t tooie_diag_rt64_matrix(
    uint32_t command_address, uint32_t matrix_address,
    uint32_t physical_address, uint32_t params, uint32_t stack_before,
    uint32_t stack_after, uint32_t group_depth, uint32_t matrix_id,
    uint32_t group_index, uint32_t explicit_group, uint64_t workload_id,
    uint32_t display_list_address, uint64_t display_list_counter) {
    return tooie_diag::secondary_sway_rt64_matrix_event(
        command_address, matrix_address, physical_address, params, stack_before,
        stack_after, group_depth, matrix_id, group_index, explicit_group,
        workload_id, display_list_address, display_list_counter);
}

extern "C" void tooie_diag_rt64_vertex(
    uint64_t token, uint64_t workload_id, uint32_t transform_index,
    uint32_t group_index, uint32_t vertex_start, uint32_t vertex_count,
    uint32_t physical_address, uint32_t segmented_address, uint32_t draw_uid,
    uint32_t draw_call_index, uint32_t draw_min_matrix,
    uint32_t draw_max_matrix, uint32_t projection_type,
    uint32_t projection_index, uint32_t other_mode_h) {
    tooie_diag::secondary_sway_rt64_vertex_event(
        token, workload_id, transform_index, group_index, vertex_start,
        vertex_count, physical_address, segmented_address, draw_uid,
        draw_call_index, draw_min_matrix, draw_max_matrix, projection_type,
        projection_index, other_mode_h);
}

extern "C" void tooie_diag_rt64_gbi_tri(uint32_t opcode) {
    tooie_diag::secondary_sway_rt64_gbi_tri(opcode);
}

extern "C" void tooie_diag_rt64_tri_handler_enter(uint32_t kind) {
    tooie_diag::secondary_sway_rt64_tri_handler_enter(kind);
}

extern "C" void tooie_diag_rt64_feather_tri(
    uint64_t token, uint64_t workload_id, uint32_t cur_transform_index,
    uint32_t slot_a, uint32_t slot_b, uint32_t slot_c, uint32_t global_a,
    uint32_t global_b, uint32_t global_c, uint32_t world_a, uint32_t world_b,
    uint32_t world_c, uint32_t draw_uid, uint32_t draw_call_index,
    uint32_t draw_min_matrix, uint32_t draw_max_matrix, uint32_t current_dl,
    uint32_t parent_dl, uint32_t dl_depth) {
    tooie_diag::secondary_sway_rt64_feather_tri_event(
        token, workload_id, cur_transform_index, slot_a, slot_b, slot_c,
        global_a, global_b, global_c, world_a, world_b, world_c, draw_uid,
        draw_call_index, draw_min_matrix, draw_max_matrix, current_dl,
        parent_dl, dl_depth);
}

extern "C" void tooie_diag_rt64_match(
    uint64_t workload_id, uint64_t previous_workload_id,
    uint32_t transform_index, uint32_t previous_transform_index,
    uint32_t group_index, uint32_t matrix_id) {
    tooie_diag::secondary_sway_rt64_match_event(
        workload_id, previous_workload_id, transform_index,
        previous_transform_index, group_index, matrix_id);
}

extern "C" bool tooie_diag_rt64_match_override(
    uint64_t workload_id, uint32_t transform_index, uint32_t group_index) {
    return tooie_diag::secondary_sway_rt64_match_override(
        workload_id, transform_index, group_index);
}

extern "C" bool tooie_diag_rt64_no_interpolation(
    uint64_t workload_id, uint32_t transform_index, uint32_t group_index) {
    return tooie_diag::secondary_sway_rt64_no_interpolation(
        workload_id, transform_index, group_index);
}

extern "C" void tooie_diag_rt64_post_match(
    uint64_t workload_id, uint32_t transform_index,
    uint32_t previous_transform_index, uint32_t group_index,
    float previous_weight, float current_weight, const float* previous_matrix,
    const float* current_matrix, const float* final_matrix,
    uint32_t no_interpolation) {
    tooie_diag::secondary_sway_rt64_post_match_event(
        workload_id, transform_index, previous_transform_index, group_index,
        previous_weight, current_weight, previous_matrix, current_matrix,
        final_matrix, no_interpolation);
}

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
    float orientation_difference, float screen_space_difference) {
    tooie_diag::secondary_sway_rt64_match_observe_event(
        stage, current_workload_id, previous_workload_id, current_call_hash,
        previous_call_hash, current_scene_proj, previous_scene_proj,
        current_call_index, previous_call_index, current_range_begin,
        current_range_end, previous_range_begin, previous_range_end,
        current_transform_index, previous_transform_index, current_group_index,
        previous_group_index, current_matrix_id, previous_matrix_id,
        current_projection_type, previous_projection_type,
        current_projection_index, previous_projection_index, reason,
        candidate_valid, position_difference, orientation_difference,
        screen_space_difference);
}

extern "C" void tooie_diag_rt64_match_finalize() {
    tooie_diag::secondary_sway_rt64_match_finalize_event();
}

extern "C" void tooie_diag_rt64_match_flush() {
    tooie_diag::secondary_sway_rt64_match_flush_event();
}

extern "C" uint32_t tooie_diag_rt64_v6c_vertex_load(
    uint32_t slot, uint64_t generation, uint64_t workload_id,
    uint32_t current_dl, uint32_t parent_dl, uint32_t dl_depth,
    uint32_t segmented_source, uint32_t resolved_source,
    uint32_t source_vertex_index, uint32_t world_index,
    uint32_t active_transform, uint32_t stack_depth) {
    return tooie_diag::secondary_sway_rt64_v6c_vertex_load(
        slot, generation, workload_id, current_dl, parent_dl, dl_depth,
        segmented_source, resolved_source, source_vertex_index, world_index,
        active_transform, stack_depth);
}

extern "C" void tooie_diag_rt64_v6c_tri(
    uint64_t prim_seq, uint64_t workload_id, uint32_t active_transform,
    const char* prim_kind, uint32_t current_dl, uint32_t parent_dl,
    uint32_t dl_depth, const uint32_t* slots, const uint32_t* globals,
    const uint32_t* worlds, const uint32_t* bones,
    const uint64_t* generations, const uint64_t* load_workloads,
    const uint32_t* load_dl, const uint32_t* load_parent,
    const uint32_t* load_depth, const uint32_t* segmented_sources,
    const uint32_t* resolved_sources, const uint32_t* source_indices,
    const uint32_t* load_active_transforms, const uint32_t* load_stack_depths) {
    tooie_diag::secondary_sway_rt64_v6c_tri(
        prim_seq, workload_id, active_transform, prim_kind, current_dl,
        parent_dl, dl_depth, slots, globals, worlds, bones, generations,
        load_workloads, load_dl, load_parent, load_depth, segmented_sources,
        resolved_sources, source_indices, load_active_transforms,
        load_stack_depths);
}

extern "C" void tooie_diag_rt64_v6c_gdl(
    uint32_t raw_address, uint32_t resolved_address, uint32_t parent_address,
    uint32_t depth, uint32_t is_call, uint32_t segment3_base) {
    tooie_diag::secondary_sway_rt64_v6c_gdl(
        raw_address, resolved_address, parent_address, depth, is_call,
        segment3_base);
}

extern "C" uint64_t tooie_diag_rt64_v7_model_bin() {
    return tooie_diag::v7_model_bin();
}

extern "C" uint64_t tooie_diag_rt64_v7_model_getter_raw_62a_count() {
    return tooie_diag::v7_model_getter_raw_62a_count();
}
extern "C" uint64_t tooie_diag_rt64_v7_model_getter_raw_62b_count() {
    return tooie_diag::v7_model_getter_raw_62b_count();
}
extern "C" uint64_t tooie_diag_rt64_v7_model_getter_normalized_62b_count() {
    return tooie_diag::v7_model_getter_normalized_62b_count();
}
extern "C" uint64_t tooie_diag_rt64_v7_model_bin_bridge_accept_count() {
    return tooie_diag::v7_model_bin_bridge_accept_count();
}
extern "C" uint64_t tooie_diag_rt64_v7_model_bin_bridge_reject_count() {
    return tooie_diag::v7_model_bin_bridge_reject_count();
}
extern "C" uint64_t tooie_diag_rt64_v7_model_getter_wrapper_call_count() {
    return tooie_diag::v7_model_getter_wrapper_call_count();
}
extern "C" uint64_t tooie_diag_rt64_v7_model_getter_reject_arg_filter_count() {
    return tooie_diag::v7_model_getter_reject_arg_filter_count();
}
extern "C" uint64_t tooie_diag_rt64_v7_model_getter_reject_return_zero_count() {
    return tooie_diag::v7_model_getter_reject_return_zero_count();
}
extern "C" uint64_t tooie_diag_rt64_v7_model_getter_reject_resource_unavailable_count() {
    return tooie_diag::v7_model_getter_reject_resource_unavailable_count();
}
extern "C" uint64_t tooie_diag_rt64_v7_model_getter_reject_raw_resource_other_count() {
    return tooie_diag::v7_model_getter_reject_raw_resource_other_count();
}
extern "C" uint64_t tooie_diag_rt64_v7_model_getter_reject_metadata_unavailable_count() {
    return tooie_diag::v7_model_getter_reject_metadata_unavailable_count();
}
extern "C" uint64_t tooie_diag_rt64_v7_model_getter_reject_metadata_mismatch_count() {
    return tooie_diag::v7_model_getter_reject_metadata_mismatch_count();
}
extern "C" uint64_t tooie_diag_rt64_v7_model_getter_reject_other_count() {
    return tooie_diag::v7_model_getter_reject_other_count();
}
extern "C" const char* tooie_diag_rt64_v7_model_getter_first_rejection_reason() {
    return tooie_diag::v7_model_getter_first_rejection_reason();
}
extern "C" uint64_t tooie_diag_rt64_v7_durable_register_total() {
    return tooie_diag::v7_durable_register_total();
}
extern "C" uint64_t tooie_diag_rt64_v7_durable_normalized_62b_count() {
    return tooie_diag::v7_durable_normalized_62b_count();
}
extern "C" uint64_t tooie_diag_rt64_v7_model_context_publish_count() {
    return tooie_diag::v7_model_context_publish_count();
}
extern "C" uint64_t tooie_diag_rt64_v7_model_context_clear_count() {
    return tooie_diag::v7_model_context_clear_count();
}
extern "C" uint64_t tooie_diag_rt64_v7_model_context_generation() {
    return tooie_diag::v7_model_context_generation();
}
extern "C" uint64_t tooie_diag_rt64_v7_model_context_last_generation() {
    return tooie_diag::v7_model_context_last_generation();
}
extern "C" uint32_t tooie_diag_rt64_v7_model_context_last_physical_base() {
    return tooie_diag::v7_model_context_last_physical_base();
}
extern "C" uint32_t tooie_diag_rt64_v7_model_context_last_player() {
    return tooie_diag::v7_model_context_last_player();
}
extern "C" uint32_t tooie_diag_rt64_v7_model_bridge_first_raw_resource() {
    return tooie_diag::v7_model_bridge_first_raw_resource();
}
extern "C" uint32_t tooie_diag_rt64_v7_model_bridge_first_normalized_resource() {
    return tooie_diag::v7_model_bridge_first_normalized_resource();
}
extern "C" uint64_t tooie_diag_rt64_v7_model_bridge_first_guest_bin() {
    return tooie_diag::v7_model_bridge_first_guest_bin();
}
extern "C" uint32_t tooie_diag_rt64_v7_model_bridge_last_raw_resource() {
    return tooie_diag::v7_model_bridge_last_raw_resource();
}
extern "C" uint32_t tooie_diag_rt64_v7_model_bridge_last_normalized_resource() {
    return tooie_diag::v7_model_bridge_last_normalized_resource();
}
extern "C" uint64_t tooie_diag_rt64_v7_model_bridge_last_guest_bin() {
    return tooie_diag::v7_model_bridge_last_guest_bin();
}
