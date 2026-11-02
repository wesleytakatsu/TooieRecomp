#include "tooie_analog_camera.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <mutex>

namespace tooie_analog_camera {
namespace {
// Enabled, sensitivity and inversion are published together, never read from UI objects on game/input threads.
std::atomic_uint32_t options{(3u << 8) | (1u << 16)};
std::array<std::atomic_bool, kControllers> eligible{};
std::mutex input_mutex;
std::array<Sample, kControllers> inputs{};
bool valid(int n) { return n >= 0 && n < kControllers; }
void update_option(uint32_t mask, uint32_t value) {
    uint32_t previous = options.load();
    while (!options.compare_exchange_weak(previous, (previous & ~mask) | value)) {}
}
}

Settings settings() {
    const auto v = options.load();
    return {bool(v & 1), (v >> 8) & 255, (v >> 16) & 3};
}
void set_enabled(bool enabled) {
    update_option(1, enabled ? 1 : 0);
    if (!enabled) reset();
}
void set_sensitivity(unsigned sensitivity) { update_option(255u << 8, std::clamp(sensitivity, 1u, 10u) << 8); }
void set_invert(unsigned invert) { update_option(3u << 16, (invert & 3) << 16); }

Axes radial_deadzone(Axes a, float deadzone) {
    if (!std::isfinite(a.x) || !std::isfinite(a.y) || !std::isfinite(deadzone)) return {};
    deadzone = std::clamp(deadzone, 0.0f, 0.95f);
    const float length = std::hypot(a.x, a.y);
    if (length <= deadzone || length == 0) return {};
    const float scale = (std::min(length, 1.0f) - deadzone) / ((1.0f - deadzone) * length);
    return {a.x * scale, a.y * scale};
}

Axes Motion::step(Axes a, float seconds, Settings config) {
    if (!config.enabled || !std::isfinite(seconds) || seconds <= 0 || seconds > 0.1f ||
        !std::isfinite(a.x) || !std::isfinite(a.y)) { velocity = {}; return {}; }
    a = radial_deadzone(a, 0);
    if (config.invert & 1) a.x = -a.x;
    if (config.invert & 2) a.y = -a.y;
    const float blend = -std::expm1(-seconds / 0.055f);
    const Axes integrated{a.x == 0 ? 0 : a.x * seconds + (velocity.x - a.x) * 0.055f * blend,
                          a.y == 0 ? 0 : a.y * seconds + (velocity.y - a.y) * 0.055f * blend};
    // No release tail: a neutral stick or Z must stop manual motion immediately.
    velocity.x = a.x == 0 ? 0 : velocity.x + (a.x - velocity.x) * blend;
    velocity.y = a.y == 0 ? 0 : velocity.y + (a.y - velocity.y) * blend;
    const float speed = 60.0f + 20.0f * std::clamp(config.sensitivity, 1u, 10u);
    return {integrated.x * speed, integrated.y * speed * (2.0f / 3.0f)};
}

bool special_map(unsigned map) {
    // Native map identities (maps.h): dedicated challenges/races, including story versions.
    // Excluding the whole room also preserves their setup/results screens.
    switch (map) {
    case 0xC6: case 0xC8: case 0xC9: case 0xCA: case 0xCB: case 0xCC: case 0xCD: case 0xCE:
    case 0xDC: case 0xDE: case 0xDF: case 0xE0: case 0xE4: case 0xE5:
    case 0x10A: case 0x123: case 0x124: case 0x12E: case 0x12F: case 0x130:
    case 0x181: case 0x13B: case 0x13C:
    case 0x137: case 0x139: case 0x13D: case 0x15F: case 0x161: case 0x162:
    case 0x164: case 0x165: case 0x166: case 0x16F: case 0x170:
    case 0x17B: case 0x17C: case 0x17D: case 0x17F: case 0x180: case 0x182: case 0x183:
    case 0x185: case 0x186: case 0x188: case 0x189: return true;
    default: return false;
    }
}

void publish(int controller, bool value) {
    if (!valid(controller)) return;
    const bool previous = eligible[controller].exchange(value);
    if (previous && !value) {
        std::lock_guard lock(input_mutex);
        inputs[controller] = {}; // a sample from before aim must not rotate the camera after aim
    }
}
bool reserve(int controller) { return valid(controller) && settings().enabled && eligible[controller].load(); }
void on_input(int controller, Axes raw, float deadzone, uint16_t buttons, bool blocked, bool reserved) {
    if (!valid(controller)) return;
    std::lock_guard lock(input_mutex);
    Sample next;
    next.reserved = reserved && reserve(controller) && !blocked && !(buttons & 0x2000);
    if (next.reserved) next.axes = radial_deadzone(raw, deadzone);
    inputs[controller] = next;
}
Sample sample(int controller) {
    std::lock_guard lock(input_mutex);
    return valid(controller) ? inputs[controller] : Sample{};
}
void reset() {
    for (auto& e : eligible) e.store(false);
    std::lock_guard lock(input_mutex);
    inputs = {};
}
}
