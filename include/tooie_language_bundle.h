#pragma once

#include "tooie_font_sprite.h"
#include "tooie_language_pack.h"
#include "tooie_text_assets.h"

namespace tooie_language_bundle {
    inline bool supported_language(const std::string& tag) {
        return tag == "pt-BR" || tag == "fr" || tag == "de" || tag == "es" || tag == "it";
    }
    struct Patch {
        uint32_t offset;
        std::vector<uint8_t> data;
    };
    struct Text {
        uint32_t id;
        uint8_t kind;
        std::vector<uint8_t> data;
    };
    struct AssetPatch {
        uint32_t id = 0;
        uint32_t base_size = 0, base_crc = 0, target_size = 0, target_crc = 0;
        std::vector<Patch> patches;
    };
    inline bool graphic_asset_id(uint32_t id) {
        return id < tooie_asset_table::kAssetCount &&
            (id < tooie_asset_table::kTextFirst || id > tooie_asset_table::kTextLast) &&
            (id < 0xC20 || id > 0xC23);
    }
    struct Bundle {
        std::string language;
        std::vector<uint8_t> charset;
        std::array<uint8_t, 30> accents{};
        uint32_t font_size = 0, font_crc = 0;
        std::vector<Patch> patches;
        std::vector<Text> texts;
        std::vector<AssetPatch> assets;
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
        uint32_t value, version;
        if (!take(4, bytes) || !std::equal(bytes.begin(), bytes.end(), "TLPB") || !number(version) || (version != 1 && version != 2) ||
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
        if (!supported_language(bundle.language) || !number(value) || value != 73u || !take(value, bytes)) {
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
        if (version == 2) {
            if (!number(count) || count > tooie_asset_table::kAssetCount) return fail();
            for (uint32_t i = 0; i < count; ++i) {
                AssetPatch asset;
                uint32_t runs;
                if (!number(asset.id) || !graphic_asset_id(asset.id) ||
                    (!bundle.assets.empty() && asset.id <= bundle.assets.back().id) ||
                    !number(asset.base_size) || !number(asset.base_crc) || !number(asset.target_size) ||
                    !number(asset.target_crc) || !number(runs) || runs > data.size() / 8 ||
                    asset.base_size == 0 || asset.base_size > 65535u * 16u ||
                    asset.target_size == 0 || asset.target_size > 65535u * 16u) return fail();
                previous_end = 0;
                for (uint32_t j = 0; j < runs; ++j) {
                    uint32_t offset, size;
                    if (!number(offset) || !number(size) || size == 0 || offset < previous_end ||
                        offset > asset.target_size || size > asset.target_size - offset || !take(size, bytes)) return fail();
                    asset.patches.push_back({offset, {bytes.begin(), bytes.end()}});
                    previous_end = size_t(offset) + size;
                }
                bundle.assets.push_back(std::move(asset));
            }
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
        if (!supported_language(bundle.language)) {
            error = "unsupported bundle language";
            return false;
        }
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
        if (!bundle.patches.empty()) {
            if (!encode_payload(raw, encoded, error)) {
                return false;
            }
            pack.entries.push_back({kFont0, std::move(encoded)});
        }
        if (!asset_payload(usa, kUsaLayout, kFont1, payload, error) || !decode_payload(payload, raw, error)) return false;
        tooie_font_sprite::Sprite font, generated;
        const auto profile = bundle.language == "pt-BR" ? tooie_font_sprite::Font1Profile::PortugueseBrazil :
                                                         tooie_font_sprite::Font1Profile::European;
        if (!tooie_font_sprite::parse(raw, font, error) || !tooie_font_sprite::generate_font1(font, generated, error, profile) ||
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
        for (const auto& asset : bundle.assets) {
            if (!graphic_asset_id(asset.id) || asset.target_size == 0 || asset.target_size > 65535u * 16u ||
                !asset_payload(usa, kUsaLayout, asset.id, payload, error) || !decode_payload(payload, raw, error)) {
                error = "invalid graphic asset patch";
                return false;
            }
            if (raw.size() != asset.base_size || image_crc32(raw) != asset.base_crc) {
                error = "graphic asset base size or CRC32 mismatch";
                return false;
            }
            raw.resize(asset.target_size, 0);
            for (const auto& patch : asset.patches) {
                if (patch.offset > raw.size() || patch.data.size() > raw.size() - patch.offset) {
                    error = "invalid graphic patch bounds";
                    return false;
                }
                std::copy(patch.data.begin(), patch.data.end(), raw.begin() + patch.offset);
            }
            if (image_crc32(raw) != asset.target_crc || !encode_payload(raw, encoded, error)) {
                error = "graphic asset target CRC32 mismatch or encoding failure";
                return false;
            }
            pack.entries.push_back({asset.id, std::move(encoded)});
        }
        if (!bundle.assets.empty()) {
            std::sort(pack.entries.begin(), pack.entries.end(), [](const auto& a, const auto& b) { return a.asset_id < b.asset_id; });
        }
        out = std::move(pack);
        return true;
    }
    inline bool build(const Bundle& bundle, std::span<const uint8_t> usa, tooie_language_pack::LanguagePack& out,
                      std::string& error) {
        return tooie_text_assets::validate_image(usa, false, error) && build_unchecked(bundle, usa, out, error);
    }
} // namespace tooie_language_bundle
