#include "tooie_text_font_map.h"
#include "recomp.h"

namespace {
    bool european_enabled = false;
    tooie_text_font_map::CharsetTable charset_table{};
    tooie_text_font_map::AccentMap accent_map{};
}

void tooie_text_font_map::set_european_font_map(std::span<const uint8_t> charset, const AccentMap& accents) {
    charset_table = build_table(charset);
    accent_map = accents;
    european_enabled = true;
}

void tooie_text_font_map::reset() {
    european_enabled = false;
    charset_table.fill(-1);
    accent_map.fill(0);
}

extern "C" void func_800D35D0(uint8_t* rdram, recomp_context* ctx) {
    int32_t font = static_cast<int32_t>(ctx->r4);
    int32_t character = static_cast<int32_t>(ctx->r5);
    int32_t result;
    if (european_enabled) {
        result = tooie_text_font_map::map_european(font, character, charset_table, accent_map);
    } else {
        // The USA function leaves the folded character in a1; keep that side effect.
        ctx->r5 = static_cast<gpr>(static_cast<int64_t>(tooie_text_font_map::fold_usa(font, character)));
        result = tooie_text_font_map::map_usa(font, character, [rdram](int32_t index) {
            return static_cast<int8_t>(MEM_B(static_cast<gpr>(static_cast<int32_t>(0x80130000u)) + index, -0x4C90));
        });
    }
    ctx->r2 = static_cast<gpr>(static_cast<int64_t>(result));
}
