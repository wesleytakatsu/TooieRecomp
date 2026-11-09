#include "tooie_font_sprite.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

int main() {
    using namespace tooie_font_sprite;
    Sprite source;
    for (int i = 0; i < 51; ++i) {
        Frame frame{8, 23, 8, std::vector<uint8_t>(8 * 23, 0)};
        for (int y = 3; y < 20; ++y) { frame.pixels[y * 8 + 3] = 0xff; }
        frame.pixels[15 * 8 + 2] = uint8_t((i % 16) * 16 + 15);
        source.frames.push_back(std::move(frame));
    }
    std::string error;
    std::vector<uint8_t> bytes;
    assert(serialize(source, bytes, error));
    Sprite parsed;
    assert(parse(bytes, parsed, error));
    assert(parsed.frames.size() == 51 && parsed.frames[0].pixels == source.frames[0].pixels);
    Sprite generated;
    assert(generate_font1(parsed, generated, error));
    assert(generated.frames.size() == 74);
    for (size_t i = 0; i < 51; ++i) {
        assert(generated.frames[i].height == 31);
        assert(std::equal(source.frames[i].pixels.begin(), source.frames[i].pixels.end(), generated.frames[i].pixels.begin() + 64));
    }
    assert(std::any_of(generated.frames[51].pixels.begin(), generated.frames[51].pixels.begin() + 64, [](uint8_t p) { return p != 0; }));
    assert(serialize(generated, bytes, error));
    assert(parse(bytes, parsed, error));
    Sprite european;
    assert(generate_font1(source, european, error, Font1Profile::European));
    assert(european.frames.size() == 74 && european.format == kIa8Format);
    assert(european.frames[51].pixels != generated.frames[51].pixels); // Ä / Ã
    assert(european.frames[58].pixels != generated.frames[58].pixels); // Ñ / Ç
    assert(european.frames[59].pixels != generated.frames[59].pixels); // Ö / Õ
    assert(european.frames[69].pixels == generated.frames[70].pixels); // Ï
    assert(european.frames[70].pixels == generated.frames[69].pixels); // Ê
    assert(european.frames[72].pixels == generated.frames[71].pixels); // Ô
    assert(european.frames[73].pixels == generated.frames[73].pixels); // comma
    assert(european.frames[58].pixels[64 + 15 * 8 + 2] == source.frames[24].pixels[15 * 8 + 2]); // N
    assert(european.frames[71].pixels[64 + 15 * 8 + 2] == source.frames[31].pixels[15 * 8 + 2]); // U

    // Every IA4 intensity/alpha combination, in high- then low-nibble order.
    std::vector<uint8_t> ia4(24, 0);
    put16(ia4, 0, 1); put16(ia4, 2, kIa4Format); put32(ia4, 8, 24);
    ia4[12] = 8; ia4[13] = 2; ia4[14] = 8; ia4[15] = 2;
    ia4.insert(ia4.end(), {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef});
    assert(parse(ia4, parsed, error));
    assert(parsed.format == kIa8Format && parsed.frames.size() == 1);
    assert((parsed.frames[0].pixels == std::vector<uint8_t>{
        0x00,0x0f,0x20,0x2f,0x40,0x4f,0x60,0x6f,0x90,0x9f,0xb0,0xbf,0xd0,0xdf,0xf0,0xff}));
    assert(serialize(parsed, bytes, error) && parse(bytes, parsed, error));
    ia4.pop_back();
    assert(!parse(ia4, parsed, error));
    bytes.resize(13);
    assert(!parse(bytes, parsed, error));
}
