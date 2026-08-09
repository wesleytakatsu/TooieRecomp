#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>
#include <span>

#include "librecomp/game.hpp"
#include "banjo_game.h"

// Tooie NTSC-U cart XXH3-64 (32 MiB .z64). For runtime we currently accept the
// *decompressed* image used by the recompiler (see GameEntry.rom_hash).
// Full cart→decompressed path lands with a dedicated decompressor callback.

namespace tooie {

// Identity: recompiler metadata is built against decompressed.us.z64.
// When the user supplies that image, no further transform is required.
std::vector<uint8_t> decompress_tooie(std::span<const uint8_t> rom) {
    return std::vector<uint8_t>(rom.begin(), rom.end());
}

void tooie_on_init(uint8_t* rdram, recomp_context* ctx) {
    (void)ctx;
    // Mirror BK: seed a small ROM window the game may read early.
    // Entry uses recomp::do_rom_read inside librecomp::init already.
    (void)rdram;
}

} // namespace tooie

// Keep Banjo symbol names used by copied headers/sources until fully renamed.
namespace banjo {
std::vector<uint8_t> decompress_bk(std::span<const uint8_t> rom) {
    return tooie::decompress_tooie(rom);
}
void bk_on_init(uint8_t* rdram, recomp_context* ctx) {
    tooie::tooie_on_init(rdram, ctx);
    // After init_overlays: fill LOOKUP_FUNC holes for unnamed boot helpers.
    banjo::register_boot_lookup_stubs();
}
} // namespace banjo
