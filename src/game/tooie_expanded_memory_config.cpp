#include "tooie_expanded_memory.h"
#include "banjo_config.h"
#include "tooie_object_distance.h"
#include "librecomp/config.hpp"
#include "librecomp/overlays.hpp"
#include "hle/rt64_host_rdram.h"

#include <cstdlib>
#include <cstring>

namespace {
std::function<recomp::config::Config&()> general;
std::function<recomp::config::Config&()> graphics;
constexpr const char* description =
    "Expands the game's memory for extended object distances. Takes effect only after restarting the game. "
    "This departs from the original hardware memory layout. Experimental.";
bool requested() {
    return general && std::get<uint32_t>(general().get_option_value(banjo::configkeys::general::expanded_memory)) == 1;
}
void update_notice() {
    if (!general) return;
    const bool restart = tooie_memory::boot.restart_required(requested());
    // Graphics API uses the standard option description for its next-launch notice.
    general().update_option_description(banjo::configkeys::general::expanded_memory,
        std::string(description) + (restart ? "<br /><br />Restart the game to apply." : ""));
    general().update_option_enum_details(banjo::configkeys::general::expanded_memory,
        restart ? "Restart the game to apply" : "");
    if (graphics) {
        const bool distance_restart = !tooie_memory::boot.active() &&
            tooie_object_distance::mode() != tooie_object_distance::Mode::Original;
        graphics().update_option_enum_details(tooie_object_distance::option,
            distance_restart ? "Restart the game to apply (using Original)" : "");
    }
}
}

void tooie_memory::install_distance_notice(std::function<recomp::config::Config&()> get_config) {
    graphics = std::move(get_config);
}

void tooie_memory::refresh_restart_notice() {
    update_notice();
}

void tooie_memory::add_config(recomp::config::Config& config, std::function<recomp::config::Config&()> get_config) {
    general = std::move(get_config);
    config.add_enum_option(banjo::configkeys::general::expanded_memory, "Expanded Memory (experimental)",
        description, {{0u, "Off", "Off"}, {1u, "On", "On"}}, 0u);
    config.add_option_change_callback(banjo::configkeys::general::expanded_memory,
        [](auto, auto, recomp::config::OptionChangeContext context) {
            if (context != recomp::config::OptionChangeContext::Temporary) update_notice();
        });
}

void tooie_memory::finish_config() {
    const char* value = std::getenv("TOOIE_TEST_EXPANDED_HEAP");
    const bool permitted = (!value || std::strcmp(value, "0") != 0) &&
        layout_available(static_overlay_end.load(std::memory_order_acquire),
            recomp::overlays::get_patch_binary().size());
    boot.latch(requested(), permitted);
    RT64::setHostRdramExpanded(boot.active());
    update_notice();
}

void tooie_memory::request_for_distance() {
    if (!general || boot.active() || requested()) return;
    general().set_option_value(banjo::configkeys::general::expanded_memory, uint32_t(1));
    general().save_config();
    update_notice();
}
