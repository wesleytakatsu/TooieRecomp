#pragma once

#include <cstddef>
#include <cstdint>

namespace tooie_asset_table {
    constexpr size_t kAssetCount = 13926;
    struct Layout { size_t table; size_t base; };
    constexpr Layout kUsaLayout{0x5188, 0x12B24};
    constexpr Layout kEuropeLayout{0x5148, 0x12AE4};
    constexpr size_t kTextFirst = 0x0CDA;
    constexpr size_t kTextLast = 0x1C6B;
    constexpr size_t kFont0 = 0x0C21;
    constexpr size_t kFont1 = 0x0C22;

    inline uint32_t read_u32_be(const uint8_t* data) {
        return uint32_t(data[0]) << 24 | uint32_t(data[1]) << 16 |
            uint32_t(data[2]) << 8 | data[3];
    }
    inline uint32_t read_u32_le(const uint8_t* data) {
        return uint32_t(data[0]) | uint32_t(data[1]) << 8 |
            uint32_t(data[2]) << 16 | uint32_t(data[3]) << 24;
    }
    inline uint16_t read_u16_be(const uint8_t* data) {
        return uint16_t(uint16_t(data[0]) << 8 | data[1]);
    }
}
