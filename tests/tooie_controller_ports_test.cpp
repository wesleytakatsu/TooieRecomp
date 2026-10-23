#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include "tooie_controller_ports.h"

using namespace tooie_controller_ports;

#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); std::exit(1); } } while (0)

int main() {
    // All assignment combinations, including non-contiguous defensive cases.
    for (unsigned mask = 0; mask < 16; mask++) {
        PortQuery query{};
        for (int port = 0; port < 4; port++) {
            query.assigned[port] = (mask & (1u << port)) != 0;
            query.assigned_count += query.assigned[port];
        }
        for (bool single_player : {false, true}) {
            query.single_player = single_player;
            for (int port = 0; port < 4; port++) {
                const bool connected = single_player || port == 0 || query.assigned[port];
                const PortInput source = !connected ? PortInput::NoResponse
                    : !single_player && mask == 0 && port == 0 ? PortInput::MergedFallback
                    : PortInput::Profiles;
                CHECK(port_connected(query, port) == connected);
                CHECK(port_input_source(query, port) == source);
            }
            for (int port : {-1, 4, 128}) {
                CHECK(!port_connected(query, port));
                CHECK(port_input_source(query, port) == PortInput::NoResponse);
            }
        }
    }
    std::puts("tooie_controller_ports_test PASS");
    return 0;
}
