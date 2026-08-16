#include "tooie_error.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include "SDL.h"
#else
#include "SDL2/SDL.h"
#endif

namespace tooie_error {

const char* error_class_to_string(ErrorClass error_class) {
    switch (error_class) {
        case ErrorClass::WINDOW_INIT_FAILED:
            return "WINDOW_INIT_FAILED";
        case ErrorClass::NO_VULKAN:
            return "NO_VULKAN";
        case ErrorClass::NO_COMPATIBLE_GPU:
            return "NO_COMPATIBLE_GPU";
        case ErrorClass::VULKAN_DEVICE_INIT_FAILED:
            return "VULKAN_DEVICE_INIT_FAILED";
        case ErrorClass::RT64_INIT_FAILED:
            return "RT64_INIT_FAILED";
        case ErrorClass::AUDIO_INIT_FAILED:
            return "AUDIO_INIT_FAILED";
        case ErrorClass::UNKNOWN_GRAPHICS_ERROR:
        default:
            return "UNKNOWN_GRAPHICS_ERROR";
    }
}

ErrorInfo format_error(ErrorClass error_class, const char* detail) {
    ErrorInfo info;
    info.error_class = error_class;
    if (detail != nullptr) {
        info.technical_detail = detail;
    }

    switch (error_class) {
        case ErrorClass::WINDOW_INIT_FAILED:
            info.user_title = "Banjo-Tooie: Window Initialization Error";
            info.user_message = "Failed to initialize the game window or display subsystem.\n"
                                "Please ensure your display server (X11 / Wayland) is active and running.";
            break;
        case ErrorClass::NO_VULKAN:
            info.user_title = "Banjo-Tooie: Vulkan Not Available";
            info.user_message = "The Vulkan graphics loader or runtime library could not be found.\n"
                                "Please ensure Vulkan runtime libraries (e.g. libvulkan.so.1 / vulkan-1.dll) and display drivers are installed.";
            break;
        case ErrorClass::NO_COMPATIBLE_GPU:
            info.user_title = "Banjo-Tooie: No Compatible GPU";
            info.user_message = "No compatible Vulkan physical graphics device was detected.\n"
                                "Please ensure your GPU and graphics drivers support Vulkan 1.0 or higher.";
            break;
        case ErrorClass::VULKAN_DEVICE_INIT_FAILED:
            info.user_title = "Banjo-Tooie: Vulkan Device Error";
            info.user_message = "Failed to initialize the Vulkan graphics device.\n"
                                "Please ensure your GPU drivers are up to date and Vulkan layers are compatible.";
            break;
        case ErrorClass::RT64_INIT_FAILED:
            info.user_title = "Banjo-Tooie: Graphics Engine Error";
            info.user_message = "Failed to initialize the RT64 rendering engine.\n"
                                "Please verify that your graphics drivers and swapchain support the required presentation mode.";
            break;
        case ErrorClass::AUDIO_INIT_FAILED:
            info.user_title = "Banjo-Tooie: Audio Initialization Error";
            info.user_message = "No audio output device could be opened.\n"
                                "Please make sure an audio device is available and enabled.";
            break;
        case ErrorClass::UNKNOWN_GRAPHICS_ERROR:
        default:
            info.user_title = "Banjo-Tooie: Graphics Error";
            info.user_message = "An unexpected error occurred while initializing the graphics subsystem.";
            break;
    }

    return info;
}

ErrorInfo format_setup_result_error(ultramodern::renderer::SetupResult result, const char* detail) {
    switch (result) {
        case ultramodern::renderer::SetupResult::DynamicLibrariesNotFound:
            return format_error(ErrorClass::NO_VULKAN, detail ? detail : "DynamicLibrariesNotFound (Vulkan library not loaded)");
        case ultramodern::renderer::SetupResult::GraphicsAPINotFound:
            return format_error(ErrorClass::NO_VULKAN, detail ? detail : "GraphicsAPINotFound (Vulkan loader not found)");
        case ultramodern::renderer::SetupResult::GraphicsDeviceNotFound:
            return format_error(ErrorClass::NO_COMPATIBLE_GPU, detail ? detail : "GraphicsDeviceNotFound (No compatible Vulkan physical device)");
        case ultramodern::renderer::SetupResult::InvalidGraphicsAPI:
            return format_error(ErrorClass::RT64_INIT_FAILED, detail ? detail : "InvalidGraphicsAPI (Unsupported graphics backend)");
        case ultramodern::renderer::SetupResult::Success:
        default:
            return format_error(ErrorClass::UNKNOWN_GRAPHICS_ERROR, detail ? detail : "Unknown setup failure");
    }
}

void report_error(const ErrorInfo& info, bool show_dialog) {
    // 1. Output comprehensive diagnostics to stderr
    std::fprintf(stderr, "\n=======================================================\n");
    std::fprintf(stderr, "[tooie-error] ERROR CLASS: %s\n", error_class_to_string(info.error_class));
    std::fprintf(stderr, "[tooie-error] TITLE:       %s\n", info.user_title.c_str());
    std::fprintf(stderr, "[tooie-error] MESSAGE:     %s\n", info.user_message.c_str());
    if (!info.technical_detail.empty()) {
        std::fprintf(stderr, "[tooie-error] DETAILS:     %s\n", info.technical_detail.c_str());
    }
    std::fprintf(stderr, "=======================================================\n\n");
    std::fflush(stderr);

    // 2. User-visible message box if requested and not disabled by environment
    const char* no_gui = std::getenv("TOOIE_NO_GUI_DIALOG");
    const bool gui_disabled = (no_gui != nullptr && no_gui[0] != '\0' && no_gui[0] != '0');

    if (show_dialog && !gui_disabled) {
        std::string full_dialog_msg = info.user_message;
        if (!info.technical_detail.empty()) {
            full_dialog_msg += "\n\nTechnical details: " + info.technical_detail;
        }

        // Attempt SDL message box if video subsystem is initialized or available
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, info.user_title.c_str(), full_dialog_msg.c_str(), nullptr);
    }
}

void report_window_init_failure(const char* detail) {
    ErrorInfo info = format_error(ErrorClass::WINDOW_INIT_FAILED, detail);
    report_error(info, true);
}

void report_graphics_init_failure(ErrorClass error_class, const char* detail) {
    ErrorInfo info = format_error(error_class, detail);
    report_error(info, true);
}

} // namespace tooie_error
