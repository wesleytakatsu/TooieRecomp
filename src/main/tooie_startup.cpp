#include "tooie_startup.h"
#include "SDL.h"
#include "banjo_config.h"
#include "recompui/config.h"
#include "recompui/program_config.h"
#include "recompui/renderer.h"
#include "util/file.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <json/json.hpp>
#include <mutex>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace {
using json = nlohmann::json;
std::atomic<bool> ready{false};
bool worker = false;
std::string backend_label(const std::string &api) {
    if (api == "d3d12")
        return "DirectX 12";
    if (api == "d3d11")
        return "DirectX 11";
    if (api == "opengl")
        return "OpenGL";
    if (api == "vulkan")
        return "Vulkan";
    if (api == "metal")
        return "Metal";
    return "graphics";
}
std::mutex log_mutex;
std::filesystem::path config_path() { return recompui::file::get_app_folder_path() / "startup.json"; }
json read_json(const std::filesystem::path &path) {
    try {
        std::ifstream f(path);
        auto j = json::parse(f);
        return j.is_object() ? j : json::object();
    } catch (...) {
        return json::object();
    }
}
bool write_json(const std::filesystem::path &path, const json &value) {
    try {
        std::filesystem::create_directories(path.parent_path());
        auto temp = path;
        temp += ".tmp";
        {
            std::ofstream f(temp, std::ios::trunc);
            f << value.dump(2) << '\n';
            f.close();
            if (!f)
                return false;
        }
#ifdef _WIN32
        return MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
        std::filesystem::rename(temp, path);
        return true;
#endif
    } catch (...) {
        return false;
    }
}
void log_event(const std::string &event, json fields = json::object()) {
    const char *path = std::getenv("TOOIE_GRAPHICS_LOG");
    if (!path || !*path)
        return;
    std::lock_guard lock(log_mutex);
    fields["event"] = event;
    fields["time_ms"] =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
#ifdef _WIN32
    fields["pid"] = GetCurrentProcessId();
#endif
    std::ofstream f(std::filesystem::u8path(path), std::ios::app);
    f << fields.dump() << '\n';
}
bool valid(const std::string &s) {
    return s == "auto" || s == "d3d12" || s == "vulkan" || s == "d3d11" || s == "opengl";
}
void set_environment(const char *name, const char *value) {
#ifdef _WIN32
    _putenv_s(name, value);
#else
    setenv(name, value, 1);
#endif
}
#ifdef _WIN32
HANDLE ready_event = nullptr;
struct Handle {
    HANDLE value = nullptr;
    explicit Handle(HANDLE h = nullptr) : value(h) {}
    ~Handle() {
        if (value && value != INVALID_HANDLE_VALUE)
            CloseHandle(value);
    }
    Handle(const Handle &) = delete;
};
#endif
} // namespace

std::vector<std::string> tooie_startup::candidates(const std::string &selected) {
    std::vector<std::string> result;
    if (valid(selected) && selected != "auto")
        result.push_back(selected);
#ifdef _WIN32
    // Match RT64's automatic Wine preference.
    const bool wine = RT64::UserConfiguration::resolveGraphicsAPI(RT64::UserConfiguration::GraphicsAPI::Automatic) ==
                      RT64::UserConfiguration::GraphicsAPI::Vulkan;
    std::vector<std::string> defaults = wine ? std::vector<std::string>{"vulkan", "d3d12", "d3d11", "opengl"}
                                             : std::vector<std::string>{"d3d12", "vulkan", "d3d11", "opengl"};
#else
    std::vector<std::string> defaults = {"vulkan", "opengl"};
#endif
    for (const auto &s : defaults)
        if (std::find(result.begin(), result.end(), s) == result.end())
            result.push_back(s);
    return result;
}
std::string tooie_startup::preference() {
    auto graphics = read_json(recompui::file::get_app_folder_path() / "graphics.json");
    if (graphics.contains(graphics_backend_option)) {
        if (!graphics[graphics_backend_option].is_string())
            return "auto";
        auto value = graphics[graphics_backend_option].get<std::string>();
        return valid(value) ? value : "auto";
    }
    auto j = read_json(config_path());
    if (j.contains("preference") && j["preference"].is_string()) {
        auto s = j["preference"].get<std::string>();
        return valid(s) ? s : "auto";
    }
    auto old = read_json(recompui::file::get_app_folder_path() / "graphics.json");
    const std::string s =
        old.contains("api_option") && old["api_option"].is_string() ? old["api_option"].get<std::string>() : "Auto";
    return s == "D3D12" ? "d3d12" : s == "Vulkan" ? "vulkan" : "auto";
}
bool tooie_startup::save_preference(const std::string &s) {
    if (!valid(s))
        return false;
    auto j = read_json(config_path());
    j["version"] = 1;
    j["preference"] = s;
    return write_json(config_path(), j);
}
bool tooie_startup::is_ready() { return ready.load(); }
bool tooie_startup::is_worker() { return worker; }

int tooie_startup::enter(int argc, char **argv) {
    recompui::programconfig::set_program_name(banjo::program_name);
    recompui::programconfig::set_program_id(banjo::program_id);
#ifdef _WIN32
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    for (int i = 1; i < argc; i++) {
        const std::string arg = argv[i];
        if (arg.starts_with("--graphics-worker=")) {
            worker = true;
            const auto backend = arg.substr(18);
            if (!valid(backend) || backend == "auto")
                return EXIT_FAILURE;
            set_environment("TK64_GRAPHICS_API", backend.c_str());
            set_environment("TOOIE_GRAPHICS_STRICT", "1");
            set_environment("TOOIE_GRAPHICS_AUTOMATIC",
                            preference() == "auto" && backend == candidates("auto").front() ? "1" : "");
            if (i + 1 >= argc)
                return EXIT_FAILURE;
            try {
                ready_event = reinterpret_cast<HANDLE>(std::stoull(argv[++i]));
            } catch (...) {
                return EXIT_FAILURE;
            }
            if (const char *log = std::getenv("TOOIE_GRAPHICS_LOG"); log && *log) {
                const auto path = std::filesystem::u8path(log).parent_path() /
                                  ("worker-" + std::to_string(GetCurrentProcessId()) + ".log");
                FILE *redirected = nullptr;
                _wfreopen_s(&redirected, path.c_str(), L"a", stderr);
                _wfreopen_s(&redirected, path.c_str(), L"a", stdout);
            }
            log_event("worker_start", {{"candidate", backend}});
            return -1;
        }
        if (arg == "--launcher-music-child" || arg == "--launcher-music-control")
            return EXIT_FAILURE;
    }
    std::wstring executable(32768, L'\0');
    const DWORD n = GetModuleFileNameW(nullptr, executable.data(), static_cast<DWORD>(executable.size()));
    if (n == 0 || n >= executable.size())
        return EXIT_FAILURE;
    executable.resize(n);
    Handle job(CreateJobObjectW(nullptr, nullptr));
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!job.value || !SetInformationJobObject(job.value, JobObjectExtendedLimitInformation, &limits, sizeof(limits)))
        return EXIT_FAILURE;
    {
        const auto selected = preference();
        log_event("startup", {{"preference", selected}});
        std::string failures;
        for (const auto &candidate : candidates(selected)) {
            SECURITY_ATTRIBUTES security{sizeof(security), nullptr, TRUE};
            Handle event(CreateEventW(&security, TRUE, FALSE, nullptr));
            if (!event.value)
                return EXIT_FAILURE;
            std::wstring command = L"\"" + executable + L"\" --graphics-worker=" +
                                   std::wstring(candidate.begin(), candidate.end()) + L" " +
                                   std::to_wstring(reinterpret_cast<uintptr_t>(event.value));
            for (int i = 1; i < argc; i++)
                if (std::string(argv[i]) == "--show-console")
                    command += L" --show-console";
            STARTUPINFOEXW startup{};
            startup.StartupInfo.cb = sizeof(startup);
            SIZE_T bytes = 0;
            InitializeProcThreadAttributeList(nullptr, 1, 0, &bytes);
            std::vector<unsigned char> attrs(bytes);
            startup.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attrs.data());
            if (!InitializeProcThreadAttributeList(startup.lpAttributeList, 1, 0, &bytes))
                return EXIT_FAILURE;
            HANDLE inherited = event.value;
            const bool attributes =
                UpdateProcThreadAttribute(startup.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, &inherited,
                                          sizeof(inherited), nullptr, nullptr) != 0;
            PROCESS_INFORMATION process{};
            log_event("attempt", {{"api", candidate}});
            const BOOL created = attributes && CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr,
                                                              TRUE, EXTENDED_STARTUPINFO_PRESENT | CREATE_SUSPENDED,
                                                              nullptr, nullptr, &startup.StartupInfo, &process);
            DeleteProcThreadAttributeList(startup.lpAttributeList);
            if (!created) {
                log_event("launch_failed", {{"api", candidate}, {"win32_error", GetLastError()}});
                continue;
            }
            Handle child(process.hProcess), thread(process.hThread);
            if (!AssignProcessToJobObject(job.value, child.value)) {
                TerminateProcess(child.value, 1);
                return EXIT_FAILURE;
            }
            ResumeThread(thread.value);
            HANDLE wait[] = {event.value, child.value};
            // Includes cold shader compilation. A hung driver must not prevent recovery forever.
            const DWORD result = WaitForMultipleObjects(2, wait, FALSE, 180000);
            if (result == WAIT_OBJECT_0) {
                log_event("launcher_ready", {{"candidate", candidate}});
                WaitForSingleObject(child.value, INFINITE);
                DWORD code = 1;
                GetExitCodeProcess(child.value, &code);
                log_event("exit", {{"code", code}});
                return static_cast<int>(code);
            }
            if (result != WAIT_OBJECT_0 + 1) {
                TerminateProcess(child.value, 1);
                WaitForSingleObject(child.value, 5000);
            }
            DWORD code = 1;
            GetExitCodeProcess(child.value, &code);
            log_event("attempt_failed", {{"api", candidate}, {"code", code}, {"timeout", result == WAIT_TIMEOUT}});
            failures += candidate + ": " +
                        (result == WAIT_TIMEOUT ? "initialization timed out"
                                                : "initialization failed (" + std::to_string(code) + ")") +
                        "\n";
        }
        MessageBoxA(nullptr,
                    ("No compatible graphics backend could start.\n\n" + failures +
                     "\nRequires Direct3D 11 FL 11_0, OpenGL 4.3, or a compatible DX12/Vulkan device and driver.")
                        .c_str(),
                    "Banjo-Tooie: Graphics Error", MB_OK | MB_ICONERROR);
        return EXIT_FAILURE;
    }
#else
    (void)argc;
    (void)argv;
    const auto s = preference();
    set_environment("TK64_GRAPHICS_API", s == "opengl" ? "opengl" : "");
    return -1;
#endif
}
void tooie_startup::launcher_ready() {
    static bool done = false;
    if (done)
        return;
    done = true;
    const std::string api = recompui::renderer::get_graphics_backend_name();
    const std::string device = recompui::renderer::get_graphics_device_name();
#ifndef TOOIE_STARTUP_TEST
    recompui::config::get_graphics_config().update_option_description(
        graphics_backend_option,
        "Using " + backend_label(api) +
            ". Auto tries DX12, Vulkan, DX11, then OpenGL until one works. Changes apply on next launch.");
#endif
    log_event("selected", {{"requested", preference()}, {"api", api}, {"device", device}, {"guest_started", false}});
    auto j = read_json(config_path());
    j["version"] = 1;
    j["preference"] = preference();
    j["last_successful_api"] = api;
    write_json(config_path(), j);
    ready.store(true);
#ifdef _WIN32
    if (ready_event) {
        SetEvent(ready_event);
        CloseHandle(ready_event);
        ready_event = nullptr;
    }
#endif
}
void tooie_startup::note_game_start() {
    log_event("game_start", {{"api", recompui::renderer::get_graphics_backend_name()},
                             {"device", recompui::renderer::get_graphics_device_name()}});
}
void tooie_startup::add_graphics_option(recomp::config::Config &config) {
    using recomp::config::ConfigOptionEnumOption;
    const std::vector<std::string> keys = {"auto", "d3d12", "vulkan", "d3d11", "opengl"};
    std::vector<ConfigOptionEnumOption> options;
#ifdef _WIN32
    options = {{0u, "auto", "Auto"},
               {1u, "d3d12", "DX12"},
               {2u, "vulkan", "Vulkan"},
               {3u, "d3d11", "DX11"},
               {4u, "opengl", "OpenGL"}};
#else
    options = {{0u, "auto", "Auto"}, {2u, "vulkan", "Vulkan"}, {4u, "opengl", "OpenGL"}};
#endif
    const auto selected = preference();
    auto found = std::find(keys.begin(), keys.end(), selected);
    const auto initial = found == keys.end() ? 0u : static_cast<uint32_t>(std::distance(keys.begin(), found));
    config.add_enum_option(graphics_backend_option, "Graphics API",
                           "Auto tries DX12, Vulkan, DX11, then OpenGL until one works. Changes apply on next launch.",
                           options, initial);
}
