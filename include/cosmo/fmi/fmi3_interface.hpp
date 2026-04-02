/**
 * @file fmi3_interface.hpp
 * @brief FMI 3.0标准接口封装
 *
 * 本文件提供了FMI 3.0标准的C++封装接口。
 * FMI 3.0支持更丰富的数据类型和统一的接口模式。
 *
 * @author COSMO Development Team
 * @version 0.1.0
 * @date 2026-03-27
 */

#ifndef COSMO_FMI_FMI3_INTERFACE_HPP
#define COSMO_FMI_FMI3_INTERFACE_HPP

#include "cosmo/fmi/fmi_types.hpp"
#include "cosmo/util/dynamic_library.hpp"

#include <string>
#include <memory>

namespace cosmo {
namespace fmi {
namespace fmi3 {

// FMI 3.0基础类型
typedef void* fmi3_instance_t;
typedef void* fmi3_instance_environment_t;
typedef uint32_t fmi3_value_reference_t;
typedef double fmi3_float64_t;
typedef float fmi3_float32_t;
typedef int64_t fmi3_int64_t;
typedef int32_t fmi3_int32_t;
typedef int fmi3_boolean_t;
typedef const char* fmi3_string_t;
typedef size_t fmi3_size_t;

// FMI 3.0状态枚举
typedef enum {
    fmi3_ok = 0,
    fmi3_warning = 1,
    fmi3_discard = 2,
    fmi3_error = 3,
    fmi3_fatal = 4
} fmi3_status_t;

// 回调函数类型
typedef void (*fmi3_callback_logger_ft)(
    fmi3_instance_environment_t env,
    fmi3_status_t status,
    fmi3_string_t category,
    fmi3_string_t message,
    ...
);

typedef void* (*fmi3_callback_allocate_memory_ft)(fmi3_size_t nobj, fmi3_size_t size);
typedef void (*fmi3_callback_free_memory_ft)(void* obj);

// 回调函数结构体
typedef struct {
    fmi3_callback_logger_ft logger;
    fmi3_callback_allocate_memory_ft allocate_memory;
    fmi3_callback_free_memory_ft free_memory;
    fmi3_instance_environment_t instance_environment;
} fmi3_callback_functions_t;

// 函数指针类型定义
typedef const char* (*fmi3_get_version_ft)();
typedef fmi3_instance_t (*fmi3_instantiate_ft)(
    fmi3_string_t instance_name,
    fmi3_string_t instantiation_token,
    fmi3_string_t resource_path,
    fmi3_boolean_t visible,
    fmi3_boolean_t logging_on,
    fmi3_instance_environment_t instance_environment,
    fmi3_callback_functions_t* callbacks
);
typedef void (*fmi3_free_instance_ft)(fmi3_instance_t instance);
typedef fmi3_status_t (*fmi3_enter_initialization_mode_ft)(
    fmi3_instance_t instance,
    fmi3_boolean_t tolerance_defined,
    fmi3_float64_t tolerance,
    fmi3_float64_t start_time,
    fmi3_boolean_t stop_time_defined,
    fmi3_float64_t stop_time
);
typedef fmi3_status_t (*fmi3_exit_initialization_mode_ft)(fmi3_instance_t instance);
typedef fmi3_status_t (*fmi3_terminate_ft)(fmi3_instance_t instance);
typedef fmi3_status_t (*fmi3_reset_ft)(fmi3_instance_t instance);

// Get/Set函数
typedef fmi3_status_t (*fmi3_get_float64_ft)(
    fmi3_instance_t instance,
    const fmi3_value_reference_t vr[],
    fmi3_size_t nvr,
    fmi3_float64_t values[],
    fmi3_size_t n_values
);
typedef fmi3_status_t (*fmi3_get_int32_ft)(
    fmi3_instance_t instance,
    const fmi3_value_reference_t vr[],
    fmi3_size_t nvr,
    fmi3_int32_t values[],
    fmi3_size_t n_values
);
typedef fmi3_status_t (*fmi3_get_boolean_ft)(
    fmi3_instance_t instance,
    const fmi3_value_reference_t vr[],
    fmi3_size_t nvr,
    fmi3_boolean_t values[],
    fmi3_size_t n_values
);
typedef fmi3_status_t (*fmi3_get_string_ft)(
    fmi3_instance_t instance,
    const fmi3_value_reference_t vr[],
    fmi3_size_t nvr,
    fmi3_string_t values[],
    fmi3_size_t n_values
);

typedef fmi3_status_t (*fmi3_set_float64_ft)(
    fmi3_instance_t instance,
    const fmi3_value_reference_t vr[],
    fmi3_size_t nvr,
    const fmi3_float64_t values[],
    fmi3_size_t n_values
);
typedef fmi3_status_t (*fmi3_set_int32_ft)(
    fmi3_instance_t instance,
    const fmi3_value_reference_t vr[],
    fmi3_size_t nvr,
    const fmi3_int32_t values[],
    fmi3_size_t n_values
);
typedef fmi3_status_t (*fmi3_set_boolean_ft)(
    fmi3_instance_t instance,
    const fmi3_value_reference_t vr[],
    fmi3_size_t nvr,
    const fmi3_boolean_t values[],
    fmi3_size_t n_values
);
typedef fmi3_status_t (*fmi3_set_string_ft)(
    fmi3_instance_t instance,
    const fmi3_value_reference_t vr[],
    fmi3_size_t nvr,
    const fmi3_string_t values[],
    fmi3_size_t n_values
);

// Co-Simulation函数
typedef fmi3_status_t (*fmi3_do_step_ft)(
    fmi3_instance_t instance,
    fmi3_float64_t current_communication_point,
    fmi3_float64_t communication_step_size,
    fmi3_boolean_t no_set_fmu_state_prior,
    fmi3_boolean_t* event_handling_needed,
    fmi3_boolean_t* terminate_simulation,
    fmi3_boolean_t* early_return,
    fmi3_float64_t* last_successful_time
);

/**
 * @brief FMI 3.0接口类
 */
class fmi3_interface
{
public:
    fmi3_interface();
    ~fmi3_interface();

    fmi3_interface(const fmi3_interface&) = delete;
    fmi3_interface& operator=(const fmi3_interface&) = delete;

    fmi3_interface(fmi3_interface&& other) noexcept;
    fmi3_interface& operator=(fmi3_interface&& other) noexcept;

    // 库管理
    bool load_library(const std::string& library_path);
    void unload();
    bool is_loaded() const;

    // 版本信息
    const char* get_version();

    // 实例管理
    fmi3_instance_t instantiate(
        const std::string& instance_name,
        const std::string& instantiation_token,
        const std::string& resource_path,
        bool visible,
        bool logging_on,
        fmi3_instance_environment_t instance_environment,
        fmi3_callback_functions_t* callbacks
    );
    void free_instance(fmi3_instance_t instance);

    fmi3_status_t enter_initialization_mode(
        fmi3_instance_t instance,
        bool tolerance_defined,
        fmi3_float64_t tolerance,
        fmi3_float64_t start_time,
        bool stop_time_defined,
        fmi3_float64_t stop_time
    );
    fmi3_status_t exit_initialization_mode(fmi3_instance_t instance);
    fmi3_status_t terminate(fmi3_instance_t instance);
    fmi3_status_t reset(fmi3_instance_t instance);

    // 变量访问
    fmi3_status_t get_float64(
        fmi3_instance_t instance,
        const fmi3_value_reference_t vr[],
        fmi3_size_t nvr,
        fmi3_float64_t values[],
        fmi3_size_t n_values
    );
    fmi3_status_t get_int32(
        fmi3_instance_t instance,
        const fmi3_value_reference_t vr[],
        fmi3_size_t nvr,
        fmi3_int32_t values[],
        fmi3_size_t n_values
    );
    fmi3_status_t get_boolean(
        fmi3_instance_t instance,
        const fmi3_value_reference_t vr[],
        fmi3_size_t nvr,
        fmi3_boolean_t values[],
        fmi3_size_t n_values
    );
    fmi3_status_t get_string(
        fmi3_instance_t instance,
        const fmi3_value_reference_t vr[],
        fmi3_size_t nvr,
        fmi3_string_t values[],
        fmi3_size_t n_values
    );

    fmi3_status_t set_float64(
        fmi3_instance_t instance,
        const fmi3_value_reference_t vr[],
        fmi3_size_t nvr,
        const fmi3_float64_t values[],
        fmi3_size_t n_values
    );
    fmi3_status_t set_int32(
        fmi3_instance_t instance,
        const fmi3_value_reference_t vr[],
        fmi3_size_t nvr,
        const fmi3_int32_t values[],
        fmi3_size_t n_values
    );
    fmi3_status_t set_boolean(
        fmi3_instance_t instance,
        const fmi3_value_reference_t vr[],
        fmi3_size_t nvr,
        const fmi3_boolean_t values[],
        fmi3_size_t n_values
    );
    fmi3_status_t set_string(
        fmi3_instance_t instance,
        const fmi3_value_reference_t vr[],
        fmi3_size_t nvr,
        const fmi3_string_t values[],
        fmi3_size_t n_values
    );

    // Co-Simulation
    fmi3_status_t do_step(
        fmi3_instance_t instance,
        fmi3_float64_t current_communication_point,
        fmi3_float64_t communication_step_size,
        bool no_set_fmu_state_prior,
        bool* event_handling_needed,
        bool* terminate_simulation,
        bool* early_return,
        fmi3_float64_t* last_successful_time
    );

    // 错误处理
    const std::string& get_error() const;

private:
    std::unique_ptr<util::dynamic_library> library_;
    std::string error_;

    // 函数指针
    fmi3_get_version_ft get_version_;
    fmi3_instantiate_ft instantiate_;
    fmi3_free_instance_ft free_instance_;
    fmi3_enter_initialization_mode_ft enter_initialization_mode_;
    fmi3_exit_initialization_mode_ft exit_initialization_mode_;
    fmi3_terminate_ft terminate_;
    fmi3_reset_ft reset_;
    fmi3_get_float64_ft get_float64_;
    fmi3_get_int32_ft get_int32_;
    fmi3_get_boolean_ft get_boolean_;
    fmi3_get_string_ft get_string_;
    fmi3_set_float64_ft set_float64_;
    fmi3_set_int32_ft set_int32_;
    fmi3_set_boolean_ft set_boolean_;
    fmi3_set_string_ft set_string_;
    fmi3_do_step_ft do_step_;

    bool load_function_pointers();
};

std::unique_ptr<fmi3_interface> create_fmi3_interface();

} // namespace fmi3
} // namespace fmi
} // namespace cosmo

#endif // COSMO_FMI_FMI3_INTERFACE_HPP
