#pragma once
// Which graphics APIs a build offers, in what order Auto tries them, and what the Graphics API menu lists.
// The policy is pure (any target can be tested on any host); probe_available() is the only part that touches the system.
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace tooie_graphics_api {

enum class Target { Windows, Linux, Handheld };

// The target this binary was compiled for: Handheld when TK64_MOBILE is on.
Target current_target();

// Every key the option has ever used, in enum-value order: auto=0, d3d12=1, vulkan=2, d3d11=3, opengl=4, opengles=5.
// Values stay stable so a saved choice keeps its meaning across builds.
const std::vector<std::string> &all_keys();
uint32_t enum_value(const std::string &key);
std::string menu_label(const std::string &key);

// The backends compiled for a target, in the order Auto tries them. Windows under Wine puts Vulkan first, as RT64 does.
std::vector<std::string> compiled(Target target, bool wine);

using Probe = std::function<bool(const std::string &key)>;

// "auto", then every compiled backend the probe reports usable. `running` (the backend in use now, may be empty) is
// always listed: it is proven available.
std::vector<std::string> menu(Target target, bool wine, const Probe &available, const std::string &running);

// The saved choice when the menu lists it, otherwise "auto". Covers a value from another platform (graphics.json copied
// from Windows to a handheld), a misspelled value, and a backend that this device no longer offers.
std::string effective(const std::string &saved, const std::vector<std::string> &menu);

// Windows supervisor order: the effective choice first, then every compiled backend. The probe does not shorten this
// list: a backend it rejected still gets its worker, so a wrong probe can never leave the game without a renderer.
std::vector<std::string> candidates(Target target, bool wine, const std::string &effective);

// Targets without a supervisor: the TK64_GRAPHICS_API value for the effective choice. Empty lets RT64 pick (Vulkan).
// Linux Auto uses OpenGL when the probe finds no Vulkan device but finds OpenGL; a handheld always uses OpenGL ES.
std::string environment_value(Target target, const std::string &effective, const Probe &available);

// Option description for the target's Auto order.
std::string auto_description(Target target, bool wine);

// Checks whether a backend can start on this machine without creating a renderer: loader libraries, plus a device
// query for D3D12, D3D11 and Vulkan. Results are cached. A Windows worker reads the list its supervisor measured from
// TOOIE_GRAPHICS_AVAILABLE instead of probing again.
bool probe_available(const std::string &key);
// The probed backends of the current target as a comma list, for TOOIE_GRAPHICS_AVAILABLE.
std::string probe_list(bool wine);

} // namespace tooie_graphics_api
