#pragma once

#include "tooie_asset_table.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace tooie_language_pack {
    using namespace tooie_asset_table;

    struct PackEntry {
        uint32_t asset_id;
        std::vector<uint8_t> payload;
    };

    struct LanguagePack {
        std::string language;
        uint32_t base_size;
        std::vector<PackEntry> entries;
        std::vector<uint8_t> font1_charset;
        std::array<uint8_t, 30> font1_accent_map{};
    };

    inline bool parse(std::span<const uint8_t> data, size_t image_size, LanguagePack& out, std::string& error) {
        auto fail = [&](const char* message) {
            error = message;
            return false;
        };

        if (data.size() < 36) {
            return fail("truncated header");
        }
        if (!std::equal(data.begin(), data.begin() + 4, "TLPK")) {
            return fail("invalid magic");
        }
        if (read_u32_le(data.data() + 4) != 1) {
            return fail("unsupported version");
        }

        size_t language_length = 0;
        while (language_length < 16 && data[8 + language_length] != 0) {
            uint8_t character = data[8 + language_length];
            if (!((character >= 'A' && character <= 'Z') ||
                  (character >= 'a' && character <= 'z') ||
                  (character >= '0' && character <= '9') || character == '-')) {
                return fail("invalid language");
            }
            ++language_length;
        }
        if (language_length == 0 || language_length == 16) {
            return fail("invalid language");
        }
        for (size_t index = language_length; index < 16; ++index) {
            if (data[8 + index] != 0) {
                return fail("invalid language padding");
            }
        }

        LanguagePack pack;
        pack.language.assign(reinterpret_cast<const char*>(data.data() + 8), language_length);
        pack.base_size = read_u32_le(data.data() + 24);
        if (pack.base_size != image_size) {
            return fail("base image size mismatch");
        }
        uint32_t entry_count = read_u32_le(data.data() + 28);
        if (entry_count == 0 || entry_count > kAssetCount) {
            return fail("invalid entry count");
        }

        uint32_t charset_length = read_u32_le(data.data() + 32);
        if (charset_length > 255) {
            return fail("invalid charset length");
        }
        size_t position = 36;
        if (charset_length != 0) {
            if (data.size() - position < charset_length) {
                return fail("truncated charset");
            }
            pack.font1_charset.assign(data.begin() + position, data.begin() + position + charset_length);
            position += charset_length;
            if (data.size() - position < pack.font1_accent_map.size()) {
                return fail("truncated accent map");
            }
            std::copy_n(data.begin() + position, pack.font1_accent_map.size(), pack.font1_accent_map.begin());
            position += pack.font1_accent_map.size();
        }
        for (uint32_t index = 0; index < entry_count; ++index) {
            if (data.size() - position < 8) {
                return fail("truncated entry");
            }
            uint32_t asset_id = read_u32_le(data.data() + position);
            uint32_t payload_size = read_u32_le(data.data() + position + 4);
            position += 8;
            if (asset_id >= kAssetCount || (index != 0 && asset_id <= pack.entries.back().asset_id)) {
                return fail("invalid asset order or id");
            }
            if (payload_size < 8 || payload_size % 8 != 0 || payload_size > data.size() - position) {
                return fail("invalid payload size");
            }
            pack.entries.push_back({asset_id, std::vector<uint8_t>(data.begin() + position, data.begin() + position + payload_size)});
            position += payload_size;
        }
        if (position != data.size()) {
            return fail("trailing pack bytes");
        }
        out = std::move(pack);
        return true;
    }

    inline bool serialize(const LanguagePack& pack, std::vector<uint8_t>& out, std::string& error) {
        auto fail = [&](const char* message) {
            error = message;
            return false;
        };
        if (pack.language.empty() || pack.language.size() > 15) {
            return fail("invalid language");
        }
        for (unsigned char character : pack.language) {
            if (!((character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z') ||
                  (character >= '0' && character <= '9') || character == '-')) {
                return fail("invalid language");
            }
        }
        if (pack.entries.empty() || pack.entries.size() > kAssetCount) {
            return fail("invalid entry count");
        }
        if (pack.font1_charset.size() > 255) {
            return fail("invalid charset length");
        }
        std::vector<uint8_t> bytes{'T', 'L', 'P', 'K'};
        auto write32 = [&](uint32_t value) {
            for (int byte = 0; byte < 4; ++byte) {
                bytes.push_back(uint8_t(value >> (byte * 8)));
            }
        };
        write32(1);
        bytes.insert(bytes.end(), pack.language.begin(), pack.language.end());
        bytes.resize(24, 0);
        write32(pack.base_size);
        write32(static_cast<uint32_t>(pack.entries.size()));
        write32(static_cast<uint32_t>(pack.font1_charset.size()));
        if (!pack.font1_charset.empty()) {
            bytes.insert(bytes.end(), pack.font1_charset.begin(), pack.font1_charset.end());
            bytes.insert(bytes.end(), pack.font1_accent_map.begin(), pack.font1_accent_map.end());
        }
        for (size_t index = 0; index < pack.entries.size(); ++index) {
            const auto& entry = pack.entries[index];
            if (entry.asset_id >= kAssetCount || (index != 0 && entry.asset_id <= pack.entries[index - 1].asset_id)) {
                return fail("invalid asset order or id");
            }
            if (entry.payload.size() < 8 || entry.payload.size() % 8 != 0 || entry.payload.size() > UINT32_MAX) {
                return fail("invalid payload size");
            }
            write32(entry.asset_id);
            write32(static_cast<uint32_t>(entry.payload.size()));
            bytes.insert(bytes.end(), entry.payload.begin(), entry.payload.end());
        }
        out = std::move(bytes);
        return true;
    }

    inline bool rewrite(std::span<const uint8_t> image, const LanguagePack& pack, std::vector<uint8_t>& out, std::string& error) {
        auto fail = [&](const char* message) {
            error = message;
            return false;
        };

        if (pack.base_size != image.size() || image.size() < kUsaLayout.table + (kAssetCount + 2) * 4) {
            return fail("invalid image size");
        }
        if (pack.entries.empty()) {
            return fail("empty pack");
        }
        for (size_t index = 0; index < pack.entries.size(); ++index) {
            const PackEntry& entry = pack.entries[index];
            if (entry.asset_id >= kAssetCount ||
                (index != 0 && entry.asset_id <= pack.entries[index - 1].asset_id) ||
                entry.payload.size() < 8 || entry.payload.size() % 8 != 0) {
                return fail("invalid pack entry");
            }
        }

        std::vector<size_t> offsets;
        offsets.reserve(kAssetCount + 1);
        for (size_t index = 0; index <= kAssetCount; ++index) {
            uint32_t entry = read_u32_be(image.data() + kUsaLayout.table + index * 4);
            uint32_t next_entry = read_u32_be(image.data() + kUsaLayout.table + (index + 1) * 4);
            // The entry after the sentinel must decrease; earlier entries must not.
            if ((next_entry < entry) != (index == kAssetCount)) {
                return fail("invalid asset sentinel");
            }
            size_t offset = kUsaLayout.base + size_t(entry >> 8) * 4;
            if (offset > image.size() || (index != 0 && offset < offsets.back())) {
                return fail("invalid asset offset");
            }
            offsets.push_back(offset);
        }

        std::vector<uint8_t> result(image.begin(), image.end());
        // Align relocated payloads to eight bytes with the image's 0xAA padding.
        while (result.size() % 8 != 0) {
            result.push_back(0xAA);
        }
        auto write_offset = [&](size_t asset_id) {
            size_t offset = result.size();
            if (offset < kUsaLayout.base || (offset - kUsaLayout.base) % 4 != 0 ||
                (offset - kUsaLayout.base) / 4 >= (size_t(1) << 24)) {
                return false;
            }
            // Encode (offset - base) / 4 << 8 | flags.
            uint32_t entry = uint32_t((offset - kUsaLayout.base) / 4) << 8 |
                image[kUsaLayout.table + asset_id * 4 + 3];
            for (int byte = 0; byte < 4; ++byte) {
                result[kUsaLayout.table + asset_id * 4 + byte] = uint8_t(entry >> (24 - byte * 8));
            }
            return true;
        };

        // Relocate the tail so every asset retains a contiguous range ending at the next offset.
        size_t pack_index = 0;
        for (size_t asset_id = pack.entries.front().asset_id; asset_id < kAssetCount; ++asset_id) {
            if (!write_offset(asset_id)) {
                return fail("new asset offset out of range or unaligned");
            }
            if (pack_index < pack.entries.size() && pack.entries[pack_index].asset_id == asset_id) {
                const auto& payload = pack.entries[pack_index++].payload;
                result.insert(result.end(), payload.begin(), payload.end());
            } else {
                result.insert(result.end(), image.begin() + offsets[asset_id], image.begin() + offsets[asset_id + 1]);
            }
        }
        if (!write_offset(kAssetCount)) {
            return fail("new sentinel offset out of range or unaligned");
        }
        out = std::move(result);
        return true;
    }
}
