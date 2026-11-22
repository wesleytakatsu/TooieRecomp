// Real librecomp Config integration for Graphics > Texture Streaming: default Off, legacy/invalid configs load Off,
// persisted levels load and reach the renderer sink on load, UI changes reach it only on Apply, cancel reverts, and the
// saved JSON uses the level names. Needs librecomp/src/config.cpp + config_option.cpp.
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>
#include "tooie_texture_streaming.h"
#include "librecomp/config.hpp"
#include "librecomp/files.hpp"
#include "librecomp/game.hpp"

static std::filesystem::path g_dir;
std::filesystem::path recomp::get_config_path() { return g_dir; }
std::ifstream recomp::open_input_backup_file(const std::filesystem::path& p, std::ios_base::openmode m) { return std::ifstream(p.string() + ".bak", m); }
std::ofstream recomp::open_output_file_with_backup(const std::filesystem::path& p, std::ios_base::openmode m) { return std::ofstream(p, m); }
bool recomp::finalize_output_file_with_backup(const std::filesystem::path&) { return true; }
const recomp::Version& recomp::get_project_version() { static recomp::Version v; return v; }

#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); std::exit(1); } } while (0)

static std::vector<uint32_t> g_sunk;
static std::unique_ptr<recomp::config::Config> g_config;

static recomp::config::Config& boot(const std::string& json_text) {
    g_sunk.clear();
    std::filesystem::remove(g_dir / "graphics.json");
    if (!json_text.empty()) std::ofstream(g_dir / "graphics.json") << json_text;
    g_config = std::make_unique<recomp::config::Config>("Graphics", "graphics", true);
    tooie_texture_streaming::add_config(*g_config, [](uint32_t level) { g_sunk.push_back(level); });
    CHECK(g_config->load_config());
    return *g_config;
}

static uint32_t level(recomp::config::Config& g, bool temp = false) {
    return std::get<uint32_t>(temp ? g.get_temp_option_value(tooie_texture_streaming::option) : g.get_option_value(tooie_texture_streaming::option));
}

static nlohmann::json saved() { nlohmann::json j; std::ifstream(g_dir / "graphics.json") >> j; return j; }

int main() {
    g_dir = std::filesystem::temp_directory_path() / "tooie_texture_streaming_test";
    std::filesystem::create_directories(g_dir);

    // Fresh install and configs from before the option: Off, and the renderer is told Off.
    for (const char* text : { "", R"({"msaa_option":"MSAA2X"})" }) {
        auto& g = boot(text);
        CHECK(level(g) == tooie_texture_streaming::off);
        CHECK(g_sunk.size() <= 1);
        for (uint32_t sunk : g_sunk) CHECK(sunk == tooie_texture_streaming::off);
    }

    // Invalid values: Off.
    for (const char* bad : { R"({"texture_streaming":"Ultra"})", R"({"texture_streaming":7})", R"({"texture_streaming":null})", R"({"texture_streaming":true})" }) {
        auto& g = boot(bad);
        CHECK(level(g) == tooie_texture_streaming::off);
        for (uint32_t sunk : g_sunk) CHECK(sunk == tooie_texture_streaming::off);
    }

    // A saved level loads and reaches the renderer.
    {
        auto& g = boot(R"({"texture_streaming":"High"})");
        CHECK(level(g) == 4);
        CHECK(!g_sunk.empty() && g_sunk.back() == 4);
    }

    // UI change: nothing until Apply; Apply sends it and saves the name.
    {
        auto& g = boot("");
        const size_t before = g_sunk.size();
        g.set_option_value(tooie_texture_streaming::option, uint32_t(5));
        CHECK(level(g, true) == 5 && level(g) == 0);
        CHECK(g_sunk.size() == before);
        CHECK(g.save_config());
        CHECK(level(g) == 5);
        CHECK(!g_sunk.empty() && g_sunk.back() == 5);
        CHECK(saved()["texture_streaming"] == "Maximum");

        // Back to Off.
        g.set_option_value(tooie_texture_streaming::option, uint32_t(0));
        CHECK(g.save_config());
        CHECK(g_sunk.back() == 0 && saved()["texture_streaming"] == "Off");
    }

    // Cancel: the pending change is dropped and the renderer keeps the applied level.
    {
        auto& g = boot(R"({"texture_streaming":"Medium"})");
        const size_t before = g_sunk.size();
        g.set_option_value(tooie_texture_streaming::option, uint32_t(1));
        g.revert_temp_config();
        CHECK(level(g) == 3 && level(g, true) == 3);
        CHECK(g_sunk.size() == before);
    }

    std::printf("tooie_texture_streaming_test: PASS\n");
    return 0;
}
