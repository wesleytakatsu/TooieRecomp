#include "tooie_dev_mode_ui.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <string>
#include <vector>

#include "recompui/config.h"
#include "recompui/recompui.h"
#include "elements/ui_button.h"
#include "elements/ui_container.h"
#include "elements/ui_label.h"
#include "elements/ui_scroll_container.h"
#include "elements/ui_select.h"
#include "elements/ui_text_input.h"
#include "elements/ui_toggle.h"
#include "util/file.h"
#include "recompinput/input_state.h"
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Input.h>

#include "tooie_dev_cheats.h"
#include "tooie_dev_forms.h"
#include "tooie_dev_warp.h"
#include "tooie_expanded_memory.h"

namespace {

using namespace recompui;
using tooie_dev_warp::Entry;
using tooie_dev_warp::Position;

// Warp modal items that are not a saved position (rows use their index).
constexpr int kSaveItem = -1;  // "Save current position"
constexpr int kCloseItem = -2; // "Close"

// UI state. Only touched on the UI thread (tab contents and button callbacks).
struct State {
    bool unlocked = false;   // the warning was accepted in this session
    bool warp_open = false;  // the Warp modal is shown
    bool entries_loaded = false;
    bool cheats_loaded = false;
    std::vector<Entry> entries;
    int renaming = -1;       // index of the entry being renamed, or -1
    std::string status;      // last message shown in the Warp modal
    Element* view = nullptr; // the DevModeView of the open tab, or nullptr
    ContextId view_context = ContextId::null(); // the UI context that owns view
    bool dirty = false;      // the view must rebuild its contents on its next update
    // Focus restored by the next build: a Warp modal item (row index, kSaveItem or kCloseItem) and its button, or the
    // Warp button once the modal closes.
    bool warp_focus_pending = false;
    int warp_focus_item = 0;
    int warp_focus_column = 0;
    bool focus_warp_button = false;
    Element* focus_target = nullptr; // set while building; focused once the new contents are laid out
    Element* focus_line = nullptr;   // scrolled into view with focus_target
};

State g_state;

// The next build focuses this Warp modal item.
void focus_warp_item(int item, int column) {
    g_state.warp_focus_pending = true;
    g_state.warp_focus_item = item;
    g_state.warp_focus_column = column;
}

void rebuild();

// Closes the Warp modal and returns the focus to the Warp button.
void close_warp_modal() {
    g_state.warp_open = false;
    g_state.renaming = -1;
    g_state.status.clear();
    g_state.focus_warp_button = true;
    rebuild();
}

// The Warp modal moves the focus itself, so it never leaves the modal: up and down pick a saved position (or the
// Save and Close buttons above and below the list), left and right pick a button in that row. The list scrolls with
// the focus, and the right stick scrolls it too. The menu's Back action (B by default) closes the modal instead of
// leaving it open and moving the focus to the tabs behind it.
class WarpModal : public Element {
public:
    WarpModal(ResourceId rid, Element* parent) : Element(rid, parent, Events(EventType::Update, EventType::Navigate)) {
        last_update = std::chrono::steady_clock::now();
        queue_update();
    }

    // Items are added top to bottom; line is what gets scrolled into view.
    void add_item(int id, Element* line, std::vector<Element*> buttons) {
        items.push_back(Item{id, line, std::move(buttons)});
    }

    void set_list(Element* list) { scroll = list; }

    // Sets the build's focus target to the item and button saved in g_state. A row that is gone (deleted) falls back
    // to the row above it, and a missing item to the first one.
    void restore_focus() const {
        if (items.empty()) {
            return;
        }

        size_t index = 0;
        for (size_t i = 0; i < items.size(); i++) {
            const int id = items[i].id;
            if ((id == g_state.warp_focus_item) ||
                ((g_state.warp_focus_item >= 0) && (id >= 0) && (id <= g_state.warp_focus_item))) {
                index = i;
            }
        }

        const Item& item = items[index];
        const int column = std::clamp(g_state.warp_focus_column, 0, int(item.buttons.size()) - 1);
        g_state.focus_target = item.buttons[size_t(column)];
        g_state.focus_line = item.line;
    }

protected:
    void process_event(const Event& e) override {
        if (e.type != EventType::Update) {
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        const float seconds = std::min(std::chrono::duration<float>(now - last_update).count(), 0.1f);
        last_update = now;
        float x = 0.0f;
        float y = 0.0f;
        recompinput::get_right_analog(-1, &x, &y);
        if ((scroll != nullptr) && (std::fabs(y) > 0.1f)) {
            // Keep the fraction: the scroll offset is rounded to whole pixels.
            constexpr float kStickScrollDp = 900.0f;
            scroll_pending += y * kStickScrollDp * scroll->get_dp_to_pixel_ratio() * seconds;
            const float step = std::trunc(scroll_pending);
            if (step != 0.0f) {
                scroll->set_scroll_top(scroll->get_scroll_top() + step);
                scroll_pending -= step;
            }
        }
        else {
            scroll_pending = 0.0f;
        }

        queue_update();
    }

    std::string_view get_type_name() override { return "WarpModal"; }

private:
    struct Item {
        int id;
        Element* line;
        std::vector<Element*> buttons;
    };

    std::vector<Item> items;
    Element* scroll = nullptr;
    float scroll_pending = 0.0f;
    int row_column = 0; // button kept while moving between saved positions
    std::chrono::steady_clock::time_point last_update;

    void focus_item(size_t index, int column) {
        const Item& item = items[index];
        column = std::clamp(column, 0, int(item.buttons.size()) - 1);
        item.buttons[size_t(column)]->focus();
        item.line->scroll_into_view(true);
        if (item.id >= 0) {
            row_column = column;
        }
    }

    bool handle_navigation_event(Rml::Event& event) override {
        if ((event.GetId() != Rml::EventId::Keydown) || !event.IsPropagating()) {
            return false;
        }

        const auto key = Rml::Input::KeyIdentifier(event.GetParameter<int>("key_identifier", Rml::Input::KI_UNKNOWN));
        if (menu_action_mapping::menu_action_from_rml_key(key) == MenuAction::Back) {
            event.StopPropagation();
            close_warp_modal();
            return true;
        }

        int dx = 0;
        int dy = 0;
        switch (key) {
            case Rml::Input::KI_UP: dy = -1; break;
            case Rml::Input::KI_DOWN: dy = 1; break;
            case Rml::Input::KI_LEFT: dx = -1; break;
            case Rml::Input::KI_RIGHT: dx = 1; break;
            default: return false;
        }

        event.StopPropagation();
        if (items.empty()) {
            return true;
        }

        const Element* focused = get_current_context().get_focused_element();
        for (size_t i = 0; i < items.size(); i++) {
            for (size_t column = 0; column < items[i].buttons.size(); column++) {
                if (items[i].buttons[column] != focused) {
                    continue;
                }

                if (dx != 0) {
                    focus_item(i, int(column) + dx);
                }
                else {
                    const size_t next = size_t(std::clamp(int(i) + dy, 0, int(items.size()) - 1));
                    if (next != i) {
                        focus_item(next, (items[next].id >= 0) ? row_column : 0);
                    }
                }

                return true;
            }
        }

        // Focus is outside the modal (it was just built, or the mouse was used): start at the first item.
        focus_item(0, 0);
        return true;
    }
};

std::filesystem::path store_path() {
    return recompui::file::get_app_folder_path() / "dev_warps.json";
}

void ensure_entries_loaded() {
    if (!g_state.entries_loaded) {
        g_state.entries = tooie_dev_warp::load_entries(store_path());
        g_state.entries_loaded = true;
    }
}

std::filesystem::path cheats_path() {
    return recompui::file::get_app_folder_path() / "dev_cheats.json";
}

void ensure_cheats_loaded() {
    if (!g_state.cheats_loaded) {
        tooie_dev_cheats::set_settings(tooie_dev_cheats::load_settings(cheats_path()));
        g_state.cheats_loaded = true;
    }
}

void store_cheats(const tooie_dev_cheats::Settings& settings) {
    tooie_dev_cheats::set_settings(settings);
    tooie_dev_cheats::save_settings(cheats_path(), settings);
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

void build_entry_row(ContextId context, WarpModal* modal, Element* list, int index) {
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
            focus_warp_item(index, 1);
            rebuild();
        });
        Button* cancel = row_button(context, line, "Cancel", ButtonStyle::Tertiary);
        cancel->add_pressed_callback([index]() {
            g_state.renaming = -1;
            focus_warp_item(index, 1);
            rebuild();
        });
        modal->add_item(index, line, {input, save, cancel});
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
            focus_warp_item(index, 0);
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
        focus_warp_item(index, 0);
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
                focus_warp_item(index, 0);
                rebuild();
            },
            []() {}, ButtonStyle::Danger, ButtonStyle::Tertiary);
    });
    modal->add_item(index, line, {teleport, rename, remove});
}

void build_warp_modal(ContextId context, Element* root) {
    ensure_entries_loaded();

    WarpModal* overlay = context.create_element<WarpModal>(root);
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
        focus_warp_item(kSaveItem, 0);
        rebuild();
    });
    if (current.has_value()) {
        overlay->add_item(kSaveItem, save, {save});
    }

    ScrollContainer* scroll = context.create_element<ScrollContainer>(panel, ScrollDirection::Vertical);
    scroll->set_width(100.0f, Unit::Percent);
    // A scroll container does not grow with its contents: size it for the rows, up to a limit.
    constexpr float kRowHeight = 76.0f;
    const float rows = float(std::max<size_t>(g_state.entries.size(), 1));
    const float height = std::min(rows * kRowHeight, 420.0f);
    scroll->set_flex(0.0f, 0.0f, height, Unit::Dp);
    scroll->set_height(height, Unit::Dp);
    scroll->set_max_height(height, Unit::Dp);
    overlay->set_list(scroll);
    Element* list = column(context, scroll, 8.0f);
    if (g_state.entries.empty()) {
        context.create_element<Label>(list, "No saved positions.", theme::Typography::LabelSM);
    }

    for (int i = 0; i < int(g_state.entries.size()); i++) {
        build_entry_row(context, overlay, list, i);
    }

    if (!g_state.status.empty()) {
        context.create_element<Label>(panel, g_state.status, theme::Typography::LabelSM);
    }

    Element* footer = row(context, panel, 12.0f);
    footer->set_justify_content(JustifyContent::FlexEnd);
    Button* close = context.create_element<Button>(footer, "Close", ButtonStyle::Tertiary, ButtonSize::Medium);
    close->add_pressed_callback([]() { close_warp_modal(); });
    overlay->add_item(kCloseItem, close, {close});

    if (g_state.warp_focus_pending) {
        g_state.warp_focus_pending = false;
        overlay->restore_focus();
    }
}

// Shared layout keeps every shortcut's switch, copy and direction aligned.
Element* option_card(ContextId context, Element* parent) {
    Element* card = column(context, parent, 12.0f);
    card->set_padding(12.0f);
    card->set_border_width(theme::border::width);
    card->set_border_radius(theme::border::radius_md);
    card->set_border_color(theme::color::Border);
    return card;
}

void option_description(ContextId context, Element* line, const char* title, const char* description) {
    Element* copy = column(context, line, 4.0f);
    copy->set_flex(1.0f, 1.0f, 0.0f, Unit::Dp);
    copy->set_min_width(160.0f);
    context.create_element<Label>(copy, title, theme::Typography::LabelMD);
    auto* detail = context.create_element<Label>(copy, description, theme::Typography::LabelSM);
    detail->set_color(theme::color::TextDim);
}

void direction_picker(ContextId context, Element* line, tooie_dev_cheats::Direction direction,
    std::function<void(tooie_dev_cheats::Direction)> changed) {
    using tooie_dev_cheats::Direction;
    Element* picker = context.create_element<Element>(line);
    // Select's own minimum is 192 dp. Reserve room for its text and caret.
    picker->set_flex(0.0f, 0.0f, 224.0f, Unit::Dp);
    picker->set_width(224.0f);
    static const char* const labels[] = {"D-Pad Up", "D-Pad Down", "D-Pad Left", "D-Pad Right"};
    std::vector<SelectOption> options;
    for (Direction value : {Direction::Up, Direction::Down, Direction::Left, Direction::Right}) {
        options.emplace_back(labels[int(value)], tooie_dev_cheats::direction_name(value));
    }
    auto* select = context.create_element<Select>(picker, options, tooie_dev_cheats::direction_name(direction));
    select->set_width(100.0f, Unit::Percent);
    select->set_padding_right(48.0f);
    select->add_change_callback([changed](SelectOption& option, int) {
        if (auto value = tooie_dev_cheats::direction_from_name(option.value)) changed(*value);
    });
}

void build_cheat_row(ContextId context, Element* root, tooie_dev_cheats::Cheat cheat,
    const char* title, const char* description) {
    const auto settings = tooie_dev_cheats::settings()[cheat];
    Element* card = option_card(context, root);
    Element* line = row(context, card, 16.0f);
    line->set_flex_wrap(FlexWrap::Wrap);
    Toggle* enabled = context.create_element<Toggle>(line, ToggleSize::Medium);
    enabled->set_flex_shrink(0.0f);
    enabled->set_checked(settings.enabled);
    enabled->add_checked_callback([cheat](bool checked) {
        auto changed = tooie_dev_cheats::settings();
        changed[cheat].enabled = checked;
        store_cheats(changed);
    });
    option_description(context, line, title, description);
    direction_picker(context, line, settings.direction, [cheat](tooie_dev_cheats::Direction direction) {
        auto changed = tooie_dev_cheats::settings();
        changed[cheat].direction = direction;
        store_cheats(changed);
    });
}

void build_cheats(ContextId context, Element* root) {
    ensure_cheats_loaded();
    context.create_element<Label>(root, "Cheats", theme::Typography::Header3);
    context.create_element<Label>(root,
        "Hold R3 or your Debug Button + the selected direction. Each shortcut controls your own player.",
        theme::Typography::LabelSM);
    build_cheat_row(context, root, tooie_dev_cheats::Cheat::Fly, "Fly", "Take off as from a flight pad.");
    build_cheat_row(context, root, tooie_dev_cheats::Cheat::Blaster, "Breegull Blaster", "Switch first-person mode on or off.");
}

void build_character_selection(ContextId context, Element* root) {
    const auto settings = tooie_dev_cheats::settings();
    context.create_element<Label>(root, "Character selection", theme::Typography::Header3);
    Element* card = option_card(context, root);
    Element* line = row(context, card, 16.0f);
    line->set_flex_wrap(FlexWrap::Wrap);
    auto* toggle = context.create_element<Toggle>(line, ToggleSize::Medium);
    toggle->set_flex_shrink(0.0f);
    toggle->set_checked(settings.character_menu);
    toggle->add_checked_callback([](bool checked) {
        auto changed = tooie_dev_cheats::settings();
        changed.character_menu = checked;
        store_cheats(changed);
    });
    option_description(context, line, "Enable character selection", "Choose a form independently for each player.");
    direction_picker(context, line, settings.character_menu_direction, [](tooie_dev_cheats::Direction direction) {
        auto changed = tooie_dev_cheats::settings();
        changed.character_menu_direction = direction;
        store_cheats(changed);
    });
    context.create_element<Label>(card, "D-pad / stick: select    A: transform    B: cancel", theme::Typography::LabelSM);

    const auto states = tooie_dev_forms::menus().snapshot();
    Element* players = row(context, card, 16.0f);
    players->set_align_items(AlignItems::Stretch);
    players->set_flex_wrap(FlexWrap::Wrap);
    for (int player = 0; player < 2; ++player) {
        Element* action = column(context, players, 8.0f);
        action->set_flex(1.0f, 1.0f, 0.0f, Unit::Dp);
        action->set_min_width(240.0f);
        auto* status = context.create_element<Label>(action,
            std::string("Player ") + std::to_string(player + 1) +
                (states[player].available ? " - Ready" : " - Waiting for gameplay"), theme::Typography::LabelSM);
        status->set_color(theme::color::TextDim);
        auto* button = context.create_element<Button>(action, "Open for Player " + std::to_string(player + 1),
            ButtonStyle::Secondary, ButtonSize::Medium);
        button->set_width(100.0f, Unit::Percent);
        button->set_flex_shrink(0.0f);
        button->add_pressed_callback([player]() {
            auto changed = tooie_dev_cheats::settings();
            changed.character_menu = true;
            store_cheats(changed);
            tooie_dev_forms::menus().request_open(player);
            recompui::hide_all_contexts();
        });
    }
    auto* hint = context.create_element<Label>(card,
        "Opening returns to the game and enables selection. The player must be active. "
        "This shortcut takes priority if another cheat uses the same direction.", theme::Typography::LabelSM);
    hint->set_color(theme::color::TextDim);
}

void build_experimental(ContextId context, Element* root) {
    context.create_element<Label>(root, "Experimental", theme::Typography::Header3);
    Element* card = option_card(context, root);
    Element* line = row(context, card, 16.0f);
    Toggle* enabled = context.create_element<Toggle>(line, ToggleSize::Medium);
    enabled->set_flex_shrink(0.0f);
    enabled->set_checked(tooie_dev_cheats::settings().two_players);
    enabled->add_checked_callback([](bool checked) {
        auto changed = tooie_dev_cheats::settings();
        changed.two_players = checked;
        store_cheats(changed);
    });
    option_description(context, line, "Two players (split screen)",
        "Applies on the next map load. Paused in minigame rooms; resumes on the next eligible map. Assign controllers in General > Multiplayer.");
    const bool expanded = tooie_memory::boot.active();
    auto* memory_hint = context.create_element<Label>(card, expanded
        ? "Expanded Memory is active. Split screen uses more memory."
        : "Recommended before co-op: enable General > Expanded Memory and restart. Split screen uses more memory.",
        theme::Typography::LabelSM);
    memory_hint->set_color(expanded ? theme::color::TextDim : theme::color::Warning);
    auto* distance_hint = context.create_element<Label>(card,
        "Object Distance uses Original during story co-op.", theme::Typography::LabelSM);
    distance_hint->set_color(theme::color::TextDim);
}

void build_contents(ContextId context, Element* root) {
    g_state.focus_target = nullptr;
    g_state.focus_line = nullptr;

    context.create_element<Label>(root, "Dev Mode", theme::Typography::Header2);
    if (!g_state.unlocked) {
        context.create_element<Label>(root,
            "These tools change the game's state directly. Used the wrong way they can soft-lock the game, "
            "corrupt your progress or crash it. Save your game before using them; they are meant for testing.",
            theme::Typography::Body);
        Button* accept = context.create_element<Button>(root, "Accept and unlock", ButtonStyle::Danger, ButtonSize::Medium);
        accept->add_pressed_callback([]() {
            g_state.unlocked = true;
            tooie_dev_cheats::set_unlocked(true);
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
        focus_warp_item(0, 0);
        rebuild();
    });
    if (g_state.focus_warp_button && !g_state.warp_open) {
        g_state.focus_warp_button = false;
        g_state.focus_target = warp;
        g_state.focus_line = warp;
    }

    build_cheats(context, root);
    build_character_selection(context, root);
    build_experimental(context, root);

    if (g_state.warp_open) {
        build_warp_modal(context, root);
    }
}

class DevModeView : public Element {
    int focus_delay = 0; // updates left before focus_target is laid out and can take focus

    void build() {
        build_contents(get_current_context(), this);
        if (g_state.focus_target != nullptr) {
            focus_delay = 2;
            queue_update();
        }
    }

protected:
    void process_event(const Event& e) override {
        if (e.type != EventType::Update) {
            return;
        }

        if (g_state.dirty) {
            g_state.dirty = false;
            clear_children();
            build();
        }
        else if (g_state.focus_target != nullptr) {
            if (focus_delay > 0) {
                focus_delay--;
                queue_update();
            }
            else {
                g_state.focus_target->focus();
                g_state.focus_line->scroll_into_view(false);
                g_state.focus_target = nullptr;
                g_state.focus_line = nullptr;
            }
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
        set_min_height(0.0f);
        set_overflow_y(Overflow::Auto);
        set_overflow_x(Overflow::Hidden);
        set_padding(24.0f);
        g_state.view = this;
        g_state.view_context = get_current_context();
        build();
    }

    ~DevModeView() override {
        if (g_state.view == this) {
            g_state.view = nullptr;
            g_state.view_context = ContextId::null();
            g_state.focus_target = nullptr;
            g_state.focus_line = nullptr;
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
