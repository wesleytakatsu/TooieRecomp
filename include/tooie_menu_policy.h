#ifndef TOOIE_MENU_POLICY_H
#define TOOIE_MENU_POLICY_H

// Pure Tooie settings policy (no SDL / UI dependencies, host-testable).

#include <algorithm>
#include <atomic>
#include <functional>
#include <memory>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <string>
#include "ultramodern/config.hpp"
#include "librecomp/config.hpp"
#include "tooie_test_interpolation_fps.h"

namespace tooie_menu {

// Native cadence: presentation rate while interpolation is OFF.
constexpr uint32_t kForcedRefreshRate = 30;

struct WindowSize {
    int w;
    int h;
};

// Target window aspect for the Aspect Ratio setting. Returns 0 when the option must not resize (Manual/unknown).
inline double window_aspect_target(ultramodern::renderer::AspectRatio ar) {
    switch (ar) {
        case ultramodern::renderer::AspectRatio::Original: return 4.0 / 3.0;
        case ultramodern::renderer::AspectRatio::Expand: return 16.0 / 9.0;
        default: return 0.0;
    }
}

// Keeps the current height, derives width from `aspect`, then shrinks (keeping aspect) to fit usable bounds.
// Returns the current size unchanged on invalid input.
inline WindowSize aspect_window_size(double aspect, int cur_w, int cur_h, int usable_w, int usable_h) {
    if (!(aspect > 0.0) || !std::isfinite(aspect) || cur_w <= 0 || cur_h <= 0) {
        return {cur_w, cur_h};
    }
    double w = std::round(cur_h * aspect);
    double h = cur_h;
    if (usable_w > 0 && w > usable_w) {
        w = usable_w;
        h = std::round(w / aspect);
    }
    if (usable_h > 0 && h > usable_h) {
        h = usable_h;
        w = std::round(h * aspect);
    }
    return {std::max(1, static_cast<int>(w)), std::max(1, static_cast<int>(h))};
}

// Cross-thread window aspect request: UI thread queues, SDL (update_gfx) thread consumes. Last request wins; 0 = none.
inline std::atomic<double>& window_aspect_request_slot() {
    static std::atomic<double> slot{0.0};
    return slot;
}
inline void request_window_aspect(double target) {
    window_aspect_request_slot().store(target, std::memory_order_release);
}
inline double take_window_aspect_request() {
    return window_aspect_request_slot().exchange(0.0, std::memory_order_acq_rel);
}

// Interpolation policy. OFF (default, legacy/invalid configs): presentation is Manual/kForcedRefreshRate, the native
// cadence, and the refresh-rate controls are locked. ON: the controls unlock; a stored rate that would not generate
// intermediate frames (Original, or Manual at/below native) becomes Manual/kInterpolationDefaultRate.
constexpr uint32_t kInterpolationDefaultRate = 60;

struct RefreshChoice {
    uint32_t mode;   // ultramodern::renderer::RefreshRate
    double manual;
    bool operator==(const RefreshChoice& o) const { return mode == o.mode && manual == o.manual; }
};

// Legacy TOOIE_TEST_INTERPOLATION_FPS (60/120) stays an explicit override: it forces interpolation ON at that rate
// regardless of the option. The caller logs when it contradicts the stored option.
inline bool interpolation_effective(bool option_on) {
    return option_on || tooie_test_interpolation_fps() != 0;
}

inline RefreshChoice coerce_refresh(bool interpolation_on, RefreshChoice cur) {
    constexpr uint32_t manual_mode = static_cast<uint32_t>(ultramodern::renderer::RefreshRate::Manual);
    constexpr uint32_t original_mode = static_cast<uint32_t>(ultramodern::renderer::RefreshRate::Original);
    if (!interpolation_on) {
        return {manual_mode, static_cast<double>(kForcedRefreshRate)};
    }
    if (const uint32_t testFps = tooie_test_interpolation_fps(); testFps != 0) {
        return {manual_mode, static_cast<double>(testFps)};
    }
    if (cur.mode == original_mode || (cur.mode == manual_mode && !(cur.manual > kForcedRefreshRate))) {
        return {manual_mode, static_cast<double>(kInterpolationDefaultRate)};
    }
    return cur;
}

// Wires the interpolation option to the refresh-rate options and to `sink`, which receives the APPLIED state on load
// and whenever an applied change settles (the RT64 request in the game; a recorder in tests).
// - Temp view (what the UI shows) is coerced and the rate controls are (un)locked on every change.
// - Applied view is coerced and sunk on Load (once the last of the three options has loaded, so a stored 120 survives
//   boot) and when the interpolation option itself is applied.
// `resolve` must look the Config up on every call: the frontend keeps configs in a std::vector, so a Config& taken at
// registration dangles (moved) once later tabs are created. The interpolation option must already be registered.
using InterpolationSink = std::function<void(bool)>;

inline void install_interpolation_policy(std::function<recomp::config::Config&()> resolve, const std::string& rr_id, const std::string& rr_manual_id,
                                         const std::string& interp_id, InterpolationSink sink) {
    using recomp::config::ConfigValueVariant;
    using recomp::config::OptionChangeContext;
    struct State {
        bool busy = false;
        int sunk = -1;
    };
    auto st = std::make_shared<State>();
    if (const uint32_t testFps = tooie_test_interpolation_fps(); testFps != 0) {
        std::fprintf(stderr, "[tooie] TOOIE_TEST_INTERPOLATION_FPS=%u overrides the Interpolation setting (forced ON at that rate)\n", testFps);
    }

    // Load settles when the option with the highest schema index has loaded.
    const auto& by_id = resolve().get_config_schema().options_by_id;
    std::string trigger = interp_id;
    for (const std::string& id : {rr_id, rr_manual_id}) {
        if (by_id.at(id) > by_id.at(trigger)) {
            trigger = id;
        }
    }

    auto run = [=](const std::string& source, OptionChangeContext ctx) {
        // Earlier options load before the interpolation value exists; coercing then would clobber a stored rate.
        const bool is_load = (ctx == OptionChangeContext::Load);
        if (st->busy || (is_load && !source.empty() && source != trigger)) {
            return;
        }
        st->busy = true;
        struct Reset { State* s; ~Reset() { s->busy = false; } } reset{st.get()};

        recomp::config::Config& c = resolve();
        auto on = [](const ConfigValueVariant& v) {
            const bool* b = std::get_if<bool>(&v);
            return interpolation_effective(b != nullptr && *b);
        };
        auto get = [&](bool temp) {
            return RefreshChoice{
                std::get<uint32_t>(temp ? c.get_temp_option_value(rr_id) : c.get_option_value(rr_id)),
                std::get<double>(temp ? c.get_temp_option_value(rr_manual_id) : c.get_option_value(rr_manual_id))};
        };
        auto set_temp = [&](const RefreshChoice& want) {
            // update_option_value also tells the open UI to redraw the widget.
            if (get(true).mode != want.mode) c.update_option_value(rr_id, want.mode);
            if (get(true).manual != want.manual) c.update_option_value(rr_manual_id, want.manual);
        };

        const bool temp_on = on(c.get_temp_option_value(interp_id));
        const bool applied_on = on(c.get_option_value(interp_id));
        // Back at the applied interpolation state (cancel, or toggled there and back): show the applied rate again.
        const bool back_to_applied = (source == interp_id && ctx == OptionChangeContext::Temporary && temp_on == applied_on);
        const RefreshChoice want_temp = coerce_refresh(temp_on, get(!back_to_applied));
        set_temp(want_temp);
        c.update_option_disabled(rr_id, !temp_on);
        c.update_option_disabled(rr_manual_id, !temp_on);
        c.update_option_enum_details(rr_id, temp_on ? "" : "Locked to 30 FPS");

        const bool settle = is_load ||
                            (ctx == OptionChangeContext::Permanent && source == interp_id);
        if (!settle) {
            return;
        }
        const RefreshChoice want_applied = coerce_refresh(applied_on, get(false));
        if (get(false).mode != want_applied.mode) {
            c.set_option_value(rr_id, want_applied.mode);
            c.apply_option_value(rr_id);
        }
        if (get(false).manual != want_applied.manual) {
            c.set_option_value(rr_manual_id, want_applied.manual);
            c.apply_option_value(rr_manual_id);
        }
        set_temp(want_temp);
        if (st->sunk != static_cast<int>(applied_on)) {
            st->sunk = static_cast<int>(applied_on);
            sink(applied_on);
        }
    };

    recomp::config::Config& config = resolve();
    for (const std::string& id : {rr_id, rr_manual_id, interp_id}) {
        config.add_option_change_callback(id, [run, id](ConfigValueVariant, ConfigValueVariant, OptionChangeContext ctx) { run(id, ctx); });
    }
    run({}, OptionChangeContext::Load);
}

} // namespace tooie_menu

#endif // TOOIE_MENU_POLICY_H
