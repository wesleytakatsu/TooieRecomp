#include "tooie_text_font_map.h"

#include <array>
#include <cstdio>
#include <cstdlib>

#define CHECK(condition) do { \
    if (!(condition)) { \
        std::fprintf(stderr, "FAIL %d: %s\n", __LINE__, #condition); \
        std::exit(1); \
    } \
} while (0)

int main() {
    using namespace tooie_text_font_map;
    const std::array<uint8_t, 6> charset = {0x41, 0x79, 0xC4, 0x41, 0x80, 0x7A};
    CharsetTable table = build_table(charset);
    AccentMap accents{};
    for (size_t index = 0; index < accents.size(); ++index) {
        accents[index] = static_cast<uint8_t>(index);
    }
    CHECK(table[0x41] == 0);
    CHECK(table[0x79] == 1);
    CHECK(table[0xC4] == 2);
    CHECK(table[0x42] == -1);
    auto usa_table = [](int32_t character) -> int8_t {
        if (character == 0x41) {
            return 7;
        }
        if (character == -1) {
            return 9;
        }
        return -1;
    };
    const std::array<int32_t, 22> characters = {
        -1, 0, 0x20, 0x21, 0x2C, 0x30, 0x39, 0x3A, 0x3B, 0x5A, 0x5B,
        0x61, 0x78, 0x79, 0x7A, 0x7B, 0x80, 0xAD, 0xAE, 0xAF, 0xC4, 0xFC
    };
    const std::array<int32_t, 22> expected_usa_font1 = {
        9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
    };
    const std::array<int32_t, 22> expected_european_font1 = {
        -1, -1, -1, -1, 0x49, -1, -1, -1, -1, -1, 0x33,
        0x39, 0x50, 1, 5, -1, 4, -1, -1, -1, 2, -1
    };
    for (size_t index = 0; index < characters.size(); ++index) {
        int32_t character = characters[index];
        CHECK(map_usa(1, character, usa_table) == expected_usa_font1[index]);
        CHECK(map_european(1, character, table, accents) == expected_european_font1[index]);
        CHECK(map_usa(0, character, usa_table) ==
            (character >= 0x21 && character < 0x7B ? character - 0x21 : -1));
        CHECK(map_european(0, character, table, accents) ==
            (character >= 0x21 && character < 0x7B ? character - 0x21 : -1));
        CHECK(map_usa(2, character, usa_table) ==
            (character > 0 && character < 0xFD ? character : -1));
        CHECK(map_european(2, character, table, accents) ==
            (character == 0xAD ? -1 : character >= 0xAF ? character - 0x1A : character));
        CHECK(map_usa(3, character, usa_table) ==
            (character >= 0x30 && character < 0x3B ? character - 0x30 : 0x0B));
        CHECK(map_european(3, character, table, accents) ==
            (character >= 0x30 && character < 0x3B ? character - 0x30 : 0x0B));
        CHECK(map_usa(4, character, usa_table) == -1);
        CHECK(map_european(4, character, table, accents) == -1);
    }
    CHECK(map_usa(1, 0x61, usa_table) == 7);
    CHECK(map_usa(1, -1, usa_table) == 9);
    CHECK(map_usa(1, 0x80, usa_table) == -1);
    CHECK(map_usa(3, 0x61, usa_table) == 0x0B);
    CHECK(map_european(1, 0x2C, table, accents) == 0x49);
    CHECK(map_european(1, 0x5A, table, accents) == -1);
    CHECK(map_european(1, 0x5B, table, accents) == 0x33);
    CHECK(map_european(1, 0x61, table, accents) == 0x39);
    CHECK(map_european(1, 0x78, table, accents) == 0x50);
    CHECK(map_european(1, 0x79, table, accents) == 1);
    CHECK(map_european(1, 0x7A, table, accents) == 5);
    CHECK(map_european(1, 0x80, table, accents) == 4);
    CHECK(map_european(1, 0xC4, table, accents) == 2);
    CHECK(map_european(1, -1, table, accents) == -1);
    CHECK(map_european(1, 0x100, table, accents) == -1);
    CHECK(map_usa(1, 0xFD, usa_table) == -1);
    CHECK(map_european(1, 0xFD, table, accents) == -1);
    CHECK(map_usa(2, 0xFD, usa_table) == -1);
    CHECK(map_european(2, 0xFD, table, accents) == 0xE3);
    std::puts("tooie_text_font_map_test PASS");
    return 0;
}
