#include "tooie_language_bundle.h"
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
        std::cout << "pt-BR: " << pack.entries.size() - 1 << " text/font 0 entries equal\n";
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
    ++bundle.font_crc;
    assert(!build_unchecked(bundle, usa, pack, error));
}
