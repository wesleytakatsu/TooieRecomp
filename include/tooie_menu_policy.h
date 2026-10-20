#ifndef TOOIE_MENU_POLICY_H
#define TOOIE_MENU_POLICY_H

// Pure Tooie settings policy (no SDL / UI dependencies, host-testable).

#include <algorithm>
#include <atomic>
#include <functional>
#include <cmath>
#include <cstdint>
#include <string>
#include "ultramodern/config.hpp"
#include "librecomp/config.hpp"

namespace tooie_menu {

// 60Hz presentation exists, but CURRENT repeats poses; user-requested temporary cap: presentation forced to 30 on config load.
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

// Forces presentation to Manual/kForcedRefreshRate for any load/set path of the two options.
// `resolve` must look the Config up on every call: the frontend keeps configs in a std::vector, so a
// Config& taken at registration dangles (moved) once later tabs are created.
inline void install_forced_refresh(std::function<recomp::config::Config&()> resolve, const std::string& rr_id, const std::string& rr_manual_id) {
    auto force = [resolve, rr_id, rr_manual_id](recomp::config::ConfigValueVariant, recomp::config::ConfigValueVariant, recomp::config::OptionChangeContext) {
        recomp::config::Config* cfg = &resolve();
        auto enforce = [cfg](const std::string& id, recomp::config::ConfigValueVariant want) {
            if (cfg->get_temp_option_value(id) != want) {
                cfg->set_option_value(id, want);
            }
            if (cfg->get_option_value(id) != want) {
                cfg->apply_option_value(id);
            }
        };
        enforce(rr_id, static_cast<uint32_t>(ultramodern::renderer::RefreshRate::Manual));
        enforce(rr_manual_id, static_cast<double>(kForcedRefreshRate));
    };
    recomp::config::Config& config = resolve();
    config.add_option_change_callback(rr_id, force);
    config.add_option_change_callback(rr_manual_id, force);
    force({}, {}, recomp::config::OptionChangeContext::Load);
}

} // namespace tooie_menu

#endif // TOOIE_MENU_POLICY_H
