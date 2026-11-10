#include "tooie_texture_streaming.h"

#include <algorithm>
#include <memory>

#include "librecomp/config.hpp"

void tooie_texture_streaming::add_config(recomp::config::Config& config, LevelSink sink) {
    config.add_enum_option(option, "Texture Streaming",
        "Keeps each texture pack image in video memory only at the detail its distance needs, and loads more when the "
        "camera gets closer. Lets large texture packs fit in less video memory. Off loads every image whole, as before. "
        "Maximum keeps all the detail the screen shows; lower levels save more memory and may look softer up close. "
        "Only with Direct3D 12 or Vulkan, and only for texture packs. "
        "<recomp-color warning>Experimental.</recomp-color>",
        {{0u, "Off", "Off"}, {1u, "Minimum", "Minimum"}, {2u, "Low", "Low"},
         {3u, "Medium", "Medium"}, {4u, "High", "High"}, {5u, "Maximum", "Maximum"}}, off);

    auto shared_sink = std::make_shared<LevelSink>(std::move(sink));
    config.add_option_change_callback(option,
        [shared_sink](recomp::config::ConfigValueVariant current, recomp::config::ConfigValueVariant,
                      recomp::config::OptionChangeContext context) {
            if (context == recomp::config::OptionChangeContext::Temporary) {
                return;
            }

            const uint32_t* value = std::get_if<uint32_t>(&current);
            const uint32_t level = (value != nullptr) ? std::min(*value, maximum) : off;
            if (*shared_sink) {
                (*shared_sink)(level);
            }
        });
}
