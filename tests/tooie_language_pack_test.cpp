#include "tooie_language_pack.h"

#include <cstdio>
#include <cstdlib>

using namespace tooie_language_pack;

#define CHECK(condition) do { \
    if (!(condition)) { \
        std::fprintf(stderr, "FAIL %d: %s\n", __LINE__, #condition); \
        std::exit(1); \
    } \
} while (0)

static void write_u32_le(std::vector<uint8_t>& bytes, size_t position, uint32_t value) {
    for (int byte = 0; byte < 4; ++byte) {
        bytes[position + byte] = uint8_t(value >> (byte * 8));
    }
}

static void write_u32_be(std::vector<uint8_t>& bytes, size_t position, uint32_t value) {
    for (int byte = 0; byte < 4; ++byte) {
        bytes[position + byte] = uint8_t(value >> (24 - byte * 8));
    }
}

static std::vector<uint8_t> make_image() {
    std::vector<uint8_t> image(kUsaLayout.base + kAssetCount * 8 + 3, 0x42);
    for (size_t asset_id = 0; asset_id <= kAssetCount; ++asset_id) {
        write_u32_be(image, kUsaLayout.table + asset_id * 4,
            uint32_t(asset_id * 2) << 8 | uint32_t(asset_id % 256));
    }
    write_u32_be(image, kUsaLayout.table + (kAssetCount + 1) * 4, 0);
    return image;
}

static std::vector<uint8_t> make_pack(size_t image_size) {
    std::vector<uint8_t> bytes(52, 0);
    bytes[0] = 'T';
    bytes[1] = 'L';
    bytes[2] = 'P';
    bytes[3] = 'K';
    bytes[8] = 'p';
    bytes[9] = 't';
    write_u32_le(bytes, 4, 1);
    write_u32_le(bytes, 24, uint32_t(image_size));
    write_u32_le(bytes, 28, 1);
    write_u32_le(bytes, 36, kAssetCount - 2);
    write_u32_le(bytes, 40, 8);
    for (size_t index = 44; index < 52; ++index) {
        bytes[index] = 0x77;
    }
    return bytes;
}

int main() {
    auto image = make_image();
    auto bytes = make_pack(image.size());
    LanguagePack pack;
    std::string error;
    std::vector<uint8_t> out;
    CHECK(parse(bytes, image.size(), pack, error));
    CHECK(rewrite(image, pack, out, error));

    size_t start = (image.size() + 7) & ~size_t(7);
    CHECK(out.size() == start + 16);
    CHECK(out[image.size()] == 0xAA);
    for (size_t index = 0; index < image.size(); ++index) {
        if (index < kUsaLayout.table + (kAssetCount - 2) * 4 ||
            index >= kUsaLayout.table + (kAssetCount + 1) * 4) {
            CHECK(out[index] == image[index]);
        }
    }
    for (size_t asset_id = kAssetCount - 2; asset_id <= kAssetCount; ++asset_id) {
        uint32_t entry = read_u32_be(out.data() + kUsaLayout.table + asset_id * 4);
        CHECK((entry & 255) == (asset_id % 256));
        CHECK(kUsaLayout.base + (entry >> 8) * 4 == start + (asset_id - (kAssetCount - 2)) * 8);
    }
    for (size_t index = 0; index < 8; ++index) {
        CHECK(out[start + index] == 0x77);
        CHECK(out[start + 8 + index] == image[kUsaLayout.base + (kAssetCount - 1) * 8 + index]);
    }

    auto with_charset = bytes;
    write_u32_le(with_charset, 32, 3);
    with_charset.insert(with_charset.begin() + 36, {0x41, 0x42, 0x43});
    with_charset.insert(with_charset.begin() + 39, 30, 7);
    CHECK(parse(with_charset, image.size(), pack, error));
    CHECK(pack.font1_charset == std::vector<uint8_t>({0x41, 0x42, 0x43}));
    CHECK(pack.font1_accent_map[29] == 7);
    std::vector<uint8_t> serialized;
    CHECK(serialize(pack, serialized, error));
    LanguagePack roundtrip;
    CHECK(parse(serialized, image.size(), roundtrip, error));
    CHECK(roundtrip.language == pack.language && roundtrip.base_size == pack.base_size);
    CHECK(roundtrip.font1_charset == pack.font1_charset && roundtrip.font1_accent_map == pack.font1_accent_map);
    CHECK(roundtrip.entries.size() == pack.entries.size());
    CHECK(roundtrip.entries[0].asset_id == pack.entries[0].asset_id && roundtrip.entries[0].payload == pack.entries[0].payload);
    pack.font1_charset.clear();
    CHECK(serialize(pack, serialized, error));
    CHECK(parse(serialized, image.size(), roundtrip, error));
    CHECK(roundtrip.font1_charset.empty());
    pack.language = "valid-123";
    pack.font1_charset.resize(255, 42);
    CHECK(serialize(pack, serialized, error));
    CHECK(parse(serialized, image.size(), roundtrip, error));
    CHECK(roundtrip.font1_charset == pack.font1_charset);
    pack.font1_charset.push_back(42);
    CHECK(!serialize(pack, serialized, error));
    pack.font1_charset.clear();
    pack.entries.push_back(pack.entries[0]);
    CHECK(!serialize(pack, serialized, error));
    pack.entries.pop_back();
    pack.language = "bad_tag";
    CHECK(!serialize(pack, serialized, error));
    auto invalid_charset = bytes;
    write_u32_le(invalid_charset, 32, 256);
    CHECK(!parse(invalid_charset, image.size(), pack, error));
    auto truncated_map = with_charset;
    truncated_map.resize(39 + 29);
    CHECK(!parse(truncated_map, image.size(), pack, error));
    CHECK(error == "truncated accent map");

    auto original = out;
    auto bad = bytes;
    bad[0] = 'X';
    CHECK(!parse(bad, image.size(), pack, error));
    bad = bytes;
    write_u32_le(bad, 4, 2);
    CHECK(!parse(bad, image.size(), pack, error));
    bad = bytes;
    bad[8] = '_';
    CHECK(!parse(bad, image.size(), pack, error));
    CHECK(error == "invalid language");
    bad = bytes;
    write_u32_le(bad, 24, 0);
    CHECK(!parse(bad, image.size(), pack, error));
    bad = bytes;
    write_u32_le(bad, 40, 7);
    CHECK(!parse(bad, image.size(), pack, error));
    bad = bytes;
    write_u32_le(bad, 36, kAssetCount);
    CHECK(!parse(bad, image.size(), pack, error));
    bad = bytes;
    bad.pop_back();
    CHECK(!parse(bad, image.size(), pack, error));
    bad = bytes;
    write_u32_le(bad, 28, 2);
    bad.insert(bad.end(), bytes.begin() + 36, bytes.end());
    CHECK(!parse(bad, image.size(), pack, error));

    CHECK(parse(bytes, image.size(), pack, error));
    auto broken = image;
    write_u32_be(broken, kUsaLayout.table + 8, 0);
    auto saved = broken;
    CHECK(!rewrite(broken, pack, out, error));
    CHECK(broken == saved);
    CHECK(out == original);
    std::puts("tooie_language_pack_test PASS");
    return 0;
}
