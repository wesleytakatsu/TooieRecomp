#pragma once

#include "tooie_font_sprite.h"
#include "tooie_language_pack.h"
#include "tooie_text_assets.h"
#include "tooie_text_font_map.h"

namespace tooie_language_font {
    // Old imported packs contain the PAL IA4 font. Normalize in memory; never rewrite user files.
    inline bool normalize(tooie_language_pack::LanguagePack& pack, std::string& error) {
        if (pack.language == "ja") {
            tooie_text_font_map::JapaneseMaps maps;
            if (!tooie_text_font_map::decode_japanese_maps(pack.font1_charset, maps)) {
                error = "invalid Japanese font maps";
                return false;
            }
            for (size_t font = 0; font < 2; ++font) {
                auto entry = std::find_if(pack.entries.begin(), pack.entries.end(), [font](const auto& e) {
                    return e.asset_id == tooie_asset_table::kFont0 + font;
                });
                std::vector<uint8_t> raw;
                tooie_font_sprite::Sprite sprite;
                if (entry == pack.entries.end() || !tooie_text_assets::decode_payload(entry->payload, raw, error) ||
                    !tooie_font_sprite::parse(raw, sprite, error)) {
                    error = "missing or invalid Japanese font";
                    return false;
                }
                if (tooie_asset_table::read_u16_be(raw.data() + 2) == tooie_font_sprite::kIa4Format &&
                    (!tooie_font_sprite::serialize(sprite, raw, error) ||
                     !tooie_text_assets::encode_payload(raw, entry->payload, error))) return false;
                for (int16_t glyph : maps[font]) {
                    if (glyph >= 0 && size_t(glyph) >= sprite.frames.size()) {
                        error = "Japanese glyph index exceeds font frames";
                        return false;
                    }
                }
            }
        }
        for (auto& entry : pack.entries) {
            if (entry.asset_id != tooie_asset_table::kFont1) continue;
            std::vector<uint8_t> raw;
            if (!tooie_text_assets::decode_payload(entry.payload, raw, error)) return false;
            tooie_font_sprite::Sprite sprite;
            if (!tooie_font_sprite::parse(raw, sprite, error)) return false;
            if (tooie_asset_table::read_u16_be(raw.data() + 2) == tooie_font_sprite::kIa4Format) {
                if (!tooie_font_sprite::serialize(sprite, raw, error) ||
                    !tooie_text_assets::encode_payload(raw, entry.payload, error)) return false;
            }
        }
        return true;
    }
}
