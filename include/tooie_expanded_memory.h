#pragma once

#include "tooie_guest_memory.h"
#include "recomp.h"
#include <functional>

namespace recomp::config { class Config; }
namespace tooie_memory {
inline constexpr uint32_t guard_free_reserve = 768 * 1024;
inline constexpr uint32_t guard_largest_reserve = 256 * 1024;
inline bool guard_allows(uint32_t free, uint32_t largest) {
    return free >= guard_free_reserve && largest >= guard_largest_reserve;
}
void add_config(recomp::config::Config& config, std::function<recomp::config::Config&()> get_config);
void install_distance_notice(std::function<recomp::config::Config&()> get_config);
void refresh_restart_notice();
void finish_config();
void request_for_distance();
bool distance_heap_available(uint8_t* rdram);
}

extern "C" void tooie_heap_end(uint8_t*, recomp_context*);
extern "C" void tooie_heap_size(uint8_t*, recomp_context*);
