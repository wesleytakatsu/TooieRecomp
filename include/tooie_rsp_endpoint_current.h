#ifndef TOOIE_RSP_ENDPOINT_CURRENT_H
#define TOOIE_RSP_ENDPOINT_CURRENT_H

// P09 shader-local CURRENT tfPos palliative.
// WORKAROUND, not a true interpolation fix. RT64 still owns the flag and
// still defaults it off when the variable is absent, so Banjo is unchanged.
// Tooie calls apply_default_before_renderer_init() once before recomp::start.
// RT64 caches the first getenv in a function-local static; a later change
// applies on the next launch.

namespace tooie_rsp_endpoint_current {

inline constexpr const char* kEnvName = "TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT";
inline constexpr const char* kDefaultValue = "1";

enum class ApplyResult {
    AppliedDefault,
    PreservedExplicit,
    SetenvFailed,
};

// Unset -> set kDefaultValue. Any existing value, including "0", is kept.
ApplyResult apply_default_before_renderer_init();

} // namespace tooie_rsp_endpoint_current

#endif
