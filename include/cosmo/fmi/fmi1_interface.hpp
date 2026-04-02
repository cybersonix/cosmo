/**
 * @file fmi1_interface.hpp
 * @brief FMI 1.0标准接口封装
 * 
 * 本文件提供了FMI 1.0标准的C++封装接口。
 * FMI 1.0是FMI标准的早期版本，与2.0有较大差异。
 * 
 * @author COSMO Development Team
 * @version 0.1.0
 * @date 2026-03-27
 * 
 * @copyright Copyright (c) 2026
 * 
 * @note FMI 1.0分为Model Exchange和Co-Simulation两种变体
 * @deprecated 建议使用FMI 2.0或3.0
 */

#ifndef COSMO_FMI_FMI1_INTERFACE_HPP
#define COSMO_FMI_FMI1_INTERFACE_HPP

#include "cosmo/fmi/fmi_types.hpp"
#include "cosmo/util/dynamic_library.hpp"

#include <string>
#include <memory>

namespace cosmo {
namespace fmi {
namespace fmi1 {

/**
 * @brief FMI 1.0基础类型定义
 */
typedef void* fmi1_component_t;           ///< 组件实例句柄
typedef const char* fmi1_string_t;        ///< 字符串类型
typedef double fmi1_real_t;               ///< 实数类型
typedef int fmi1_integer_t;               ///< 整数类型
typedef int fmi1_boolean_t;               ///< 布尔类型
typedef unsigned int fmi1_value_reference_t; ///< 值引用类型

/**
 * @brief FMI 1.0状态枚举
 */
typedef enum {
    fmi1_ok = 0,           ///< 成功
    fmi1_warning = 1,      ///< 警告
    fmi1_discard = 2,      ///< 丢弃
    fmi1_error = 3,        ///< 错误
    fmi1_fatal = 4,        ///< 致命错误
    fmi1_pending = 5       ///< 进行中
} fmi1_status_t;

/**
 * @brief FMI 1.0回调函数类型
 */
typedef void (*fmi1_callback_logger_ft)(
    fmi1_component_t c,
    fmi1_string_t instance_name,
    fmi1_status_t status,
    fmi1_string_t category,
    fmi1_string_t message,
    ...
);

typedef void* (*fmi1_allocate_memory_ft)(size_t nobj, size_t size);
typedef void (*fmi1_free_memory_ft)(void* obj);

/**
 * @brief FMI 1.0事件信息结构体（前置声明）
 */
typedef struct {
    fmi1_boolean_t iteration_converged;
    fmi1_boolean_t state_value_references_changed;
    fmi1_boolean_t state_values_changed;
    fmi1_boolean_t terminate_simulation;
    fmi1_boolean_t upcoming_time_event;
    fmi1_real_t next_event_time;
} fmi1_event_info_t;

/**
 * @brief FMI 1.0回调函数结构体
 */
typedef struct {
    fmi1_callback_logger_ft logger;
    fmi1_allocate_memory_ft allocate_memory;
    fmi1_free_memory_ft free_memory;
    void* step_finished;  // FMI 1.0 ME不使用
} fmi1_callback_functions_t;

/** @name FMI 1.0函数指针类型 @{ */
typedef const char* (*fmi1_get_version_ft)();
typedef fmi1_status_t (*fmi1_set_debug_logging_ft)(
    fmi1_component_t c, fmi1_boolean_t logging_on
);
typedef fmi1_component_t (*fmi1_instantiate_model_ft)(
    fmi1_string_t instance_name,
    fmi1_string_t GUID,
    fmi1_callback_functions_t* functions,
    fmi1_boolean_t logging_on
);
typedef fmi1_component_t (*fmi1_instantiate_slave_ft)(
    fmi1_string_t instance_name,
    fmi1_string_t GUID,
    fmi1_string_t fmu_location,
    fmi1_string_t mime_type,
    fmi1_real_t timeout,
    fmi1_boolean_t visible,
    fmi1_boolean_t interactive,
    fmi1_callback_functions_t* functions,
    fmi1_boolean_t logging_on
);
typedef void (*fmi1_free_model_instance_ft)(fmi1_component_t c);
typedef void (*fmi1_free_slave_instance_ft)(fmi1_component_t c);
typedef fmi1_status_t (*fmi1_set_time_ft)(fmi1_component_t c, fmi1_real_t time);
typedef fmi1_status_t (*fmi1_set_continuous_states_ft)(
    fmi1_component_t c, const fmi1_real_t x[], size_t nx
);
typedef fmi1_status_t (*fmi1_completed_integrator_step_ft)(
    fmi1_component_t c, fmi1_boolean_t* call_event_update
);
typedef fmi1_status_t (*fmi1_initialize_ft)(
    fmi1_component_t c, fmi1_boolean_t tolerance_controlled,
    fmi1_real_t relative_tolerance, fmi1_event_info_t* event_info
);
typedef fmi1_status_t (*fmi1_get_derivatives_ft)(
    fmi1_component_t c, fmi1_real_t derivatives[], size_t nx
);
typedef fmi1_status_t (*fmi1_get_event_indicators_ft)(
    fmi1_component_t c, fmi1_real_t event_indicators[], size_t ni
);
typedef fmi1_status_t (*fmi1_event_update_ft)(
    fmi1_component_t c, fmi1_boolean_t intermediate_results,
    fmi1_event_info_t* event_info
);
typedef fmi1_status_t (*fmi1_get_continuous_states_ft)(
    fmi1_component_t c, fmi1_real_t states[], size_t nx
);
typedef fmi1_status_t (*fmi1_get_nominal_continuous_states_ft)(
    fmi1_component_t c, fmi1_real_t x_nominal[], size_t nx
);
typedef fmi1_status_t (*fmi1_get_state_value_references_ft)(
    fmi1_component_t c, fmi1_value_reference_t vrx[], size_t nx
);
typedef fmi1_status_t (*fmi1_terminate_ft)(fmi1_component_t c);

// Co-Simulation特定函数
typedef fmi1_status_t (*fmi1_initialize_slave_ft)(
    fmi1_component_t c,
    fmi1_real_t tStart,
    fmi1_boolean_t StopTimeDefined,
    fmi1_real_t tStop
);
typedef fmi1_status_t (*fmi1_terminate_slave_ft)(fmi1_component_t c);
typedef fmi1_status_t (*fmi1_reset_slave_ft)(fmi1_component_t c);
typedef fmi1_status_t (*fmi1_do_step_ft)(
    fmi1_component_t c,
    fmi1_real_t current_communication_point,
    fmi1_real_t communication_step_size,
    fmi1_boolean_t new_step
);

// 变量访问函数
typedef fmi1_status_t (*fmi1_get_real_ft)(
    fmi1_component_t c, const fmi1_value_reference_t vr[], size_t nvr, fmi1_real_t value[]
);
typedef fmi1_status_t (*fmi1_get_integer_ft)(
    fmi1_component_t c, const fmi1_value_reference_t vr[], size_t nvr, fmi1_integer_t value[]
);
typedef fmi1_status_t (*fmi1_get_boolean_ft)(
    fmi1_component_t c, const fmi1_value_reference_t vr[], size_t nvr, fmi1_boolean_t value[]
);
typedef fmi1_status_t (*fmi1_get_string_ft)(
    fmi1_component_t c, const fmi1_value_reference_t vr[], size_t nvr, fmi1_string_t value[]
);
typedef fmi1_status_t (*fmi1_set_real_ft)(
    fmi1_component_t c, const fmi1_value_reference_t vr[], size_t nvr, const fmi1_real_t value[]
);
typedef fmi1_status_t (*fmi1_set_integer_ft)(
    fmi1_component_t c, const fmi1_value_reference_t vr[], size_t nvr, const fmi1_integer_t value[]
);
typedef fmi1_status_t (*fmi1_set_boolean_ft)(
    fmi1_component_t c, const fmi1_value_reference_t vr[], size_t nvr, const fmi1_boolean_t value[]
);
typedef fmi1_status_t (*fmi1_set_string_ft)(
    fmi1_component_t c, const fmi1_value_reference_t vr[], size_t nvr, const fmi1_string_t value[]
);
/** @} */

// fmi1_event_info_t 已在前面定义

/**
 * @brief FMI 1.0接口类
 * 
 * 封装FMI 1.0标准的所有功能。
 * 支持Model Exchange和Co-Simulation两种模式。
 */
class fmi1_interface
{
public:
    /** @name 构造与析构 @{ */
    fmi1_interface();
    ~fmi1_interface();
    
    fmi1_interface(const fmi1_interface&) = delete;
    fmi1_interface& operator=(const fmi1_interface&) = delete;
    
    fmi1_interface(fmi1_interface&& other) noexcept;
    fmi1_interface& operator=(fmi1_interface&& other) noexcept;
    /** @} */

    /** @name 库管理 @{ */
    bool load_library(const std::string& library_path);
    bool load_library(const std::string& library_path, const std::string& model_identifier);
    void unload();
    bool is_loaded() const;
    /** @} */

    /** @name 版本信息 @{ */
    const char* get_version();
    /** @} */

    /** @name Model Exchange实例管理 @{ */
    fmi1_component_t instantiate_model(
        const std::string& instance_name,
        const std::string& guid,
        const fmi1_callback_functions_t* functions,
        bool logging_on
    );
    void free_model_instance(fmi1_component_t component);
    /** @} */

    /** @name Co-Simulation实例管理 @{ */
    fmi1_component_t instantiate_slave(
        const std::string& instance_name,
        const std::string& guid,
        const std::string& fmu_location,
        fmi1_real_t timeout,
        bool visible,
        bool interactive,
        const fmi1_callback_functions_t* functions,
        bool logging_on
    );
    void free_slave_instance(fmi1_component_t component);
    
    fmi1_status_t initialize_slave(
        fmi1_component_t component,
        fmi1_real_t start_time,
        bool stop_time_defined,
        fmi1_real_t stop_time
    );
    fmi1_status_t terminate_slave(fmi1_component_t component);
    fmi1_status_t reset_slave(fmi1_component_t component);
    
    /**
     * @brief 执行共仿真步进
     * @param component 实例句柄
     * @param current_communication_point 当前通信点
     * @param communication_step_size 通信步长
     * @param new_step 是否为新步
     * @return 操作状态码
     */
    fmi1_status_t do_step(
        fmi1_component_t component,
        fmi1_real_t current_communication_point,
        fmi1_real_t communication_step_size,
        bool new_step
    );
    /** @} */

    /** @name Model Exchange函数 @{ */
    fmi1_status_t set_time(fmi1_component_t component, fmi1_real_t time);
    fmi1_status_t initialize(
        fmi1_component_t component,
        bool tolerance_controlled,
        fmi1_real_t relative_tolerance,
        fmi1_event_info_t* event_info
    );
    fmi1_status_t get_derivatives(
        fmi1_component_t component,
        fmi1_real_t derivatives[],
        size_t nx
    );
    fmi1_status_t get_event_indicators(
        fmi1_component_t component,
        fmi1_real_t event_indicators[],
        size_t ni
    );
    fmi1_status_t event_update(
        fmi1_component_t component,
        bool intermediate_results,
        fmi1_event_info_t* event_info
    );
    fmi1_status_t get_continuous_states(
        fmi1_component_t component,
        fmi1_real_t states[],
        size_t nx
    );
    fmi1_status_t set_continuous_states(
        fmi1_component_t component,
        const fmi1_real_t states[],
        size_t nx
    );
    /** @} */

    /** @name 变量访问 @{ */
    fmi1_status_t get_real(
        fmi1_component_t component,
        const fmi1_value_reference_t vr[],
        size_t nvr,
        fmi1_real_t value[]
    );
    fmi1_status_t get_integer(
        fmi1_component_t component,
        const fmi1_value_reference_t vr[],
        size_t nvr,
        fmi1_integer_t value[]
    );
    fmi1_status_t get_boolean(
        fmi1_component_t component,
        const fmi1_value_reference_t vr[],
        size_t nvr,
        fmi1_boolean_t value[]
    );
    fmi1_status_t get_string(
        fmi1_component_t component,
        const fmi1_value_reference_t vr[],
        size_t nvr,
        fmi1_string_t value[]
    );
    fmi1_status_t set_real(
        fmi1_component_t component,
        const fmi1_value_reference_t vr[],
        size_t nvr,
        const fmi1_real_t value[]
    );
    fmi1_status_t set_integer(
        fmi1_component_t component,
        const fmi1_value_reference_t vr[],
        size_t nvr,
        const fmi1_integer_t value[]
    );
    fmi1_status_t set_boolean(
        fmi1_component_t component,
        const fmi1_value_reference_t vr[],
        size_t nvr,
        const fmi1_boolean_t value[]
    );
    fmi1_status_t set_string(
        fmi1_component_t component,
        const fmi1_value_reference_t vr[],
        size_t nvr,
        const fmi1_string_t value[]
    );
    /** @} */

    /** @name 错误处理 @{ */
    const std::string& get_error() const;
    /** @} */

private:
    std::unique_ptr<util::dynamic_library> library_;
    std::string error_;
    bool is_cs_;  ///< 是否为Co-Simulation模式

    // FMI 1.0函数指针
    fmi1_get_version_ft get_version_;
    fmi1_instantiate_model_ft instantiate_model_;
    fmi1_instantiate_slave_ft instantiate_slave_;
    fmi1_free_model_instance_ft free_model_instance_;
    fmi1_free_slave_instance_ft free_slave_instance_;
    fmi1_initialize_slave_ft initialize_slave_;
    fmi1_terminate_slave_ft terminate_slave_;
    fmi1_reset_slave_ft reset_slave_;
    fmi1_do_step_ft do_step_;
    fmi1_set_time_ft set_time_;
    fmi1_initialize_ft initialize_;
    fmi1_get_derivatives_ft get_derivatives_;
    fmi1_get_event_indicators_ft get_event_indicators_;
    fmi1_event_update_ft event_update_;
    fmi1_get_continuous_states_ft get_continuous_states_;
    fmi1_set_continuous_states_ft set_continuous_states_;
    fmi1_get_real_ft get_real_;
    fmi1_get_integer_ft get_integer_;
    fmi1_get_boolean_ft get_boolean_;
    fmi1_get_string_ft get_string_;
    fmi1_set_real_ft set_real_;
    fmi1_set_integer_ft set_integer_;
    fmi1_set_boolean_ft set_boolean_;
    fmi1_set_string_ft set_string_;

    bool load_function_pointers();

    std::string model_identifier_;  ///< 模型标识符（FMI 1.0函数名前缀）
};

/**
 * @brief 创建FMI 1.0接口实例（工厂函数）
 * @return 新的FMI 1.0接口实例
 */
std::unique_ptr<fmi1_interface> create_fmi1_interface();

} // namespace fmi1
} // namespace fmi
} // namespace cosmo

#endif // COSMO_FMI_FMI1_INTERFACE_HPP
