#include "cosmo/util/dynamic_library.hpp"
#include "cosmo/logger/logger.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace cosmo {
namespace util {

dynamic_library::dynamic_library()
    : handle_(nullptr)
{
}

dynamic_library::~dynamic_library()
{
    unload();
}

dynamic_library::dynamic_library(dynamic_library&& other) noexcept
    : handle_(other.handle_)
    , path_(std::move(other.path_))
    , error_(std::move(other.error_))
{
    other.handle_ = nullptr;
}

dynamic_library& dynamic_library::operator=(dynamic_library&& other) noexcept
{
    if (this != &other) {
        unload();
        handle_ = other.handle_;
        path_ = std::move(other.path_);
        error_ = std::move(other.error_);
        other.handle_ = nullptr;
    }
    return *this;
}

bool dynamic_library::load(const std::string& library_path)
{
    if (is_loaded()) {
        unload();
    }

    path_ = library_path;
    error_.clear();

    if (!platform_load(library_path)) {
        error_ = platform_get_error();
        cosmo::log::error("Failed to load library: " + library_path + " - " + error_);
        return false;
    }

    cosmo::log::info("Successfully loaded library: " + library_path);
    return true;
}

void dynamic_library::unload()
{
    if (handle_) {
        platform_unload();
        handle_ = nullptr;
        cosmo::log::info("Unloaded library: " + path_);
    }
    path_.clear();
    error_.clear();
}

bool dynamic_library::is_loaded() const
{
    return handle_ != nullptr;
}

void* dynamic_library::get_symbol(const std::string& symbol_name)
{
    if (!is_loaded()) {
        error_ = "Library not loaded";
        return nullptr;
    }

    void* symbol = platform_get_symbol(symbol_name);
    if (!symbol) {
        error_ = platform_get_error();
        cosmo::log::warn("Failed to get symbol: " + symbol_name + " - " + error_);
    }

    return symbol;
}

const std::string& dynamic_library::get_error() const
{
    return error_;
}

const std::string& dynamic_library::get_path() const
{
    return path_;
}

bool dynamic_library::platform_load(const std::string& path)
{
#ifdef _WIN32
    handle_ = LoadLibraryA(path.c_str());
    return handle_ != nullptr;
#else
    handle_ = dlopen(path.c_str(), RTLD_LAZY);
    return handle_ != nullptr;
#endif
}

void dynamic_library::platform_unload()
{
#ifdef _WIN32
    if (handle_) {
        FreeLibrary(static_cast<HMODULE>(handle_));
    }
#else
    if (handle_) {
        dlclose(handle_);
    }
#endif
}

void* dynamic_library::platform_get_symbol(const std::string& symbol_name)
{
#ifdef _WIN32
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle_), symbol_name.c_str()));
#else
    return dlsym(handle_, symbol_name.c_str());
#endif
}

std::string dynamic_library::platform_get_error()
{
#ifdef _WIN32
    DWORD error = GetLastError();
    if (error == 0) {
        return "";
    }

    LPSTR message_buffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&message_buffer),
        0,
        nullptr
    );

    std::string message(message_buffer, size);
    LocalFree(message_buffer);
    return message;
#else
    char* error = dlerror();
    return error ? error : "";
#endif
}

// 工厂函数
std::unique_ptr<dynamic_library> create_dynamic_library()
{
    return std::unique_ptr<dynamic_library>(new dynamic_library());
}

} // namespace util
} // namespace cosmo