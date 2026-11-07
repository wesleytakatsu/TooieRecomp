#include "tooie_startup.h"
#include "tooie_graphics_api.h"
#include "SDL.h"
#include "banjo_config.h"
#include "recompui/config.h"
#include "recompui/program_config.h"
#include "recompui/renderer.h"
#include "util/file.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <json/json.hpp>
#include <mutex>
#include <thread>
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
    if (api == "opengles")
        return "OpenGL ES";
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
    const std::string line = fields.dump() + '\n';
    // The supervisor and its worker append to the same file; one append-only write keeps each line whole.
    HANDLE file = CreateFileW(std::filesystem::u8path(path).c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                              nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file != INVALID_HANDLE_VALUE) {
        DWORD written = 0;
        WriteFile(file, line.data(), static_cast<DWORD>(line.size()), &written, nullptr);
        CloseHandle(file);
    }
#else
    const std::string line = fields.dump() + '\n';
    std::ofstream f(std::filesystem::u8path(path), std::ios::app);
    f << line;
#endif
}
namespace api = tooie_graphics_api;
bool wine() {
#ifdef _WIN32
    // Match RT64's automatic Wine preference.
    return RT64::UserConfiguration::resolveGraphicsAPI(RT64::UserConfiguration::GraphicsAPI::Automatic) ==
           RT64::UserConfiguration::GraphicsAPI::Vulkan;
#else
    return false;
#endif
}
// A backend this build has. A choice saved by another platform's build (d3d12 on Linux) is not valid here.
bool compiled(const std::string &s) {
    const auto list = api::compiled(api::current_target(), wine());
    return std::find(list.begin(), list.end(), s) != list.end();
}
bool valid(const std::string &s) { return s == "auto" || compiled(s); }
// The backend this process renders with when the host already named it (a Windows worker, or a launcher).
std::string running_backend() {
    const char *s = std::getenv("TK64_GRAPHICS_API");
    return s != nullptr && compiled(s) ? s : "";
}
std::vector<std::string> menu() {
    return api::menu(api::current_target(), wine(), api::probe_available, running_backend());
}
void set_environment(const char *name, const char *value) {
#ifdef _WIN32
    _putenv_s(name, value);
#else
    setenv(name, value, 1);
#endif
}
// A failure is recorded per API and GPU: "d3d12|NVIDIA GeForce GTX 1650", or the API alone when no GPU was named.
std::string attempt_key(const std::string &api, const std::string &gpu) { return gpu.empty() ? api : api + "|" + gpu; }
// Attempts that failed to start or crashed before proving themselves, oldest first. Auto tries them last.
std::vector<std::string> failed_attempts() {
    std::vector<std::string> result;
    const auto j = read_json(config_path());
    if (j.contains("failed_apis") && j["failed_apis"].is_array())
        for (const auto &value : j["failed_apis"]) {
            if (!value.is_string())
                continue;
            const auto key = value.get<std::string>();
            if (compiled(key.substr(0, key.find('|'))) && std::find(result.begin(), result.end(), key) == result.end())
                result.push_back(key);
        }
    return result;
}
// The API order for one GPU: an explicit API first; Auto keeps the policy order with the APIs that failed on this GPU
// moved last, so a machine where one API crashes does not show and lose a window on every launch.
std::vector<std::string> api_order(const std::string &selected, const std::string &gpu) {
    const std::string chosen = valid(selected) ? selected : "auto";
    auto order = api::candidates(api::current_target(), wine(), chosen);
    if (chosen == "auto") {
        const auto failed = failed_attempts();
        std::stable_partition(order.begin(), order.end(), [&](const std::string &key) {
            const auto attempt = attempt_key(key, key == "opengl" ? "" : gpu);
            return std::find(failed.begin(), failed.end(), attempt) == failed.end();
        });
    }
    return order;
}
// The GPU menu exists on Windows when there is a choice to make.
bool gpu_menu_listed() { return api::current_target() == api::Target::Windows && api::gpus().size() >= 2; }
// A compact menu label: the name without trademark marks, which only make the option wrap.
std::string gpu_label(std::string name) {
    for (const char *mark : {"(TM)", "(tm)", "(R)", "(r)"})
        for (size_t at = name.find(mark); at != std::string::npos; at = name.find(mark))
            name.erase(at, std::strlen(mark));
    return name;
}
std::string gpu_description() {
    const auto gpus = api::gpus();
    const std::string fast = gpus.empty() ? "" : " Here that is the " + gpu_label(gpus.front()) + ".";
    return "Auto starts on the high-performance GPU (the dedicated one in a laptop) and tries the others only if every "
           "graphics API fails there." +
           fast + " OpenGL always uses the GPU the video driver picks. Takes effect the next time the game starts.";
}
// The saved GPU when this machine still has it, otherwise "auto".
std::string gpu_choice(const std::vector<std::string> &gpus) {
    const auto graphics = read_json(recompui::file::get_app_folder_path() / "graphics.json");
    if (!graphics.contains(tooie_startup::graphics_device_option) || !graphics[tooie_startup::graphics_device_option].is_string())
        return "auto";
    const auto value = graphics[tooie_startup::graphics_device_option].get<std::string>();
    return std::find(gpus.begin(), gpus.end(), value) != gpus.end() ? value : "auto";
}
// The GPUs in the order they are tried: the chosen one first, then Windows' high-performance order (dedicated first).
// One unnamed entry when DXGI found none, which lets each API pick as before.
std::vector<std::string> gpu_order(const std::vector<std::string> &gpus) {
    if (gpus.empty())
        return {""};
    auto order = gpus;
    const auto chosen = gpu_choice(gpus);
    if (chosen != "auto")
        std::stable_partition(order.begin(), order.end(), [&](const std::string &name) { return name == chosen; });
    return order;
}
#ifdef _WIN32
HANDLE ready_event = nullptr;
HANDLE stable_event = nullptr;
// How long the game must run after Start Game before a crash stops counting against the API. Nothing can be saved yet.
#ifdef TOOIE_STARTUP_TEST
constexpr DWORD stable_after_game_start_ms = 300;
#else
constexpr DWORD stable_after_game_start_ms = 15000;
#endif
// An unhandled exception, fast fail (0xC0000409) or abort(): the process did not choose to exit.
bool crash_code(DWORD code) { return code >= 0x80000000u || code == 3; }
std::string hex_code(DWORD code) {
    char text[16];
    std::snprintf(text, sizeof(text), "0x%08lX", static_cast<unsigned long>(code));
    return text;
}
struct Failure {
    enum class Kind { Launch, Failed, Crashed, Timeout } kind;
    std::string api;
    std::string gpu;
    DWORD code;
};
// The one dialog a player sees when no API works: every API tried, what happened, and what to do about it.
std::string graphics_error_text(const std::vector<Failure> &failures, const std::string &available, bool portuguese) {
    const auto detected = [&](const std::string &key) {
        return ("," + available + ",").find("," + key + ",") != std::string::npos;
    };
    std::string text = portuguese
                           ? "O Banjo-Tooie não pôde iniciar porque nenhuma API gráfica funcionou neste computador.\n\n"
                             "APIs testadas:\n"
                           : "Banjo-Tooie could not start because no graphics API worked on this computer.\n\n"
                             "APIs tried:\n";
    for (const auto &f : failures) {
        std::string reason;
        if (!detected(f.api))
            reason = portuguese ? "não encontrada neste computador; " : "not found on this computer; ";
        switch (f.kind) {
        case Failure::Kind::Launch:
            reason += (portuguese ? "o processo não pôde ser criado (erro " : "the process could not be created (error ") +
                      std::to_string(f.code) + ")";
            break;
        case Failure::Kind::Failed:
            reason += (portuguese ? "não iniciou (código " : "could not start (code ") + std::to_string(f.code) + ")";
            break;
        case Failure::Kind::Crashed:
            reason += (portuguese ? "fechou inesperadamente (" : "closed unexpectedly (") + hex_code(f.code) + ")";
            break;
        case Failure::Kind::Timeout:
            reason += portuguese ? "não respondeu em 3 minutos" : "did not respond within 3 minutes";
            break;
        }
        const std::string where = f.gpu.empty() ? "" : (portuguese ? " na " : " on ") + f.gpu;
        text += "  \xE2\x80\xA2 " + backend_label(f.api) + where + ": " + reason + "\n";
    }
    text += portuguese
                ? "\nO driver de vídeo provavelmente não está instalado, está desatualizado ou parou de funcionar. "
                  "Instale o driver mais recente do fabricante da placa de vídeo (NVIDIA, AMD ou Intel), reinicie o "
                  "computador e tente de novo.\n\n"
                  "O jogo precisa de uma destas: DirectX 12, Vulkan 1.2, DirectX 11 (nível de recursos 11_0) ou "
                  "OpenGL 4.3."
                : "\nThe video driver is probably not installed, out of date or no longer working. Install the latest "
                  "driver from your graphics card maker (NVIDIA, AMD or Intel), restart the computer and try again.\n\n"
                  "The game needs one of: DirectX 12, Vulkan 1.2, DirectX 11 (feature level 11_0) or OpenGL 4.3.";
    return text;
}
#ifndef TOOIE_STARTUP_TEST
std::wstring widen(const std::string &utf8) {
    const int n = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring wide(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), wide.data(), n);
    return wide;
}
#endif
// Called by the supervisor after a worker exits, so the worker never writes startup.json at the same time.
void record_attempt(const std::string &api, const std::string &gpu, bool failed) {
    auto j = read_json(config_path());
    const auto before = j;
    auto list = failed_attempts();
    const auto key = attempt_key(api, gpu);
    list.erase(std::remove(list.begin(), list.end(), key), list.end());
    if (failed) {
        list.push_back(key);
        // The worker records the API it created on readiness; a crash afterwards means it did not work.
        if (j.contains("last_successful_api") && j["last_successful_api"] == api)
            j.erase("last_successful_api");
    }
    if (list.empty())
        j.erase("failed_apis");
    else
        j["failed_apis"] = list;
    if (j != before) {
        j["version"] = 1;
        write_json(config_path(), j);
    }
}
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

std::vector<std::string> tooie_startup::candidates(const std::string &selected) { return api_order(selected, ""); }
std::vector<std::pair<std::string, std::string>> tooie_startup::attempts(const std::string &selected) {
    // GPU-major: every API on the preferred GPU before any API on the next one. OpenGL cannot be pointed at a GPU (the
    // driver picks it, preferring the dedicated one through the exported hints), so it is tried once, on the first pass.
    std::vector<std::pair<std::string, std::string>> result;
    const auto gpus = gpu_order(api::gpus());
    for (size_t i = 0; i < gpus.size(); i++)
        for (const auto &backend : api_order(selected, gpus[i]))
            if (backend != "opengl")
                result.emplace_back(backend, gpus[i]);
            else if (i == 0)
                result.emplace_back(backend, "");
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
            // RT64's own Automatic choice (with its driver workarounds) only for the policy's first backend, even when
            // a failure moved it later; another worker must create the API it was given.
            set_environment("TOOIE_GRAPHICS_AUTOMATIC",
                            preference() == "auto" &&
                                    backend == api::candidates(api::current_target(), wine(), "auto").front()
                                ? "1"
                                : "");
            if (i + 2 >= argc)
                return EXIT_FAILURE;
            try {
                ready_event = reinterpret_cast<HANDLE>(std::stoull(argv[++i]));
                stable_event = reinterpret_cast<HANDLE>(std::stoull(argv[++i]));
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
            const char *device = std::getenv("TOOIE_GRAPHICS_DEVICE");
            log_event("worker_start", {{"candidate", backend}, {"gpu", device ? device : ""}});
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
        // Workers inherit what this process measured, so the menu needs no second probe.
        const auto available = api::probe_list(wine());
        set_environment("TOOIE_GRAPHICS_AVAILABLE", available.c_str());
        const auto gpus = api::gpus();
        set_environment("TOOIE_GRAPHICS_DEVICES", api::gpu_list_value(gpus).c_str());
        log_event("startup", {{"preference", selected}, {"available", available}, {"gpus", gpus}});
        std::vector<Failure> failures;
        for (const auto &[candidate, gpu] : attempts(selected)) {
            // The worker's renderer creates its device on this GPU (RT64 reads TOOIE_GRAPHICS_DEVICE).
            set_environment("TOOIE_GRAPHICS_DEVICE", gpu.c_str());
            SECURITY_ATTRIBUTES security{sizeof(security), nullptr, TRUE};
            Handle event(CreateEventW(&security, TRUE, FALSE, nullptr));
            Handle stable(CreateEventW(&security, TRUE, FALSE, nullptr));
            if (!event.value || !stable.value)
                return EXIT_FAILURE;
            std::wstring command = L"\"" + executable + L"\" --graphics-worker=" +
                                   std::wstring(candidate.begin(), candidate.end()) + L" " +
                                   std::to_wstring(reinterpret_cast<uintptr_t>(event.value)) + L" " +
                                   std::to_wstring(reinterpret_cast<uintptr_t>(stable.value));
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
            HANDLE inherited[] = {event.value, stable.value};
            const bool attributes =
                UpdateProcThreadAttribute(startup.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, inherited,
                                          sizeof(inherited), nullptr, nullptr) != 0;
            PROCESS_INFORMATION process{};
            log_event("attempt", {{"api", candidate}, {"gpu", gpu}});
            const BOOL created = attributes && CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr,
                                                              TRUE, EXTENDED_STARTUPINFO_PRESENT | CREATE_SUSPENDED,
                                                              nullptr, nullptr, &startup.StartupInfo, &process);
            DeleteProcThreadAttributeList(startup.lpAttributeList);
            if (!created) {
                const DWORD error = GetLastError();
                log_event("launch_failed", {{"api", candidate}, {"gpu", gpu}, {"win32_error", error}});
                failures.push_back({Failure::Kind::Launch, candidate, gpu, error});
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
                log_event("launcher_ready", {{"candidate", candidate}, {"gpu", gpu}});
                // A driver can accept the device and lose it on the first frames (DXGI_ERROR_DEVICE_REMOVED). Until the
                // game has run for a while, a crash means this API does not work here: show the launcher on the next.
                HANDLE running[] = {stable.value, child.value};
                const bool proven = WaitForMultipleObjects(2, running, FALSE, INFINITE) == WAIT_OBJECT_0;
                if (proven)
                    log_event("stable", {{"api", candidate}, {"gpu", gpu}});
                WaitForSingleObject(child.value, INFINITE);
                DWORD code = 1;
                GetExitCodeProcess(child.value, &code);
                if (!proven && crash_code(code)) {
                    log_event("crashed_after_ready", {{"api", candidate}, {"gpu", gpu}, {"code", code}});
                    record_attempt(candidate, gpu, true);
                    failures.push_back({Failure::Kind::Crashed, candidate, gpu, code});
                    continue;
                }
                record_attempt(candidate, gpu, false);
                log_event("exit", {{"code", code}});
                return static_cast<int>(code);
            }
            if (result != WAIT_OBJECT_0 + 1) {
                TerminateProcess(child.value, 1);
                WaitForSingleObject(child.value, 5000);
            }
            DWORD code = 1;
            GetExitCodeProcess(child.value, &code);
            log_event("attempt_failed", {{"api", candidate}, {"gpu", gpu}, {"code", code}, {"timeout", result == WAIT_TIMEOUT}});
            record_attempt(candidate, gpu, true);
            failures.push_back({result == WAIT_TIMEOUT ? Failure::Kind::Timeout
                                : crash_code(code)     ? Failure::Kind::Crashed
                                                       : Failure::Kind::Failed,
                                candidate, gpu, code});
        }
        const bool portuguese = PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_PORTUGUESE;
        const std::string title = portuguese ? "Banjo-Tooie: erro gráfico" : "Banjo-Tooie: Graphics Error";
        const std::string message = graphics_error_text(failures, available, portuguese);
        log_event("all_failed", {{"title", title}, {"message", message}});
#ifndef TOOIE_STARTUP_TEST
        MessageBoxW(nullptr, widen(message).c_str(), widen(title).c_str(), MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
#endif
        return EXIT_FAILURE;
    }
#else
    (void)argc;
    (void)argv;
    // A launcher that names a backend this build has wins. Otherwise the menu choice, when this device offers it, or the
    // target's Auto: OpenGL ES on handhelds, Vulkan on Linux unless only OpenGL is found.
    if (running_backend().empty()) {
        const auto chosen = api::effective(preference(), menu());
        const auto value = api::environment_value(api::current_target(), chosen, api::probe_available);
        set_environment("TK64_GRAPHICS_API", value.c_str());
        log_event("startup", {{"preference", preference()}, {"effective", chosen}, {"api", value}});
    }
    return -1;
#endif
}
void tooie_startup::launcher_frame() {
    if (ready.load())
        return;
#ifdef _WIN32
    if (worker) {
        // Each call follows a submitted and presented frame. Device removal (DXGI_ERROR_DEVICE_REMOVED) surfaces a few
        // frames after the GPU runs the first work, so both a frame count and a time bound must pass while hidden.
        using clock = std::chrono::steady_clock;
        constexpr int warmup_frames = 60;
        constexpr auto warmup_time = std::chrono::milliseconds(1000);
        static int frames = 0;
        static clock::time_point first;
        if (frames++ == 0)
            first = clock::now();
        if (frames < warmup_frames || clock::now() - first < warmup_time)
            return;
        log_event("warmup_done", {{"frames", frames}});
    }
#endif
    launcher_ready();
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
        "Using " + backend_label(api) + ". " + api::auto_description(api::current_target(), wine()));
    if (gpu_menu_listed())
        recompui::config::get_graphics_config().update_option_description(graphics_device_option,
                                                                          "Using " + device + ". " + gpu_description());
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
#ifdef _WIN32
    if (stable_event) {
        const HANDLE event = stable_event;
        stable_event = nullptr;
        std::thread([event]() {
            Sleep(stable_after_game_start_ms);
            SetEvent(event);
            CloseHandle(event);
        }).detach();
    }
#endif
}
void tooie_startup::add_graphics_option(recomp::config::Config &config) {
    using recomp::config::ConfigOptionEnumOption;
    // Only the backends this build has and this device can start; the values keep their meaning on every platform.
    const auto listed = menu();
    std::vector<ConfigOptionEnumOption> options;
    for (const auto &key : listed)
        options.emplace_back(api::enum_value(key), key, api::menu_label(key));
    const auto initial = api::enum_value(api::effective(preference(), listed));
    config.add_enum_option(graphics_backend_option, "Graphics API", api::auto_description(api::current_target(), wine()),
                           options, initial);
    // GPU choice: Auto, then each hardware GPU in Windows' high-performance order. The value is the GPU name, so a
    // saved choice survives a GPU being added; a GPU that is gone is shown and used as Auto.
    if (!gpu_menu_listed())
        return;
    const auto gpus = api::gpus();
    std::vector<ConfigOptionEnumOption> devices;
    devices.emplace_back(0u, "auto", "Auto");
    for (size_t i = 0; i < gpus.size(); i++)
        devices.emplace_back(static_cast<uint32_t>(i + 1), gpus[i], gpu_label(gpus[i]));
    const auto chosen = gpu_choice(gpus);
    const auto found = std::find(gpus.begin(), gpus.end(), chosen);
    const uint32_t initial_gpu = found == gpus.end() ? 0u : static_cast<uint32_t>(found - gpus.begin() + 1);
    config.add_enum_option(graphics_device_option, "GPU", gpu_description(), devices, initial_gpu);
}

#if defined(_WIN32) && !defined(TOOIE_STARTUP_TEST)
// Hybrid laptops: ask the NVIDIA and AMD drivers to run this process on the high-performance GPU. This matters for
// OpenGL and for an API whose GPU is not named; D3D12, Vulkan and D3D11 also receive the GPU by name from the supervisor.
// They must be exported by the executable itself (RT64's copy in a static library is not linked in).
extern "C" {
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif
