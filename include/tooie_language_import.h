#pragma once

#include "tooie_language_pack.h"
#include "tooie_text_assets.h"

namespace tooie_language_import {
    constexpr size_t kEuropeCore2DataOffset = 0x1E8AD81;
    constexpr size_t kCharsetOffset = 0x3928;
    constexpr size_t kAccentMapOffset = 0x3974;

    inline bool build_unchecked(std::span<const uint8_t> europe, std::span<const uint8_t> usa,
                                const std::string& language, tooie_language_pack::LanguagePack& out,
                                std::string& error) {
        using namespace tooie_text_assets;
        int language_index = language == "fr" ? 1 : language == "de" ? 2 : language == "es" ? 3 : 0;
        if (language_index == 0 || europe.size() <= kEuropeCore2DataOffset) {
            error = "invalid language or European ROM";
            return false;
        }
        std::vector<uint8_t> core;
        if (!decode_payload(europe.subspan(kEuropeCore2DataOffset), core, error) || core.size() < kAccentMapOffset + 30) {
            error = "invalid European font map";
            return false;
        }
        auto end = std::find(core.begin() + kCharsetOffset, core.end(), 0);
        constexpr char prefix[] = "0123456789:ABCDEFGHIJKLMNOPQRSTUVWXYZ@%?()<>\".;-!/";
        if (end - (core.begin() + kCharsetOffset) != 73 ||
            !std::equal(prefix, prefix + sizeof(prefix) - 1, core.begin() + kCharsetOffset)) {
            error = "invalid European font charset";
            return false;
        }
        tooie_language_pack::LanguagePack pack;
        pack.language = language;
        pack.base_size = static_cast<uint32_t>(usa.size());
        pack.font1_charset.assign(core.begin() + kCharsetOffset, end);
        std::copy_n(core.begin() + kAccentMapOffset, 30, pack.font1_accent_map.begin());
        std::span<const uint8_t> payload;
        std::vector<uint8_t> raw;
        if (!asset_payload(europe, kEuropeLayout, kFont1, payload, error) ||
            !decode_payload(payload, raw, error)) {
            return false;
        }
        std::vector<uint8_t> encoded;
        if (!encode_payload(raw, encoded, error)) {
            return false;
        }
        pack.entries.push_back({kFont1, std::move(encoded)});
        for (size_t id = kTextFirst; id <= kTextLast; ++id) {
            if (kUsaLayout.table + (id + 1) * 4 > usa.size()) {
                error = "truncated USA asset table";
                return false;
            }
            if ((read_u32_be(usa.data() + kUsaLayout.table + id * 4) & 255) != 0x13) {
                continue;
            }
            if (!asset_payload(usa, kUsaLayout, id, payload, error) || !decode_payload(payload, raw, error)) {
                return false;
            }
            if (raw.size() < 3 || raw[0] != 1 || raw[1] != 3 || raw[2] != 0) {
                continue;
            }
            Section original;
            if (!parse_section(std::span<const uint8_t>(raw).subspan(3), original, error) ||
                !asset_payload(europe, kEuropeLayout, id, payload, error)) {
                return false;
            }
            if (payload.empty()) {
                continue;
            }
            if (!decode_payload(payload, raw, error) || raw.size() < 9 || raw[0] != 4) {
                error = "invalid European text asset";
                return false;
            }
            size_t offsets[5];
            for (size_t i = 0; i < 4; ++i) {
                offsets[i] = raw[1 + i * 2] | size_t(raw[2 + i * 2]) << 8;
            }
            offsets[4] = raw.size();
            if (offsets[0] != 9 || !std::is_sorted(offsets, offsets + 5)) {
                error = "invalid European section offsets";
                return false;
            }
            Section sections[4];
            for (size_t i = 0; i < 4; ++i) {
                if (!parse_section(std::span<const uint8_t>(raw).subspan(offsets[i], offsets[i + 1] - offsets[i]), sections[i], error)) {
                    return false;
                }
            }
            Section selected;
            if (choose_text(original, sections[0], sections[language_index], selected) == 0) {
                continue;
            }
            if (!serialize_section(selected, raw, error)) {
                return false;
            }
            raw.insert(raw.begin(), {1, 3, 0});
            if (!encode_payload(raw, encoded, error)) {
                return false;
            }
            pack.entries.push_back({static_cast<uint32_t>(id), std::move(encoded)});
        }
        out = std::move(pack);
        return true;
    }

    inline bool build(std::span<const uint8_t> europe, std::span<const uint8_t> usa,
                      const std::string& language, tooie_language_pack::LanguagePack& out, std::string& error) {
        return tooie_text_assets::validate_image(europe, true, error) &&
            tooie_text_assets::validate_image(usa, false, error) &&
            build_unchecked(europe, usa, language, out, error);
    }
}
