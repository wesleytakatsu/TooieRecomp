#include "tooie_materials.h"

#include <memory>

#include "librecomp/config.hpp"

void tooie_materials::add_config(recomp::config::Config& config, ModeSink sink) {
    config.add_enum_option(option, "Materials",
        "Lets material texture packs (normal maps) add relief to the textures they cover, lit by a fixed light from "
        "above. Needs a material pack enabled together with the color packs it was made for. Relief View shows only the "
        "relief, as gray (middle gray: unchanged). Turning it on or off reloads the texture packs. "
        "Only with Direct3D 12 or Vulkan. "
        "<recomp-color warning>Experimental.</recomp-color>",
        {{off, "Off", "Off"}, {on, "On", "On"}, {relief_view, "ReliefView", "Relief View"}}, off);

    struct State {
        ModeSink sink;
        bool enabled = false;
    };

    auto state = std::make_shared<State>();
    state->sink = std::move(sink);
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
}
