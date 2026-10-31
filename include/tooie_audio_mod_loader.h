#pragma once
namespace recomp::mods { class ModContext; class ModHandle; }
namespace tooie_audio_mod {
void install_hooks();
void on_enabled(recomp::mods::ModContext&, const recomp::mods::ModHandle&);
void on_disabled(recomp::mods::ModContext&, const recomp::mods::ModHandle&);
void on_reordered(recomp::mods::ModContext&);
}
