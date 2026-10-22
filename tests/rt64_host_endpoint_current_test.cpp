// RT64 host endpoint-current switch: env override > host request latched per workload > off.
// Run three times: env unset (host controls), env=0 (forced interpolate), env=1 (forced CURRENT).
#include <cstdio>
#include <cstdlib>
#include "hle/rt64_host_endpoint_current.h"

#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); std::exit(1); } } while (0)

int main() {
    using namespace RT64;
    const char *env = std::getenv("TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT");
    const int envState = (env == nullptr || env[0] == '\0') ? -1 : (env[0] != '0' ? 1 : 0);
    CHECK(endpointCurrentEnvState() == envState);

    // Never requested (Banjo, or before the host config loaded): off unless the env says otherwise.
    CHECK(endpointCurrentActive() == (envState == 1));

    // A request alone changes nothing until the render thread latches at a workload boundary.
    requestHostEndpointCurrent(true);
    CHECK(endpointCurrentActive() == (envState == 1));
    latchHostEndpointCurrent();
    CHECK(endpointCurrentActive() == (envState >= 0 ? envState == 1 : true));

    // ON -> OFF -> ON at runtime; the request mid-workload is invisible until the next latch.
    requestHostEndpointCurrent(false);
    CHECK(endpointCurrentActive() == (envState >= 0 ? envState == 1 : true));
    latchHostEndpointCurrent();
    CHECK(endpointCurrentActive() == (envState == 1));
    requestHostEndpointCurrent(true);
    latchHostEndpointCurrent();
    CHECK(endpointCurrentActive() == (envState >= 0 ? envState == 1 : true));

    std::printf("rt64_host_endpoint_current_test PASS env=%d\n", envState);
    return 0;
}
