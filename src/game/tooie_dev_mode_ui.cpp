#include "tooie_dev_mode_ui.h"

#include <algorithm>
#include <string>
#include <vector>

#include "recompui/config.h"
#include "recompui/recompui.h"
#include "elements/ui_button.h"
#include "elements/ui_container.h"
#include "elements/ui_label.h"
#include "elements/ui_scroll_container.h"
#include "elements/ui_text_input.h"
#include "util/file.h"

#include "tooie_dev_warp.h"

namespace {

using namespace recompui;
using tooie_dev_warp::Entry;
using tooie_dev_warp::Position;

// UI state. Only touched on the UI thread (tab contents and button callbacks).
struct State {
    bool unlocked = false;   // the warning was accepted in this session
    bool warp_open = false;  // the Warp modal is shown
    bool entries_loaded = false;
    std::vector<Entry> entries;
    int renaming = -1;       // index of the entry being renamed, or -1
    std::string status;      // last message shown in the Warp modal
    Element* view = nullptr; // the DevModeView of the open tab, or nullptr
    ContextId view_context = ContextId::null(); // the UI context that owns view
    bool dirty = false;      // the view must rebuild its contents on its next update
};

State g_state;

std::filesystem::path store_path() {
    return recompui::file::get_app_folder_path() / "dev_warps.json";
}

void ensure_entries_loaded() {
    if (!g_state.entries_loaded) {
        g_state.entries = tooie_dev_warp::load_entries(store_path());
        g_state.entries_loaded = true;
    }
}

void store_entries() {
    if (!tooie_dev_warp::save_entries(store_path(), g_state.entries)) {
        g_state.status = "Could not write " + store_path().string() + ".";
    }
}

// Rebuilding from a button callback would destroy the button while its callback runs: mark the view instead, and it
// rebuilds on its next update event. Callbacks of a prompt run in the prompt's UI context, and an element can only be
// updated from its own context: switch to the view's context for the update, then back.
void rebuild() {
    g_state.dirty = true;
    if ((g_state.view == nullptr) || (g_state.view_context == ContextId::null())) {
        return;
    }

    ContextId previous = recompui::try_close_current_context();
    g_state.view_context.open();
    g_state.view->queue_update();
    g_state.view_context.close();
    if (previous != ContextId::null()) {
        previous.open();
    }
}

Element* column(ContextId context, Element* parent, float gap) {
    Element* element = context.create_element<Element>(parent);
    element->set_display(Display::Flex);
    element->set_flex_direction(FlexDirection::Column);
    element->set_align_items(AlignItems::FlexStart);
    element->set_gap(gap);
    element->set_width(100.0f, Unit::Percent);
    element->set_height_auto();
    return element;
}

Element* row(ContextId context, Element* parent, float gap) {
    Element* element = context.create_element<Element>(parent);
    element->set_display(Display::Flex);
    element->set_flex_direction(FlexDirection::Row);
    element->set_align_items(AlignItems::Center);
    element->set_gap(gap);
    element->set_width(100.0f, Unit::Percent);
    element->set_height_auto();
    return element;
}

std::string next_default_name(const Position& position) {
    const char* name = tooie_dev_warp::map_name(position.map);
    const std::string base = (name != nullptr) ? std::string(name) : "Map " + std::to_string(position.map);
    int count = 1;
    for (const Entry& entry : g_state.entries) {
        if (entry.position.map == position.map) {
            count++;
        }
    }

    return base + " #" + std::to_string(count);
}

// Row buttons keep their text width; the name column takes what is left.
Button* row_button(ContextId context, Element* line, const std::string& text, ButtonStyle style) {
    Button* button = context.create_element<Button>(line, text, style, ButtonSize::Medium);
    button->set_flex_shrink(0.0f);
    button->set_min_width(104.0f);
    return button;
}

void build_entry_row(ContextId context, Element* list, int index) {
    Entry& entry = g_state.entries[size_t(index)];
    Element* line = row(context, list, 12.0f);
    line->set_padding(8.0f);
    line->set_border_width(theme::border::width);
    line->set_border_radius(theme::border::radius_md);
    line->set_border_color(theme::color::Border);

    if (g_state.renaming == index) {
        TextInput* input = context.create_element<TextInput>(line);
        input->set_flex(1.0f, 1.0f);
        input->set_text(entry.name);
        Button* save = row_button(context, line, "Save", ButtonStyle::Success);
        save->add_pressed_callback([input, index]() {
            if ((index < int(g_state.entries.size())) && !input->get_text().empty()) {
                g_state.entries[size_t(index)].name = input->get_text();
                store_entries();
            }

            g_state.renaming = -1;
            rebuild();
        });
        Button* cancel = row_button(context, line, "Cancel", ButtonStyle::Tertiary);
        cancel->add_pressed_callback([]() {
            g_state.renaming = -1;
            rebuild();
        });
        return;
    }

    Element* text = column(context, line, 2.0f);
    text->set_flex(1.0f, 1.0f);
    text->set_min_width(0.0f);
    context.create_element<Label>(text, entry.name, theme::Typography::LabelMD);
    context.create_element<Label>(text, tooie_dev_warp::describe_map(entry.position.map) + ", entrance " +
        std::to_string(entry.position.entrance), theme::Typography::LabelSM);

    Button* teleport = row_button(context, line, "Teleport", ButtonStyle::Primary);
    teleport->add_pressed_callback([index]() {
        if (index >= int(g_state.entries.size())) {
            return;
        }

        if (!tooie_dev_warp::current_position().has_value()) {
            g_state.status = "Teleporting needs Banjo in a level, unpaused and outside cutscenes.";
            rebuild();
            return;
        }

        tooie_dev_warp::request_teleport(g_state.entries[size_t(index)].position);
        g_state.status = "Teleporting to " + g_state.entries[size_t(index)].name + ".";
        // The game carries the teleport out while it runs, so leave the menu.
        recompui::config::close();
    });
    Button* rename = row_button(context, line, "Rename", ButtonStyle::Secondary);
    rename->add_pressed_callback([index]() {
        g_state.renaming = index;
        rebuild();
    });
    Button* remove = row_button(context, line, "Delete", ButtonStyle::Danger);
    remove->add_pressed_callback([index]() {
        if (index >= int(g_state.entries.size())) {
            return;
        }

        const std::string name = g_state.entries[size_t(index)].name;
        recompui::open_choice_prompt("Delete position", "Delete \"" + name + "\"?", "Delete", "Cancel",
            [index]() {
                if (index < int(g_state.entries.size())) {
                    g_state.entries.erase(g_state.entries.begin() + index);
                    store_entries();
                }

                g_state.renaming = -1;
                rebuild();
            },
            []() {}, ButtonStyle::Danger, ButtonStyle::Tertiary);
    });
}

void build_warp_modal(ContextId context, Element* root) {
    ensure_entries_loaded();

    Element* overlay = context.create_element<Element>(root);
    overlay->set_position(recompui::Position::Absolute);
    overlay->set_top(0);
    overlay->set_right(0);
    overlay->set_bottom(0);
    overlay->set_left(0);
    overlay->set_display(Display::Flex);
    overlay->set_align_items(AlignItems::Center);
    overlay->set_justify_content(JustifyContent::Center);
    overlay->set_background_color(theme::color::BGOverlay);

    Element* panel = column(context, overlay, 16.0f);
    panel->set_max_width(820.0f, Unit::Dp);
    panel->set_max_height(95.0f, Unit::Percent);
    panel->set_padding(20.0f);
    panel->set_border_width(theme::border::width);
    panel->set_border_radius(theme::border::radius_lg);
    panel->set_border_color(theme::color::Border);
    panel->set_background_color(theme::color::ModalOverlay);

    // A heading label needs its own block, or the next line overlaps it.
    Element* heading = context.create_element<Element>(panel);
    heading->set_display(Display::Block);
    heading->set_width(100.0f, Unit::Percent);
    heading->set_height_auto();
    heading->set_padding_bottom(12.0f);
    context.create_element<Label>(heading, "Warp", theme::Typography::Header3);

    const std::optional<Position> current = tooie_dev_warp::current_position();
    const std::string where = current.has_value()
        ? "Banjo is in " + tooie_dev_warp::describe_map(current->map) + "."
        : "Banjo is not in a level right now (title screen, pause, loading or a cutscene).";
    context.create_element<Label>(panel, where, theme::Typography::Body);

    Button* save = context.create_element<Button>(panel, "Save current position", ButtonStyle::Success, ButtonSize::Medium);
    save->set_enabled(current.has_value());
    save->add_pressed_callback([]() {
        const std::optional<Position> position = tooie_dev_warp::current_position();
        if (!position.has_value()) {
            return;
        }

        Entry entry;
        entry.name = next_default_name(*position);
        entry.position = *position;
        g_state.entries.push_back(entry);
        g_state.status = "Saved \"" + entry.name + "\".";
        store_entries();
        rebuild();
    });

    ScrollContainer* scroll = context.create_element<ScrollContainer>(panel, ScrollDirection::Vertical);
    scroll->set_width(100.0f, Unit::Percent);
    // A scroll container does not grow with its contents: size it for the rows, up to a limit.
    constexpr float kRowHeight = 76.0f;
    const float rows = float(std::max<size_t>(g_state.entries.size(), 1));
    const float height = std::min(rows * kRowHeight, 420.0f);
    scroll->set_flex(0.0f, 0.0f, height, Unit::Dp);
    scroll->set_height(height, Unit::Dp);
    scroll->set_max_height(height, Unit::Dp);
    Element* list = column(context, scroll, 8.0f);
    if (g_state.entries.empty()) {
        context.create_element<Label>(list, "No saved positions.", theme::Typography::LabelSM);
    }

    for (int i = 0; i < int(g_state.entries.size()); i++) {
        build_entry_row(context, list, i);
    }

    if (!g_state.status.empty()) {
        context.create_element<Label>(panel, g_state.status, theme::Typography::LabelSM);
    }

    Element* footer = row(context, panel, 12.0f);
    footer->set_justify_content(JustifyContent::FlexEnd);
    Button* close = context.create_element<Button>(footer, "Close", ButtonStyle::Tertiary, ButtonSize::Medium);
    close->add_pressed_callback([]() {
        g_state.warp_open = false;
        g_state.renaming = -1;
        g_state.status.clear();
        rebuild();
    });
}

void build_contents(ContextId context, Element* root) {

    context.create_element<Label>(root, "Dev Mode", theme::Typography::Header2);
    if (!g_state.unlocked) {
        context.create_element<Label>(root,
            "These tools change the game's state directly. Used the wrong way they can soft-lock the game, "
            "corrupt your progress or crash it. Save your game before using them; they are meant for testing.",
            theme::Typography::Body);
        Button* accept = context.create_element<Button>(root, "Accept and unlock", ButtonStyle::Danger, ButtonSize::Medium);
        accept->add_pressed_callback([]() {
            g_state.unlocked = true;
            rebuild();
        });
        return;
    }

    context.create_element<Label>(root, "Game functions. Unlocked until the game is closed.", theme::Typography::LabelSM);
    Element* functions = row(context, root, 12.0f);
    Button* warp = context.create_element<Button>(functions, "Warp", ButtonStyle::Primary, ButtonSize::Medium);
    warp->add_pressed_callback([]() {
        g_state.warp_open = true;
        g_state.status.clear();
        rebuild();
    });

    if (g_state.warp_open) {
        build_warp_modal(context, root);
    }
}

class DevModeView : public Element {
protected:
    void process_event(const Event& e) override {
        if ((e.type == EventType::Update) && g_state.dirty) {
            g_state.dirty = false;
            clear_children();
            build_contents(get_current_context(), this);
        }
    }

    std::string_view get_type_name() override { return "DevModeView"; }

public:
    DevModeView(ResourceId rid, Element* parent) : Element(rid, parent, Events(EventType::Update)) {
        set_display(Display::Flex);
        set_flex_direction(FlexDirection::Column);
        set_align_items(AlignItems::FlexStart);
        set_gap(16.0f);
        set_position(recompui::Position::Relative);
        set_width(100.0f, Unit::Percent);
        set_height(100.0f, Unit::Percent);
        set_padding(24.0f);
        g_state.view = this;
        g_state.view_context = get_current_context();
        build_contents(get_current_context(), this);
    }

    ~DevModeView() override {
        if (g_state.view == this) {
            g_state.view = nullptr;
            g_state.view_context = ContextId::null();
        }
    }
};

} // namespace

void tooie_dev_mode_ui::create_tab() {
    recompui::config::create_tab("Dev Mode", "dev_mode",
        [](ContextId context, Element* body) {
            g_state.renaming = -1;
            g_state.dirty = false;
            context.create_element<DevModeView>(body);
        });
}
