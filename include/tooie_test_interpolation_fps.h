#ifndef TOOIE_TEST_INTERPOLATION_FPS_H
#define TOOIE_TEST_INTERPOLATION_FPS_H

// Opt-in visual test. 0 keeps the stable Manual/30 + CURRENT=1 path.
// Accepted presentation rates are 60 and 120. Anything else is off.

#include <cstdint>
#include <cstdlib>

inline uint32_t tooie_test_interpolation_fps() {
    const char* env = std::getenv("TOOIE_TEST_INTERPOLATION_FPS");
    if (env == nullptr || env[0] == '\0' || env[0] == '0') {
        return 0;
    }
    char* end = nullptr;
    const long parsed = std::strtol(env, &end, 10);
    if (end == env || end == nullptr || *end != '\0') {
        return 0;
    }
    if (parsed == 60 || parsed == 120) {
        return static_cast<uint32_t>(parsed);
    }
    return 0;
}

#endif
