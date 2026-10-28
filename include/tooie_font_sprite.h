#pragma once

#include "tooie_asset_table.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace tooie_font_sprite {
    struct Frame {
        uint8_t padded_width = 0;
        uint8_t height = 0;
        uint8_t width = 0;
        std::vector<uint8_t> pixels;
    };
    struct Sprite {
        uint16_t format = 0x100;
        std::vector<Frame> frames;
    };
    constexpr size_t kGlyphHeight = 23;
    constexpr size_t kTopPadding = 8;
    constexpr size_t kPaddedHeight = kGlyphHeight + kTopPadding;
    constexpr size_t kFont1FrameCount = 51;
    constexpr size_t kSpriteHeaderSize = 12;
    constexpr size_t kSpriteRecordSize = 8;
    constexpr uint16_t kIa8Format = 0x100;

    inline void put16(std::vector<uint8_t>& out, size_t at, uint16_t value) {
        out[at] = uint8_t(value >> 8);
        out[at + 1] = uint8_t(value);
    }
    inline void put32(std::vector<uint8_t>& out, size_t at, uint32_t value) {
        out[at] = uint8_t(value >> 24);
        out[at + 1] = uint8_t(value >> 16);
        out[at + 2] = uint8_t(value >> 8);
        out[at + 3] = uint8_t(value);
    }
    inline bool parse(std::span<const uint8_t> data, Sprite& out, std::string& error) {
        if (data.size() < kSpriteHeaderSize) {
            error = "truncated sprite header";
            return false;
        }
        size_t count = tooie_asset_table::read_u16_be(data.data());
        // Header is followed by fixed-size frame records and aligned pixel data.
        size_t base = tooie_asset_table::read_u32_be(data.data() + 8);
        if (tooie_asset_table::read_u16_be(data.data() + 2) != kIa8Format ||
            base != (kSpriteHeaderSize + count * kSpriteRecordSize + 7) / 8 * 8 || base > data.size()) {
            error = "invalid IA8 sprite header";
            return false;
        }
        Sprite sprite;
        for (size_t i = 0; i < count; ++i) {
            const uint8_t* record = data.data() + kSpriteHeaderSize + i * kSpriteRecordSize;
            Frame frame{record[0], record[1], record[2], {}};
            size_t offset = tooie_asset_table::read_u32_be(record + 4);
            size_t size = size_t(frame.padded_width) * frame.height;
            if (frame.padded_width == 0 || frame.padded_width % 8 || frame.width > frame.padded_width ||
                record[3] != frame.height || offset > data.size() - base || size > data.size() - base - offset) {
                error = "invalid sprite frame";
                return false;
            }
            frame.pixels.assign(data.begin() + base + offset, data.begin() + base + offset + size);
            sprite.frames.push_back(std::move(frame));
        }
        out = std::move(sprite);
        return true;
    }
    inline bool serialize(const Sprite& sprite, std::vector<uint8_t>& out, std::string& error) {
        if (sprite.format != kIa8Format || sprite.frames.size() > 65535) {
            error = "invalid sprite";
            return false;
        }
        size_t base = (kSpriteHeaderSize + sprite.frames.size() * kSpriteRecordSize + 7) / 8 * 8;
        std::vector<uint8_t> result(base, 0);
        put16(result, 0, uint16_t(sprite.frames.size()));
        put16(result, 2, sprite.format);
        put32(result, 8, uint32_t(base));
        uint16_t max_width = 0, max_height = 0;
        for (size_t i = 0; i < sprite.frames.size(); ++i) {
            const auto& frame = sprite.frames[i];
            if (!frame.padded_width || frame.padded_width % 8 || frame.width > frame.padded_width ||
                frame.pixels.size() != size_t(frame.padded_width) * frame.height) {
                error = "invalid sprite frame";
                return false;
            }
            size_t at = kSpriteHeaderSize + i * kSpriteRecordSize;
            result[at] = frame.padded_width;
            result[at + 1] = frame.height;
            result[at + 2] = frame.width;
            result[at + 3] = frame.height;
            put32(result, at + 4, uint32_t(result.size() - base));
            result.insert(result.end(), frame.pixels.begin(), frame.pixels.end());
            max_width = std::max<uint16_t>(max_width, frame.padded_width);
            max_height = std::max<uint16_t>(max_height, frame.height);
        }
        put16(result, 4, max_width);
        put16(result, 6, max_height);
        out = std::move(result);
        return true;
    }

    inline void paint(Frame& frame, std::span<const char* const> mask, bool under, uint8_t fill, uint8_t outline) {
        int min_x = frame.padded_width, max_x = -1, min_y = int(kPaddedHeight), max_y = -1;
        for (int y = 0; y < int(kPaddedHeight); ++y) {
            for (int x = 0; x < frame.padded_width; ++x) {
                if (frame.pixels[size_t(y) * frame.padded_width + x] & 15) {
                    min_x = std::min(min_x, x);
                    max_x = std::max(max_x, x);
                    min_y = std::min(min_y, y);
                    max_y = std::max(max_y, y);
                }
            }
        }
        if (max_x < 0) {
            min_x = 0;
            max_x = frame.width - 1;
            min_y = 8;
            max_y = 27;
        }
        int width = int(std::char_traits<char>::length(mask[0]));
        int left = (min_x + max_x + 1 - width) / 2;
        int top = under ? max_y - 1 : min_y - 1 - int(mask.size());
        std::vector<std::pair<int, int>> points;
        for (size_t y = 0; y < mask.size(); ++y) {
            for (int x = 0; x < width; ++x) {
                if (mask[y][x] == 'X') {
                    points.emplace_back(left + x, top + int(y));
                }
            }
        }
        for (auto [x, y] : points) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    int xx = x + dx, yy = y + dy;
                    if (xx >= 0 && xx < frame.padded_width && yy >= 0 && yy < int(kPaddedHeight)) {
                        auto& pixel = frame.pixels[size_t(yy) * frame.padded_width + xx];
                        if (!(pixel & 15)) {
                            pixel = outline;
                        }
                    }
                }
            }
        }
        for (auto [x, y] : points) {
            if (x >= 0 && x < frame.padded_width && y >= 0 && y < int(kPaddedHeight)) {
                frame.pixels[size_t(y) * frame.padded_width + x] = fill;
            }
        }
    }

    inline bool generate_font1(const Sprite& source, Sprite& out, std::string& error) {
        if (source.format != kIa8Format || source.frames.size() != kFont1FrameCount) {
            error = "font 1 must contain 51 IA8 frames";
            return false;
        }
        std::array<size_t, 256> frequencies{};
        for (const auto& frame : source.frames) {
            if (frame.height != kGlyphHeight || frame.pixels.size() != size_t(frame.padded_width) * kGlyphHeight ||
                frame.padded_width == 0 || frame.padded_width % 8 || frame.width > frame.padded_width) {
                error = "invalid font 1 frame";
                return false;
            }
            for (uint8_t pixel : frame.pixels) {
                if ((pixel & 15) == 15) {
                    ++frequencies[pixel];
                }
            }
        }
        int lowest = 16, highest = -1;
        for (int value = 0; value < 256; ++value) {
            if (frequencies[value]) {
                lowest = std::min(lowest, value >> 4);
                highest = std::max(highest, value >> 4);
            }
        }
        if (highest < 0) {
            error = "font 1 has no opaque pixels";
            return false;
        }
        auto color = [](int intensity) {
            int best = intensity * 16 + 15;
            return uint8_t(best);
        };
        uint8_t fill = color(highest), outline = color(lowest);
        Sprite result;
        for (const auto& frame : source.frames) {
            Frame padded = frame;
            // Eight transparent rows leave room for accents above each glyph.
            padded.height = kPaddedHeight;
            padded.pixels.assign(size_t(frame.padded_width) * kTopPadding, 0);
            padded.pixels.insert(padded.pixels.end(), frame.pixels.begin(), frame.pixels.end());
            result.frames.push_back(std::move(padded));
        }
        constexpr const char* chars = "0123456789:ABCDEFGHIJKLMNOPQRSTUVWXYZ@%?()<>\".;-!/'";
        auto index_of = [&](char character) {
            return size_t(std::find(chars, chars + kFont1FrameCount, character) - chars);
        };
        constexpr const char* tilde[] = {".XX..X.", "X..XX.."};
        constexpr const char* acute[] = {"...XX..", "..XX...", ".XX...."};
        constexpr const char* grave[] = {".XX....", "..XX...", "...XX.."};
        constexpr const char* circumflex[] = {"...X...", "..X.X..", ".X...X."};
        constexpr const char* diaeresis[] = {".XX.XX.", ".XX.XX."};
        constexpr const char* cedilla[] = {"...X...", "....X..", "..XX..."};
        constexpr const char* ordinal[] = {"..XXX..", ".X...X.", ".XXXXX.", ".X...X.", ".......", ".XXXXX."};
        constexpr const char* degree[] = {"..XXX..", ".X...X.", ".X...X.", "..XXX.."};
        auto add = [&](char c, std::span<const char* const> mask, bool under = false) {
            Frame frame = result.frames[index_of(c)];
            paint(frame, mask, under, fill, outline);
            result.frames.push_back(std::move(frame));
        };
        add('A', tilde);
        add('A', acute);
        add('A', grave);
        add('E', acute);
        add('E', grave);
        add('I', acute);
        add('I', grave);
        add('C', cedilla, true);
        add('O', tilde);
        add('O', acute);
        add('O', grave);
        add('U', diaeresis);
        add('U', acute);
        add('U', grave);
        for (char c : {'?', '!'}) {
            Frame frame = result.frames[index_of(c)];
            // Rotate punctuation within the original glyph area, keeping the added rows on top.
            std::reverse(frame.pixels.begin() + size_t(frame.padded_width) * kTopPadding, frame.pixels.end());
            result.frames.push_back(std::move(frame));
        }
        for (auto mask : {std::span<const char* const>(ordinal), std::span<const char* const>(degree)}) {
            Frame frame = result.frames[index_of('A')];
            std::fill(frame.pixels.begin(), frame.pixels.end(), 0);
            paint(frame, mask, false, fill, outline);
            result.frames.push_back(std::move(frame));
        }
        add('E', circumflex);
        add('I', diaeresis);
        add('O', circumflex);
        add('A', circumflex);
        Frame comma = result.frames[index_of('\'')];
        int bottom = -1;
        for (int y = 0; y < int(kPaddedHeight); ++y) {
            for (int x = 0; x < comma.padded_width; ++x) {
                if (comma.pixels[size_t(y) * comma.padded_width + x] & 15) {
                    bottom = y;
                }
            }
        }
        if (bottom >= 0 && bottom < int(kPaddedHeight) - 1) {
            size_t shift = size_t(int(kPaddedHeight) - 1 - bottom) * comma.padded_width;
            std::move_backward(comma.pixels.begin(), comma.pixels.end() - shift, comma.pixels.end());
            std::fill(comma.pixels.begin(), comma.pixels.begin() + shift, 0);
        }
        result.frames.push_back(std::move(comma));
        out = std::move(result);
        return true;
    }
} // namespace tooie_font_sprite
