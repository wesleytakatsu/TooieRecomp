#pragma once

#include <cstdint>
#include <functional>

namespace recomp::config {
    class Config;
}

// Graphics > "Materials": normal maps of material texture packs relight the scene, provided by the renderer (TK64,
// hle/rt64_host_materials.h). Off by default: nothing changes and no material map is loaded. Experimental, for testing.
// Graphics > "Materials Depth", right below: scale of the relief, in percent of the pack's own strength.
namespace tooie_materials {
    inline constexpr char option[] = "materials";
    inline constexpr char strength_option[] = "materials_strength";

    // Option values: the renderer's requests. 0 Off, 1 On, 2 Relief View (on, with the relighting shown as gray).
    inline constexpr uint32_t off = 0;
    inline constexpr uint32_t on = 1;
    inline constexpr uint32_t relief_view = 2;

    // Depth in percent: 0 flat, 100 the pack's own, up to 400 (the renderer's maximum scale is 4).
    inline constexpr double strength_min = 0.0;
    inline constexpr double strength_max = 400.0;
    inline constexpr double strength_default = 100.0;

    // Receives the mode on load and on Apply (never for unapplied UI changes), and whether material maps must be
    // loaded or released (materials turned on or off), so the host reloads its texture packs. The game passes
    // RT64::requestMaterials and recompui::renderer::trigger_texture_pack_update; tests pass their own.
    using ModeSink = std::function<void(uint32_t mode, bool reload_packs)>;

    // Receives the depth as a scale (percent / 100) on load and on Apply. The game passes RT64::requestMaterialStrength.
    using StrengthSink = std::function<void(float scale)>;

    void add_config(recomp::config::Config& config, ModeSink mode_sink, StrengthSink strength_sink);
}
