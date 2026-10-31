#include "tooie_dev_forms.h"
#include "recompui/recompui.h"
#include "elements/ui_label.h"
#include <algorithm>
#include <string>

namespace tooie_dev_forms {
bool update_overlay(bool shutting_down) {
    using namespace recompui;
    static ContextId context = ContextId::null();
    static std::array<Element*, kPlayers> panels{};
    static std::array<Label*, kPlayers> titles{};
    static std::array<std::array<Label*, 7>, kPlayers> rows{};
    if (shutting_down) {
        context = ContextId::null();
        panels = {}; titles = {}; rows = {};
        return false;
    }
    auto states = menus().snapshot();
    // ESC/config and other global UI take precedence without capturing either player's overlay input.
    const bool global_menu = is_any_context_shown();
    bool visible = !global_menu && (states[0].open || states[1].open);
    if (context == ContextId::null() && !visible) return false;
    if (context == ContextId::null()) {
        context = create_context();
        context.open();
        for (int p = 0; p < kPlayers; ++p) {
            auto* box = context.create_element<Element>(context.get_root_element());
            panels[p] = box;
            box->set_position(Position::Absolute);
            box->set_display(Display::Flex);
            box->set_flex_direction(FlexDirection::Column);
            box->set_align_items(AlignItems::Center);
            box->set_justify_content(JustifyContent::Center);
            box->set_background_color(Color{8, 12, 24, 225});
            box->set_overflow(Overflow::Hidden);
            box->set_pointer_events(PointerEvents::None);
            titles[p] = context.create_element<Label>(box, "", LabelStyle::Normal);
            titles[p]->set_font_size(26.0f);
            titles[p]->set_width(100.0f, Unit::Percent);
            titles[p]->set_height(36.0f);
            titles[p]->set_line_height(36.0f);
            titles[p]->set_flex_shrink(0.0f);
            titles[p]->set_white_space(WhiteSpace::Nowrap);
            titles[p]->set_text_align(TextAlign::Center);
            titles[p]->set_margin_bottom(12.0f);
            titles[p]->set_color(Color{255, 221, 90, 255});
            // Fixed full-width rows prevent flex min-content widths and wrapped help text from
            // collapsing into the selectable names (especially in half-height viewports).
            for (auto*& label : rows[p]) {
                label = context.create_element<Label>(box, "", LabelStyle::Normal);
                label->set_font_size(24.0f);
                label->set_width(100.0f, Unit::Percent);
                label->set_height(32.0f);
                label->set_line_height(32.0f);
                label->set_flex_shrink(0.0f);
                label->set_white_space(WhiteSpace::Nowrap);
                label->set_text_align(TextAlign::Center);
            }
            auto* footer = context.create_element<Label>(box, "A: Transform     B: Cancel", LabelStyle::Normal);
            footer->set_width(100.0f, Unit::Percent);
            footer->set_height(24.0f);
            footer->set_line_height(24.0f);
            footer->set_flex_shrink(0.0f);
            footer->set_font_size(18.0f);
            footer->set_white_space(WhiteSpace::Nowrap);
            footer->set_text_align(TextAlign::Center);
            footer->set_margin_top(12.0f);
        }
        context.close();
    }
    if (!visible) { context.get_document()->Hide(); return false; }
    context.get_document()->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);
    context.open();
    for (int p = 0; p < kPlayers; ++p) {
        const auto& state = states[p];
        auto* box = panels[p];
        box->set_display(state.open ? Display::Flex : Display::None);
        box->set_left(state.rect.x * 100, Unit::Percent);
        box->set_top(state.rect.y * 100, Unit::Percent);
        box->set_width(state.rect.width * 100, Unit::Percent);
        box->set_height(state.rect.height * 100, Unit::Percent);
        titles[p]->set_text("Player " + std::to_string(p + 1) + " - Transformation (" +
            std::to_string(state.selected + 1) + "/" + std::to_string(kForms.size()) + ")");
        const int first = std::clamp(state.selected - 3, 0, int(kForms.size()) - 7);
        for (int row = 0; row < 7; ++row) {
            const int index = first + row;
            rows[p][row]->set_text(std::string(index == state.selected ? "> " : "  ") + kForms[index].name);
            rows[p][row]->set_color(index == state.selected ? Color{255,221,90,255} : Color{235,235,245,255});
        }
    }
    context.process_updates();
    context.close();
    return true;
}
}
