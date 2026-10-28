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
    bytes.resize(13);
    assert(!parse(bytes, parsed, error));
}
