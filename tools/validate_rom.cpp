// Standalone ROM gate for P05 — no GUI.
// Usage: validate_rom <path-to-rom.z64>
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#define XXH_INLINE_ALL
#include "xxh3.h"

static constexpr uint64_t kDecompXxh3 = 0x9F843AD3455EA395ULL;
static constexpr uint64_t kCartXxh3 = 0x00F70DE5F2D70EA2ULL;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "Usage: %s <rom.z64>\n", argv[0]);
        return 2;
    }
    std::ifstream in(argv[1], std::ios::binary);
    if (!in) {
        std::fprintf(stderr, "Failed to open: %s\n", argv[1]);
        return 1;
    }
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), {});
    if (data.size() < 0x40) {
        std::fprintf(stderr, "Not a ROM (too small)\n");
        return 1;
    }
    if (!(data[0] == 0x80 && data[1] == 0x37 && data[2] == 0x12 && data[3] == 0x40)) {
        std::fprintf(stderr, "Not big-endian .z64 (magic mismatch)\n");
        return 1;
    }
    std::string name(reinterpret_cast<char*>(data.data() + 0x20), 20);
    while (!name.empty() && (name.back() == ' ' || name.back() == '\0')) name.pop_back();
    uint64_t h = XXH3_64bits(data.data(), data.size());
    std::printf("path=%s size=%zu name='%s' xxh3=0x%016llX\n",
                argv[1], data.size(), name.c_str(), (unsigned long long)h);
    if (h == kDecompXxh3) {
        std::printf("OK: Banjo-Tooie NTSC-U decompressed image (recompiler layout)\n");
        return 0;
    }
    if (h == kCartXxh3) {
        std::printf("OK: Banjo-Tooie NTSC-U cart baserom (needs decompress for recomp layout)\n");
        return 0;
    }
    std::fprintf(stderr, "FAIL: Incorrect ROM (expected Tooie NTSC-U Rev.0 cart or decompressed)\n");
    return 1;
}
