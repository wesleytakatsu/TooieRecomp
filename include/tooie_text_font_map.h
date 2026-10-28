#pragma once

#include <array>
#include <cstdint>
#include <span>

namespace tooie_text_font_map {
    using CharsetTable = std::array<int8_t, 256>;
    using AccentMap = std::array<uint8_t, 30>;

    inline CharsetTable build_table(std::span<const uint8_t> charset) {
        CharsetTable table;
        table.fill(-1);
        for (size_t index = 0; index < charset.size(); ++index) {
            if (table[charset[index]] == -1) {
                table[charset[index]] = static_cast<int8_t>(index);
            }
        }
        return table;
    }

    // The USA build folds lowercase to uppercase for fonts other than 0 and 2.
    inline int32_t fold_usa(int32_t font, int32_t character) {
        if (font != 0 && font != 2 && character >= 0x61 && character < 0x7B) {
            return character - 0x20;
        }
        return character;
    }

    template <typename ReadTable>
    int32_t map_usa(int32_t font, int32_t character, ReadTable read_table) {
        character = fold_usa(font, character);
        switch (font) {
            case 0:
                return character >= 0x21 && character < 0x7B ? character - 0x21 : -1;
            case 1:
                if (character < 0x80) {
                    int32_t value = read_table(character);
                    return value < 0 ? -1 : value;
                }
                return -1;
            case 2:
                return character > 0 && character < 0xFD ? character : -1;
            case 3:
                return character >= 0x30 && character < 0x3B ? character - 0x30 : 0x0B;
            default:
                return -1;
        }
    }

    inline int32_t map_european(int32_t font, int32_t character, const CharsetTable& table, const AccentMap& accents) {
        switch (font) {
            case 0:
                return character >= 0x21 && character < 0x7B ? character - 0x21 : -1;
            case 1:
                if (character == 0x2C) {
                    return 0x49;
                }
                if (character >= 0x5B && character <= 0x78) {
                    return 0x33 + accents[character - 0x5B];
                }
                if (character >= 0 && character < 0x100) {
                    return table[character];
                }
                return -1;
            case 2:
                if (character == 0xAD) {
                    return -1;
                }
                return character >= 0xAF ? character - 0x1A : character;
            case 3:
                return character >= 0x30 && character < 0x3B ? character - 0x30 : 0x0B;
            default:
                return -1;
        }
    }

    void set_european_font_map(std::span<const uint8_t> charset, const AccentMap& accents);
    void reset();
}
