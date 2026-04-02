/**
 * @file fmi_interface.hpp
 * @brief FMI 2.0标准接口封装
 * 
 * 本文件提供了FMI 2.0标准的C++封装接口。
 * 通过动态加载FMU共享库，提供统一的FMI函数调用机制。
 * 
 * @author COSMO Development Team
 * @version 0.1.0
 * @date 2026-03-27
 * 
 * @copyright Copyright (c) 2026
 * 
 * @note 本接口仅支持FMI 2.0 for Co-Simulation标准
 * @see https://fmi-standard.org/
 */

#ifndef COSMO_FMI_FMI_INTERFACE_HPP
#define COSMO_FMI_FMI_INTERFACE_HPP

#include "cosmo/fmi/fmi_types.hpp"
#include "cosmo/fmi/model_description.hpp"
#include "cosmo/util/dynamic_library.hpp"

#include <string>
#include <memory>
#include <map>

namespace cosmo {
namespace fmi {

/**
 * @brief FMI 2.0类型和函数指针定义
 * 
 * 此命名空间包含FMI 2.0标准定义的所有类型和函数指针类型。
 * 这些定义与FMI 2.0标准头文件保持一致。
 */
namespace fmi2 {
    /** @name FMI 2.0基础类型定义 @{ */
    typedef void* fmi2_component_t;                    ///< FMU组件实例句柄
    typedef void* fmi2_component_environment_t;        ///< 组件环境指针
    typedef unsigned int fmi2_value_reference_t;       ///< 值引用类型
    typedef double fmi2_real_t;                        ///< 实数类型
    typedef int fmi2_integer_t;                        ///< 整数类型
    typedef int fmi2_boolean_t;                        ///< 布尔类型（FMI使用int）
    typedef char fmi2_char_t;                          ///< 字符类型
    typedef const fmi2_char_t* fmi2_string_t;          ///< 字符串类型
    typedef char fmi2_byte_t;                          ///< 字节类型
    /** @} */

    /**
     * @brief FMI 2.0状态枚举
     * 
     * 定义FMI 2.0函数可能返回的状态码。
     */
    typedef enum {
        fmi2_status_ok = 0,       ///< 成功
        fmi2_status_warning = 1,  ///< 警告
        fmi2_status_discard = 2,  ///< 丢弃
        fmi2_status_error = 3,    ///< 错误
        fmi2_status_fatal = 4,    ///< 致命错误
        fmi2_status_pending = 5   ///< 异步操作进行中
    } fmi2_status_t;

    /**
     * @brief FMI 2.0回调函数类型定义
     * 
     * 这些函数指针类型用于FMU回调仿真环境。
     */
    
    /**
     * @brief 日志回调函数类型
     * 
     * FMU通过此回调函数输出日志信息。
     * 
     * @param component_environment 组件环境指针
     * @param instance_name 实例名称
     * @param status 状态码
     * @param category 日志类别
     * @param message 日志消息（printf格式）
     * @param ... 可变参数
     */
    typedef void (*fmi2_callback_logger_ft)(
        fmi2_component_environment_t, 
        fmi2_string_t, 
        fmi2_status_t, 
        fmi2_string_t, 
        fmi2_string_t, 
        ...
    );
    
    /**
     * @brief 内存分配回调函数类型
     */
    typedef void* (*fmi2_allocate_memory_ft)(size_t, size_t);
    
    /**
     * @brief 内存释放回调函数类型
     */
    typedef void (*fmi2_free_memory_ft)(void*);
    
    /**
     * @brief 步进完成回调函数类型
     */
    typedef void (*fmi2_step_finished_ft)(
        fmi2_component_environment_t, 
        fmi2_status_t
    );

    /**
     * @brief FMI 2.0回调函数结构体
     * 
     * 包含所有FMU可能调用的回调函数指针。
     * 在实例化FMU时传递给fmi2Instantiate函数。
     */
    typedef struct {
        fmi2_callback_logger_ft logger;                 ///< 日志回调
        fmi2_allocate_memory_ft allocate_memory;        ///< 内存分配回调
        fmi2_free_memory_ft free_memory;                ///< 内存释放回调
        fmi2_step_finished_ft step_finished;            ///< 步进完成回调
        fmi2_component_environment_t component_environment; ///< 组件环境
    } fmi2_callback_functions_t;

    /** @name FMI 2.0 API函数指针类型 @{ */
    typedef const char* (*fmi2_get_version_ft)();
    typedef const char* (*fmi2_get_types_platform_ft)();
    typedef fmi2_status_t (*fmi2_set_debug_logging_ft)(
        fmi2_component_t, fmi2_boolean_t, size_t, const fmi2_string_t[]
    );
    typedef fmi2_component_t (*fmi2_instantiate_ft)(
        fmi2_string_t, fmi2_integer_t, fmi2_string_t, fmi2_string_t, 
        const fmi2_callback_functions_t*, fmi2_boolean_t, fmi2_boolean_t
    );
    typedef void (*fmi2_free_instance_ft)(fmi2_component_t);
    typedef fmi2_status_t (*fmi2_setup_experiment_ft)(
        fmi2_component_t, fmi2_boolean_t, fmi2_real_t, fmi2_real_t, 
        fmi2_boolean_t, fmi2_real_t
    );
    typedef fmi2_status_t (*fmi2_enter_initialization_mode_ft)(fmi2_component_t);
    typedef fmi2_status_t (*fmi2_exit_initialization_mode_ft)(fmi2_component_t);
    typedef fmi2_status_t (*fmi2_terminate_ft)(fmi2_component_t);
    typedef fmi2_status_t (*fmi2_reset_ft)(fmi2_component_t);
    typedef fmi2_status_t (*fmi2_get_real_ft)(
        fmi2_component_t, const fmi2_value_reference_t[], size_t, fmi2_real_t[]
    );
    typedef fmi2_status_t (*fmi2_get_integer_ft)(
        fmi2_component_t, const fmi2_value_reference_t[], size_t, fmi2_integer_t[]
    );
    typedef fmi2_status_t (*fmi2_get_boolean_ft)(
        fmi2_component_t, const fmi2_value_reference_t[], size_t, fmi2_boolean_t[]
    );
    typedef fmi2_status_t (*fmi2_get_string_ft)(
        fmi2_component_t, const fmi2_value_reference_t[], size_t, fmi2_string_t[]
    );
    typedef fmi2_status_t (*fmi2_set_real_ft)(
        fmi2_component_t, const fmi2_value_reference_t[], size_t, const fmi2_real_t[]
    );
    typedef fmi2_status_t (*fmi2_set_integer_ft)(
        fmi2_component_t, const fmi2_value_reference_t[], size_t, const fmi2_integer_t[]
    );
    typedef fmi2_status_t (*fmi2_set_boolean_ft)(
        fmi2_component_t, const fmi2_value_reference_t[], size_t, const fmi2_boolean_t[]
    );
    typedef fmi2_status_t (*fmi2_set_string_ft)(
        fmi2_component_t, const fmi2_value_reference_t[], size_t, const fmi2_string_t[]
    );
    typedef fmi2_status_t (*fmi2_get_fmu_state_ft)(fmi2_component_t, void**);
    typedef fmi2_status_t (*fmi2_set_fmu_state_ft)(fmi2_component_t, void*);
    typedef fmi2_status_t (*fmi2_free_fmu_state_ft)(fmi2_component_t, void**);
    typedef fmi2_status_t (*fmi2_do_step_ft)(
        fmi2_component_t, fmi2_real_t, fmi2_real_t, fmi2_boolean_t
    );
    /** @} */
}

/**
 * @brief FMI 2.0接口封装类
 * 
 * 提供FMI 2.0标准函数的C++封装。
 * 负责动态加载FMU共享库，并管理FMI函数指针。
 * 
 * @note 此类实现了移动语义，支持资源的安全转移
 * @warning 不支持拷贝，确保每个实例独占FMU资源
 */
class fmi_interface
{
public:
    /** @name 构造与析构 @{ */
    
    /**
     * @brief 默认构造函数
     * 
     * 创建一个未加载任何库的FMI接口实例。
     */
    fmi_interface();
    
    /**
     * @brief 析构函数
     * 
     * 自动卸载已加载的库。
     */
    ~fmi_interface();
    
    /** @} */

    /** @name 禁用拷贝 @{ */
    
    /**
     * @brief 禁用拷贝构造函数
     * 
     * FMI接口实例不可拷贝，以确保资源独占性。
     */
    fmi_interface(const fmi_interface&) = delete;
    
    /**
     * @brief 禁用拷贝赋值运算符
     */
    fmi_interface& operator=(const fmi_interface&) = delete;
    
    /** @} */

    /** @name 移动语义 @{ */
    
    /**
     * @brief 移动构造函数
     * @param other 要移动的源对象
     * 
     * 转移FMI接口资源的所有权。
     */
    fmi_interface(fmi_interface&& other) noexcept;
    
    /**
     * @brief 移动赋值运算符
     * @param other 要移动的源对象
     * @return 当前对象的引用
     */
    fmi_interface& operator=(fmi_interface&& other) noexcept;
    
    /** @} */

    /** @name 库管理 @{ */
    
    /**
     * @brief 加载FMU共享库
     * @param library_path 共享库文件路径
     * @return 加载成功返回true，失败返回false
     * 
     * 加载FMU共享库并解析所有必需的FMI函数符号。
     * 如果已加载其他库，会先自动卸载。
     * 
     * @note 支持的库格式：Windows(.dll), Linux(.so), macOS(.dylib)
     */
    bool load_library(const std::string& library_path);

    /**
     * @brief 卸载共享库
     * 
     * 释放已加载的共享库资源。
     * 安全函数，即使未加载库也可调用。
     */
    void unload();

    /**
     * @brief 检查库是否已加载
     * @return 已加载返回true，否则返回false
     */
    bool is_loaded() const;
    
    /** @} */

    /** @name FMI版本信息 @{ */
    
    /**
     * @brief 获取FMI版本字符串
     * @return FMI版本号字符串（如"2.0"）
     * @pre 必须先成功调用load_library()
     */
    const char* get_version();

    /**
     * @brief 获取类型平台标识
     * @return 平台标识字符串（如"default"或"win64"等）
     * @pre 必须先成功调用load_library()
     */
    const char* get_types_platform();
    
    /** @} */

    /** @name 实例生命周期管理 @{ */
    
    /**
     * @brief 实例化FMU
     * @param instance_name 实例名称（用于日志等）
     * @param fmu_type FMU类型（1=模型交换, 2=共仿真）
     * @param fmu_guid FMU的GUID标识符
     * @param fmu_resource_location FMU资源位置（文件路径或URI）
     * @param functions 回调函数结构体
     * @param visible 是否可见（UI显示）
     * @param logging_on 是否启用日志
     * @return FMU组件实例句柄，失败返回nullptr
     * 
     * 创建FMU模型的新实例。每个实例有独立的内部状态。
     * 
     * @pre 必须先成功调用load_library()
     * @see fmi2Instantiate
     */
    fmi2::fmi2_component_t instantiate(
        const std::string& instance_name,
        fmi2::fmi2_integer_t fmu_type,
        const std::string& fmu_guid,
        const std::string& fmu_resource_location,
        const fmi2::fmi2_callback_functions_t* functions,
        bool visible,
        bool logging_on
    );

    /**
     * @brief 释放FMU实例
     * @param component 要释放的实例句柄
     * 
     * 释放由instantiate()创建的实例资源。
     */
    void free_instance(fmi2::fmi2_component_t component);

    /**
     * @brief 设置调试日志
     * @param component 实例句柄
     * @param logging_on 是否启用日志
     * @param n_categories 日志类别数量
     * @param categories 日志类别名称数组
     * @return 操作状态码
     */
    fmi2::fmi2_status_t set_debug_logging(
        fmi2::fmi2_component_t component,
        bool logging_on,
        size_t n_categories,
        const char* categories[]
    );

    /**
     * @brief 设置仿真实验参数
     * @param component 实例句柄
     * @param tolerance_defined 容差是否定义
     * @param tolerance 容差值（如定义）
     * @param start_time 仿真开始时间
     * @param stop_time_defined 停止时间是否定义
     * @param stop_time 停止时间（如定义）
     * @return 操作状态码
     * 
     * 在初始化之前设置仿真的时间参数。
     */
    fmi2::fmi2_status_t setup_experiment(
        fmi2::fmi2_component_t component,
        bool tolerance_defined,
        double tolerance,
        double start_time,
        bool stop_time_defined,
        double stop_time
    );

    /**
     * @brief 进入初始化模式
     * @param component 实例句柄
     * @return 操作状态码
     * 
     * 开始初始化阶段，此时可以设置初始参数值。
     */
    fmi2::fmi2_status_t enter_initialization_mode(fmi2::fmi2_component_t component);

    /**
     * @brief 退出初始化模式
     * @param component 实例句柄
     * @return 操作状态码
     * 
     * 完成初始化，准备开始仿真。
     */
    fmi2::fmi2_status_t exit_initialization_mode(fmi2::fmi2_component_t component);

    /**
     * @brief 终止仿真
     * @param component 实例句柄
     * @return 操作状态码
     * 
     * 正常结束仿真，释放仿真资源。
     */
    fmi2::fmi2_status_t terminate(fmi2::fmi2_component_t component);

    /**
     * @brief 重置实例
     * @param component 实例句柄
     * @return 操作状态码
     * 
     * 将实例重置为初始化后的状态，可重新配置参数。
     */
    fmi2::fmi2_status_t reset(fmi2::fmi2_component_t component);
    
    /** @} */

    /** @name 变量访问 @{ */
    
    /**
     * @brief 获取实数值
     * @param component 实例句柄
     * @param vr 值引用数组
     * @param nvr 值引用数量
     * @param value 输出值数组（由调用者分配）
     * @return 操作状态码
     */
    fmi2::fmi2_status_t get_real(
        fmi2::fmi2_component_t component,
        const fmi2::fmi2_value_reference_t* vr,
        size_t nvr,
        fmi2::fmi2_real_t* value
    );

    /**
     * @brief 获取整数值
     * @param component 实例句柄
     * @param vr 值引用数组
     * @param nvr 值引用数量
     * @param value 输出值数组
     * @return 操作状态码
     */
    fmi2::fmi2_status_t get_integer(
        fmi2::fmi2_component_t component,
        const fmi2::fmi2_value_reference_t* vr,
        size_t nvr,
        fmi2::fmi2_integer_t* value
    );

    /**
     * @brief 获取布尔值
     * @param component 实例句柄
     * @param vr 值引用数组
     * @param nvr 值引用数量
     * @param value 输出值数组
     * @return 操作状态码
     */
    fmi2::fmi2_status_t get_boolean(
        fmi2::fmi2_component_t component,
        const fmi2::fmi2_value_reference_t* vr,
        size_t nvr,
        fmi2::fmi2_boolean_t* value
    );

    /**
     * @brief 获取字符串值
     * @param component 实例句柄
     * @param vr 值引用数组
     * @param nvr 值引用数量
     * @param value 输出值数组（指向FMU内部字符串，无需释放）
     * @return 操作状态码
     */
    fmi2::fmi2_status_t get_string(
        fmi2::fmi2_component_t component,
        const fmi2::fmi2_value_reference_t* vr,
        size_t nvr,
        fmi2::fmi2_string_t* value
    );

    /**
     * @brief 设置实数值
     * @param component 实例句柄
     * @param vr 值引用数组
     * @param nvr 值引用数量
     * @param value 输入值数组
     * @return 操作状态码
     */
    fmi2::fmi2_status_t set_real(
        fmi2::fmi2_component_t component,
        const fmi2::fmi2_value_reference_t* vr,
        size_t nvr,
        const fmi2::fmi2_real_t* value
    );

    /**
     * @brief 设置整数值
     * @param component 实例句柄
     * @param vr 值引用数组
     * @param nvr 值引用数量
     * @param value 输入值数组
     * @return 操作状态码
     */
    fmi2::fmi2_status_t set_integer(
        fmi2::fmi2_component_t component,
        const fmi2::fmi2_value_reference_t* vr,
        size_t nvr,
        const fmi2::fmi2_integer_t* value
    );

    /**
     * @brief 设置布尔值
     * @param component 实例句柄
     * @param vr 值引用数组
     * @param nvr 值引用数量
     * @param value 输入值数组
     * @return 操作状态码
     */
    fmi2::fmi2_status_t set_boolean(
        fmi2::fmi2_component_t component,
        const fmi2::fmi2_value_reference_t* vr,
        size_t nvr,
        const fmi2::fmi2_boolean_t* value
    );

    /**
     * @brief 设置字符串值
     * @param component 实例句柄
     * @param vr 值引用数组
     * @param nvr 值引用数量
     * @param value 输入值数组（C字符串）
     * @return 操作状态码
     */
    fmi2::fmi2_status_t set_string(
        fmi2::fmi2_component_t component,
        const fmi2::fmi2_value_reference_t* vr,
        size_t nvr,
        const fmi2::fmi2_string_t* value
    );
    
    /** @} */

    /** @name 仿真步进 @{ */
    
    /**
     * @brief 执行仿真步进
     * @param component 实例句柄
     * @param current_communication_point 当前通信时间点
     * @param communication_step_size 通信步长
     * @param no_set_fmu_state_prior_to_current_point 是否禁止回退到当前点之前的状态
     * @return 操作状态码
     * 
     * 执行一个共仿真步进。这是共仿真模式的核心函数。
     * 
     * @note 此函数仅在共仿真模式下可用
     */
    fmi2::fmi2_status_t do_step(
        fmi2::fmi2_component_t component,
        double current_communication_point,
        double communication_step_size,
        bool no_set_fmu_state_prior_to_current_point
    );
    
    /** @} */

    /** @name 错误处理 @{ */
    
    /**
     * @brief 获取最后错误信息
     * @return 错误描述字符串
     * 
     * 获取最近一次操作失败的错误信息。
     */
    const std::string& get_error() const;
    
    /** @} */

private:
    std::unique_ptr<util::dynamic_library> library_;  ///< 动态库加载器
    std::string error_;                               ///< 最后错误信息

    /** @name FMI函数指针 @{ */
    fmi2::fmi2_get_version_ft get_version_;
    fmi2::fmi2_get_types_platform_ft get_types_platform_;
    fmi2::fmi2_set_debug_logging_ft set_debug_logging_;
    fmi2::fmi2_instantiate_ft instantiate_;
    fmi2::fmi2_free_instance_ft free_instance_;
    fmi2::fmi2_setup_experiment_ft setup_experiment_;
    fmi2::fmi2_enter_initialization_mode_ft enter_initialization_mode_;
    fmi2::fmi2_exit_initialization_mode_ft exit_initialization_mode_;
    fmi2::fmi2_terminate_ft terminate_;
    fmi2::fmi2_reset_ft reset_;
    fmi2::fmi2_get_real_ft get_real_;
    fmi2::fmi2_get_integer_ft get_integer_;
    fmi2::fmi2_get_boolean_ft get_boolean_;
    fmi2::fmi2_get_string_ft get_string_;
    fmi2::fmi2_set_real_ft set_real_;
    fmi2::fmi2_set_integer_ft set_integer_;
    fmi2::fmi2_set_boolean_ft set_boolean_;
    fmi2::fmi2_set_string_ft set_string_;
    fmi2::fmi2_do_step_ft do_step_;
    /** @} */

    /**
     * @brief 加载函数指针
     * @return 成功加载所有必需函数返回true
     * 
     * 从已加载的共享库中解析所有FMI函数符号。
     */
    bool load_function_pointers();
};

/**
 * @brief 创建FMI接口实例（工厂函数）
 * @return 新的FMI接口实例
 */
std::unique_ptr<fmi_interface> create_fmi_interface();

} // namespace fmi
} // namespace cosmo

#endif // COSMO_FMI_FMI_INTERFACE_HPP
