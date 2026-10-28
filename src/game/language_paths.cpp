#include "tooie_language_paths.h"
#include "util/file.h"

#include <fstream>
#include <iterator>
#include <string>

namespace tooie_language_paths {
    std::filesystem::path language_pack_path(const char* tag) {
        return recompui::file::get_app_folder_path() / "languages" / (std::string(tag) + ".tlp");
    }
    std::filesystem::path language_bundle_path() {
        return recompui::file::get_program_path() / "assets" / "languages" / "pt-BR.tlpb";
    }
    std::filesystem::path usa_image_path() {
        return recompui::file::get_app_folder_path() / "bt.n64.us.1.0.z64";
    }
    bool read_binary_file(const std::filesystem::path& path, std::vector<uint8_t>& bytes) {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            return false;
        }
        bytes.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        return !file.bad();
    }
}
