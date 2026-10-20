#include <cstdio>
#include <cstdlib>
#include "tooie_menu_policy.h"

using namespace tooie_menu;
using ultramodern::renderer::AspectRatio;
using ultramodern::renderer::Resolution;

#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); std::exit(1); } } while (0)

int main() {
    // Enum values preserved, new ones appended.
    CHECK(static_cast<int>(Resolution::Original) == 0 && static_cast<int>(Resolution::Original2x) == 1 && static_cast<int>(Resolution::Auto) == 2);
    CHECK(static_cast<int>(Resolution::Original3x) == 3 && static_cast<int>(Resolution::Original4x) == 4);
    nlohmann::json j = Resolution::Original3x;
    CHECK(j == "Original3x");
    CHECK(nlohmann::json("Original4x").get<Resolution>() == Resolution::Original4x);
    CHECK(nlohmann::json("Original2x").get<Resolution>() == Resolution::Original2x);

    CHECK(kForcedRefreshRate == 30);

    CHECK(window_aspect_target(AspectRatio::Manual) == 0.0);
    // 4:3 -> 16:9 keeps height.
    WindowSize s = aspect_window_size(window_aspect_target(AspectRatio::Expand), 960, 720, 1920, 1080);
    CHECK(s.w == 1280 && s.h == 720);
    s = aspect_window_size(window_aspect_target(AspectRatio::Original), 1280, 720, 1920, 1080);
    CHECK(s.w == 960 && s.h == 720);
    // Clamp to usable width, aspect kept.
    s = aspect_window_size(16.0 / 9.0, 1000, 1000, 1280, 1080);
    CHECK(s.w == 1280 && s.h == 720);
    // Clamp to usable height.
    s = aspect_window_size(4.0 / 3.0, 100, 2000, 5000, 1000);
    CHECK(s.h == 1000 && s.w == 1333);
    // Invalid inputs unchanged.
    s = aspect_window_size(0.0, 640, 480, 1920, 1080);
    CHECK(s.w == 640 && s.h == 480);
    s = aspect_window_size(16.0 / 9.0, 0, 480, 1920, 1080);
    CHECK(s.w == 0 && s.h == 480);

    // Request: consume once, last wins, nothing queued for non-resizing options.
    CHECK(take_window_aspect_request() == 0.0);
    request_window_aspect(4.0 / 3.0);
    request_window_aspect(16.0 / 9.0);
    CHECK(take_window_aspect_request() == 16.0 / 9.0);
    CHECK(take_window_aspect_request() == 0.0);
    // Only live targets get queued by the caller; Manual yields 0.
    double t = window_aspect_target(AspectRatio::Manual);
    if (t > 0.0) request_window_aspect(t);
    CHECK(take_window_aspect_request() == 0.0);
    // Geometry preserved when already at target.
    s = aspect_window_size(16.0 / 9.0, 1280, 720, 1920, 1080);
    CHECK(s.w == 1280 && s.h == 720);

    std::puts("tooie_menu_policy_test PASS");
    return 0;
}
