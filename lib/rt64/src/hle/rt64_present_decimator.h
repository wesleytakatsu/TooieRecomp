//
// RT64
//

#pragma once

#include <cstdint>

namespace RT64 {
    // Below-native presentation cap. When the target rate is under the VI original rate, only
    // target/original of the presents are shown. The rest take the catch-up skip path, so every
    // workload is still consumed and the guest never waits. Integer accumulator handles
    // non-divisor rates (45, 70, ...). Never touches the guest, VI or audio.
    struct PresentDecimator {
        uint32_t ticks = 0;
        uint32_t lastTarget = 0;
        uint32_t lastOriginal = 0;

        bool shouldPresent(bool manualRate, bool paused, uint32_t targetRate, uint32_t originalRate) {
            if (!manualRate || paused || (targetRate == 0) || (originalRate == 0) || (targetRate >= originalRate)) {
                lastTarget = lastOriginal = 0;
                return true;
            }

            if ((targetRate != lastTarget) || (originalRate != lastOriginal)) {
                lastTarget = targetRate;
                lastOriginal = originalRate;
                ticks = originalRate - targetRate;
            }

            ticks += targetRate;
            if (ticks >= originalRate) {
                ticks -= originalRate;
                return true;
            }

            return false;
        }
    };
};
