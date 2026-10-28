#include "tooie_language_import.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <fstream>
#include <iterator>
#include <iostream>

static std::vector<uint8_t> read_file(const char* path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

int main(int argc, char** argv) {
    using namespace tooie_text_assets;
    std::string error;
    if (argc == 5) {
        auto europe = read_file(argv[1]);
        auto usa = read_file(argv[2]);
        auto reference_bytes = read_file(argv[3]);
        tooie_language_pack::LanguagePack actual, reference;
        if (!tooie_language_import::build(europe, usa, argv[4], actual, error) ||
            !tooie_language_pack::parse(reference_bytes, usa.size(), reference, error)) {
            std::cerr << error << '\n';
            return 1;
        }
        if (actual.language != reference.language || actual.base_size != reference.base_size ||
            actual.font1_charset != reference.font1_charset || actual.font1_accent_map != reference.font1_accent_map ||
            actual.entries.size() != reference.entries.size()) {
            std::cerr << "pack header or entry count mismatch\n";
            return 1;
        }
        for (size_t i = 0; i < actual.entries.size(); ++i) {
            std::vector<uint8_t> a, b;
            if (actual.entries[i].asset_id != reference.entries[i].asset_id ||
                !decode_payload(actual.entries[i].payload, a, error) ||
                !decode_payload(reference.entries[i].payload, b, error) || a != b) {
                std::cerr << "entry mismatch " << actual.entries[i].asset_id << ": " << error << '\n';
                return 1;
            }
        }
        std::cout << argv[4] << ": " << actual.entries.size() << " entries equal\n";
        return 0;
    }
    Section original = {{{3, 7, {'h', 'i'}}, {2, 0, {1}}}};
    std::vector<uint8_t> bytes;
    assert(serialize_section(original, bytes, error));
    Section parsed;
    assert(parse_section(bytes, parsed, error));
    assert(parsed == original);
    auto truncated = bytes;
    truncated.pop_back();
    assert(!parse_section(truncated, parsed, error));
    std::vector<uint8_t> payload;
    std::vector<uint8_t> decoded;
    assert(encode_payload(bytes, payload, error));
    assert(payload.size() % 8 == 0);
    assert(decode_payload(payload, decoded, error));
    assert(decoded == bytes);
    payload[0] = 0xff;
    assert(!decode_payload(payload, decoded, error));
    assert(image_crc32(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>("123456789"), 9)) == 0xcbf43926);
    tooie_language_pack::LanguagePack sample;
    sample.language = "fr";
    sample.base_size = 123;
    sample.font1_charset.resize(73, 'a');
    assert(encode_payload(bytes, payload, error));
    sample.entries.push_back({0xC22, payload});
    std::vector<uint8_t> packed;
    assert(tooie_language_pack::serialize(sample, packed, error));
    tooie_language_pack::LanguagePack parsed_pack;
    assert(tooie_language_pack::parse(packed, 123, parsed_pack, error));
    assert(parsed_pack.entries[0].payload == payload);
    auto make_image = [&](Layout layout, const std::vector<std::pair<size_t, std::vector<uint8_t>>>& assets) {
        std::vector<uint8_t> image(layout.base, 0);
        size_t next = 0;
        for (size_t id = 0; id <= kAssetCount; ++id) {
            uint32_t word = uint32_t((image.size() - layout.base) / 4) << 8;
            if (id == 0xCDA && layout.table == kUsaLayout.table) { word |= 0x13; }
            for (int i = 0; i < 4; ++i) { image[layout.table + id * 4 + i] = uint8_t(word >> (24 - i * 8)); }
            if (next < assets.size() && assets[next].first == id) {
                std::vector<uint8_t> compressed;
                assert(encode_payload(assets[next++].second, compressed, error));
                image.insert(image.end(), compressed.begin(), compressed.end());
            }
        }
        return image;
    };
    std::vector<uint8_t> usa_image = make_image(kUsaLayout, {{0xCDA, {1,3,0,1,128,1,'a'}}});
    std::vector<uint8_t> pal_raw{4,9,0,13,0,17,0,21,0,1,128,1,'a',1,128,1,'f',1,128,1,'d',1,128,1,'s'};
    std::vector<uint8_t> europe = make_image(kEuropeLayout, {{0xC22, {1,2,3}}, {0xCDA, pal_raw}});
    std::vector<uint8_t> core(0x3974 + 30, 0);
    std::string charset = "0123456789:ABCDEFGHIJKLMNOPQRSTUVWXYZ@%?()<>\".;-!/";
    charset.resize(73, 'X');
    std::copy(charset.begin(), charset.end(), core.begin() + 0x3928);
    assert(encode_payload(core, payload, error));
    europe.resize(0x1E8AD81, 0);
    europe.insert(europe.end(), payload.begin(), payload.end());
    tooie_language_pack::LanguagePack imported;
    if (!tooie_language_import::build_unchecked(europe, usa_image, "fr", imported, error)) {
        std::cerr << error << '\n'; return 1;
    }
    assert(imported.entries.size() == 2);
    assert(decode_payload(imported.entries[1].payload, decoded, error));
    assert((decoded == std::vector<uint8_t>{1,3,0,1,128,1,'f'}));
    assert(!tooie_language_import::build(europe, usa_image, "fr", imported, error));

    Section translated = {{{3, 7, {'o', 'i'}}, {2, 0, {1}}}};
    Section selected;
    assert(choose_text(original, original, translated, selected) == 1);
    assert(selected == translated);

    Section menu = {{{128, 0, {'a'}}, {129, 0, {'b'}}}};
    Section english = {{{128, 0, {'a'}}}};
    Section foreign = {{{128, 0, {'x'}}}};
    assert(choose_text(menu, english, foreign, selected) == 2);
    assert(selected[0][0].data == foreign[0][0].data);
    assert(selected[0][1].data == menu[0][1].data);

    Section usa = {{{128, 0, {'a'}}, {2, 0, {1}}}};
    Section pal_english = {{{129, 0, {'a'}}, {2, 0, {1}}}};
    Section pal_text = {{{130, 0, {'x'}}, {2, 0, {1}}}};
    assert(choose_text(usa, pal_english, pal_text, selected) == 3);
    assert(selected[0][0].type == 128 && selected[0][0].data == pal_text[0][0].data);
    pal_text[0][1].data = {2};
    assert(choose_text(usa, pal_english, pal_text, selected) == 0);
}
