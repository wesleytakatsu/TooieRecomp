#pragma once

#include <cstddef>

namespace tooie_controller_ports {
    struct PortQuery {
        bool single_player;
        bool assigned[4];
        std::size_t assigned_count;
    };

    enum class PortInput {
        Profiles,
        MergedFallback,
        NoResponse
    };

    constexpr bool port_connected(const PortQuery& query, int port) {
        return port >= 0 && port < 4 && (query.single_player || port == 0 || query.assigned[port]);
    }

    constexpr PortInput port_input_source(const PortQuery& query, int port) {
        if (!port_connected(query, port)) {
            return PortInput::NoResponse;
        }
        if (!query.single_player && port == 0 && !query.assigned[0] && query.assigned_count == 0) {
            return PortInput::MergedFallback;
        }
        return PortInput::Profiles;
    }
}
