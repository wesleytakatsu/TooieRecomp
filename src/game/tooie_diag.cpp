#include "tooie_diag.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <thread>

#include <SDL2/SDL.h>

namespace tooie_diag {

// Defined below; used by ensure_init's background thread.
void report_now(const char* reason);

namespace {

std::atomic<bool> g_enabled{false};
std::atomic<bool> g_timing_enabled{false};
std::atomic<bool> g_movement_enabled{false};
std::atomic<bool> g_input_enabled{false};
std::atomic<bool> g_frontend_enabled{false};
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

std::chrono::steady_clock::time_point g_last_report{};
std::chrono::steady_clock::time_point g_timing_start{};
int g_interval_ms = 2000;

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

bool frontend_enabled() {
    ensure_init();
    return g_frontend_enabled.load(std::memory_order_relaxed);
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
        } else {
            phase = 9;
            phase_name = "GAMESELECT_WAIT_START";
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
                     "[tooie-timing] wall=%.3fs vi_hz=%.2f (total=%llu) "
                     "logic_hz=%.2f (total=%llu) game_frame_hz=%.2f (total=%llu) "
                     "anim_hz=%.2f (total=%llu) host_gfx_hz=%.2f (total=%llu) "
                     "divisor=%d inp_hz=%.2f\n",
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
    if (g_movement_enabled.load(std::memory_order_relaxed)) {
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

} // namespace tooie_diag
