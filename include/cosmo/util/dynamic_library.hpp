#ifndef COSMO_UTIL_DYNAMIC_LIBRARY_HPP
#define COSMO_UTIL_DYNAMIC_LIBRARY_HPP

#include <string>
#include <memory>
#include <functional>

namespace cosmo {
namespace util {

// 动态库加载器类
class dynamic_library
{
public:
    dynamic_library();
    ~dynamic_library();

    // 禁用拷贝
    dynamic_library(const dynamic_library&) = delete;
    dynamic_library& operator=(const dynamic_library&) = delete;

    // 允许移动
    dynamic_library(dynamic_library&& other) noexcept;
    dynamic_library& operator=(dynamic_library&& other) noexcept;

    // 加载动态库
    bool load(const std::string& library_path);

    // 卸载动态库
    void unload();

    // 检查是否已加载
    bool is_loaded() const;

    // 获取函数指针
    void* get_symbol(const std::string& symbol_name);

    // 获取函数指针（模板版本）
    template<typename T>
    std::function<T> get_function(const std::string& function_name)
    {
        void* symbol = get_symbol(function_name);
        if (symbol) {
            return get_function_ptr<T>(symbol);
        }
        return std::function<T>();
    }

    // 将 void* 安全转换为函数指针（绕过 ISO C++ 限制）
    template<typename T>
    static std::function<T> get_function_ptr(void* symbol)
    {
        union {
            void* ptr;
            T func;
        } cast;
        cast.ptr = symbol;
        return std::function<T>(cast.func);
    }

    // 获取错误信息
    const std::string& get_error() const;

    // 获取库路径
    const std::string& get_path() const;

private:
    void* handle_;
    std::string path_;
    std::string error_;

    // 平台相关的实现
    bool platform_load(const std::string& path);
    void platform_unload();
    void* platform_get_symbol(const std::string& symbol_name);
    std::string platform_get_error();
};

// 工厂函数
std::unique_ptr<dynamic_library> create_dynamic_library();

} // namespace util
} // namespace cosmo

#endif // COSMO_UTIL_DYNAMIC_LIBRARY_HPP