#include "ovl_patches.hpp"
#include "librecomp/overlays.hpp"

// No native patches yet — empty registration keeps the boot path compiling.
void tooie::register_tooie_patches() {
    // Intentionally empty for P05. Patches land after first-frame parity work.
}
