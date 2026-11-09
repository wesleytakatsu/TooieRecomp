#include "tooie_expanded_memory.h"
#include "tooie_object_distance.h"
#include "librecomp/config.hpp"

void tooie_object_distance::add_config(recomp::config::Config& config) {
    config.add_enum_option(option, "Object Distance",
        "Extends object visibility, update and resource retention distances in story mode. "
        "Story split-screen co-op always uses Original. "
        "Original keeps native distances. Higher settings require active Expanded Memory; selecting them enables it for the next restart. "
        "Unlimited removes the covered distance limits in the current map; camera, sectors and game states still apply. "
        "<recomp-color warning>Experimental.</recomp-color>",
        {{0u, "Original", "Original"}, {1u, "Double", "2x"},
         {2u, "Quadruple", "4x"}, {3u, "Unlimited", "Unlimited"}}, 0u);
    config.add_option_change_callback(option,
        [](recomp::config::ConfigValueVariant current, recomp::config::ConfigValueVariant,
           recomp::config::OptionChangeContext context) {
            if (context != recomp::config::OptionChangeContext::Temporary) {
                const uint32_t value = std::get<uint32_t>(current);
                set_mode(value);
                if (context == recomp::config::OptionChangeContext::Permanent && value != 0)
                    tooie_memory::request_for_distance();
                tooie_memory::refresh_restart_notice();
            }
        });
}
