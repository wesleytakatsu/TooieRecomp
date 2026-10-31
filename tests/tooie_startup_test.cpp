// Real Windows supervisor/config/logs with fake renderer workers: no game, ROM, GPU or user config.
#include "SDL.h"
#include "librecomp/files.hpp"
#include "librecomp/game.hpp"
#include "recompui/program_config.h"
#include "recompui/renderer.h"
#include "tooie_startup.h"
#include "util/file.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <fstream>
#include <iostream>
#include <json/json.hpp>
#include <set>
namespace recompui::programconfig {
void set_program_name(const std::string &) {}
void set_program_id(const std::u8string &) {}
} // namespace recompui::programconfig
namespace recompui::file {
std::filesystem::path get_app_folder_path() { return std::filesystem::u8path(std::getenv("TOOIE_STARTUP_TEST_DIR")); }
} // namespace recompui::file
namespace recompui::renderer {
std::string get_graphics_backend_name() {
    const char *s = std::getenv("TK64_GRAPHICS_API");
    return s ? s : "unknown";
}
std::string get_graphics_device_name() { return "fake test renderer (not hardware validation)"; }
} // namespace recompui::renderer
std::filesystem::path recomp::get_config_path() { return recompui::file::get_app_folder_path(); }
std::ifstream recomp::open_input_backup_file(const std::filesystem::path &p, std::ios_base::openmode m) {
    return std::ifstream(p.string() + ".bak", m);
}
std::ofstream recomp::open_output_file_with_backup(const std::filesystem::path &p, std::ios_base::openmode m) {
    return std::ofstream(p, m);
}
bool recomp::finalize_output_file_with_backup(const std::filesystem::path &) { return true; }
const recomp::Version &recomp::get_project_version() {
    static recomp::Version v;
    return v;
}
int main(int argc, char **argv) {
    if (argc > 1) {
        if (tooie_startup::enter(argc, argv) != -1)
            return 21;
        const std::string backend = std::getenv("TK64_GRAPHICS_API");
        if (backend == "d3d12")
            return 22;
        if (backend == "vulkan")
            TerminateProcess(GetCurrentProcess(), 0xC0000005);
        const char *target = std::getenv("TOOIE_STARTUP_TEST_TARGET");
        if (backend != (target ? target : "d3d11"))
            return 23;
        tooie_startup::launcher_ready();
        return 0;
    }
    const auto dir =
        std::filesystem::temp_directory_path() / ("tooie-startup-test-" + std::to_string(GetCurrentProcessId()));
    std::filesystem::create_directories(dir);
    _putenv_s("TOOIE_STARTUP_TEST_DIR", dir.string().c_str());
    _putenv_s("TOOIE_GRAPHICS_LOG", (dir / "graphics.jsonl").string().c_str());
    auto fail = [&](const char *msg) {
        std::cerr << "FAIL: " << msg << "; evidence: " << dir << '\n';
        return 1;
    };
    {
        std::ofstream f(dir / "graphics.json");
        f << R"({"api_option":"Vulkan","wm_option":"Windowed"})";
    }
    if (tooie_startup::preference() != "vulkan")
        return fail("legacy migration");
    if (!tooie_startup::save_preference("d3d12"))
        return fail("save preference");
    if (tooie_startup::save_preference("bogus"))
        return fail("accepted invalid preference");
    const auto order = tooie_startup::candidates("d3d11");
    if (order.front() != "d3d11" || std::set<std::string>(order.begin(), order.end()).size() != 4)
        return fail("candidate duplication");
    if (tooie_startup::enter(argc, argv) != 0)
        return fail("supervisor did not recover");
    std::ifstream logs(dir / "graphics.jsonl");
    std::string line;
    std::vector<std::string> attempts;
    int selected = 0;
    while (std::getline(logs, line)) {
        auto j = nlohmann::json::parse(line);
        if (j["event"] == "attempt")
            attempts.push_back(j["api"]);
        if (j["event"] == "selected") {
            ++selected;
            if (j["api"] != "d3d11" || j["guest_started"] != false)
                return fail("incorrect effective metadata");
        }
        if (j["event"] == "game_start")
            return fail("guest boot before click");
    }
    logs.close();
    if (attempts != std::vector<std::string>{"d3d12", "vulkan", "d3d11"} || selected != 1)
        return fail("fallback/log handshake");
    if (tooie_startup::preference() != "d3d12")
        return fail("fallback overwrote requested API");
    nlohmann::json j;
    {
        std::ifstream f(dir / "startup.json");
        f >> j;
    }
    if (j["last_successful_api"] != "d3d11")
        return fail("effective API not persisted");
    {
        std::ofstream f(dir / "startup.json");
        f << "[]";
    }
    if (tooie_startup::preference() != "vulkan")
        return fail("corrupt config recovery");
    {
        recomp::config::Config graphics("Graphics", "graphics", true);
        tooie_startup::add_graphics_option(graphics);
        graphics.add_string_option("wm_option", "Window mode", "", "Windowed");
        if (!graphics.load_config())
            return fail("native Graphics load");
        graphics.set_option_value(tooie_startup::graphics_backend_option, uint32_t{4});
        if (tooie_startup::preference() != "vulkan")
            return fail("pending edit changed startup API");
        graphics.revert_temp_config();
        if (std::get<uint32_t>(graphics.get_temp_option_value(tooie_startup::graphics_backend_option)) != 2)
            return fail("cancel API edit");
        graphics.set_option_value(tooie_startup::graphics_backend_option, uint32_t{4});
        if (!graphics.save_config() || tooie_startup::preference() != "opengl")
            return fail("Graphics Apply not used by startup");
        nlohmann::json saved;
        {
            std::ifstream f(dir / "graphics.json");
            f >> saved;
        }
        if (saved["wm_option"] != "Windowed")
            return fail("API Apply changed window mode");
        recomp::config::Config reopened("Graphics", "graphics", true);
        tooie_startup::add_graphics_option(reopened);
        if (!reopened.load_config() ||
            std::get<uint32_t>(reopened.get_option_value(tooie_startup::graphics_backend_option)) != 4)
            return fail("reopen API preference");
        reopened.set_option_value(tooie_startup::graphics_backend_option, uint32_t{0});
        if (!reopened.save_config() || tooie_startup::preference() != "auto")
            return fail("automatic default persistence");
    }
    std::filesystem::remove(dir / "graphics.json");
    std::filesystem::remove(dir / "startup.json");
    if (tooie_startup::preference() != "auto")
        return fail("fresh installation is not automatic");
    {
        recomp::config::Config fresh("Graphics", "graphics", true);
        tooie_startup::add_graphics_option(fresh);
        if (std::get<recomp::config::ConfigOptionEnum>(fresh.get_option(tooie_startup::graphics_backend_option).variant)
                .default_value != 0)
            return fail("Graphics default is not automatic");
    }
    // Auto must reach the last compatible API even when both modern APIs and DX11 fail.
    _putenv_s("TOOIE_STARTUP_TEST_TARGET", "opengl");
    {
        std::ofstream reset_log(dir / "graphics.jsonl", std::ios::trunc);
    }
    if (tooie_startup::enter(argc, argv) != 0)
        return fail("automatic did not recover through OpenGL");
    std::vector<std::string> auto_attempts;
    int auto_selected = 0;
    {
        std::ifstream auto_log(dir / "graphics.jsonl");
        while (std::getline(auto_log, line)) {
            auto event = nlohmann::json::parse(line);
            if (event["event"] == "attempt")
                auto_attempts.push_back(event["api"]);
            if (event["event"] == "selected") {
                if (event["api"] != "opengl" || event["requested"] != "auto" || event["guest_started"] != false)
                    return fail("incorrect automatic effective API");
                ++auto_selected;
            }
        }
    }
    if (auto_attempts != tooie_startup::candidates("auto") || auto_attempts.size() != 4 || auto_selected != 1)
        return fail("automatic skipped a compatibility backend");
    std::filesystem::remove_all(dir);
    std::cout << "PASS: real Win32 supervisor recovers from failure/crash, retains ready worker and preference; guest "
                 "never started; native Graphics Apply/cancel/reopen; Auto reaches OpenGL after three failures PASS\n";
    return 0;
}
