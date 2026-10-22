#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "tooie_error.h"

#ifdef _WIN32
static int setenv(const char *name, const char *value, int overwrite) {
    (void)overwrite;
    return _putenv_s(name, value);
}
#endif

static int g_test_count = 0;
static int g_pass_count = 0;

#define TEST_ASSERT(cond, msg) do { \
    g_test_count++; \
    if (cond) { \
        g_pass_count++; \
    } else { \
        std::fprintf(stderr, "FAIL: %s (line %d): %s\n", __FUNCTION__, __LINE__, msg); \
        std::exit(1); \
    } \
} while(0)

static void test_format_error_classes() {
    // 1. WINDOW_INIT_FAILED
    auto err = tooie_error::format_error(tooie_error::ErrorClass::WINDOW_INIT_FAILED, "SDL_CreateWindow failed: No video device available");
    TEST_ASSERT(err.error_class == tooie_error::ErrorClass::WINDOW_INIT_FAILED, "error class WINDOW_INIT_FAILED");
    TEST_ASSERT(err.user_title.find("Window") != std::string::npos, "title mentions Window");
    TEST_ASSERT(!err.user_message.empty(), "user message not empty");
    TEST_ASSERT(err.technical_detail.find("No video device") != std::string::npos, "technical detail preserved");

    // 2. NO_VULKAN
    err = tooie_error::format_error(tooie_error::ErrorClass::NO_VULKAN, "libvulkan.so.1: cannot open shared object file");
    TEST_ASSERT(err.error_class == tooie_error::ErrorClass::NO_VULKAN, "error class NO_VULKAN");
    TEST_ASSERT(err.user_title.find("Vulkan Not Available") != std::string::npos, "title mentions Vulkan Not Available");
    TEST_ASSERT(err.user_message.find("Vulkan runtime libraries") != std::string::npos, "user message actionable");
    TEST_ASSERT(err.technical_detail.find("libvulkan.so.1") != std::string::npos, "technical detail preserved");

    // 3. NO_COMPATIBLE_GPU
    err = tooie_error::format_error(tooie_error::ErrorClass::NO_COMPATIBLE_GPU, "vkEnumeratePhysicalDevices returned 0 devices");
    TEST_ASSERT(err.error_class == tooie_error::ErrorClass::NO_COMPATIBLE_GPU, "error class NO_COMPATIBLE_GPU");
    TEST_ASSERT(err.user_title.find("No Compatible GPU") != std::string::npos, "title mentions No Compatible GPU");
    TEST_ASSERT(err.user_message.find("support Vulkan") != std::string::npos, "user message mentions Vulkan support");

    // 4. VULKAN_DEVICE_INIT_FAILED
    err = tooie_error::format_error(tooie_error::ErrorClass::VULKAN_DEVICE_INIT_FAILED, "vkCreateDevice returned VK_ERROR_DEVICE_LOST");
    TEST_ASSERT(err.error_class == tooie_error::ErrorClass::VULKAN_DEVICE_INIT_FAILED, "error class VULKAN_DEVICE_INIT_FAILED");
    TEST_ASSERT(err.user_title.find("Vulkan Device Error") != std::string::npos, "title mentions Vulkan Device Error");

    // 5. RT64_INIT_FAILED
    err = tooie_error::format_error(tooie_error::ErrorClass::RT64_INIT_FAILED, "Shader compilation or swapchain setup failed");
    TEST_ASSERT(err.error_class == tooie_error::ErrorClass::RT64_INIT_FAILED, "error class RT64_INIT_FAILED");
    TEST_ASSERT(err.user_title.find("Graphics Engine Error") != std::string::npos, "title mentions Graphics Engine Error");

    // 6. AUDIO_INIT_FAILED
    err = tooie_error::format_error(tooie_error::ErrorClass::AUDIO_INIT_FAILED, "SDL_OpenAudioDevice failed");
    TEST_ASSERT(err.error_class == tooie_error::ErrorClass::AUDIO_INIT_FAILED, "error class AUDIO_INIT_FAILED");
}

static void test_format_setup_result_mapping() {
    // DynamicLibrariesNotFound -> NO_VULKAN
    auto err = tooie_error::format_setup_result_error(ultramodern::renderer::SetupResult::DynamicLibrariesNotFound, "libvulkan.so.1 missing");
    TEST_ASSERT(err.error_class == tooie_error::ErrorClass::NO_VULKAN, "DynamicLibrariesNotFound maps to NO_VULKAN");

    // GraphicsAPINotFound -> NO_VULKAN
    err = tooie_error::format_setup_result_error(ultramodern::renderer::SetupResult::GraphicsAPINotFound, "Vulkan API not found");
    TEST_ASSERT(err.error_class == tooie_error::ErrorClass::NO_VULKAN, "GraphicsAPINotFound maps to NO_VULKAN");

    // GraphicsDeviceNotFound -> NO_COMPATIBLE_GPU
    err = tooie_error::format_setup_result_error(ultramodern::renderer::SetupResult::GraphicsDeviceNotFound, "No physical GPU");
    TEST_ASSERT(err.error_class == tooie_error::ErrorClass::NO_COMPATIBLE_GPU, "GraphicsDeviceNotFound maps to NO_COMPATIBLE_GPU");

    // InvalidGraphicsAPI -> RT64_INIT_FAILED
    err = tooie_error::format_setup_result_error(ultramodern::renderer::SetupResult::InvalidGraphicsAPI, "DirectX 12 on Linux");
    TEST_ASSERT(err.error_class == tooie_error::ErrorClass::RT64_INIT_FAILED, "InvalidGraphicsAPI maps to RT64_INIT_FAILED");
}

static void test_simulated_failures_no_crash() {
    // Set environment variable to disable GUI popup during automated test
    setenv("TOOIE_NO_GUI_DIALOG", "1", 1);

    // Simulated Window Init Failure
    auto window_err = tooie_error::format_error(tooie_error::ErrorClass::WINDOW_INIT_FAILED, "Simulated Window Failure");
    tooie_error::report_error(window_err, true);
    TEST_ASSERT(true, "SIMULATED_WINDOW_INIT_FAILURE reports without crash");

    // Simulated Graphics Init Failure
    auto gfx_err = tooie_error::format_error(tooie_error::ErrorClass::RT64_INIT_FAILED, "Simulated RT64 Failure");
    tooie_error::report_error(gfx_err, true);
    TEST_ASSERT(true, "SIMULATED_GRAPHICS_INIT_FAILURE reports without crash");

    // Simulated No Compatible Vulkan Device
    auto gpu_err = tooie_error::format_error(tooie_error::ErrorClass::NO_COMPATIBLE_GPU, "Simulated No GPU");
    tooie_error::report_error(gpu_err, true);
    TEST_ASSERT(true, "SIMULATED_NO_COMPATIBLE_VULKAN_DEVICE reports without crash");

    // String conversion
    TEST_ASSERT(std::strcmp(tooie_error::error_class_to_string(tooie_error::ErrorClass::NO_VULKAN), "NO_VULKAN") == 0, "string NO_VULKAN");
    TEST_ASSERT(std::strcmp(tooie_error::error_class_to_string(tooie_error::ErrorClass::NO_COMPATIBLE_GPU), "NO_COMPATIBLE_GPU") == 0, "string NO_COMPATIBLE_GPU");
}

int main() {
    std::printf("=== Running tooie_error / P08-T08 Unit Tests ===\n");
    test_format_error_classes();
    test_format_setup_result_mapping();
    test_simulated_failures_no_crash();
    std::printf("=== PASS: %d/%d assertions passed ===\n", g_pass_count, g_test_count);
    return 0;
}
