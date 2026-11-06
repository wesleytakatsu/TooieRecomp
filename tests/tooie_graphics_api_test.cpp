// Graphics API menu policy for Windows, Linux and handheld builds (include/tooie_graphics_api.h). Pure: no GPU, no probe.
#include "tooie_graphics_api.h"
#include <iostream>
#include <set>

using namespace tooie_graphics_api;
using List = std::vector<std::string>;

static int failures = 0;
static void check(bool ok, const char *what) {
    if (!ok) {
        std::cerr << "FAIL: " << what << '\n';
        failures++;
    }
}

static Probe only(const List &found) {
    return [found](const std::string &key) {
        for (const auto &item : found)
            if (item == key)
                return true;
        return false;
    };
}

int main() {
    // Saved values keep their meaning on every build.
    check(all_keys() == List{"auto", "d3d12", "vulkan", "d3d11", "opengl", "opengles"}, "enum keys");
    check(enum_value("auto") == 0 && enum_value("vulkan") == 2 && enum_value("opengl") == 4 && enum_value("opengles") == 5,
          "enum values");
    check(enum_value("bogus") == 0, "unknown key is Auto");
    check(menu_label("opengles") == "OpenGL ES" && menu_label("d3d12") == "DX12", "labels");

    // What each target builds, in Auto order.
    check(compiled(Target::Windows, false) == List{"d3d12", "vulkan", "d3d11", "opengl"}, "Windows order");
    check(compiled(Target::Windows, true) == List{"vulkan", "d3d12", "d3d11", "opengl"}, "Wine order");
    check(compiled(Target::Linux, false) == List{"vulkan", "opengl"}, "Linux backends");
    check(compiled(Target::Handheld, false) == List{"opengles"}, "handheld backends");

    // The menu lists only what the device can start, in the historical order.
    const auto everything = only({"d3d12", "vulkan", "d3d11", "opengl", "opengles"});
    check(menu(Target::Windows, false, everything, "") == List{"auto", "d3d12", "vulkan", "d3d11", "opengl"}, "Windows menu");
    check(menu(Target::Windows, true, everything, "") == List{"auto", "d3d12", "vulkan", "d3d11", "opengl"},
          "Wine menu keeps display order");
    check(menu(Target::Windows, false, only({"d3d11", "opengl"}), "") == List{"auto", "d3d11", "opengl"},
          "Windows without DX12 or Vulkan");
    check(menu(Target::Linux, false, everything, "") == List{"auto", "vulkan", "opengl"}, "Linux menu");
    check(menu(Target::Linux, false, only({"opengl"}), "") == List{"auto", "opengl"}, "Linux without Vulkan");
    check(menu(Target::Handheld, false, everything, "") == List{"auto", "opengles"}, "handheld menu");
    check(menu(Target::Handheld, false, only({}), "") == List{"auto"}, "handheld without EGL");
    check(menu(Target::Windows, false, only({}), "d3d11") == List{"auto", "d3d11"}, "running backend stays listed");
    check(menu(Target::Linux, false, only({}), "d3d12") == List{"auto"}, "running backend must be compiled");
    check(menu(Target::Windows, false, nullptr, "") == List{"auto"}, "no probe lists Auto");

    // A saved choice the menu does not list becomes Auto.
    const List linux_menu = {"auto", "vulkan", "opengl"};
    check(effective("opengl", linux_menu) == "opengl", "listed choice kept");
    check(effective("d3d12", linux_menu) == "auto", "choice from another platform");
    check(effective("opengles", linux_menu) == "auto", "handheld choice on desktop");
    check(effective("bogus", linux_menu) == "auto", "misspelled choice");
    check(effective("", linux_menu) == "auto", "empty choice");
    check(effective("vulkan", List{"auto", "opengl"}) == "auto", "backend the device lost");

    // Windows supervisor: the choice first, then every compiled backend once, whatever the probe said.
    check(candidates(Target::Windows, false, "d3d11") == List{"d3d11", "d3d12", "vulkan", "opengl"}, "explicit first");
    check(candidates(Target::Windows, false, "auto") == compiled(Target::Windows, false), "Auto order");
    check(candidates(Target::Windows, true, "auto") == compiled(Target::Windows, true), "Wine Auto order");
    check(candidates(Target::Windows, false, "opengles") == compiled(Target::Windows, false), "foreign choice ignored");
    {
        const auto order = candidates(Target::Windows, false, "opengl");
        check(std::set<std::string>(order.begin(), order.end()).size() == order.size() && order.size() == 4,
              "no duplicate candidates");
    }

    // Unsupervised targets: the TK64_GRAPHICS_API value.
    check(environment_value(Target::Handheld, "auto", everything) == "opengles", "handheld Auto");
    check(environment_value(Target::Handheld, "auto", only({})) == "opengles", "handheld has one backend");
    check(environment_value(Target::Linux, "auto", everything).empty(), "Linux Auto is Vulkan");
    check(environment_value(Target::Linux, "vulkan", everything).empty(), "Linux Vulkan");
    check(environment_value(Target::Linux, "opengl", everything) == "opengl", "Linux OpenGL");
    check(environment_value(Target::Linux, "auto", only({"opengl"})) == "opengl", "Linux Auto without Vulkan");
    check(environment_value(Target::Linux, "auto", only({})).empty(), "Linux Auto with nothing found keeps RT64");

    // Descriptions follow the target's order.
    check(auto_description(Target::Windows, false) ==
              "Auto tries DX12, Vulkan, DX11, then OpenGL until one works. Changes apply on next launch.",
          "Windows description");
    check(auto_description(Target::Windows, true).rfind("Auto tries Vulkan, DX12, DX11, then OpenGL", 0) == 0,
          "Wine description");
    check(auto_description(Target::Handheld, false) == "Auto uses OpenGL ES. Changes apply on next launch.",
          "handheld description");

    if (failures != 0)
        return 1;
    std::cout << "PASS: Graphics API menu policy (Windows, Wine, Linux, handheld)\n";
    return 0;
}
