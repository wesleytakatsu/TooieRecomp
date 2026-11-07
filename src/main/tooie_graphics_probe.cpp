// Finds which graphics backends can start here without creating a renderer (include/tooie_graphics_api.h).
#include "tooie_graphics_api.h"
#include <algorithm>
#include <cstdlib>
#include <map>
#include <mutex>
#include <initializer_list>
#include <sstream>
#include <vector>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan_core.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#else
#include <dlfcn.h>
#endif

namespace tooie_graphics_api {
namespace {

#ifdef _WIN32
using Library = HMODULE;
Library open_library(const char *name) {
    return LoadLibraryExA(name, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32 | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
}
void *symbol(Library library, const char *name) { return reinterpret_cast<void *>(GetProcAddress(library, name)); }
void close_library(Library library) { FreeLibrary(library); }
#else
using Library = void *;
Library open_library(const char *name) { return dlopen(name, RTLD_NOW | RTLD_LOCAL); }
void *symbol(Library library, const char *name) { return dlsym(library, name); }
void close_library(Library library) { dlclose(library); }
#endif

// Opens the first library of the list that loads, as the backends do, and closes it again.
bool any_library(std::initializer_list<const char *> names) {
    for (const char *name : names) {
        if (Library library = open_library(name)) {
            close_library(library);
            return true;
        }
    }
    return false;
}

// An instance at Vulkan 1.2 (plume's version) with at least one physical device that reports 1.2.
bool probe_vulkan() {
#ifdef _WIN32
    Library library = open_library("vulkan-1.dll");
#else
    Library library = open_library("libvulkan.so.1");
    if (!library)
        library = open_library("libvulkan.so");
#endif
    if (!library)
        return false;
    bool found = false;
    auto get_proc = reinterpret_cast<PFN_vkGetInstanceProcAddr>(symbol(library, "vkGetInstanceProcAddr"));
    auto create = get_proc ? reinterpret_cast<PFN_vkCreateInstance>(get_proc(nullptr, "vkCreateInstance")) : nullptr;
    VkApplicationInfo app{};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "TooieRecompiled probe";
    app.apiVersion = VK_API_VERSION_1_2;
    VkInstanceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.pApplicationInfo = &app;
    VkInstance instance = VK_NULL_HANDLE;
    if (create && create(&info, nullptr, &instance) == VK_SUCCESS) {
        auto destroy = reinterpret_cast<PFN_vkDestroyInstance>(get_proc(instance, "vkDestroyInstance"));
        auto enumerate =
            reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(get_proc(instance, "vkEnumeratePhysicalDevices"));
        auto properties = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(
            get_proc(instance, "vkGetPhysicalDeviceProperties"));
        uint32_t count = 0;
        if (enumerate && properties && enumerate(instance, &count, nullptr) == VK_SUCCESS && count > 0) {
            std::vector<VkPhysicalDevice> devices(count);
            if (enumerate(instance, &count, devices.data()) >= VK_SUCCESS) {
                for (uint32_t i = 0; i < count && !found; i++) {
                    VkPhysicalDeviceProperties props{};
                    properties(devices[i], &props);
                    found = props.apiVersion >= VK_API_VERSION_1_2;
                }
            }
        }
        if (destroy)
            destroy(instance, nullptr);
    }
    close_library(library);
    return found;
}

#ifdef _WIN32
// Any hardware adapter that takes a D3D12 device at feature level 11_0 (plume's minimum). No device is created.
bool probe_d3d12() {
    Library dxgi = open_library("dxgi.dll");
    Library d3d12 = open_library("d3d12.dll");
    bool found = false;
    auto create_factory =
        dxgi ? reinterpret_cast<HRESULT(WINAPI *)(REFIID, void **)>(symbol(dxgi, "CreateDXGIFactory1")) : nullptr;
    auto create_device = d3d12 ? reinterpret_cast<PFN_D3D12_CREATE_DEVICE>(symbol(d3d12, "D3D12CreateDevice")) : nullptr;
    IDXGIFactory1 *factory = nullptr;
    if (create_factory && create_device && SUCCEEDED(create_factory(__uuidof(IDXGIFactory1), reinterpret_cast<void **>(&factory)))) {
        IDXGIAdapter1 *adapter = nullptr;
        for (UINT i = 0; !found && factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++) {
            DXGI_ADAPTER_DESC1 desc{};
            if (SUCCEEDED(adapter->GetDesc1(&desc)) && !(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
                found = SUCCEEDED(create_device(adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr));
            adapter->Release();
        }
        factory->Release();
    }
    if (d3d12)
        close_library(d3d12);
    if (dxgi)
        close_library(dxgi);
    return found;
}

// A hardware D3D11 device at feature level 11_0 or above. No device is created.
bool probe_d3d11() {
    Library d3d11 = open_library("d3d11.dll");
    if (!d3d11)
        return false;
    auto create = reinterpret_cast<PFN_D3D11_CREATE_DEVICE>(symbol(d3d11, "D3D11CreateDevice"));
    const D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL_11_0;
    const bool found = create && SUCCEEDED(create(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, &level, 1,
                                                  D3D11_SDK_VERSION, nullptr, nullptr, nullptr));
    close_library(d3d11);
    return found;
}
#endif

bool probe(const std::string &key) {
    if (key == "vulkan")
        return probe_vulkan();
#ifdef _WIN32
    if (key == "d3d12")
        return probe_d3d12();
    if (key == "d3d11")
        return probe_d3d11();
    if (key == "opengl")
        return any_library({"opengl32.dll"}); // The 4.3 context is checked by the worker that starts it.
#else
    // plume's OpenGL backend uses EGL with the GL or GLES client library (lib/rt64/src/contrib/plume/plume_gles.cpp).
    if (key == "opengl")
        return any_library({"libEGL.so.1", "libEGL.so"}) && any_library({"libOpenGL.so.0", "libGL.so.1"});
    if (key == "opengles")
        return any_library({"libEGL.so.1", "libEGL.so"}) && any_library({"libGLESv2.so.2", "libGLESv2.so"});
#endif
    return false;
}

std::mutex cache_mutex;
std::map<std::string, bool> cache;

} // namespace

bool probe_available(const std::string &key) {
    std::lock_guard lock(cache_mutex);
    if (auto found = cache.find(key); found != cache.end())
        return found->second;
    bool result;
    if (const char *measured = std::getenv("TOOIE_GRAPHICS_AVAILABLE"); measured != nullptr) {
        // A supervisor measured this already; an empty list means nothing was found.
        result = false;
        std::stringstream list(measured);
        for (std::string item; std::getline(list, item, ',');)
            result = result || item == key;
    } else {
        result = probe(key);
    }
    cache[key] = result;
    return result;
}

std::vector<std::string> gpus() {
    std::vector<std::string> result;
    if (const char *measured = std::getenv("TOOIE_GRAPHICS_DEVICES"); measured != nullptr) {
        std::stringstream list(measured);
        for (std::string item; std::getline(list, item, '|');)
            if (!item.empty())
                result.push_back(item);
        return result;
    }
#ifdef _WIN32
    Library dxgi = open_library("dxgi.dll");
    auto create_factory =
        dxgi ? reinterpret_cast<HRESULT(WINAPI *)(REFIID, void **)>(symbol(dxgi, "CreateDXGIFactory1")) : nullptr;
    IDXGIFactory1 *factory = nullptr;
    if (create_factory && SUCCEEDED(create_factory(__uuidof(IDXGIFactory1), reinterpret_cast<void **>(&factory)))) {
        // Windows 10 1803+ orders adapters the way Settings -> Graphics does; older systems keep enumeration order.
        IDXGIFactory6 *factory6 = nullptr;
        factory->QueryInterface(__uuidof(IDXGIFactory6), reinterpret_cast<void **>(&factory6));
        IDXGIAdapter1 *adapter = nullptr;
        for (UINT i = 0;; i++) {
            const HRESULT found =
                factory6 ? factory6->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                                                __uuidof(IDXGIAdapter1), reinterpret_cast<void **>(&adapter))
                         : factory->EnumAdapters1(i, &adapter);
            if (found == DXGI_ERROR_NOT_FOUND || FAILED(found))
                break;
            DXGI_ADAPTER_DESC1 desc{};
            if (SUCCEEDED(adapter->GetDesc1(&desc)) && !(desc.Flags & (DXGI_ADAPTER_FLAG_SOFTWARE | DXGI_ADAPTER_FLAG_REMOTE))) {
                // Plume names a D3D device by this description, converted to UTF-8.
                const int n = WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, nullptr, 0, nullptr, nullptr);
                std::string name(n > 0 ? static_cast<size_t>(n - 1) : 0, '\0');
                if (n > 1)
                    WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, name.data(), n, nullptr, nullptr);
                if (!name.empty() && name.find('|') == std::string::npos &&
                    std::find(result.begin(), result.end(), name) == result.end())
                    result.push_back(name);
            }
            adapter->Release();
        }
        if (factory6)
            factory6->Release();
        factory->Release();
    }
    if (dxgi)
        close_library(dxgi);
#endif
    return result;
}

std::string gpu_list_value(const std::vector<std::string> &gpus) {
    // A lone '|' stands for "measured, none found": an empty variable would be removed on Windows.
    std::string value;
    for (const auto &name : gpus)
        value += (value.empty() ? "" : "|") + name;
    return value.empty() ? "|" : value;
}

std::string probe_list(bool wine) {
    std::string list;
    for (const auto &key : compiled(current_target(), wine)) {
        if (probe_available(key))
            list += (list.empty() ? "" : ",") + key;
    }
    return list;
}

} // namespace tooie_graphics_api
