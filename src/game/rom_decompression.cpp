#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>
#include <span>

#include "librecomp/game.hpp"
#include "banjo_game.h"
#include "xxHash/xxh3.h"
#include "rom_decompressor_lib.h"

// Tooie NTSC-U retail cart (32 MiB, .z64 byte order) XXH3-64. GameEntry.rom_hash is the XXH3-64 of the *decompressed*
// image the recompiled code expects; the cart is converted to it by tooie_decompress_rom (vendored rom_decompressor).

namespace tooie {

constexpr size_t kCartSize = 0x2000000;
constexpr uint64_t kCartHash = 0x00F70DE5F2D70EA2ULL;

// Retail cart -> decompressed image. Anything else (notably the already decompressed image) is returned unchanged.
std::vector<uint8_t> decompress_tooie(std::span<const uint8_t> rom) {
    if (rom.size() == kCartSize && XXH3_64bits(rom.data(), rom.size()) == kCartHash) {
        std::vector<char> cart(rom.begin(), rom.end());
        std::vector<char> decompressed;
        if (tooie_decompress_rom(cart, decompressed, nullptr)) {
            return std::vector<uint8_t>(decompressed.begin(), decompressed.end());
        }
        return {};
    }
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
