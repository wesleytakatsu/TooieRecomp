#pragma once

#include "tooie_font_sprite.h"
#include "tooie_language_pack.h"
#include "tooie_text_assets.h"

namespace tooie_language_font {
    // Old imported packs contain the PAL IA4 font. Normalize in memory; never rewrite user files.
    inline bool normalize(tooie_language_pack::LanguagePack& pack, std::string& error) {
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
