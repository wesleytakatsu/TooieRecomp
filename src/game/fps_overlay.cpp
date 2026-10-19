// Top-left present-rate counter. Reads RT64's successful-present count and draws through the frontend's
// always-on overlay hook, so it runs on the present thread and shares no state with the game threads.
#include "tooie_fps_overlay.h"
#include "tooie_fps_meter.h"

#include <atomic>
#include <cstdio>

#include "recompui/recompui.h"
#include "elements/ui_label.h"
#include "hle/rt64_present_stats.h"

namespace {
    std::atomic<bool> overlay_enabled{ false };

    // Present thread only (the frontend calls the overlay callback from its draw hook).
    recompui::ContextId overlay_context = recompui::ContextId::null();
    recompui::Element *overlay_label = nullptr;
    tooie::PresentRateMeter overlay_meter;
    bool overlay_shown = false;

    void build_overlay() {
        overlay_context = recompui::create_context();
        overlay_context.open();

        recompui::Element *box = overlay_context.create_element<recompui::Element>(overlay_context.get_root_element());
        box->set_position(recompui::Position::Absolute);
        box->set_top(16.0f);
        box->set_left(16.0f);
        box->set_padding_left(10.0f);
        box->set_padding_right(10.0f);
        box->set_padding_top(4.0f);
        box->set_padding_bottom(4.0f);
        box->set_border_radius(6.0f);
        box->set_background_color(recompui::Color{ 0, 0, 0, 170 });
        box->set_pointer_events(recompui::PointerEvents::None);

        overlay_label = overlay_context.create_element<recompui::Label>(box, std::string("-- FPS"), recompui::LabelStyle::Normal);
        overlay_label->set_color(recompui::Color{ 255, 255, 255, 255 });
        overlay_label->set_pointer_events(recompui::PointerEvents::None);

        overlay_context.close();
    }

    bool update_overlay(bool shutting_down) {
        if (shutting_down) {
            // The frontend destroys every context next.
            overlay_context = recompui::ContextId::null();
            overlay_label = nullptr;
            overlay_shown = false;
            overlay_meter.reset();
            return false;
        }

        if (!overlay_enabled.load(std::memory_order_relaxed)) {
            if (overlay_shown) {
                overlay_context.get_document()->Hide();
                overlay_shown = false;
            }

            overlay_meter.reset();
            return false;
        }

        if (overlay_context == recompui::ContextId::null()) {
            build_overlay();
        }

        if (!overlay_shown) {
            overlay_context.get_document()->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);
            overlay_shown = true;
        }

        const bool changed = overlay_meter.sample(RT64::presentSuccessCount.load(std::memory_order_relaxed), tooie::PresentRateMeter::Clock::now());
        overlay_context.open();
        if (changed) {
            char text[24];
            if (overlay_meter.has_value()) {
                std::snprintf(text, sizeof(text), "%u FPS", overlay_meter.value());
            }
            else {
                std::snprintf(text, sizeof(text), "-- FPS");
            }

            overlay_label->set_text(text);
        }

        overlay_context.process_updates();
        overlay_context.close();
        return true;
    }
}

void tooie::register_fps_overlay() {
    recompui::set_overlay_update_callback(update_overlay);
}

void tooie::set_fps_overlay_enabled(bool enabled) {
    overlay_enabled.store(enabled, std::memory_order_relaxed);
}
