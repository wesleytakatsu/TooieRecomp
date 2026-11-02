// Exercise the actual frontend shortcut and librecomp JSON persistence with a
// private config registry and a renderer sink; no SDL window or game is started.
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include "librecomp/files.hpp"
#include "recompui/config.h"
#include "recompui/renderer.h"

#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL window mode line %d: %s\n", __LINE__, #c); std::exit(1); } } while (0)

static std::unique_ptr<recomp::config::Config> g_graphics;
static ultramodern::renderer::GraphicsConfig applied;
static unsigned applications = 0;

namespace recompui {
bool is_steam_deck() { return false; }
namespace renderer {
RT64::UserConfiguration::Antialiasing RT64MaxMSAA() { return RT64::UserConfiguration::Antialiasing::MSAA4X; }
}
namespace config {
recomp::config::Config& get_config(const std::string& id) {
    CHECK(id == "graphics" && g_graphics);
    return *g_graphics;
}
recomp::config::Config& create_config_tab(const std::string& name, const std::string& id, bool confirmation) {
    g_graphics = std::make_unique<recomp::config::Config>(name, id, confirmation);
    return *g_graphics;
}
}
}
void ultramodern::renderer::set_graphics_config(const GraphicsConfig& config) {
    applied = config;
    ++applications;
}

void test_graphics_window_mode() {
    using namespace recompui::config;
    using ultramodern::renderer::WindowMode;
    std::filesystem::remove(recomp::get_config_path() / "graphics.json");
    auto boot = []() -> recomp::config::Config& {
        auto& config = create_graphics_tab("Graphics");
        CHECK(config.load_config());
        return config;
    };
    auto json = [] {
        nlohmann::json result;
        std::ifstream(recomp::get_config_path() / "graphics.json") >> result;
        return result;
    };

    auto& config = boot();
    config.set_option_value(graphics::options::wm_option, static_cast<uint32_t>(WindowMode::Fullscreen));
    CHECK(config.save_config());
    CHECK(json()["wm_option"] == "Fullscreen");

    // Shortcut applies and saves immediately, without applying pending edits.
    const auto rate = config.get_option_value(graphics::options::rr_manual_value);
    config.set_option_value(graphics::options::rr_manual_value, 90.0);
    const unsigned before = applications;
    graphics::toggle_fullscreen();
    CHECK(applied.wm_option == WindowMode::Windowed && applications == before + 1);
    CHECK(json()["wm_option"] == "Windowed");
    CHECK(config.get_option_value(graphics::options::rr_manual_value) == rate);
    CHECK(config.get_temp_option_value(graphics::options::rr_manual_value) == recomp::config::ConfigValueVariant{90.0});
    CHECK(config.is_dirty());

    auto& reopened = boot();
    CHECK(applied.wm_option == WindowMode::Windowed);
    CHECK(reopened.get_option_value(graphics::options::rr_manual_value) == rate);
    CHECK(!reopened.is_dirty());
    graphics::toggle_fullscreen();
    CHECK(json()["wm_option"] == "Fullscreen");
    boot();
    CHECK(applied.wm_option == WindowMode::Fullscreen);
    // The menu's Apply route keeps the same persistence contract.
    g_graphics->set_option_value(graphics::options::wm_option, static_cast<uint32_t>(WindowMode::Windowed));
    CHECK(g_graphics->save_config());
    boot();
    CHECK(applied.wm_option == WindowMode::Windowed);
    std::puts("graphics window mode persistence PASS");
}
