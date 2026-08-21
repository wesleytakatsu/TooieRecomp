#include <cassert>
#include <cstdint>
#include <cstring>

#include "recomp.h"

namespace {

float from_bits(uint32_t bits) {
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

uint32_t to_bits(float value) {
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

}

int main() {
    // Neighbouring half-way cases prove the architectural ties-to-even rule.
    assert(ROUND_W_S(0.5f) == 0);
    assert(ROUND_W_S(1.5f) == 2);
    assert(ROUND_W_S(2.5f) == 2);
    assert(ROUND_W_S(3.5f) == 4);
    assert(ROUND_W_S(-0.5f) == 0);
    assert(ROUND_W_S(-1.5f) == -2);
    assert(ROUND_W_S(-2.5f) == -2);
    assert(ROUND_W_S(-3.5f) == -4);
    assert(ROUND_W_D(2.5) == 2);
    assert(ROUND_L_S(3.5f) == 4);
    assert(ROUND_L_D(4.5) == 4);

    // USER_DEMO_4's operands exercise the real boundary without special-casing it.
    const float f12 = from_bits(0x4399BCF1u);
    const float f14 = from_bits(0x3B360B61u);
    const float table_scale = from_bits(0x4B000000u);
    const float turns = 360.0f;
    const int32_t whole_turns = TRUNC_W_S(MUL_S(f12, f14));
    const float remainder = f12 - MUL_S(CVT_S_W(whole_turns), turns);
    const float table_coordinate = MUL_S(MUL_S(remainder, f14), table_scale);

    assert(to_bits(table_coordinate) == 0x4ADAA651u);
    assert(ROUND_W_S(table_coordinate) == 0x006D5328);
    // This calls the target-level compatibility mapping used by existing generated C.
    assert(lroundf(table_coordinate) == 0x006D5328L);
    assert(ROUND_W_S(from_bits(0x4ADAA650u)) == 0x006D5328);
    assert(ROUND_W_S(from_bits(0x4ADAA652u)) == 0x006D5329);

    // MIPS ROUND.W defaults to 2^31 - 1 for Invalid Operation when masked.
    assert(ROUND_W_S(NAN) == INT32_MAX);
    assert(ROUND_W_S(INFINITY) == INT32_MAX);
    assert(ROUND_W_S(-INFINITY) == INT32_MAX);
    assert(ROUND_W_S(from_bits(0x4F000000u)) == INT32_MAX);
    assert(ROUND_W_S(from_bits(0xCF000000u)) == INT32_MIN);
    return 0;
}
