#pragma once

#include <chrono>
#include "elements/ui_label.h"

namespace recompui {

// Updated by the Controls page's frame hook, including while guest input is blocked.
class InputActivity : public Element {
    Element *dot;
    Label *label;
    std::string current_text;
    bool active = false;

public:
    InputActivity(ResourceId rid, Element *parent) : Element(rid, parent, 0, "div", false) {
        set_display(Display::Flex);
        set_align_items(AlignItems::Center);
        set_gap(8.0f);
        auto context = get_current_context();
        dot = context.create_element<Element>(this);
        dot->set_width(8.0f);
        dot->set_height(8.0f);
        dot->set_flex_shrink(0.0f);
        dot->set_border_radius(4.0f);
        dot->set_background_color(theme::color::TextInactive);
        label = context.create_element<Label>(this, "Input: Idle", theme::Typography::LabelSM);
        label->set_color(theme::color::TextDim);
        current_text = "Input: Idle";
    }

    void update(std::chrono::steady_clock::duration age, const std::string &device) {
        bool now_active = age < std::chrono::milliseconds(250);
        std::string text = now_active ? "Input: " + device : "Input: Idle";
        if (text != current_text) {
            label->set_text(text);
            current_text = text;
        }
        if (active != now_active) {
            active = now_active;
            dot->set_background_color(active ? theme::color::Success : theme::color::TextInactive);
            label->set_color(active ? theme::color::SuccessL : theme::color::TextDim);
        }
    }
};

} // namespace recompui
