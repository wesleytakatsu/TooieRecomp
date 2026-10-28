#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace tooie_language_paths {
    std::filesystem::path language_pack_path(const char* tag);
    std::filesystem::path language_bundle_path();
    std::filesystem::path usa_image_path();
    bool read_binary_file(const std::filesystem::path& path, std::vector<uint8_t>& bytes);
}
