// Present-rate meter: successful swapchain presents per second over a monotonic window.
// Measures presentation cadence only, not simulation or unique-pose rate.
#pragma once

#include <chrono>
#include <cstdint>

namespace tooie {
    class PresentRateMeter {
    public:
        using Clock = std::chrono::steady_clock;

        static constexpr std::chrono::milliseconds window{ 1000 };
        // A longer gap between samples means presentation stalled (pause, resize, minimize): restart the window
        // instead of averaging the stall into the reading.
        static constexpr std::chrono::milliseconds max_gap{ 3000 };

        // Feed the running success count; returns true when value() changed.
        bool sample(uint64_t count, Clock::time_point now) {
            if (!primed || count < base_count || (now - base_time) > max_gap) {
                base_count = count;
                base_time = now;
                primed = true;
                const bool changed = valid;
                valid = false;
                return changed;
            }

            const auto elapsed = now - base_time;
            if (elapsed < window) {
                return false;
            }

            const double seconds = std::chrono::duration<double>(elapsed).count();
            const uint32_t next = uint32_t((double(count - base_count) / seconds) + 0.5);
            const bool changed = !valid || (next != fps);
            fps = next;
            valid = true;
            base_count = count;
            base_time = now;
            return changed;
        }

        void reset() { primed = false; valid = false; }
        bool has_value() const { return valid; }
        uint32_t value() const { return fps; }

    private:
        uint64_t base_count = 0;
        Clock::time_point base_time{};
        uint32_t fps = 0;
        bool primed = false;
        bool valid = false;
    };
}
