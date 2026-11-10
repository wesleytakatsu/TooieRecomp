#pragma once

#include <cstdint>
#include <functional>

namespace recomp::config {
    class Config;
}

// Graphics > "Materials": normal maps of material texture packs relight the scene, provided by the renderer (TK64,
// hle/rt64_host_materials.h). Off by default: nothing changes and no material map is loaded. Experimental, for testing.
namespace tooie_materials {
    inline constexpr char option[] = "materials";

    // Option values: the renderer's requests. 0 Off, 1 On, 2 Relief View (on, with the relighting shown as gray).
    inline constexpr uint32_t off = 0;
    inline constexpr uint32_t on = 1;
    inline constexpr uint32_t relief_view = 2;

    // Receives the mode on load and on Apply (never for unapplied UI changes), and whether material maps must be
    // loaded or released (materials turned on or off), so the host reloads its texture packs. The game passes
    // RT64::requestMaterials and recompui::renderer::trigger_texture_pack_update; tests pass their own.
    using ModeSink = std::function<void(uint32_t mode, bool reload_packs)>;

    void add_config(recomp::config::Config& config, ModeSink sink);
}
