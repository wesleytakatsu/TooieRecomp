// Retail cart import: decompress_tooie (the GameEntry decompression_routine) must turn the 32 MiB retail cart into the
// decompressed image the recompiled code expects, for the .z64 layout and for the 2-byte-swapped (.n64) layout after the
// same normalisation librecomp select_rom applies, and must pass an already decompressed image through unchanged.
// Needs a private ROM: $TOOIE_ROM, else "Banjo-Tooie (USA).z64" in the working directory or one of its parents. SKIPs (exit 0) without one.
// ROM bytes are never printed or written.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <vector>

#include "xxHash/xxh3.h"
#include "rom_decompressor_lib.h"

namespace tooie {
std::vector<uint8_t> decompress_tooie(std::span<const uint8_t> rom);
}
namespace banjo {
// Referenced by rom_decompression.cpp (bk_on_init); not exercised here.
void register_boot_lookup_stubs() {}
}

namespace {

constexpr size_t kCartSize = 33554432;
constexpr size_t kDecompressedSize = 35098224;
constexpr uint64_t kDecompressedHash = 0x9F843AD3455EA395ULL;
constexpr uint64_t kCartHash = 0x00F70DE5F2D70EA2ULL;

int g_failures = 0;

void expect(bool condition, const char* name) {
    if (!condition) {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++g_failures;
    }
}

std::filesystem::path find_rom() {
    namespace fs = std::filesystem;
    if (const char* env = std::getenv("TOOIE_ROM"); env != nullptr && *env != '\0') {
        return fs::path(env);
    }
    std::error_code ec;
    fs::path dir = fs::current_path(ec);
    for (int i = 0; i < 6 && !dir.empty(); ++i) {
        fs::path candidate = dir / "Banjo-Tooie (USA).z64";
        if (fs::exists(candidate, ec)) {
            return candidate;
        }
        if (dir == dir.parent_path()) {
            break;
        }
        dir = dir.parent_path();
    }
    return {};
}

// Same transformation as librecomp byteswap_data(rom, 1): .z64 -> 2-byte-swapped (.n64 layout), and back.
void swap2(std::vector<uint8_t>& data) {
    for (size_t i = 0; i + 1 < data.size(); i += 2) {
        std::swap(data[i], data[i + 1]);
    }
}

} // namespace

int main() {
    std::filesystem::path rom_path = find_rom();
    std::ifstream file(rom_path, std::ios::binary);
    if (rom_path.empty() || !file.good()) {
        std::printf("SKIP rom_cart_import_test: no ROM (set TOOIE_ROM or put \"Banjo-Tooie (USA).z64\" in the repo root)\n");
        return 0;
    }
    std::vector<uint8_t> cart((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (cart.size() != kCartSize) {
        std::printf("SKIP rom_cart_import_test: ROM is %zu bytes, not the %zu byte retail cart\n", cart.size(), kCartSize);
        return 0;
    }

    expect(XXH3_64bits(cart.data(), cart.size()) == kCartHash, "cart .z64 XXH3-64 is the expected cart hash");

    // .z64 cart
    std::vector<uint8_t> decompressed = tooie::decompress_tooie(cart);
    expect(decompressed.size() == kDecompressedSize, "cart .z64 -> decompressed size");
    expect(XXH3_64bits(decompressed.data(), decompressed.size()) == kDecompressedHash, "cart .z64 -> decompressed XXH3-64");

    // .n64 layout, normalised back to big-endian the way select_rom does before calling the routine
    std::vector<uint8_t> swapped = cart;
    swap2(swapped);
    expect(XXH3_64bits(swapped.data(), swapped.size()) != kCartHash, "swapped copy differs from .z64");
    swap2(swapped);
    std::vector<uint8_t> from_swapped = tooie::decompress_tooie(swapped);
    expect(from_swapped.size() == kDecompressedSize, "cart .n64 (normalised) -> decompressed size");
    expect(XXH3_64bits(from_swapped.data(), from_swapped.size()) == kDecompressedHash, "cart .n64 (normalised) -> decompressed XXH3-64");

    // already decompressed image: unchanged
    std::vector<uint8_t> again = tooie::decompress_tooie(decompressed);
    expect(again == decompressed, "decompressed image passes through unchanged");

    // not a cart (right size, wrong content): returned unchanged, no crash
    std::vector<uint8_t> junk(kCartSize, 0xA5);
    expect(tooie::decompress_tooie(junk) == junk, "unknown 32 MiB image passes through unchanged");

    // malformed cart that still has the cart hash is impossible; corrupt copy has another hash -> unchanged
    std::vector<uint8_t> corrupt = cart;
    corrupt[0x1E29B60 + 8] ^= 0xFF;
    expect(tooie::decompress_tooie(corrupt) == corrupt, "corrupted cart passes through unchanged");

    // The library function itself must fail cleanly (no crash, no output) on malformed input.
    {
        std::vector<char> in(cart.begin(), cart.end());
        std::vector<char> out;
        std::string error;
        const bool valid_ok = tooie_decompress_rom(in, out, nullptr, &error);
        if (!valid_ok) {
            std::fprintf(stderr, "library error: %s\n", error.c_str());
        }
        expect(valid_ok && out.size() == kDecompressedSize, "library: valid cart decompresses");
        std::vector<char> truncated(in.begin(), in.begin() + 0x1E29B60 + 0x1000);
        expect(!tooie_decompress_rom(truncated, out, nullptr, &error) && out.empty() && !error.empty(), "library: truncated cart fails cleanly");
        std::vector<char> tiny(16, 0);
        expect(!tooie_decompress_rom(tiny, out, nullptr), "library: tiny input fails cleanly");
        std::vector<char> bad_core = in;
        for (size_t i = 0; i < 64; ++i) {
            bad_core[0x1E29B60 + 2 + i] ^= 0x5A;
        }
        expect(!tooie_decompress_rom(bad_core, out, nullptr), "library: corrupted core1 stream fails cleanly");
        std::vector<char> bad_table = in;
        bad_table[0x1E899B0] = (char)0x7F; // overlay table size -> huge
        expect(!tooie_decompress_rom(bad_table, out, nullptr), "library: bogus overlay table size fails cleanly");
        // Corrupt random overlay bytes; any outcome is fine as long as the call returns (ASAN-free smoke test).
        std::vector<char> bad_ovl = in;
        for (size_t i = 0x1E899B0 + 4; i < 0x1E899B0 + 0x400; i += 7) {
            bad_ovl[i] ^= (char)(i * 13);
        }
        (void)tooie_decompress_rom(bad_ovl, out, nullptr);
    }

    if (g_failures != 0) {
        std::fprintf(stderr, "rom_cart_import_test: %d failure(s)\n", g_failures);
        return 1;
    }
    std::printf("PASS rom_cart_import_test (decompressed %zu bytes, XXH3-64 0x%016llX)\n", decompressed.size(),
                (unsigned long long)XXH3_64bits(decompressed.data(), decompressed.size()));
    return 0;
}
