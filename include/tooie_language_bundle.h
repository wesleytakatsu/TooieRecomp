#pragma once

#include "tooie_font_sprite.h"
#include "tooie_language_pack.h"
#include "tooie_text_assets.h"

namespace tooie_language_bundle {
    struct Patch {
        uint32_t offset;
        std::vector<uint8_t> data;
    };
    struct Text {
        uint32_t id;
        uint8_t kind;
        std::vector<uint8_t> data;
    };
    struct Bundle {
        std::string language;
        std::vector<uint8_t> charset;
        std::array<uint8_t, 30> accents{};
        uint32_t font_size = 0, font_crc = 0;
        std::vector<Patch> patches;
        std::vector<Text> texts;
    };
    inline bool parse(std::span<const uint8_t> data, Bundle& out, std::string& error) {
        size_t pos = 0;
        auto take = [&](size_t n, std::span<const uint8_t>& value) {
            if (n > data.size() - pos) {
                return false;
            }
            value = data.subspan(pos, n);
            pos += n;
            return true;
        };
        auto number = [&](uint32_t& value) {
            std::span<const uint8_t> bytes;
            if (!take(4, bytes)) {
                return false;
            }
            value = tooie_language_pack::read_u32_le(bytes.data());
            return true;
        };
        auto fail = [&]() {
            error = "invalid or truncated language bundle";
            return false;
        };
        std::span<const uint8_t> bytes;
        uint32_t value;
        if (!take(4, bytes) || !std::equal(bytes.begin(), bytes.end(), "TLPB") || !number(value) || value != 1 ||
            !take(16, bytes)) {
            return fail();
        }
        Bundle bundle;
        auto end = std::find(bytes.begin(), bytes.end(), 0);
        if (end == bytes.begin() || end == bytes.end() ||
            !std::all_of(end, bytes.end(), [](uint8_t c) { return c == 0; })) {
            return fail();
        }
        bundle.language.assign(bytes.begin(), end);
        if (bundle.language != "pt-BR" || !number(value) || value != 73 || !take(value, bytes)) {
            return fail();
        }
        bundle.charset.assign(bytes.begin(), bytes.end());
        if (!take(30, bytes)) {
            return fail();
        }
        std::copy(bytes.begin(), bytes.end(), bundle.accents.begin());
        uint32_t count;
        if (!number(bundle.font_size) || !number(bundle.font_crc) || !number(count) || count > data.size() / 8) {
            return fail();
        }
        size_t previous_end = 0;
        for (uint32_t i = 0; i < count; ++i) {
            uint32_t offset, size;
            if (!number(offset) || !number(size) || size == 0 || offset < previous_end || offset > bundle.font_size ||
                size > bundle.font_size - offset || !take(size, bytes)) {
                return fail();
            }
            bundle.patches.push_back({offset, {bytes.begin(), bytes.end()}});
            previous_end = size_t(offset) + size;
        }
        if (!number(count) || count > tooie_asset_table::kAssetCount) {
            return fail();
        }
        for (uint32_t i = 0; i < count; ++i) {
            uint32_t id, size;
            if (!number(id) || id < tooie_asset_table::kTextFirst || id > tooie_asset_table::kTextLast || (!bundle.texts.empty() && id <= bundle.texts.back().id) ||
                !take(1, bytes)) {
                return fail();
            }
            uint8_t kind = bytes[0];
            if (kind > 1 || !number(size) || !take(size, bytes)) {
                return fail();
            }
            bundle.texts.push_back({id, kind, {bytes.begin(), bytes.end()}});
        }
        if (pos != data.size()) {
            return fail();
        }
        out = std::move(bundle);
        return true;
    }
    inline bool build_unchecked(const Bundle& bundle, std::span<const uint8_t> usa,
                                tooie_language_pack::LanguagePack& out, std::string& error) {
        using namespace tooie_text_assets;
        tooie_language_pack::LanguagePack pack;
        pack.language = bundle.language;
        pack.base_size = static_cast<uint32_t>(usa.size());
        pack.font1_charset = bundle.charset;
        pack.font1_accent_map = bundle.accents;
        std::span<const uint8_t> payload;
        std::vector<uint8_t> raw, encoded;
        if (!asset_payload(usa, kUsaLayout, kFont0, payload, error) || !decode_payload(payload, raw, error)) {
            return false;
        }
        if (raw.size() != bundle.font_size || image_crc32(raw) != bundle.font_crc) {
            error = "font 0 size or CRC32 mismatch";
            return false;
        }
        for (const auto& patch : bundle.patches) {
            if (patch.offset > raw.size() || patch.data.size() > raw.size() - patch.offset) {
                error = "invalid font patch";
                return false;
            }
            std::copy(patch.data.begin(), patch.data.end(), raw.begin() + patch.offset);
        }
        if (!encode_payload(raw, encoded, error)) {
            return false;
        }
        pack.entries.push_back({kFont0, std::move(encoded)});
        if (!asset_payload(usa, kUsaLayout, kFont1, payload, error) || !decode_payload(payload, raw, error)) {
            return false;
        }
        tooie_font_sprite::Sprite font, generated;
        if (!tooie_font_sprite::parse(raw, font, error) || !tooie_font_sprite::generate_font1(font, generated, error) ||
            !tooie_font_sprite::serialize(generated, raw, error) || !encode_payload(raw, encoded, error)) {
            return false;
        }
        pack.entries.push_back({kFont1, std::move(encoded)});
        for (const auto& text : bundle.texts) {
            if (text.kind == 0) {
                Section check;
                if (!parse_section(text.data, check, error)) {
                    return false;
                }
                raw = text.data;
            } else if (text.kind == 1) {
                if (!asset_payload(usa, kUsaLayout, text.id, payload, error) || !decode_payload(payload, raw, error)) {
                    return false;
                }
                if (raw.size() < 3 || raw[0] != 1 || raw[1] != 3 || raw[2] != 0) {
                    error = "invalid USA text header";
                    return false;
                }
                Section section;
                if (!parse_section(std::span<const uint8_t>(raw).subspan(3), section, error)) {
                    return false;
                }
                size_t position = 0;
                for (auto& group : section) {
                    for (auto& record : group) {
                        if (!is_text(record)) {
                            continue;
                        }
                        if (position >= text.data.size()) {
                            error = "truncated text overlay";
                            return false;
                        }
                        uint8_t flag = text.data[position++];
                        if (flag > 1) {
                            error = "invalid text overlay flag";
                            return false;
                        }
                        if (flag) {
                            if (position >= text.data.size()) {
                                error = "truncated text overlay";
                                return false;
                            }
                            size_t length = text.data[position++];
                            if (length > text.data.size() - position) {
                                error = "truncated text overlay";
                                return false;
                            }
                            record.data.assign(text.data.begin() + position, text.data.begin() + position + length);
                            position += length;
                        }
                    }
                }
                if (position != text.data.size() || !serialize_section(section, raw, error)) {
                    error = "invalid text overlay length";
                    return false;
                }
            } else {
                error = "invalid text kind";
                return false;
            }
            raw.insert(raw.begin(), {1, 3, 0});
            if (!encode_payload(raw, encoded, error)) {
                return false;
            }
            pack.entries.push_back({text.id, std::move(encoded)});
        }
        out = std::move(pack);
        return true;
    }
    inline bool build(const Bundle& bundle, std::span<const uint8_t> usa, tooie_language_pack::LanguagePack& out,
                      std::string& error) {
        return tooie_text_assets::validate_image(usa, false, error) && build_unchecked(bundle, usa, out, error);
    }
} // namespace tooie_language_bundle
