#include "tooie_rsp_endpoint_current.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(_WIN32)
#include <stdlib.h>
#endif

namespace tooie_rsp_endpoint_current {

ApplyResult apply_default_before_renderer_init() {
    // std::getenv is what RT64 caches. On Windows that is the CRT environment,
    // so the write has to be _putenv_s rather than SetEnvironmentVariable.
    const char* existing = std::getenv(kEnvName);
    if (existing != nullptr) {
        return ApplyResult::PreservedExplicit;
    }

#if defined(_WIN32)
    if (_putenv_s(kEnvName, kDefaultValue) != 0) {
        std::fprintf(stderr,
                     "[tooie] rsp endpoint current workaround: _putenv_s(%s) failed: %s\n",
                     kEnvName, std::strerror(errno));
        return ApplyResult::SetenvFailed;
    }
#else
    if (setenv(kEnvName, kDefaultValue, 0) != 0) {
        std::fprintf(stderr,
                     "[tooie] rsp endpoint current workaround: setenv(%s) failed: %s\n",
                     kEnvName, std::strerror(errno));
        return ApplyResult::SetenvFailed;
    }
#endif
    return ApplyResult::AppliedDefault;
}

} // namespace tooie_rsp_endpoint_current
