// Host test of the present-rate meter (header-only).
#include "tooie_fps_meter.h"

#include <cstdio>
#include <cstdlib>

#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, #c); std::exit(1); } } while (0)

using tooie::PresentRateMeter;
using ms = std::chrono::milliseconds;

int main() {
    const PresentRateMeter::Clock::time_point t0{};
    PresentRateMeter m;

    // No value before a full window.
    CHECK(!m.has_value());
    m.sample(0, t0);
    CHECK(!m.sample(30, t0 + ms(500)));
    CHECK(!m.has_value());

    // 60 presents over 1 s.
    CHECK(m.sample(60, t0 + ms(1000)));
    CHECK(m.value() == 60);
    // Same rate again: unchanged, no text update needed.
    CHECK(!m.sample(120, t0 + ms(2000)));
    CHECK(m.value() == 60);
    // Rounds a non-integer window (1.25 s, 75 presents -> 60).
    CHECK(!m.sample(195, t0 + ms(3250)));
    CHECK(m.value() == 60);
    // Rate change: 30 presents in 1 s.
    CHECK(m.sample(225, t0 + ms(4250)));
    CHECK(m.value() == 30);
    // Zero presents but time still advancing within max_gap reads 0, never divides by zero.
    CHECK(m.sample(225, t0 + ms(5250)));
    CHECK(m.value() == 0);

    // Stall longer than max_gap restarts the window and hides the value.
    CHECK(m.sample(226, t0 + ms(60000)));
    CHECK(!m.has_value());
    m.sample(286, t0 + ms(61000));
    CHECK(m.has_value() && m.value() == 60);

    // Counter going backwards restarts rather than underflowing.
    m.sample(5, t0 + ms(62000));
    CHECK(!m.has_value());

    // reset() (counter disabled) drops history.
    m.reset();
    CHECK(!m.has_value());
    CHECK(!m.sample(1000, t0 + ms(70000)));
    CHECK(!m.has_value());

    std::puts("tooie_fps_meter_test PASS");
    return 0;
}
