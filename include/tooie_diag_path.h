#pragma once

#include <cstdlib>
#include <string>
#if defined(_WIN32)
#include <filesystem>
#endif

// Diagnostic output location shared by fopen call sites.
// TOOIE_DIAG_DIR, when set and non-empty, is "<dir>/<file_name>" on every OS.
// Otherwise Linux/macOS stays exactly "/tmp/<file_name>".
// Windows uses the process temp directory as a native narrow path: fopen reads the ANSI code page.
inline std::string tooie_diag_path(const char* file_name) {
    const char* dir = std::getenv("TOOIE_DIAG_DIR");
    if (dir != nullptr && dir[0] != '\0') {
        std::string out(dir);
        const char last = out.back();
        if (last != '/' && last != '\\') {
            out.push_back('/');
        }
        out += file_name;
        return out;
    }
#if defined(_WIN32)
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / file_name;
    return path.string();
#else
    return std::string("/tmp/") + file_name;
#endif
}
