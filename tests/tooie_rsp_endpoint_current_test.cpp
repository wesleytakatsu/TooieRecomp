#include "tooie_rsp_endpoint_current.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

#if defined(_WIN32)
#include <stdlib.h>
#endif

static int g_test_count = 0;
static int g_pass_count = 0;

#define TEST_ASSERT(cond, msg) do { \
    ++g_test_count; \
    if (cond) { \
        ++g_pass_count; \
    } else { \
        std::fprintf(stderr, "FAIL: %s (line %d): %s\n", __FUNCTION__, __LINE__, msg); \
        std::exit(1); \
    } \
} while (0)

static void clear_env() {
#if defined(_WIN32)
    TEST_ASSERT(_putenv_s(tooie_rsp_endpoint_current::kEnvName, "") == 0,
                "clear env");
#else
    TEST_ASSERT(unsetenv(tooie_rsp_endpoint_current::kEnvName) == 0, "clear env");
#endif
    TEST_ASSERT(std::getenv(tooie_rsp_endpoint_current::kEnvName) == nullptr,
                "env is unset");
}

static void set_env(const char* value) {
#if defined(_WIN32)
    TEST_ASSERT(_putenv_s(tooie_rsp_endpoint_current::kEnvName, value) == 0,
                "set env");
#else
    TEST_ASSERT(setenv(tooie_rsp_endpoint_current::kEnvName, value, 1) == 0,
                "set env");
#endif
}

static void test_unset_applies_palliative() {
    clear_env();
    const auto result = tooie_rsp_endpoint_current::apply_default_before_renderer_init();
    const char* value = std::getenv(tooie_rsp_endpoint_current::kEnvName);
    TEST_ASSERT(result == tooie_rsp_endpoint_current::ApplyResult::AppliedDefault,
                "unset applies the default");
    TEST_ASSERT(value != nullptr && std::strcmp(value, "1") == 0,
                "unset enables shader-local CURRENT (value 1)");
}

static void test_explicit_zero_stays_zero() {
    set_env("0");
    const auto result = tooie_rsp_endpoint_current::apply_default_before_renderer_init();
    const char* value = std::getenv(tooie_rsp_endpoint_current::kEnvName);
    TEST_ASSERT(result == tooie_rsp_endpoint_current::ApplyResult::PreservedExplicit,
                "explicit 0 is preserved");
    TEST_ASSERT(value != nullptr && std::strcmp(value, "0") == 0,
                "explicit 0 stays 0");
}

static void test_explicit_one_stays_one() {
    set_env("1");
    const auto result = tooie_rsp_endpoint_current::apply_default_before_renderer_init();
    const char* value = std::getenv(tooie_rsp_endpoint_current::kEnvName);
    TEST_ASSERT(result == tooie_rsp_endpoint_current::ApplyResult::PreservedExplicit,
                "explicit 1 is preserved");
    TEST_ASSERT(value != nullptr && std::strcmp(value, "1") == 0,
                "explicit 1 stays 1");
}

static void test_other_explicit_value_is_not_overwritten() {
    set_env("keep");
    const auto result = tooie_rsp_endpoint_current::apply_default_before_renderer_init();
    const char* value = std::getenv(tooie_rsp_endpoint_current::kEnvName);
    TEST_ASSERT(result == tooie_rsp_endpoint_current::ApplyResult::PreservedExplicit,
                "other explicit values are preserved");
    TEST_ASSERT(value != nullptr && std::strcmp(value, "keep") == 0,
                "explicit keep stays keep");
}

static std::string trim(const std::string& line) {
    const auto first = line.find_first_not_of(" \t");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = line.find_last_not_of(" \t\r");
    return line.substr(first, last - first + 1);
}

// The renderer is created inside recomp::start via renderer_callbacks.
// Prove the production call is wired after those callbacks are built and
// before that start call. This does not launch the game.
static void test_apply_is_before_renderer_init() {
    std::ifstream in(TOOIE_MAIN_SOURCE_PATH);
    TEST_ASSERT(static_cast<bool>(in), "main.cpp opens");

    int line_no = 0;
    int apply_line = -1;
    int start_line = -1;
    int callbacks_line = -1;
    std::string line;
    while (std::getline(in, line)) {
        ++line_no;
        const std::string text = trim(line);
        if (text.rfind("//", 0) != 0 &&
            text.find("tooie_rsp_endpoint_current::apply_default_before_renderer_init(") != std::string::npos) {
            TEST_ASSERT(apply_line < 0, "exactly one apply call");
            apply_line = line_no;
        }
        if (callbacks_line < 0 &&
            text.find("ultramodern::renderer::callbacks_t renderer_callbacks{") != std::string::npos) {
            callbacks_line = line_no;
        }
        if (start_line < 0 && text.rfind("recomp::start(", 0) == 0) {
            start_line = line_no;
        }
    }

    TEST_ASSERT(apply_line > 0, "apply is called from main");
    TEST_ASSERT(callbacks_line > 0, "renderer callbacks are built in main");
    TEST_ASSERT(start_line > 0, "recomp::start is the renderer init");
    TEST_ASSERT(callbacks_line < apply_line,
                "apply is after the renderer callback object is built");
    TEST_ASSERT(apply_line < start_line,
                "apply is before recomp::start renderer init");
}

int main() {
    test_unset_applies_palliative();
    test_explicit_zero_stays_zero();
    test_explicit_one_stays_one();
    test_other_explicit_value_is_not_overwritten();
    test_apply_is_before_renderer_init();
    std::printf("=== PASS: %d/%d assertions passed ===\n", g_pass_count, g_test_count);
    return 0;
}
