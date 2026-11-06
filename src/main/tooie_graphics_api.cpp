#include "tooie_graphics_api.h"
#include <algorithm>

namespace tooie_graphics_api {
namespace {
bool contains(const std::vector<std::string> &list, const std::string &key) {
    return std::find(list.begin(), list.end(), key) != list.end();
}
} // namespace

Target current_target() {
#if defined(TK64_MOBILE)
    return Target::Handheld;
#elif defined(_WIN32)
    return Target::Windows;
#else
    return Target::Linux;
#endif
}

const std::vector<std::string> &all_keys() {
    static const std::vector<std::string> keys = {"auto", "d3d12", "vulkan", "d3d11", "opengl", "opengles"};
    return keys;
}

uint32_t enum_value(const std::string &key) {
    const auto &keys = all_keys();
    const auto found = std::find(keys.begin(), keys.end(), key);
    return found == keys.end() ? 0u : static_cast<uint32_t>(found - keys.begin());
}

std::string menu_label(const std::string &key) {
    if (key == "d3d12")
        return "DX12";
    if (key == "vulkan")
        return "Vulkan";
    if (key == "d3d11")
        return "DX11";
    if (key == "opengl")
        return "OpenGL";
    if (key == "opengles")
        return "OpenGL ES";
    return "Auto";
}

std::vector<std::string> compiled(Target target, bool wine) {
    switch (target) {
    case Target::Windows:
        return wine ? std::vector<std::string>{"vulkan", "d3d12", "d3d11", "opengl"}
                    : std::vector<std::string>{"d3d12", "vulkan", "d3d11", "opengl"};
    case Target::Linux:
        return {"vulkan", "opengl"};
    case Target::Handheld:
        // The handheld renderer (TK64_MOBILE shaders, weak profile) is only validated on OpenGL ES.
        return {"opengles"};
    }
    return {};
}

std::vector<std::string> menu(Target target, bool wine, const Probe &available, const std::string &running) {
    const auto built = compiled(target, wine);
    std::vector<std::string> result = {"auto"};
    for (const auto &key : all_keys())
        if (contains(built, key) && (key == running || (available && available(key))))
            result.push_back(key);
    return result;
}

std::string effective(const std::string &saved, const std::vector<std::string> &menu) {
    return saved != "auto" && contains(menu, saved) ? saved : "auto";
}

std::vector<std::string> candidates(Target target, bool wine, const std::string &effective) {
    const auto built = compiled(target, wine);
    std::vector<std::string> result;
    if (contains(built, effective))
        result.push_back(effective);
    for (const auto &key : built)
        if (!contains(result, key))
            result.push_back(key);
    return result;
}

std::string environment_value(Target target, const std::string &effective, const Probe &available) {
    switch (target) {
    case Target::Handheld:
        return "opengles";
    case Target::Linux:
        if (effective == "opengl")
            return "opengl";
        if (effective == "auto" && available && !available("vulkan") && available("opengl"))
            return "opengl";
        return "";
    case Target::Windows:
        return effective == "auto" ? "" : effective;
    }
    return "";
}

std::string auto_description(Target target, bool wine) {
    const char *apply = " Changes apply on next launch.";
    switch (target) {
    case Target::Windows: {
        const auto order = compiled(target, wine);
        std::string text = "Auto tries ";
        for (size_t i = 0; i < order.size(); i++) {
            if (i > 0)
                text += i + 1 == order.size() ? ", then " : ", ";
            text += menu_label(order[i]);
        }
        return text + " until one works." + apply;
    }
    case Target::Linux:
        return std::string("Auto uses Vulkan, or OpenGL when no Vulkan device is found.") + apply;
    case Target::Handheld:
        return std::string("Auto uses OpenGL ES.") + apply;
    }
    return apply;
}

} // namespace tooie_graphics_api
