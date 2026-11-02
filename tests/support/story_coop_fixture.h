#pragma once
#include "tooie_story_coop.h"
#include "recomp.h"
#include <cstdlib>

// Subsystem fixture: explicitly prove allocation provenance. This is not a
// replacement for the real-native creation/teardown regression in story_coop_test.
inline void commit_fixture_story_pair(uint8_t* rdram) {
    tooie_story_coop::begin_creation(MEM_HU(0, int32_t(0x80132DC2)), true);
    for (int n = 0; n < 2; ++n) {
        tooie_story_coop::begin_normal_allocation();
        tooie_story_coop::end_normal_allocation(rdram, n);
    }
    if (!tooie_story_coop::finish_creation(rdram)) std::abort();
}
