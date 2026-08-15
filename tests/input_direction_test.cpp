#include <cstdint>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Real runtime converter (ultramodern/src/input.cpp). Not reimplemented here.
void convert_to_n64_range(float x, float y, int8_t& stick_x, int8_t& stick_y);

// Stubs for other symbols referenced by compiling the same input.cpp TU.
namespace ultramodern {
void send_si_message() {}
}

struct Case {
    const char* name;
    float host_x;
    float host_y;
    int expect_sx_sign; // -1, 0, +1
    int expect_sy_sign;
};

static int sign_of(int v, int eps) {
    if (v > eps) {
        return 1;
    }
    if (v < -eps) {
        return -1;
    }
    return 0;
}

int main() {
    std::printf("=== INPUT DIRECTION HARNESS ===\n");
    std::printf("Uses real convert_to_n64_range (ultramodern).\n");

    const Case cases[] = {
        {"center",      0.0f,  0.0f,  0,  0},
        {"right",       1.0f,  0.0f,  1,  0},
        {"left",       -1.0f,  0.0f, -1,  0},
        {"up",          0.0f,  1.0f,  0,  1},
        {"down",        0.0f, -1.0f,  0, -1},
        {"up-right",    1.0f,  1.0f,  1,  1},
        {"up-left",    -1.0f,  1.0f, -1,  1},
        {"down-right",  1.0f, -1.0f,  1, -1},
        {"down-left",  -1.0f, -1.0f, -1, -1},
    };

    int failures = 0;
    for (const Case& c : cases) {
        int8_t sx = 0;
        int8_t sy = 0;
        convert_to_n64_range(c.host_x, c.host_y, sx, sy);
        const int sx_sign = sign_of(int(sx), 2);
        const int sy_sign = sign_of(int(sy), 2);
        const bool ok = (sx_sign == c.expect_sx_sign) && (sy_sign == c.expect_sy_sign);
        std::printf("HOST=(%+.1f,%+.1f) PAD=(%+d,%+d) signs=(%+d,%+d) expect=(%+d,%+d) %s\n",
                    c.host_x, c.host_y, int(sx), int(sy),
                    sx_sign, sy_sign, c.expect_sx_sign, c.expect_sy_sign,
                    ok ? "PASS" : "FAIL");
        if (!ok) {
            ++failures;
        }
    }

    // Signed byte store/load: MEM_B is int8_t*. -80 must survive as -80, not 176.
    {
        int8_t stored = static_cast<int8_t>(-80);
        const int as_signed = int(stored);
        const int as_unsigned = int(static_cast<uint8_t>(stored));
        const bool signed_ok = (as_signed == -80);
        const bool unsigned_is_176 = (as_unsigned == 176);
        std::printf("SIGNED_BYTE -80 as_s8=%d as_u8=%d signed_survives=%s u8_is_176=%s\n",
                    as_signed, as_unsigned,
                    signed_ok ? "YES" : "NO",
                    unsigned_is_176 ? "YES" : "NO");
        if (!signed_ok) {
            ++failures;
        }
    }

    // Tooie func_80015D60 scale word identity.
    // IPL3 residue 0xAD090010 produces 0xE955CCDD - word = 0x3C4CCCCD = 0.0125.
    {
        const uint32_t magic = 0xE955CCDDu;
        const uint32_t ipl3_word = 0xAD090010u;
        const uint32_t derived_bits = magic - ipl3_word;
        float derived = 0.0f;
        static_assert(sizeof(float) == 4, "IEEE float");
        std::memcpy(&derived, &derived_bits, sizeof(derived));
        const bool scale_ok = (derived_bits == 0x3C4CCCCDu) &&
                              (std::fabs(derived - 0.0125f) < 1e-6f);
        std::printf("SCALE IPL3_WORD=%08x derived_bits=%08x derived=%g expect=0.0125 %s\n",
                    ipl3_word, derived_bits, derived, scale_ok ? "PASS" : "FAIL");
        if (!scale_ok) {
            ++failures;
        }
        const uint32_t zero_bits = magic - 0u;
        float zero_scale = 0.0f;
        std::memcpy(&zero_scale, &zero_bits, sizeof(zero_scale));
        std::printf("SCALE UNSEEDED_BSS derived_bits=%08x derived=%g (Tooie D_8007DB54==0)\n",
                    zero_bits, zero_scale);
    }

    std::printf("RESULT failures=%d %s\n", failures, failures == 0 ? "PASS" : "FAIL");
    return failures == 0 ? 0 : 1;
}
