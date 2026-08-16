#ifndef TOOIE_ERROR_H
#define TOOIE_ERROR_H

#include <string>
#include <string_view>
#include "ultramodern/renderer_context.hpp"

namespace tooie_error {

enum class ErrorClass {
    WINDOW_INIT_FAILED,
    NO_VULKAN,
    NO_COMPATIBLE_GPU,
    VULKAN_DEVICE_INIT_FAILED,
    RT64_INIT_FAILED,
    AUDIO_INIT_FAILED,
    UNKNOWN_GRAPHICS_ERROR
};

struct ErrorInfo {
    ErrorClass error_class;
    std::string user_title;
    std::string user_message;
    std::string technical_detail;
};

// Translates an ErrorClass and optional technical detail into a user-facing ErrorInfo.
ErrorInfo format_error(ErrorClass error_class, const char* detail = nullptr);

// Translates an ultramodern SetupResult into a structured ErrorInfo.
ErrorInfo format_setup_result_error(ultramodern::renderer::SetupResult result, const char* detail = nullptr);

// Reports error to stderr, displays user-visible message box if appropriate, and logs diagnostic info.
void report_error(const ErrorInfo& info, bool show_dialog = true);

// Convenience helpers
void report_window_init_failure(const char* detail = nullptr);
void report_graphics_init_failure(ErrorClass error_class, const char* detail = nullptr);

// Converts ErrorClass to string identifier
const char* error_class_to_string(ErrorClass error_class);

} // namespace tooie_error

#endif // TOOIE_ERROR_H
