#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <cassert>
#include <unordered_map>
#include <vector>
#include <array>
#include <filesystem>
#include <numeric>
#include <stdexcept>
#include <cinttypes>
#include <atomic>
#include <chrono>
#include <thread>

#if defined(__linux__)
#include <csignal>
#include <limits.h>
#include <spawn.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;
#endif

#include "nfd.h"

#include "ultramodern/ultra64.h"
#include "ultramodern/ultramodern.hpp"
#include "ultramodern/config.hpp"
#define SDL_MAIN_HANDLED
#ifdef _WIN32
#include "SDL.h"
#else
#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"
// Undefine x11 macros that get included by SDL_syswm.h.
#undef None
#undef Status
#undef LockMask
#undef ControlMask
#undef Success
#undef Always
#endif

#include "recompui/recompui.h"
#include "recompui/program_config.h"
#include "recompui/renderer.h"
#include "recompui/config.h"
#include "util/file.h"
#include "recompinput/input_events.h"
#include "recompinput/recompinput.h"
#include "recompinput/profiles.h"
#include "banjo_config.h"
#include "banjo_sound.h"
#include "banjo_support.h"
#include "banjo_game.h"
#include "banjo_launcher.h"
#include "tooie_diag.h"
#include "recomp_data.h"
#include "ovl_patches.hpp"
#include "theme.h"
#include "librecomp/game.hpp"
#include "librecomp/mods.hpp"
#include "librecomp/helpers.hpp"

#include "../../patches/graphics.h"
#include "../../patches/input.h"
#include "../../patches/sound.h"
#include "../../patches/misc_funcs.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <timeapi.h>
#include "SDL_syswm.h"
#define APP_ICON_B 1
#define APP_ICON_K 2
#endif

#include "stb/stb_image.h"

const std::string version_string = "0.1.0-p08";

namespace {

bool launcher_music_child = false;
bool launcher_music_preview_started = false;

#if defined(__linux__)
std::atomic<pid_t> launcher_music_pid{-1};

void stop_launcher_music_preview() {
    const pid_t child = launcher_music_pid.exchange(-1);
    if (child <= 0) {
        return;
    }

    int status = 0;
    pid_t wait_result = 0;
    do {
        wait_result = waitpid(child, &status, WNOHANG);
    } while (wait_result < 0 && errno == EINTR);
    if (wait_result == child || (wait_result < 0 && errno == ECHILD)) {
        return;
    }
    if (wait_result < 0) {
        std::fprintf(stderr, "[tooie] launcher music: waitpid failed (%d)\n", errno);
        return;
    }

    if (kill(child, SIGTERM) != 0 && errno != ESRCH) {
        std::fprintf(stderr, "[tooie] launcher music: SIGTERM failed (%d)\n", errno);
    }

    // Reap the helper so a normal Start Game does not leave a zombie for the
    // lifetime of the visible game process. Give graceful termination one
    // second, then force only this known child to exit.
    for (int attempt = 0; attempt < 100; attempt++) {
        wait_result = waitpid(child, &status, WNOHANG);
        if (wait_result == child || (wait_result < 0 && errno == ECHILD)) {
            return;
        }
        if (wait_result < 0 && errno != EINTR) {
            std::fprintf(stderr, "[tooie] launcher music: reap failed (%d)\n", errno);
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (kill(child, SIGKILL) == 0 || errno == ESRCH) {
        do {
            wait_result = waitpid(child, &status, 0);
        } while (wait_result < 0 && errno == EINTR);
    }
}

bool start_launcher_music_preview() {
    std::array<char, PATH_MAX> executable{};
    const ssize_t length = readlink("/proc/self/exe", executable.data(), executable.size() - 1);
    if (length <= 0 || static_cast<size_t>(length) >= executable.size()) {
        std::fprintf(stderr, "[tooie] launcher music: failed to resolve executable\n");
        return false;
    }
    executable[static_cast<size_t>(length)] = '\0';

    char child_flag[] = "--launcher-music-child";
    char* child_argv[] = {executable.data(), child_flag, nullptr};
    pid_t child = -1;
    const int result = posix_spawn(&child, executable.data(), nullptr, nullptr, child_argv, environ);
    if (result != 0) {
        std::fprintf(stderr, "[tooie] launcher music: posix_spawn failed (%d)\n", result);
        return false;
    }

    launcher_music_pid.store(child);
    launcher_music_preview_started = true;
    std::atexit(stop_launcher_music_preview);

    // start_game is single-shot in ModernRuntime. Keep the preview in its own
    // process, then stop it as soon as the visible instance starts from boot.
    std::thread([child]() {
        while (!ultramodern::is_game_started()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        if (launcher_music_pid.load() == child) {
            std::fprintf(stderr, "[tooie] launcher music: restarting with visible game boot\n");
            stop_launcher_music_preview();
        }
    }).detach();

    std::fprintf(stderr, "[tooie] launcher music: hidden ROM-backed preview started\n");
    return true;
}
#else
void stop_launcher_music_preview() {}
bool start_launcher_music_preview() { return false; }
#endif

} // namespace

template<typename... Ts>
void exit_error(const char* str, Ts ...args) {
    // TODO pop up an error
    ((void)fprintf(stderr, str, args), ...);
    assert(false);
        
    ultramodern::error_handling::quick_exit(__FILE__, __LINE__, __FUNCTION__);
}

ultramodern::gfx_callbacks_t::gfx_data_t create_gfx() {
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
    SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, "0");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS4_RUMBLE, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5_RUMBLE, "1");
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC) > 0) {
        exit_error("Failed to initialize SDL2: %s\n", SDL_GetError());
    }

    fprintf(stdout, "SDL Video Driver: %s\n", SDL_GetCurrentVideoDriver());

    return {};
}

ultramodern::input::connected_device_info_t get_connected_device_info(int controller_num) {
    if (recompinput::players::is_single_player_mode() || recompinput::players::get_player_is_assigned(controller_num)) {
        return ultramodern::input::connected_device_info_t{
            .connected_device = ultramodern::input::Device::Controller,
            .connected_pak = ultramodern::input::Pak::RumblePak,
        };
    }

    return ultramodern::input::connected_device_info_t{
        .connected_device = ultramodern::input::Device::None,
        .connected_pak = ultramodern::input::Pak::None,
    };
}

#include "icon_bytes.h"

#if defined(__gnu_linux__)
bool SetImageAsIcon(const char* filename, SDL_Window* window)
{
    // Read data
    int width, height, bytesPerPixel;
    if (sizeof(icon_bytes) <= 1) {
        return false;
    }
    void* data = stbi_load_from_memory(reinterpret_cast<const uint8_t*>(icon_bytes), sizeof(icon_bytes), &width, &height, &bytesPerPixel, 4);

    // Calculate pitch
    int pitch;
    pitch = width * 4;
    pitch = (pitch + 3) & ~3;

    // Setup relevance bitmask
    int Rmask, Gmask, Bmask, Amask;

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    Rmask = 0x000000FF;
    Gmask = 0x0000FF00;
    Bmask = 0x00FF0000;
    Amask = 0xFF000000;
#else
    Rmask = 0xFF000000;
    Gmask = 0x00FF0000;
    Bmask = 0x0000FF00;
    Amask = 0x000000FF;
#endif

    SDL_Surface* surface = nullptr;
    if (data != nullptr) {
        surface = SDL_CreateRGBSurfaceFrom(data, width, height, 32, pitch, Rmask, Gmask,
                            Bmask, Amask);
    }

    if (surface == nullptr) {   
        if (data != nullptr) {
            stbi_image_free(data);
        }
        return false;
	} else {
        SDL_SetWindowIcon(window,surface);
        SDL_FreeSurface(surface);
        stbi_image_free(data);
        return true;
    }
}
#endif

SDL_Window* window;

ultramodern::renderer::WindowHandle create_window(ultramodern::gfx_callbacks_t::gfx_data_t) {
    uint32_t flags = SDL_WINDOW_RESIZABLE;

    if (launcher_music_child) {
        flags |= SDL_WINDOW_HIDDEN;
    }

#if defined(__APPLE__)
    flags |= SDL_WINDOW_METAL;
#elif defined(RT64_SDL_WINDOW_VULKAN)
    flags |= SDL_WINDOW_VULKAN;
#endif

    window = SDL_CreateWindow(banjo::program_name.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1600, 900, flags);

    if (window == nullptr) {
        exit_error("Failed to create window: %s\n", SDL_GetError());
    }

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window, &wmInfo);

#if defined(_WIN32)
    // There's a 50/50 chance to choose the icon where the smallest variant is either Banjo or Kazooie alone.
    bool choose_kazooie_icon = (rand() % 2 == 0);
    HICON new_icon = LoadIcon(GetModuleHandle(NULL), choose_kazooie_icon ? MAKEINTRESOURCE(APP_ICON_K) : MAKEINTRESOURCE(APP_ICON_B));
    SendMessage(wmInfo.info.win.window, WM_SETICON, ICON_SMALL2, (LPARAM)(new_icon));
#elif defined(__linux__)
    SetImageAsIcon("icons/app.png", window);
#endif

#if defined(_WIN32)
    return ultramodern::renderer::WindowHandle{ wmInfo.info.win.window, GetCurrentThreadId() };
#elif defined(__linux__) || defined(__ANDROID__)
    return ultramodern::renderer::WindowHandle{ window };
#elif defined(__APPLE__)
    SDL_MetalView view = SDL_Metal_CreateView(window);
    return ultramodern::renderer::WindowHandle{ wmInfo.info.cocoa.window,  SDL_Metal_GetLayer(view) };
#else
    static_assert(false && "Unimplemented");
#endif
}

void update_gfx(void*) {
    recompinput::handle_events();
    tooie_diag::note_host_update_gfx();
    tooie_diag::tick();
}

static SDL_AudioCVT audio_convert;
static SDL_AudioDeviceID audio_device = 0;

// Samples per channel per second.
static uint32_t sample_rate = 48000;
static uint32_t output_sample_rate = 48000;
// Channel count.
constexpr uint32_t input_channels = 2;
static uint32_t output_channels = 2;

// Terminology: a frame is a collection of samples for each channel. e.g. 2 input samples is one input frame. This is unrelated to graphical frames.

// Number of frames to duplicate for fixing interpolation at the start and end of a chunk.
constexpr uint32_t duplicated_input_frames = 4;
// The number of output frames to skip for playback (to avoid playing duplicate inputs twice).
static uint32_t discarded_output_frames;

constexpr uint32_t bytes_per_frame = input_channels * sizeof(float);

void queue_samples(int16_t* audio_data, size_t sample_count) {
    // Buffer for holding the output of swapping the audio channels. This is reused across
    // calls to reduce runtime allocations.
    static std::vector<float> swap_buffer;
    static std::array<float, duplicated_input_frames * input_channels> duplicated_sample_buffer;

    // Make sure the swap buffer is large enough to hold the audio data, including any extra space needed for resampling.
    size_t resampled_sample_count = sample_count + duplicated_input_frames * input_channels;
    size_t max_sample_count = std::max(resampled_sample_count, resampled_sample_count * audio_convert.len_mult);
    if (max_sample_count > swap_buffer.size()) {
        swap_buffer.resize(max_sample_count);
    }
    
    // Copy the duplicated frames from last chunk into this chunk
    for (size_t i = 0; i < duplicated_input_frames * input_channels; i++) {
        swap_buffer[i] = duplicated_sample_buffer[i];
    }

    // Convert the audio from 16-bit values to floats and swap the audio channels into the
    // swap buffer to correct for the address xor caused by endianness handling.
    float cur_main_volume = static_cast<float>(recompui::config::sound::get_main_volume()) / 100.0f; // Get the current main volume, normalized to 0.0-1.0.
    for (size_t i = 0; i < sample_count; i += input_channels) {
        swap_buffer[i + 0 + duplicated_input_frames * input_channels] = audio_data[i + 1] * (0.5f / 32768.0f) * cur_main_volume;
        swap_buffer[i + 1 + duplicated_input_frames * input_channels] = audio_data[i + 0] * (0.5f / 32768.0f) * cur_main_volume;
    }
    
    // TODO handle cases where a chunk is smaller than the duplicated frame count.
    assert(sample_count > duplicated_input_frames * input_channels);

    // Copy the last converted samples into the duplicated sample buffer to reuse in resampling the next queued chunk.
    for (size_t i = 0; i < duplicated_input_frames * input_channels; i++) {
        duplicated_sample_buffer[i] = swap_buffer[i + sample_count];
    }
    
    audio_convert.buf = reinterpret_cast<Uint8*>(swap_buffer.data());
    audio_convert.len = (sample_count + duplicated_input_frames * input_channels) * sizeof(swap_buffer[0]);

    int ret = SDL_ConvertAudio(&audio_convert);

    if (ret < 0) {
        printf("Error using SDL audio converter: %s\n", SDL_GetError());
        throw std::runtime_error("Error using SDL audio converter");
    }

    uint64_t cur_queued_microseconds = uint64_t(SDL_GetQueuedAudioSize(audio_device)) / bytes_per_frame * 1000000 / sample_rate;
    uint32_t num_bytes_to_queue = audio_convert.len_cvt - output_channels * discarded_output_frames * sizeof(swap_buffer[0]);
    float* samples_to_queue = swap_buffer.data() + output_channels * discarded_output_frames / 2;

    // Prevent audio latency from building up by skipping samples in incoming audio when too many samples are already queued.
    // Skip samples based on how many microseconds of samples are queued already.
    uint32_t skip_factor = cur_queued_microseconds / 100000;
    if (skip_factor != 0) {
        uint32_t skip_ratio = 1 << skip_factor;
        num_bytes_to_queue /= skip_ratio;
        for (size_t i = 0; i < num_bytes_to_queue / (output_channels * sizeof(swap_buffer[0])); i++) {
            samples_to_queue[2 * i + 0] = samples_to_queue[2 * skip_ratio * i + 0];
            samples_to_queue[2 * i + 1] = samples_to_queue[2 * skip_ratio * i + 1];
        }
    }

    // Queue the swapped audio data.
    // Offset the data start by only half the discarded frame count as the other half of the discarded frames are at the end of the buffer.
    SDL_QueueAudio(audio_device, samples_to_queue, num_bytes_to_queue);
    int16_t peak = 0;
    size_t nonzero = 0;
    for (size_t i = 0; i < sample_count; ++i) {
        int16_t s = audio_data[i];
        if (s != 0) {
            ++nonzero;
            int16_t a = s < 0 ? static_cast<int16_t>(-s) : s;
            if (a > peak) {
                peak = a;
            }
        }
    }
    tooie_diag::note_queue_samples(
        sample_count,
        static_cast<uint32_t>(SDL_GetQueuedAudioSize(audio_device)),
        peak,
        nonzero);
}

size_t get_frames_remaining() {
    constexpr float buffer_offset_frames = 1.0f;
    // Get the number of remaining buffered audio bytes.
    uint64_t buffered_byte_count = SDL_GetQueuedAudioSize(audio_device);

    // Scale the byte count based on the ratio of sample rates and channel counts.
    buffered_byte_count = buffered_byte_count * 2 * sample_rate / output_sample_rate / output_channels;

    // Adjust the reported count to be some number of refreshes in the future, which helps ensure that
    // there are enough samples even if the audio thread experiences a small amount of lag. This prevents
    // audio popping on games that use the buffered audio byte count to determine how many samples
    // to generate.
    uint32_t frames_per_vi = (sample_rate / 60);
    if (buffered_byte_count > (buffer_offset_frames * bytes_per_frame * frames_per_vi)) {
        buffered_byte_count -= (buffer_offset_frames * bytes_per_frame * frames_per_vi);
    }
    else {
        buffered_byte_count = 0;
    }
    // Convert from byte count to sample count.
    return static_cast<uint32_t>(buffered_byte_count / bytes_per_frame);
}

void update_audio_converter() {
    int ret = SDL_BuildAudioCVT(&audio_convert, AUDIO_F32, input_channels, sample_rate, AUDIO_F32, output_channels, output_sample_rate);

    if (ret < 0) {
        printf("Error creating SDL audio converter: %s\n", SDL_GetError());
        throw std::runtime_error("Error creating SDL audio converter");
    }

    // Calculate the number of samples to discard based on the sample rate ratio and the duplicate frame count.
    discarded_output_frames = duplicated_input_frames * output_sample_rate / sample_rate;
}

void set_frequency(uint32_t freq) {
    sample_rate = freq;
    
    update_audio_converter();
}

bool reset_audio(uint32_t output_freq) {
    SDL_AudioSpec spec_desired{
        .freq = (int)output_freq,
        .format = AUDIO_F32,
        .channels = (Uint8)output_channels,
        .silence = 0, // calculated
        .samples = 0x100, // Fairly small sample count to reduce the latency of internal buffering
        .padding = 0, // unused
        .size = 0, // calculated
        .callback = nullptr,
        .userdata = nullptr
    };

    audio_device = SDL_OpenAudioDevice(nullptr, false, &spec_desired, nullptr, 0);
    if (audio_device == 0) {
        std::string audio_error = std::string("No audio device could be found. Please make sure an audio device is available.\nError opening audio device: ") + std::string(SDL_GetError());
        recompui::message_box(audio_error.c_str());
        return false;
    }

    SDL_PauseAudioDevice(audio_device, 0);

    output_sample_rate = output_freq;
    update_audio_converter();

    // nullptr device name = SDL default (PipeWire/Pulse default sink on Linux).
    const char* driver = SDL_GetCurrentAudioDriver();
    const char* dev_name = SDL_GetAudioDeviceName(-1, 0); // default output
    tooie_diag::note_audio_device(static_cast<uint32_t>(audio_device),
                                   driver ? driver : "?",
                                   dev_name ? dev_name : "(default)");

    return true;
}

extern RspUcodeFunc n_aspMain;

// P07: strict RSP dispatch. Graphics (M_GFXTASK) is HLE'd by RT64 via
// ultramodern::submit_rsp_task and must never reach this callback. Only
// M_AUDTASK is recompiled (Tooie n_aspMain from decompressed ROM).
// Set TOOIE_LOG_RSP=1 to inventory every non-gfx task that hits the SP path.
RspUcodeFunc* get_rsp_microcode(const OSTask* task) {
    static const bool log_rsp = [] {
        const char* e = std::getenv("TOOIE_LOG_RSP");
        return e != nullptr && e[0] != '\0' && e[0] != '0';
    }();

    if (log_rsp) {
        std::fprintf(stderr,
            "[rsp] OSTask type=%" PRIu32 " flags=%" PRIu32
            " ucode=0x%08" PRIx32 " ucode_data=0x%08" PRIx32
            " data_ptr=0x%08" PRIx32 " data_size=%" PRIu32 "\n",
            task->t.type,
            task->t.flags,
            static_cast<uint32_t>(task->t.ucode),
            static_cast<uint32_t>(task->t.ucode_data),
            static_cast<uint32_t>(task->t.data_ptr),
            task->t.data_size);
    }

    switch (task->t.type) {
    case M_AUDTASK:
        tooie_diag::note_rsp_audtask();
        return n_aspMain;

    case M_GFXTASK:
        // Routing bug if we get here: gfx is handled by the RT64 action queue.
        std::fprintf(stderr,
            "[rsp] unexpected M_GFXTASK in get_rsp_microcode "
            "(ucode=0x%08" PRIx32 " ucode_data=0x%08" PRIx32 ") — "
            "expected RT64 HLE path\n",
            static_cast<uint32_t>(task->t.ucode),
            static_cast<uint32_t>(task->t.ucode_data));
        return nullptr;

    default:
        std::fprintf(stderr,
            "[rsp] unknown OSTask type=%" PRIu32 " flags=%" PRIu32
            " ucode=0x%08" PRIx32 " ucode_data=0x%08" PRIx32
            " data_ptr=0x%08" PRIx32 " data_size=%" PRIu32
            " (no recompiled microcode registered)\n",
            task->t.type,
            task->t.flags,
            static_cast<uint32_t>(task->t.ucode),
            static_cast<uint32_t>(task->t.ucode_data),
            static_cast<uint32_t>(task->t.data_ptr),
            task->t.data_size);
        return nullptr;
    }
}

extern "C" void recomp_entrypoint(uint8_t * rdram, recomp_context * ctx);
gpr get_entrypoint_address();

// array of supported GameEntry objects
std::vector<recomp::GameEntry> supported_games = {
    {
        // XXH3-64 of banjo-tooie-Recomp/decompressed.us.z64 (recompiler layout).
        // Cart baserom is 0x00F70DE5F2D70EA2 — cart→decomp integration is follow-up.
        .rom_hash = 0x9F843AD3455EA395ULL,
        .internal_name = "BANJO TOOIE",
        .display_name = "Banjo-Tooie",
        .game_id = u8"bt.n64.us.1.0",
        .mod_game_id = "bt",
        .save_type = recomp::SaveType::Eep16k,
        .thumbnail_bytes = std::span<const char>(icon_bytes, sizeof(icon_bytes)),
        .is_enabled = true,
        .decompression_routine = banjo::decompress_bk,
        .has_compressed_code = false,
        .entrypoint_address = get_entrypoint_address(),
        .entrypoint = recomp_entrypoint,
        .on_init_callback = banjo::bk_on_init,
    },
};

// TODO: move somewhere else
namespace banjo {
    std::string get_game_thread_name(const OSThread* t) {
        std::string name = "[Game] ";

        switch (t->id) {
            case 0:
                switch (t->priority) {
                    case 150:
                        name += "PIMGR";
                        break;

                    case 80:
                        name += "VIMGR";
                        break;

                    default:
                        name += std::to_string(t->id);
                        break;
                }
                break;
            // Tooie THREAD_ID_* (include/threads.h) — not Banjo-Kazooie IDs.
            case 1:
                name += "IDLE";
                break;
            case 2:
                name += "TIDY";
                break;
            case 3:
                name += "CIC6105";
                break;
            case 4:
                name += "AUDIO";
                break;
            case 5:
                name += "SCHEDULER";
                break;
            case 6:
                name += "MAIN";
                break;
            case 7:
                name += "JOY";
                break;
            case 8:
                name += "MOTOR";
                break;
            case 9:
                name += "CRASH";
                break;
            default:
                name += std::to_string(t->id);
                break;
        }

        return name;
    }
}

#ifdef _WIN32

struct PreloadContext {
    HANDLE handle;
    HANDLE mapping_handle;
    SIZE_T size;
    PVOID view;
};

bool preload_executable(PreloadContext& context) {
    wchar_t module_name[MAX_PATH];
    GetModuleFileNameW(NULL, module_name, MAX_PATH);

    context.handle = CreateFileW(module_name, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (context.handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Failed to load executable into memory!");
        context = {};
        return false;
    }

    LARGE_INTEGER module_size;
    if (!GetFileSizeEx(context.handle, &module_size)) {
        fprintf(stderr, "Failed to get size of executable!");
        CloseHandle(context.handle);
        context = {};
        return false;
    }

    context.size = module_size.QuadPart;

    context.mapping_handle = CreateFileMappingW(context.handle, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (context.mapping_handle == nullptr) {
        fprintf(stderr, "Failed to create file mapping of executable!");
        CloseHandle(context.handle);
        context = {};
        return EXIT_FAILURE;
    }

    context.view = MapViewOfFile(context.mapping_handle, FILE_MAP_READ, 0, 0, 0);
    if (context.view == nullptr) {
        fprintf(stderr, "Failed to map view of of executable!");
        CloseHandle(context.mapping_handle);
        CloseHandle(context.handle);
        context = {};
        return false;
    }

    DWORD pid = GetCurrentProcessId();
    HANDLE process_handle = OpenProcess(PROCESS_SET_QUOTA | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (process_handle == nullptr) {
        fprintf(stderr, "Failed to open own process!");
        CloseHandle(context.mapping_handle);
        CloseHandle(context.handle);
        context = {};
        return false;
    }

    SIZE_T minimum_set_size, maximum_set_size;
    if (!GetProcessWorkingSetSize(process_handle, &minimum_set_size, &maximum_set_size)) {
        fprintf(stderr, "Failed to get working set size!");
        CloseHandle(context.mapping_handle);
        CloseHandle(context.handle);
        context = {};
        return false;
    }

    if (!SetProcessWorkingSetSize(process_handle, minimum_set_size + context.size, maximum_set_size + context.size)) {
        fprintf(stderr, "Failed to set working set size!");
        CloseHandle(context.mapping_handle);
        CloseHandle(context.handle);
        context = {};
        return false;
    }

    if (VirtualLock(context.view, context.size) == 0) {
        fprintf(stderr, "Failed to lock view of executable! (Error: %08lx)\n", GetLastError());
        CloseHandle(context.mapping_handle);
        CloseHandle(context.handle);
        context = {};
        return false;
    }
    
    return true;
}

void release_preload(PreloadContext& context) {
    VirtualUnlock(context.view, context.size);
    CloseHandle(context.mapping_handle);
    CloseHandle(context.handle);
    context = {};
}

#elif defined(__linux__) || defined(APPLE)

struct PreloadContext {

};

bool preload_executable(PreloadContext& context) {
    // Preloading isn't implemented on Linux and MacOS, but it's also unnecessary there, as the OS already preloads the executable.
    // Therefore, we can just consider the executable to be preloaded.
    return true;
}

void release_preload(PreloadContext& context) {
}

#else

struct PreloadContext {};

bool preload_executable(PreloadContext& context) {
    return false;
}

void release_preload(PreloadContext& context) {
}

#endif

void enable_texture_pack(recomp::mods::ModContext& context, const recomp::mods::ModHandle& mod) {
    recompui::renderer::enable_texture_pack(context, mod);
}

void disable_texture_pack(recomp::mods::ModContext&, const recomp::mods::ModHandle& mod) {
    recompui::renderer::disable_texture_pack(mod);
}

void reorder_texture_pack(recomp::mods::ModContext&) {
    recompui::renderer::trigger_texture_pack_update();
}

void on_launcher_init(recompui::LauncherMenu *menu) {
    auto game_options_menu = menu->init_game_options_menu(
        supported_games[0].game_id,
        supported_games[0].mod_game_id,
        supported_games[0].display_name,
        supported_games[0].thumbnail_bytes,
        recompui::GameOptionsMenuLayout::Center
    );

    game_options_menu->add_default_options();
    game_options_menu->set_width(30, recompui::Unit::Percent);

    for (auto option : game_options_menu->get_options()) {
        option->set_justify_content(recompui::JustifyContent::FlexEnd);
        option->set_border_radius(0);

        std::vector<recompui::Style *> hover_focus = {&option->hover_style, &option->focus_style};
        for (auto style : hover_focus) {
            style->set_background_color(recompui::theme::color::Transparent);
        }
    }

    recompui::Element *menu_container = menu->get_menu_container();
    menu_container->set_width(1440);
    menu_container->unset_left();
    menu_container->set_top(banjo::launcher_options_top_offset);
    menu_container->set_bottom(-banjo::launcher_options_top_offset);
    menu_container->set_right(50, recompui::Unit::Percent);
    menu_container->set_translate_2D(50.0f, 0.0f, recompui::Unit::Percent);

    game_options_menu->unset_left();
    game_options_menu->set_bottom(50.0f, recompui::Unit::Percent);
    game_options_menu->set_translate_2D(0.0f, 50.0f, recompui::Unit::Percent);
    game_options_menu->set_right(banjo::launcher_options_right_position_start);

    menu->remove_default_title();

    banjo::launcher_animation_setup(menu);
}

#define REGISTER_FUNC(name) recomp::overlays::register_base_export(#name, name)

// PATCH 4: Tooie-scoped Hybrid F' activation. Startup/quiescent only.
// Unset → ON. Explicit false values → OFF for the whole process.
// Not a public hot-toggle. Guest OSThreads must not exist yet.
static bool env_is_explicit_false(const char* raw) {
    if (raw == nullptr || raw[0] == '\0') {
        return false;
    }
    if (raw[0] == '0' && raw[1] == '\0') {
        return true;
    }
    char folded[8]{};
    size_t n = 0;
    for (; raw[n] != '\0' && n + 1 < sizeof(folded); ++n) {
        const unsigned char c = static_cast<unsigned char>(raw[n]);
        folded[n] = static_cast<char>((c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c);
    }
    if (raw[n] != '\0') {
        return false;
    }
    return std::strcmp(folded, "false") == 0 ||
           std::strcmp(folded, "off") == 0 ||
           std::strcmp(folded, "no") == 0;
}

static void activate_tooie_guest_preemption() {
    const char* raw = std::getenv("ULTRAMODERN_GUEST_PREEMPTION");
    const bool enabled = !env_is_explicit_false(raw);
    ultramodern::set_guest_preemption_safepoints(enabled);
    std::fprintf(stderr, "[tooie] guest preemption: %s (ULTRAMODERN_GUEST_PREEMPTION=%s)\n",
                 enabled ? "ON" : "OFF",
                 raw == nullptr ? "<unset>" : raw);
}

int main(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--launcher-music-child") == 0) {
            launcher_music_child = true;
        }
    }

#if defined(__linux__)
    if (launcher_music_child) {
        // Never leave an invisible audio/runtime process behind if the launcher
        // is closed or crashes before Start Game is selected.
        const pid_t launcher_parent = getppid();
        // A crashed/killed parent cannot perform the normal SIGTERM + reap
        // sequence, so use SIGKILL for the kernel-owned orphan safeguard.
        if (launcher_parent <= 1 || prctl(PR_SET_PDEATHSIG, SIGKILL) != 0) {
            std::fprintf(stderr, "[tooie] launcher music: failed to bind child lifetime\n");
            return EXIT_FAILURE;
        }
        // Close the fork-to-prctl race: the parent may have exited between
        // posix_spawn and PR_SET_PDEATHSIG becoming active.
        if (getppid() != launcher_parent) {
            return EXIT_FAILURE;
        }
    }
#endif
    recomp::Version project_version{};
    if (!recomp::Version::from_string(version_string, project_version)) {
        ultramodern::error_handling::message_box(("Invalid version string: " + version_string).c_str());
        return EXIT_FAILURE;
    }

    // Map this executable into memory and lock it, which should keep it in physical memory. This ensures
    // that there are no stutters from the OS having to load new pages of the executable whenever a new code page is run.
    PreloadContext preload_context;
    bool preloaded = preload_executable(preload_context);

    if (!preloaded) {
        fprintf(stderr, "Failed to preload executable!\n");
    }

    // Initialize random seed for icon easter egg.
    std::srand(std::time(nullptr));

#ifdef _WIN32
    // Set up high resolution timing period.
    timeBeginPeriod(1);

    // Process arguments.
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--show-console") == 0)
        {
            if (GetConsoleWindow() == nullptr)
            {
                AllocConsole();
                freopen("CONIN$", "r", stdin);
                freopen("CONOUT$", "w", stderr);
                freopen("CONOUT$", "w", stdout);
            }

            break;
        }
    }

    // Set up console output to accept UTF-8 on windows
    SetConsoleOutputCP(CP_UTF8);

    // Change to a font that supports Japanese characters
    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize = sizeof cfi;
    cfi.nFont = 0;
    cfi.dwFontSize.X = 0;
    cfi.dwFontSize.Y = 16;
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
    wcscpy_s(cfi.FaceName, L"NSimSun");
    SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
#endif

#ifdef _WIN32
    // Force wasapi on Windows, as there seems to be some issue with sample queueing with directsound currently.
    SDL_setenv("SDL_AUDIODRIVER", "wasapi", true);
#endif

#if defined(__linux__) && defined(RECOMP_FLATPAK)
    // When using Flatpak, applications tend to launch from the home directory by default.
    // Mods might use the current working directory to store the data, so we switch it to a directory
    // with persistent data storage and write permissions under Flatpak to ensure it works.
    std::error_code ec;
    std::filesystem::current_path("/var/data", ec);
#endif

    // Initialize native file dialogs.
    NFD_Init();

    // Initialize program settings.
    recompui::programconfig::set_program_name(banjo::program_name);
    recompui::programconfig::set_program_id(banjo::program_id);
    
    // Initialize SDL audio and set the output frequency.
    SDL_InitSubSystem(SDL_INIT_AUDIO);
    if (!reset_audio(48000)) {
        // It is not possible to initialize without an audio device.
        return EXIT_FAILURE;
    }

    // Source controller mappings file
    std::u8string controller_db_path = (recompui::file::get_program_path() / "recompcontrollerdb.txt").u8string();
    if (SDL_GameControllerAddMappingsFromFile(reinterpret_cast<const char *>(controller_db_path.c_str())) < 0) {
        fprintf(stderr, "Failed to load controller mappings: %s\n", SDL_GetError());
    }

    // Register fonts.
    recompui::register_primary_font("InterVariable.ttf", "Inter Variable");
    recompui::register_extra_font("Suplexmentary Comic NC.ttf");

    // Register configuration path.
    recomp::register_config_path(recompui::file::get_app_folder_path());

    // Register supported games and patches
    for (const auto& game : supported_games) {
        recomp::register_game(game);
    }

    const char* auto_start_env = std::getenv("TOOIE_AUTO_START");
    const bool explicit_auto_start = auto_start_env != nullptr &&
        auto_start_env[0] != '\0' && auto_start_env[0] != '0';
    const char* disable_launcher_music = std::getenv("TOOIE_DISABLE_LAUNCHER_MUSIC");
    const bool launcher_music_disabled = disable_launcher_music != nullptr &&
        disable_launcher_music[0] != '\0' && disable_launcher_music[0] != '0';

    if (!launcher_music_child && !explicit_auto_start && !launcher_music_disabled) {
        recomp::check_all_stored_roms();
        if (recomp::is_rom_valid(supported_games[0].game_id)) {
            start_launcher_music_preview();
        }
    }

    recomp::mods::register_deprecated_mod("bk_recomp_mod_fov_slider", recomp::mods::DeprecationStatus::BrokenVersion, recomp::Version(1, 1, 0));

    REGISTER_FUNC(recomp_get_window_resolution);
    REGISTER_FUNC(recomp_get_target_aspect_ratio);
    REGISTER_FUNC(recomp_get_target_framerate);
    REGISTER_FUNC(recomp_get_cutscene_aspect_ratio);
    REGISTER_FUNC(recomp_get_analog_cam_enabled);
    REGISTER_FUNC(recomp_get_right_analog_inputs);
    REGISTER_FUNC(recomp_get_bgm_volume);
    // REGISTER_FUNC(recomp_get_gyro_deltas);
    // REGISTER_FUNC(recomp_get_mouse_deltas);
    REGISTER_FUNC(recomp_get_inverted_axes);
    REGISTER_FUNC(recomp_get_analog_inverted_axes);
    recompui::register_ui_exports();
    recomputil::register_data_api_exports();
    recomptheme::set_custom_theme();

    banjo::register_bk_overlays();
    banjo::register_bk_patches();

    // Register extensions for two types: Props and ActorMarkers.
    recomputil::init_extended_object_data(2);

    recompinput::players::set_single_player_mode(true);

    banjo::init_config();

    recompui::register_launcher_init_callback(on_launcher_init);
    recompui::register_launcher_update_callback([](recompui::LauncherMenu* menu) {
        banjo::launcher_animation_update(menu);

        // If this is the first run and the ROM is selected from the launcher,
        // begin the preview as soon as validation succeeds.
        static int launcher_music_check_frames = 0;
        if (!launcher_music_child && !launcher_music_preview_started &&
            ++launcher_music_check_frames >= 60) {
            launcher_music_check_frames = 0;
            const char* disable_music = std::getenv("TOOIE_DISABLE_LAUNCHER_MUSIC");
            const char* forced_start = std::getenv("TOOIE_AUTO_START");
            const bool disabled = disable_music != nullptr && disable_music[0] != '\0' && disable_music[0] != '0';
            const bool auto_starting = forced_start != nullptr && forced_start[0] != '\0' && forced_start[0] != '0';
            if (!disabled && !auto_starting) {
                recomp::check_all_stored_roms();
                if (recomp::is_rom_valid(supported_games[0].game_id)) {
                    start_launcher_music_preview();
                }
            }
        }

        // Deferred + frame delay so RT64/UI finish first init before game thread starts.
        static int auto_start_frames = 0;
        static bool auto_start_done = false;
        if (auto_start_done) {
            return;
        }
        const char* auto_start = std::getenv("TOOIE_AUTO_START");
        const char* launcher_music_smoke = std::getenv("TOOIE_LAUNCHER_MUSIC_SMOKE");
        const bool smoke_start_requested = launcher_music_smoke != nullptr &&
            launcher_music_smoke[0] != '\0' && launcher_music_smoke[0] != '0';
        const bool auto_start_requested = launcher_music_child ||
            smoke_start_requested ||
            (auto_start != nullptr && auto_start[0] != '\0' && auto_start[0] != '0');
        if (!auto_start_requested) {
            return;
        }
        const int start_delay_frames = launcher_music_child ? 1 : (smoke_start_requested ? 180 : 30);
        if (++auto_start_frames < start_delay_frames) {
            return;
        }
        auto_start_done = true;
        recomp::check_all_stored_roms();
        std::u8string game_id = supported_games[0].game_id;
        if (recomp::is_rom_valid(game_id)) {
            // Match launcher Start Game: hide UI so RT64 game frames are visible.
            std::fprintf(stderr, "[tooie] TOOIE_AUTO_START: starting game\n");
            recomp::start_game(game_id, {});
            recompui::hide_all_contexts();
        } else {
            std::fprintf(stderr, "[tooie] TOOIE_AUTO_START: ROM not valid/stored; staying on launcher\n");
        }
    });

    recomp::rsp::callbacks_t rsp_callbacks{
        .get_rsp_microcode = get_rsp_microcode,
    };

    ultramodern::renderer::callbacks_t renderer_callbacks{
        .create_render_context = [](uint8_t* rdram, ultramodern::renderer::WindowHandle window_handle, bool developer_mode) {
            auto presentation_mode = ultramodern::renderer::PresentationMode::PresentEarly;
            return recompui::renderer::create_render_context(rdram, window_handle, presentation_mode, developer_mode);
        },
    };

    ultramodern::gfx_callbacks_t gfx_callbacks{
        .create_gfx = create_gfx,
        .create_window = create_window,
        .update_gfx = update_gfx,
    };

    ultramodern::audio_callbacks_t audio_callbacks{
        .queue_samples = queue_samples,
        .get_frames_remaining = get_frames_remaining,
        .set_frequency = set_frequency,
    };

    auto tooie_get_n64_input = [](int controller, uint16_t* buttons, float* x, float* y) -> bool {
        const bool ok = recompinput::profiles::get_n64_input(controller, buttons, x, y);
        if (x != nullptr && y != nullptr) {
            tooie_diag::apply_synthetic_input(controller, buttons, x, y);
        }
        tooie_diag::note_host_input(controller,
                                    x != nullptr ? *x : 0.0f,
                                    y != nullptr ? *y : 0.0f,
                                    buttons != nullptr ? *buttons : 0);
        return ok;
    };

    ultramodern::input::callbacks_t input_callbacks{
        .poll_input = recompinput::poll_inputs,
        .get_input = tooie_get_n64_input,
        .set_rumble = recompinput::set_rumble,
        .get_connected_device_info = get_connected_device_info,
    };

    ultramodern::events::callbacks_t thread_callbacks{
        .vi_callback = recompinput::update_rumble,
        .gfx_init_callback = nullptr,
    };

    ultramodern::error_handling::callbacks_t error_handling_callbacks{
        .message_box = recompui::message_box,
    };

    ultramodern::threads::callbacks_t threads_callbacks{
        .get_game_thread_name = banjo::get_game_thread_name,
    };

    // Register the texture pack content type with rt64.json as its content file.
    recomp::mods::ModContentType texture_pack_content_type{
        .content_filename = "rt64.json",
        .allow_runtime_toggle = true,
        .on_enabled = enable_texture_pack,
        .on_disabled = disable_texture_pack,
        .on_reordered = reorder_texture_pack,
    };
    auto texture_pack_content_type_id = recomp::mods::register_mod_content_type(texture_pack_content_type);

    // Register the .rtz texture pack file format with the previous content type as its only allowed content type.
    recomp::mods::register_mod_container_type("rtz", std::vector{ texture_pack_content_type_id }, false);

    // Before recomp::start / ultramodern::preinit / any guest OSThread.
    activate_tooie_guest_preemption();

    recomp::start(
        project_version,
        {},
        rsp_callbacks,
        renderer_callbacks,
        audio_callbacks,
        input_callbacks,
        gfx_callbacks,
        thread_callbacks,
        error_handling_callbacks,
        threads_callbacks
    );

    stop_launcher_music_preview();

    NFD_Quit();

    if (preloaded) {
        release_preload(preload_context);
    }

#ifdef _WIN32
    // End high resolution timing period.
    timeEndPeriod(1);
#endif

    return EXIT_SUCCESS;
}
