#ifndef __ultramodern_HPP__
#define __ultramodern_HPP__

#include <thread>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <span>
#include <chrono>
#include <filesystem>

#undef MOODYCAMEL_DELETE_FUNCTION
#define MOODYCAMEL_DELETE_FUNCTION = delete
#include "lightweightsemaphore.h"

#include "ultra64.h"

#include "ultramodern/error_handling.hpp"
#include "ultramodern/events.hpp"
#include "ultramodern/input.hpp"
#include "ultramodern/renderer_context.hpp"
#include "ultramodern/rsp.hpp"
#include "ultramodern/threads.hpp"

struct UltraThreadContext {
    std::thread host_thread;
    moodycamel::LightweightSemaphore running;
    moodycamel::LightweightSemaphore initialized;
};

namespace ultramodern {

constexpr uint32_t save_size = 1024 * 1024 / 8; // Maximum save size, 1Mbit for flash

// Initialization.
void preinit(RDRAM_ARG renderer::WindowHandle window_handle);
void init_saving(RDRAM_ARG1);
void init_events(RDRAM_ARG renderer::WindowHandle window_handle);
void init_timers(RDRAM_ARG1);
void init_thread_cleanup();

// Saving
void change_save_file(const std::u8string& subfolder, const std::u8string& name);
std::filesystem::path get_save_file_path();

// Thread queues.
constexpr PTR(PTR(OSThread)) running_queue = (PTR(PTR(OSThread)))-1;

void thread_queue_insert(RDRAM_ARG PTR(PTR(OSThread)) queue, PTR(OSThread) toadd);
PTR(OSThread) thread_queue_pop(RDRAM_ARG PTR(PTR(OSThread)) queue);
bool thread_queue_remove(RDRAM_ARG PTR(PTR(OSThread)) queue_, PTR(OSThread) t_);
bool thread_queue_empty(RDRAM_ARG PTR(PTR(OSThread)) queue);
PTR(OSThread) thread_queue_peek(RDRAM_ARG PTR(PTR(OSThread)) queue);

// Hardware-like external events carry a cause. Multiplicity is preserved;
// do not collapse pending events into a bitmap. Default None keeps the
// existing 4-argument enqueue source-compatible (software IPC).
enum class InterruptCause : uint8_t {
    None = 0,
    SP,
    SI,
    AI,
    VI,
    PI,
    DP,
    Timer,
};

// Message queues.
void enqueue_external_message(PTR(OSMesgQueue) mq, OSMesg msg, bool jam, bool requeue_if_blocked,
                             InterruptCause cause = InterruptCause::None);
void wait_for_external_message(RDRAM_ARG1);
void wait_for_external_message_timed(RDRAM_ARG1, u32 millis);

// Guest preemption (Hybrid F').
// PATCH 2: real epoch/service/intmask. PATCH 3.5: cause-aware hold-until-eligible.
// Feature remains OFF for games (hint false; service is a no-op unless the
// harness enables the flag). Generator safepoints are PATCH 3. Env override
// is PATCH 4.
//
// C ABI ctx stays void* so this header does not depend on N64Recomp/recomp.h
// (anonymous struct typedef). Do not add a recomp_context* declaration.
// Interrupt state is passed as uint32_t status_reg (SR IE/EXL/ERL/IM).
void set_guest_preemption_safepoints(bool enabled);
bool guest_preemption_safepoints_enabled();
void publish_external_work();
bool dequeue_external_messages(RDRAM_ARG1);
bool cpu_preemptible(uint32_t status_reg);
void service_preemption(RDRAM_ARG uint32_t* status_reg);
void after_interrupt_state_change(RDRAM_ARG uint32_t* status_reg);
void service_pending_interrupts_for_current_guest(RDRAM_ARG1);
void set_current_guest_interrupt_tls(uint8_t* rdram, uint32_t* status_reg);
uint8_t* current_guest_rdram();
uint32_t* current_guest_status_reg();
void flush_held_ineligible_to_external();
size_t held_ineligible_count();
InterruptCause held_ineligible_cause_at(size_t index);

namespace intmask {
constexpr uint32_t SR_IE = 0x00000001u;
constexpr uint32_t SR_EXL = 0x00000002u;
constexpr uint32_t SR_ERL = 0x00000004u;
constexpr uint32_t SR_IM2 = 0x00000400u;
constexpr uint32_t SR_IM7 = 0x00008000u;
constexpr uint32_t SR_IMASK = 0x0000FF00u;
constexpr uint32_t OS_IM_NONE = 0x00000001u;
constexpr uint32_t OS_IM_ALL = 0x003FFF01u;
constexpr uint32_t OS_IM_SP = 0x00010401u;
constexpr uint32_t OS_IM_SI = 0x00020401u;
constexpr uint32_t OS_IM_AI = 0x00040401u;
constexpr uint32_t OS_IM_VI = 0x00080401u;
constexpr uint32_t OS_IM_PI = 0x00100401u;
constexpr uint32_t OS_IM_DP = 0x00200401u;
constexpr uint32_t OS_IM_COUNTER = 0x00008401u;
constexpr uint32_t RCP_IMASK = 0x003F0000u;
constexpr uint32_t MI_INTR_SP = 0x01u;
constexpr uint32_t MI_INTR_SI = 0x02u;
constexpr uint32_t MI_INTR_AI = 0x04u;
constexpr uint32_t MI_INTR_VI = 0x08u;
constexpr uint32_t MI_INTR_PI = 0x10u;
constexpr uint32_t MI_INTR_DP = 0x20u;

uint32_t status_reg_init();
uint32_t current_combined_mask(uint32_t status_reg);
uint32_t apply_os_set_int_mask(RDRAM_ARG uint32_t* status_reg, uint32_t a0);
uint32_t disable_int(uint32_t* status_reg);
void restore_int(RDRAM_ARG uint32_t* status_reg, uint32_t a0);
bool cause_eligible(uint32_t status_reg, InterruptCause cause);
bool cause_eligible(const uint32_t* status_reg, InterruptCause cause);
}

// Thread scheduling.
void check_running_queue(RDRAM_ARG1);
void run_next_thread_and_wait(RDRAM_ARG1);
void resume_thread_and_wait(RDRAM_ARG OSThread* t);
void schedule_running_thread(RDRAM_ARG PTR(OSThread) t);
void cleanup_thread(UltraThreadContext* thread_context);
struct thread_terminated : std::exception {};

enum class ThreadPriority {
    Low,
    Normal,
    High,
    VeryHigh,
    Critical
};

void set_native_thread_name(const std::string& name);
void set_native_thread_priority(ThreadPriority pri);
PTR(OSThread) this_thread();
void set_main_thread();
bool is_game_thread();
void submit_rsp_task(RDRAM_ARG PTR(OSTask) task);
void send_si_message();
uint32_t get_speed_multiplier();

// Time
std::chrono::high_resolution_clock::time_point get_start();
std::chrono::high_resolution_clock::duration time_since_start();
void measure_input_latency();
void sleep_milliseconds(uint32_t millis);
void sleep_until(const std::chrono::high_resolution_clock::time_point& time_point);

// Graphics
uint32_t get_target_framerate(uint32_t original);
uint32_t get_display_refresh_rate();
float get_resolution_scale();
void trigger_config_action();

// Audio
void init_audio();
void set_audio_frequency(uint32_t freq);
void queue_audio_buffer(RDRAM_ARG PTR(s16) audio_data, uint32_t byte_count);
uint32_t get_remaining_audio_bytes();

struct audio_callbacks_t {
    using queue_samples_t = void(int16_t*, size_t);
    using get_samples_remaining_t = size_t();
    using set_frequency_t = void(uint32_t);
    queue_samples_t* queue_samples;
    get_samples_remaining_t* get_frames_remaining;
    set_frequency_t* set_frequency;
};

// TODO: Most of the members of this struct are not used by ultramodern. Should we move them to librecomp instead?
struct gfx_callbacks_t {
    using gfx_data_t = void*;
    using create_gfx_t = gfx_data_t();
    using create_window_t = renderer::WindowHandle(gfx_data_t);
    using update_gfx_t = void(gfx_data_t);

    create_gfx_t* create_gfx;
    create_window_t* create_window;
    update_gfx_t* update_gfx;
};

bool is_game_started();
void quit();
void join_event_threads();
void join_thread_cleaner_thread();
void join_saving_thread();

void set_audio_callbacks(const audio_callbacks_t& callbacks);

/**
 * Register all the callbacks used by `ultramodern`, most of them being optional.
 *
 * The following arguments contain mandatory callbacks that need to be registered (i.e., can't be `nullptr`):
 * - `rsp_callbacks`
 * - `renderer_callbacks`
 *
 * It must be called only once and it must be called before `ultramodern::preinit`.
 */
void set_callbacks(
    const rsp::callbacks_t& rsp_callbacks,
    const renderer::callbacks_t& renderer_callbacks,
    const audio_callbacks_t& audio_callbacks,
    const input::callbacks_t& input_callbacks,
    const gfx_callbacks_t& gfx_callbacks,
    const events::callbacks_t& events_callbacks,
    const error_handling::callbacks_t& error_handling_callbacks,
    const threads::callbacks_t& threads_callbacks
);
} // namespace ultramodern

// C ABI for generated code (PATCH 3). Declared at file scope so the
// unmangled names match pause_self. The second argument is an opaque
// uint32_t* status_reg (typically (void*)&ctx->status_reg). void* avoids
// a N64Recomp/recomp.h dependency (anonymous struct typedef).
extern "C" {
bool ultramodern_preemption_hint(void);
void ultramodern_service_preemption(uint8_t* rdram, void* status_reg);
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define debug_printf(...)
//#define debug_printf(...) printf(__VA_ARGS__);

#endif
