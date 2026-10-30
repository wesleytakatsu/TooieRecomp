#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// Declare the writer before mods.hpp selects the read-only miniz API.
#include "miniz.h"
#include "miniz_zip.h"
#include "librecomp/mods.hpp"

namespace {
    namespace fs = std::filesystem;
    using namespace recomp::mods;
    using Entries = std::vector<std::pair<std::string, std::string>>;

    void require(bool condition, const char* message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }

    struct TemporaryDirectory {
        fs::path path;

        TemporaryDirectory() {
            fs::path parent = fs::temp_directory_path();
            path = parent / ("tooie-texture-zip-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
            require(fs::create_directory(path), "create test directory");
        }

        ~TemporaryDirectory() {
            std::error_code error;
            if (path.parent_path() == fs::temp_directory_path() && path.filename().string().starts_with("tooie-texture-zip-")) {
                fs::remove_all(path, error);
            }
        }
    };

    void write_zip(const fs::path& path, const Entries& entries) {
        mz_zip_archive archive{};
        require(mz_zip_writer_init_heap(&archive, 0, 0), "initialize fixture ZIP writer");
        bool good = true;
        for (const auto& [name, contents] : entries) {
            good = good && mz_zip_writer_add_mem(&archive, name.c_str(), contents.data(), contents.size(), MZ_NO_COMPRESSION);
        }
        void* data = nullptr;
        size_t size = 0;
        good = good && mz_zip_writer_finalize_heap_archive(&archive, &data, &size);
        {
            std::ofstream output(path, std::ios::binary);
            if (good && output) {
                output.write(static_cast<const char*>(data), size);
            }
            else {
                good = false;
            }
            output.close();
            good = good && !output.fail();
        }
        mz_free(data);
        mz_zip_writer_end(&archive);
        require(good, "write fixture ZIP");
    }

    std::string manifest(const std::string& id) {
        return "{\"game_id\":\"bt\",\"id\":\"" + id +
            "\",\"display_name\":\"Test Texture Pack\",\"version\":\"0.1.1\",\"minimum_recomp_version\":\"0.0.1\",\"authors\":[\"Test\"]}";
    }

    std::unique_ptr<ZipModFileHandle> open_zip(const fs::path& path) {
        ModOpenError error;
        auto handle = std::make_unique<ZipModFileHandle>(path, error);
        require(error == ModOpenError::Good, "open fixture ZIP");
        return handle;
    }

    ZipContentRootResult probe(const fs::path& path) {
        return find_zip_content_root(*open_zip(path), "rt64.json");
    }

    std::string read_text(const ZipModFileHandle& handle, const std::string& name) {
        bool exists = false;
        auto bytes = handle.read_file(name, exists);
        require(exists, "read fixture entry");
        return {bytes.begin(), bytes.end()};
    }
}

int main() {
    int failures = 0;
    auto run = [&](const char* name, const std::function<void()>& test) {
        try {
            test();
            std::printf("PASS %s\n", name);
        }
        catch (const std::exception& error) {
            std::printf("FAIL %s: %s\n", name, error.what());
            failures++;
        }
    };

    try {
        TemporaryDirectory temporary;
        const fs::path root = temporary.path;
        const Entries nested = {
            {"BANJO TOOIE/mod.json", manifest("test-nested")},
            {"BANJO TOOIE/rt64.json", "{}"},
            {"BANJO TOOIE/thumb.dds", "test-thumbnail"},
            {"leia.txt", "outside the texture pack"}
        };
        write_zip(root / "root.zip", {{"mod.json", manifest("test-root")}, {"rt64.json", "{}"}});
        write_zip(root / "nested.zip", nested);

        run("C1 archive root", [&] {
            auto result = probe(root / "root.zip");
            require(result.status == ZipContentRootStatus::Found && result.root.empty(), "select archive root");
        });
        run("C2 relative metadata and thumbnail", [&] {
            auto handle = open_zip(root / "nested.zip");
            auto result = find_zip_content_root(*handle, "rt64.json");
            require(result.status == ZipContentRootStatus::Found && result.root == "BANJO TOOIE", "select nested root");
            handle->content_root = result.root;
            require(read_text(*handle, "mod.json") == manifest("test-nested"), "read nested manifest");
            require(read_text(*handle, "thumb.dds") == "test-thumbnail", "read nested thumbnail");
            require(handle->file_exists("rt64.json") && !handle->file_exists("leia.txt"), "isolate metadata lookup");
            bool exists = true;
            require(handle->read_file("leia.txt", exists).empty() && !exists, "isolate file reads");
            require(handle->get_content_root() == "BANJO TOOIE", "query handle root");
        });
        run("C3 arbitrary nested directory", [&] {
            write_zip(root / "deep.zip", {{"a/b/Pack X/rt64.json", "{}"}});
            auto result = probe(root / "deep.zip");
            require(result.status == ZipContentRootStatus::Found && result.root == "a/b/Pack X", "select arbitrary root");
        });
        run("C4 archive root wins", [&] {
            write_zip(root / "priority.zip", {{"sub/rt64.json", "{}"}, {"rt64.json", "{}"}});
            auto result = probe(root / "priority.zip");
            require(result.status == ZipContentRootStatus::Found && result.root.empty(), "prefer archive root");
        });
        run("C5 ambiguous roots", [&] {
            write_zip(root / "ambiguous.zip", {{"A/rt64.json", "{}"}, {"B/rt64.json", "{}"}});
            auto result = probe(root / "ambiguous.zip");
            require(result.status == ZipContentRootStatus::Ambiguous && result.candidates == std::vector<std::string>{"A", "B"}, "report both roots");
        });
        run("C6 non-texture ZIPs", [&] {
            write_zip(root / "images.zip", {{"image.png", "synthetic image"}});
            write_zip(root / "bundle.zip", {{"pack.rtz", "synthetic container"}, {"code.nrm", "synthetic container"}});
            require(probe(root / "images.zip").status == ZipContentRootStatus::NotFound, "ignore image collection");
            require(probe(root / "bundle.zip").status == ZipContentRootStatus::NotFound, "preserve bundle routing");
        });
        run("C7 directory prefix boundary", [&] {
            write_zip(root / "boundary.zip", {{"BANJO TOOIE/rt64.json", "{}"}, {"BANJO TOOIE_OUTRO/mod.json", manifest("wrong")}});
            auto handle = open_zip(root / "boundary.zip");
            handle->content_root = find_zip_content_root(*handle, "rt64.json").root;
            require(handle->content_root == "BANJO TOOIE" && !handle->file_exists("mod.json"), "exclude neighboring root");
        });
        run("C8 invalid path components", [&] {
            for (const std::string& name : {"../x/rt64.json", "x/../rt64.json", "x/./rt64.json"}) {
                write_zip(root / "invalid.zip", {{name, "{}"}});
                require(probe(root / "invalid.zip").status == ZipContentRootStatus::Invalid, "reject invalid root");
            }
        });
        run("C9 case-sensitive marker", [&] {
            write_zip(root / "case.zip", {{"RT64.JSON", "{}"}});
            require(probe(root / "case.zip").status == ZipContentRootStatus::NotFound, "preserve marker case");
        });
        run("C10 unchanged rootless handle", [&] {
            auto handle = open_zip(root / "root.zip");
            require(handle->get_content_root().empty(), "default empty root");
            require(handle->file_exists("mod.json") && read_text(*handle, "mod.json") == manifest("test-root"), "preserve rootless reads");
        });

        fs::path scan = root / "scan";
        fs::create_directory(scan);
        fs::copy_file(root / "nested.zip", scan / "nested.zip");
        fs::copy_file(root / "bundle.zip", scan / "bundle.zip");
        fs::copy_file(root / "ambiguous.zip", scan / "ambiguous.zip");
        write_zip(scan / "legacy.rtz", {{"rt64.json", "{}"}});
        ModContext context;
        context.register_game("bt");
        context.set_mods_config_path(root / "mods.json");
        context.set_mod_config_directory(root / "config");
        auto textures = context.register_content_type({
            .content_filename = "rt64.json",
            .allow_runtime_toggle = true,
            .on_enabled = nullptr,
            .on_disabled = nullptr,
            .on_reordered = nullptr
        });
        require(context.register_container_type("rtz", {textures}, false), "register legacy container");
        require(context.register_container_type("zip", {textures}, false, true), "register ZIP container");

        run("C11 real mod folder scan", [&] {
            auto errors = context.scan_mod_folder(scan);
            require(errors.size() == 1 && errors.front().error == ModOpenError::AmbiguousContentRoot, "report only ambiguous ZIP");
            require(errors.front().error_param == "A, B", "expose ambiguous candidates");
            require(context.num_opened_mods() == 2, "ignore bundle and open texture ZIP plus RTZ");
            auto details = context.get_details_for_mod("test-nested");
            require(details && details->display_name == "Test Texture Pack" && details->version.to_string() == "0.1.1", "read nested identity");
            require(context.get_mod_content_root("test-nested") == "BANJO TOOIE", "expose nested root");
            require(context.get_mod_filename("test-nested") == scan / "nested.zip", "preserve physical filename");
            require(context.get_mod_content_root("legacy").empty() && context.get_details_for_mod("legacy").has_value(), "preserve RTZ behavior");
            auto thumbnail = context.get_mod_thumbnail("test-nested");
            require(std::string(thumbnail.begin(), thumbnail.end()) == "test-thumbnail", "scan nested thumbnail");
            require(context.get_mod_content_root("unknown").empty(), "unknown id has no root");
        });
        run("C12 default manifest for nested ZIP", [&] {
            write_zip(scan / "no-manifest.zip", {{"Pack/rt64.json", "{}"}});
            context.scan_mod_folder(scan);
            require(context.get_details_for_mod("no-manifest").has_value(), "use filename as default id");
            require(context.get_mod_content_root("no-manifest") == "Pack", "retain root with default manifest");
        });
        run("C13 texture-only content detection", [&] {
            write_zip(scan / "with-code.zip", {{"Pack/mod.json", manifest("test-with-code")}, {"Pack/rt64.json", "{}"}, {"Pack/mod_syms.bin", "invalid code symbols"}});
            write_zip(scan / "control.nrm", {{"mod.json", manifest("test-code-control")}, {"rt64.json", "{}"}, {"mod_syms.bin", "invalid code symbols"}});
            context.scan_mod_folder(scan);
            auto texture_details = context.get_details_for_mod("test-with-code");
            auto code_details = context.get_details_for_mod("test-code-control");
            require(texture_details && texture_details->runtime_toggleable, "ZIP must not detect code content");
            require(code_details && !code_details->runtime_toggleable, "control NRM must detect code content");
        });
    }
    catch (const std::exception& error) {
        std::printf("FAIL test setup: %s\n", error.what());
        failures++;
    }
    std::printf("tooie_texture_zip_mod_test: %s\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
