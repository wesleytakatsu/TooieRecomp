#ifndef __OVL_PATCHES_HPP__
#define __OVL_PATCHES_HPP__

namespace tooie {
    void register_tooie_overlays();
    void register_tooie_patches();
}

// Temporary aliases so shared Banjo-origin sources still compile while migrating.
namespace banjo {
    inline void register_bk_overlays() { tooie::register_tooie_overlays(); }
    inline void register_bk_patches() { tooie::register_tooie_patches(); }
}

#endif
