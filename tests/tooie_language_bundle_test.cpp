#include "tooie_language_bundle.h"
#include "tooie_language_font.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>

static std::vector<uint8_t> read_file(const char* path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}
static void number(std::vector<uint8_t>& out, uint32_t n) {
    for (int i = 0; i < 4; ++i) {
        out.push_back(uint8_t(n >> (8 * i)));
    }
}
int main(int argc, char** argv) {
    using namespace tooie_language_bundle;
    std::string error;
    if (argc == 4) {
        auto bytes = read_file(argv[1]), usa = read_file(argv[2]), reference_bytes = read_file(argv[3]);
        Bundle bundle;
        tooie_language_pack::LanguagePack pack, reference;
        if (!parse(bytes, bundle, error) || !build(bundle, usa, pack, error) ||
            !tooie_language_pack::parse(reference_bytes, usa.size(), reference, error)) {
            std::cerr << error << '\n';
            return 1;
        }
        if (pack.language != reference.language || pack.base_size != reference.base_size ||
            pack.font1_charset != reference.font1_charset || pack.font1_accent_map != reference.font1_accent_map ||
            pack.entries.size() != reference.entries.size()) {
            std::cerr << "pack header or entry count mismatch\n";
            return 1;
        }
        for (const auto& entry : pack.entries) {
            if (entry.asset_id == 0xC22) {
                std::vector<uint8_t> raw;
                tooie_font_sprite::Sprite font;
                if (!tooie_text_assets::decode_payload(entry.payload, raw, error) ||
                    tooie_asset_table::read_u16_be(raw.data() + 2) != tooie_font_sprite::kIa8Format ||
                    !tooie_font_sprite::parse(raw, font, error) || font.frames.size() != 74) {
                    std::cerr << "generated font must contain 74 IA8 frames\n";
                    return 1;
                }
                continue;
            }
            auto found = std::find_if(reference.entries.begin(), reference.entries.end(),
                                      [&](const auto& e) { return e.asset_id == entry.asset_id; });
            std::vector<uint8_t> a, b;
            if (found == reference.entries.end() || !tooie_text_assets::decode_payload(entry.payload, a, error) ||
                !tooie_text_assets::decode_payload(found->payload, b, error) || a != b) {
                std::cerr << "entry mismatch " << entry.asset_id << ": " << error << '\n';
                return 1;
            }
        }
        std::vector<uint8_t> rewritten;
        if (!tooie_language_pack::rewrite(usa, pack, rewritten, error)) {
            std::cerr << "rewrite failed: " << error << '\n';
            return 1;
        }
        for (const auto& entry : pack.entries) {
            std::span<const uint8_t> payload;
            std::vector<uint8_t> a, b;
            if (!tooie_text_assets::asset_payload(rewritten, tooie_asset_table::kUsaLayout, entry.asset_id, payload, error) ||
                !tooie_text_assets::decode_payload(payload, a, error) ||
                !tooie_text_assets::decode_payload(entry.payload, b, error) || a != b) {
                std::cerr << "rewritten asset mismatch " << entry.asset_id << '\n';
                return 1;
            }
        }
        if (pack.language == "it") {
            // Rareware and Dolby stay original; the Italian title has locale-specific HD identities.
            for (uint32_t id : {0x632u, 0x8ACu}) {
                if (std::any_of(pack.entries.begin(), pack.entries.end(),
                                [id](const auto& entry) { return entry.asset_id == id; })) {
                    std::cerr << "Italian pack must preserve USA branding asset " << id << '\n';
                    return 1;
                }
                std::span<const uint8_t> original, actual;
                if (!tooie_text_assets::asset_payload(usa, tooie_asset_table::kUsaLayout, id, original, error) ||
                    !tooie_text_assets::asset_payload(rewritten, tooie_asset_table::kUsaLayout, id, actual, error) ||
                    original.size() != actual.size() || !std::equal(original.begin(), original.end(), actual.begin())) {
                    std::cerr << "rewritten branding differs from USA for asset " << id << '\n';
                    return 1;
                }
            }
            if (std::none_of(pack.entries.begin(), pack.entries.end(),
                             [](const auto& entry) { return entry.asset_id == 0x8A8u; })) {
                std::cerr << "Italian title model missing\n";
                return 1;
            }
            std::cout << "2 startup branding assets byte-identical to USA; Italian title present\n";
        }
        std::cout << pack.language << ": " << pack.entries.size() << " assets equal; rewritten image " << rewritten.size()
                  << " bytes; 74 IA8 glyphs\n";
        return 0;
    }
    std::vector<uint8_t> bytes{'T', 'L', 'P', 'B'};
    number(bytes, 1);
    bytes.insert(bytes.end(), {'p', 't', '-', 'B', 'R'});
    bytes.resize(24, 0);
    number(bytes, 73);
    bytes.resize(bytes.size() + 73 + 30, 1);
    number(bytes, 4);
    number(bytes, 7);
    number(bytes, 1);
    number(bytes, 1);
    number(bytes, 2);
    bytes.insert(bytes.end(), {9, 10});
    number(bytes, 2);
    number(bytes, 0xCDA);
    bytes.push_back(0);
    number(bytes, 4);
    bytes.insert(bytes.end(), {1, 128, 1, 'x'});
    number(bytes, 0xCDB);
    bytes.push_back(1);
    number(bytes, 3);
    bytes.insert(bytes.end(), {1, 1, 'y'});
    Bundle bundle;
    assert(parse(bytes, bundle, error));
    assert(bundle.patches.size() == 1 && bundle.texts.size() == 2);
    for (const std::string tag : {"fr", "de", "es"}) {
        auto european_bytes = bytes;
        std::fill(european_bytes.begin() + 8, european_bytes.begin() + 24, 0);
        std::copy(tag.begin(), tag.end(), european_bytes.begin() + 8);
        assert(parse(european_bytes, bundle, error) && bundle.language == tag);
    }
    auto unsupported = bytes;
    unsupported[8] = 'x'; unsupported[9] = 'x';
    std::fill(unsupported.begin() + 10, unsupported.begin() + 24, 0);
    assert(!parse(unsupported, bundle, error));
    auto bad = bytes;
    bad.push_back(0);
    assert(!parse(bad, bundle, error));
    assert(parse(bytes, bundle, error));
    tooie_font_sprite::Sprite sprite;
    for (int i = 0; i < 51; ++i) {
        tooie_font_sprite::Frame frame{8, 23, 8, std::vector<uint8_t>(184, 0)};
        frame.pixels[8 * 3 + 3] = 0xff;
        frame.pixels[8 * 5 + 1] = uint8_t(i * 16 + 15);
        sprite.frames.push_back(std::move(frame));
    }
    std::vector<uint8_t> font;
    assert(tooie_font_sprite::serialize(sprite, font, error));
    std::vector<uint8_t> usa(tooie_asset_table::kUsaLayout.base, 0);
    size_t offset = 0;
    auto entry = [&](size_t id, const std::vector<uint8_t>& raw) {
        std::vector<uint8_t> payload;
        assert(tooie_text_assets::encode_payload(raw, payload, error));
        auto write = [&](size_t index, size_t off) {
            uint32_t word = uint32_t(off / 4) << 8;
            for (int i = 0; i < 4; ++i) {
                usa[tooie_asset_table::kUsaLayout.table + index * 4 + i] = uint8_t(word >> (24 - i * 8));
            }
        };
        write(id, offset);
        usa.insert(usa.end(), payload.begin(), payload.end());
        offset += payload.size();
        write(id + 1, offset);
    };
    std::vector<uint8_t> font0{1, 2, 3, 4};
    bundle.font_crc = tooie_text_assets::image_crc32(font0);
    entry(0x5DD, {1, 2, 3, 4});
    entry(0xC21, font0);
    entry(0xC22, font);
    entry(0xCDB, {1, 3, 0, 1, 128, 1, 'a'});
    tooie_language_pack::LanguagePack pack;
    assert(build_unchecked(bundle, usa, pack, error));
    assert(pack.entries.size() == 4);
    std::vector<uint8_t> generated_font_bytes;
    assert(tooie_text_assets::decode_payload(pack.entries[1].payload, generated_font_bytes, error));
    tooie_font_sprite::Sprite generated_font;
    assert(tooie_font_sprite::parse(generated_font_bytes, generated_font, error));
    for (size_t index : {size_t(65), size_t(66)}) {
        const auto& pixels = generated_font.frames[index].pixels;
        assert(std::all_of(pixels.begin(), pixels.begin() + 8 * 8, [](uint8_t pixel) { return pixel == 0; }));
        const auto& original = sprite.frames[index == 65 ? 39 : 48].pixels;
        assert(std::equal(pixels.begin() + 8 * 8, pixels.end(), original.rbegin(), original.rend()));
    }
    std::vector<uint8_t> raw;
    assert(tooie_text_assets::decode_payload(pack.entries[0].payload, raw, error));
    assert((raw == std::vector<uint8_t>{1, 9, 10, 4}));
    assert(tooie_text_assets::decode_payload(pack.entries[3].payload, raw, error));
    assert((raw == std::vector<uint8_t>{1, 3, 0, 1, 128, 1, 'y'}));
    for (const std::string tag : {"fr", "de", "es"}) {
        auto european = bundle;
        european.language = tag;
        european.patches.clear();
        assert(build_unchecked(european, usa, pack, error));
        assert(pack.language == tag && pack.entries.size() == 3 && pack.entries[0].asset_id == 0xC22);
        assert(tooie_language_font::normalize(pack, error));
    }
    // A legacy IA4 font is converted, while an already normalized pack remains byte-identical.
    std::vector<uint8_t> ia4(24, 0);
    tooie_font_sprite::put16(ia4, 0, 1); tooie_font_sprite::put16(ia4, 2, 0x80);
    tooie_font_sprite::put32(ia4, 8, 24);
    ia4[12] = 8; ia4[13] = 1; ia4[14] = 8; ia4[15] = 1;
    ia4.insert(ia4.end(), {0x01, 0x23, 0x45, 0x67});
    assert(tooie_text_assets::encode_payload(ia4, pack.entries[0].payload, error));
    assert(tooie_language_font::normalize(pack, error));
    assert(tooie_text_assets::decode_payload(pack.entries[0].payload, raw, error));
    assert(tooie_asset_table::read_u16_be(raw.data() + 2) == 0x100);
    const auto normalized = pack.entries[0].payload;
    assert(tooie_language_font::normalize(pack, error) && normalized == pack.entries[0].payload);
    pack.entries[0].payload = {1, 2, 3};
    assert(!tooie_language_font::normalize(pack, error));
    ++bundle.font_crc;
    assert(!build_unchecked(bundle, usa, pack, error));

    // Version 2 adds graphic deltas alongside the generated IA8 font and text overlays.
    std::vector<uint8_t> italian{'T','L','P','B'};
    number(italian, 2); italian.insert(italian.end(), {'i','t'}); italian.resize(24, 0);
    number(italian, 73); italian.insert(italian.end(), bytes.begin() + 28, bytes.begin() + 131);
    number(italian, 4); number(italian, tooie_text_assets::image_crc32(font0)); number(italian, 0);
    number(italian, 1);
    const size_t asset_id_offset = italian.size();
    number(italian, 0x5DD); number(italian, 4); number(italian, tooie_text_assets::image_crc32(font0));
    const std::vector<uint8_t> graphic{1,9,3,4};
    number(italian, 4); number(italian, tooie_text_assets::image_crc32(graphic)); number(italian, 1);
    number(italian, 1); number(italian, 1); italian.push_back(9);
    number(italian, 1); number(italian, 0xCDA); italian.push_back(0); number(italian, 4);
    italian.insert(italian.end(), {1,128,1,'i'});
    Bundle it;
    assert(parse(italian, it, error) && it.language == "it" && it.charset.size() == 73 && it.assets.size() == 1);
    assert(build_unchecked(it, usa, pack, error));
    assert(pack.entries.size() == 3 && pack.entries[0].asset_id == 0x5DD && pack.entries[1].asset_id == 0xC22 && pack.entries[2].asset_id == 0xCDA);
    assert(pack.font1_charset.size() == 73);
    assert(tooie_text_assets::decode_payload(pack.entries[0].payload, raw, error) && raw == graphic);
    ++it.assets[0].target_crc;
    assert(!build_unchecked(it, usa, pack, error));
    assert(parse(italian, it, error)); ++it.assets[0].base_crc;
    assert(!build_unchecked(it, usa, pack, error));
    auto invalid_graphic = italian;
    invalid_graphic[asset_id_offset] = 0x22; invalid_graphic[asset_id_offset + 1] = 0x0C;
    assert(!parse(invalid_graphic, it, error));
    italian.pop_back();
    assert(!parse(italian, it, error));
}
