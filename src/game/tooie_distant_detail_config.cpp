#include "tooie_distant_detail.h"

#include "librecomp/config.hpp"

void tooie_distant_detail::add_config(recomp::config::Config& config) {
    config.add_enum_option(option, "Distant Detail",
        "Keeps characters and objects in full detail far from the camera, instead of switching to their simplified "
        "models. Does not change how far objects are drawn (see Object Distance). May cost some performance in "
        "large areas. <recomp-color warning>Experimental.</recomp-color>",
        {{off, "Off", "Off"}, {on, "On", "On"}}, off);
    config.add_option_change_callback(option,
        [](recomp::config::ConfigValueVariant current, recomp::config::ConfigValueVariant,
           recomp::config::OptionChangeContext context) {
            if (context != recomp::config::OptionChangeContext::Temporary) {
                set_enabled(std::get<uint32_t>(current) == on);
            }
        });
}
