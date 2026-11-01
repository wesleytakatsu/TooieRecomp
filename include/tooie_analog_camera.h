#pragma once

#include <array>
#include <cstdint>

namespace recomp::config { class Config; }
namespace tooie_analog_camera {
constexpr int kControllers = 4;
struct Axes { float x = 0, y = 0; };
struct Settings { bool enabled; unsigned sensitivity, invert; };
struct Sample { Axes axes; bool reserved = false; };
struct Motion {
    Axes velocity;
    Axes step(Axes axes, float seconds, Settings settings);
};

Axes radial_deadzone(Axes axes, float deadzone);
bool special_map(unsigned map);
Settings settings();
void set_enabled(bool enabled);
void set_sensitivity(unsigned sensitivity);
void set_invert(unsigned invert);
void install_config(recomp::config::Config& config);

// Game thread publishes eligibility; each controller read retains its own Z priority.
void publish(int controller, bool eligible);
bool reserve(int controller);
void on_input(int controller, Axes raw, float deadzone, uint16_t buttons, bool blocked, bool reserved);
Sample sample(int controller);
void reset();
}
