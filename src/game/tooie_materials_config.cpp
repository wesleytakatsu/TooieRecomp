#include "tooie_materials.h"

#include <algorithm>
#include <cmath>
#include <memory>

#include "librecomp/config.hpp"

void tooie_materials::add_config(recomp::config::Config& config, ModeSink mode_sink, StrengthSink strength_sink) {
    config.add_enum_option(option, "Materials",
        "Lets material texture packs (normal maps) add relief to the textures they cover, lit by a fixed light from "
        "above. Needs a material pack enabled together with the color packs it was made for. Off changes nothing. On "
        "shows the relief on the textures. Relief View shows only the relief, as gray (middle gray: unchanged; lighter: "
        "lit; darker: shaded), to check the maps. Turning it on or off reloads the texture packs. "
        "Only with Direct3D 12 or Vulkan. "
        "<recomp-color warning>Experimental.</recomp-color>",
        {{off, "Off", "Off"}, {on, "On", "On"}, {relief_view, "ReliefView", "Relief View"}}, off);

    config.add_number_option(strength_option, "Materials Depth",
        "How deep the relief of the materials looks: 100% is the pack's own, lower is softer (0% flat), higher is "
        "stronger, up to 400%.",
        strength_min, strength_max, 10, 0, true, strength_default);

    std::vector<recomp::config::ConfigValueVariant> materials_off{ off };
    config.add_option_disable_dependency(strength_option, option, materials_off);

    struct State {
        ModeSink sink;
        bool enabled = false;
    };

    auto state = std::make_shared<State>();
    state->sink = std::move(mode_sink);
    config.add_option_change_callback(option,
        [state](recomp::config::ConfigValueVariant current, recomp::config::ConfigValueVariant,
                recomp::config::OptionChangeContext context) {
            if (context == recomp::config::OptionChangeContext::Temporary) {
                return;
            }

            const uint32_t* value = std::get_if<uint32_t>(&current);
            const uint32_t mode = ((value != nullptr) && (*value <= relief_view)) ? *value : off;
            const bool enabled = (mode != off);

            // Maps are loaded with the packs: reload them when materials turn on or off (also when a saved On is loaded,
            // in case the packs were loaded before it).
            const bool reload_packs = (enabled != state->enabled);
            state->enabled = enabled;
            if (state->sink) {
                state->sink(mode, reload_packs);
            }
        });

    auto shared_strength_sink = std::make_shared<StrengthSink>(std::move(strength_sink));
    config.add_option_change_callback(strength_option,
        [shared_strength_sink](recomp::config::ConfigValueVariant current, recomp::config::ConfigValueVariant,
                               recomp::config::OptionChangeContext context) {
            if (context == recomp::config::OptionChangeContext::Temporary) {
                return;
            }

            const double* value = std::get_if<double>(&current);
            const double percent = ((value != nullptr) && std::isfinite(*value))
                ? std::clamp(*value, strength_min, strength_max) : strength_default;
            if (*shared_strength_sink) {
                (*shared_strength_sink)(static_cast<float>(percent / 100.0));
            }
        });
}
