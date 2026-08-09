// Placeholder RSP audio microcode until Tooie n_aspMain is recompiled (P07).
#include "librecomp/rsp.hpp"

// Match Banjo main: `extern RspUcodeFunc n_aspMain;` (C++ linkage).
RspExitReason n_aspMain(uint8_t* /*rdram*/, uint32_t /*ucode_addr*/) {
    return RspExitReason::Broke;
}
