#include "ovl_patches.hpp"
#include "../../RecompiledFuncs/recomp_overlays.inl"

#include "librecomp/overlays.hpp"
#include "tooie_guest_memory.h"

void tooie::register_tooie_overlays() {
    uint32_t high_water = 0x80800000u;
    for (const auto& section : section_table) {
        const uint32_t address = uint32_t(section.ram_addr);
        if (address >= 0x80800000u && address < 0xA0000000u) {
            const uint64_t end = uint64_t(address) + section.size;
            if (end > UINT32_MAX) {
                high_water = UINT32_MAX;
            } else if (end > high_water) {
                high_water = uint32_t(end);
            }
        }
    }
    tooie_memory::static_overlay_end.store(high_water, std::memory_order_release);
    recomp::overlays::overlay_section_table_data_t sections {
        .code_sections = section_table,
        .num_code_sections = ARRLEN(section_table),
        .total_num_sections = num_sections,
    };

    recomp::overlays::overlays_by_index_t overlays {
        .table = overlay_sections_by_index,
        .len = ARRLEN(overlay_sections_by_index),
    };

    recomp::overlays::register_overlays(sections, overlays);
}
