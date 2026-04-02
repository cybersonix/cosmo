/**
 * @file fmi_model_instance.hpp
 * @brief 基于真实FMI接口的模型实例实现
 *
 * 将FMU模型的FMI接口桥接到仿真引擎的model_instance抽象接口。
 * 支持FMI 1.0 Co-Simulation和FMI 2.0 Co-Simulation。
 *
 * @copyright Copyright (c) 2026 COSMO Development Team
 */

#ifndef COSMO_SIMULATION_FMI_MODEL_INSTANCE_HPP
#define COSMO_SIMULATION_FMI_MODEL_INSTANCE_HPP

#include "cosmo/simulation/model_instance.hpp"
#include "cosmo/fmi/fmu_processor.hpp"
#include "cosmo/fmi/fmi1_interface.hpp"
#include "cosmo/fmi/fmi_interface.hpp"

#include <memory>

namespace cosmo {
namespace simulation {

/**
 * @brief 基于真实FMI接口的模型实例
 *
 * 通过加载FMU文件并调用FMI标准函数来实现model_instance接口。
 * 支持FMI 1.0和FMI 2.0 Co-Simulation模式。
 */
class fmi_model_instance : public model_instance
{
public:
    /**
     * @brief 构造函数 - 从FMU文件创建实例
     * @param instance_name 实例名称
     * @param fmu_path FMU文件路径
     * @param step_size_hint 建议步长
     *
     * 自动检测FMI版本并加载对应的接口。
     */
    fmi_model_instance(const std::string& instance_name,
                       const std::string& fmu_path,
                       double step_size_hint = -1.0);

    ~fmi_model_instance() override;

    // 禁用拷贝
    fmi_model_instance(const fmi_model_instance&) = delete;
    fmi_model_instance& operator=(const fmi_model_instance&) = delete;

    /** @name 生命周期管理 @{ */

    fmi::fmi_status enter_initialization_mode(double start_time = 0.0) override;
    fmi::fmi_status exit_initialization_mode() override;
    fmi::fmi_status do_step(double current_time, double step_size) override;
    fmi::fmi_status terminate() override;
    fmi::fmi_status reset() override;

    /** @} */

    /** @name 变量访问 @{ */

    fmi::fmi_status get_real(fmi::value_reference vr, fmi::fmi_real& value) override;
    fmi::fmi_status get_integer(fmi::value_reference vr, fmi::fmi_integer& value) override;
    fmi::fmi_status get_boolean(fmi::value_reference vr, fmi::fmi_boolean& value) override;
    fmi::fmi_status get_string(fmi::value_reference vr, fmi::fmi_string& value) override;

    fmi::fmi_status set_real(fmi::value_reference vr, fmi::fmi_real value) override;
    fmi::fmi_status set_integer(fmi::value_reference vr, fmi::fmi_integer value) override;
    fmi::fmi_status set_boolean(fmi::value_reference vr, fmi::fmi_boolean value) override;
    fmi::fmi_status set_string(fmi::value_reference vr, const fmi::fmi_string& value) override;

    /** @} */

    /**
     * @brief 获取最后错误信息
     */
    const std::string& get_last_error() const { return last_error_; }

    /**
     * @brief 检查FMU是否成功加载
     */
    bool is_fmu_loaded() const { return fmu_loaded_; }

    /** @name 参数集管理（覆写基类）@{ */
    bool apply_parameter_set(const std::string& name) override;
    /** @} */

private:
    bool fmu_loaded_;                          ///< FMU是否成功加载
    std::string last_error_;                   ///< 最后错误信息
    fmi::fmi_version fmi_version_;             ///< FMI版本

    // FMU处理器
    std::unique_ptr<fmi::fmu_processor> fmu_processor_;

    // FMI 1.0接口
    std::unique_ptr<fmi::fmi1::fmi1_interface> fmi1_iface_;
    fmi::fmi1::fmi1_component_t fmi1_component_;

    // FMI 2.0接口
    std::unique_ptr<fmi::fmi_interface> fmi2_iface_;
    fmi::fmi2::fmi2_component_t fmi2_component_;

    // 状态追踪
    bool inited_;
    bool terminated_;

    // 加载FMI 1.0 FMU
    bool load_fmi1(const std::string& fmu_path);

    // 加载FMI 2.0 FMU
    bool load_fmi2(const std::string& fmu_path);

    // FMI 1.0 日志回调（variadic, 不能精确声明）
    static void fmi1_logger(void* component, const char* instance_name,
                            int status, const char* category,
                            const char* message, ...);

    // FMI 2.0 日志回调（variadic, 不能精确声明）
    static void fmi2_logger(void* component_env, const char* instance_name,
                            int status, const char* category,
                            const char* message, ...);

    // FMI 1.0 内存管理回调
    static void* fmi1_allocate_memory(size_t n, size_t size);
    static void fmi1_free_memory(void* obj);
};

/**
 * @brief 创建基于FMI的模型实例
 * @param instance_name 实例名称
 * @param fmu_path FMU文件路径
 * @param step_size_hint 建议步长
 * @return 模型实例指针，失败返回nullptr
 */
std::unique_ptr<model_instance> create_fmi_model_instance(
    const std::string& instance_name,
    const std::string& fmu_path,
    double step_size_hint = -1.0);

} // namespace simulation
} // namespace cosmo

#endif // COSMO_SIMULATION_FMI_MODEL_INSTANCE_HPP
