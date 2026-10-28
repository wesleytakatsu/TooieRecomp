#include "tooie_font_sprite.h"
#include "tooie_text_assets.h"

#include <fstream>
#include <iostream>
#include <iterator>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: tooie_font_preview <USA image> <output.ppm>\n";
        return 1;
    }
    std::ifstream input(argv[1], std::ios::binary);
    if (!input) {
        std::cerr << "cannot open USA image\n";
        return 1;
    }
    std::vector<uint8_t> image((std::istreambuf_iterator<char>(input)), {});
    std::span<const uint8_t> payload;
    std::vector<uint8_t> raw;
    tooie_font_sprite::Sprite source, generated;
    std::string error;
    if (input.bad() || !tooie_text_assets::validate_image(image, false, error) ||
        !tooie_text_assets::asset_payload(image, tooie_asset_table::kUsaLayout, 0xC22, payload, error) ||
        !tooie_text_assets::decode_payload(payload, raw, error) ||
        !tooie_font_sprite::parse(raw, source, error) ||
        !tooie_font_sprite::generate_font1(source, generated, error)) {
        std::cerr << "font generation failed: " << error << '\n';
        return 1;
    }
    size_t width = 0;
    for (const auto& frame : generated.frames) {
        width += frame.padded_width;
    }
    constexpr size_t scale = 4;
    std::ofstream output(argv[2], std::ios::binary);
    output << "P6\n" << width * scale << ' ' << 31 * scale << "\n255\n";
    for (size_t y = 0; y < 31 * scale; ++y) {
        for (const auto& frame : generated.frames) {
            for (size_t x = 0; x < frame.padded_width * scale; ++x) {
                uint8_t pixel = frame.pixels[(y / scale) * frame.padded_width + x / scale];
                unsigned alpha = (pixel & 15) * 17;
                unsigned intensity = (pixel >> 4) * 17;
                char rgb[] = {char((intensity * alpha + 255 * (255 - alpha)) / 255),
                              char(intensity * alpha / 255),
                              char((intensity * alpha + 255 * (255 - alpha)) / 255)};
                output.write(rgb, 3);
            }
        }
    }
    if (!output) {
        std::cerr << "cannot write preview\n";
        return 1;
    }
    return 0;
}
