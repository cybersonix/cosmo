/**
 * @file model_instance.hpp
 * @brief 模型实例抽象接口定义
 *
 * 定义模型实例的抽象基类和工厂函数。
 * 模型实例是FMU模型在仿真引擎中的运行时表示。
 *
 * @copyright Copyright (c) 2026 COSMO Development Team
 */

#ifndef COSMO_SIMULATION_MODEL_INSTANCE_HPP
#define COSMO_SIMULATION_MODEL_INSTANCE_HPP

#include "cosmo/simulation/simulation_types.hpp"
#include "cosmo/fmi/model_description.hpp"
#include "cosmo/fmi/fmi_types.hpp"

#include <string>
#include <memory>
#include <map>
#include <vector>
#include <functional>

namespace cosmo {
namespace simulation {

/**
 * @brief 模型实例抽象基类
 *
 * 封装FMU模型的运行时实例，提供统一的变量访问和生命周期管理接口。
 * 具体实现可以是FMI桥接实例或用于测试的模拟实例。
 */
class model_instance
{
protected:
    std::string instance_name_;                               ///< 实例名称
    std::unique_ptr<fmi::model_description> model_description_; ///< 模型描述
    std::map<std::string, parameter_set> parameter_sets_;     ///< 参数集映射
    double step_size_hint_;                                   ///< 建议步长

public:
    /**
     * @brief 构造函数
     * @param instance_name 实例名称（在仿真中必须唯一）
     * @param model_desc 模型描述（可为空）
     * @param step_size_hint 建议步长（-1表示不限制）
     */
    model_instance(const std::string& instance_name,
                   std::unique_ptr<fmi::model_description> model_desc,
                   double step_size_hint = -1.0);

    virtual ~model_instance() = default;

    /** @name 生命周期管理 @{ */

    virtual fmi::fmi_status enter_initialization_mode(double start_time = 0.0) = 0;
    virtual fmi::fmi_status exit_initialization_mode() = 0;
    virtual fmi::fmi_status do_step(double current_time, double step_size) = 0;
    virtual fmi::fmi_status terminate() = 0;
    virtual fmi::fmi_status reset() = 0;

    /** @} */

    /** @name 参数集管理 @{ */

    void add_parameter_set(const std::string& name, const parameter_set& params);
    virtual bool apply_parameter_set(const std::string& name);

    /** @} */

    /** @name 变量访问 @{ */

    virtual fmi::fmi_status get_real(fmi::value_reference vr, fmi::fmi_real& value) = 0;
    virtual fmi::fmi_status get_integer(fmi::value_reference vr, fmi::fmi_integer& value) = 0;
    virtual fmi::fmi_status get_boolean(fmi::value_reference vr, fmi::fmi_boolean& value) = 0;
    virtual fmi::fmi_status get_string(fmi::value_reference vr, fmi::fmi_string& value) = 0;

    virtual fmi::fmi_status set_real(fmi::value_reference vr, fmi::fmi_real value) = 0;
    virtual fmi::fmi_status set_integer(fmi::value_reference vr, fmi::fmi_integer value) = 0;
    virtual fmi::fmi_status set_boolean(fmi::value_reference vr, fmi::fmi_boolean value) = 0;
    virtual fmi::fmi_status set_string(fmi::value_reference vr, const fmi::fmi_string& value) = 0;

    /** @} */

    /** @name 访问器 @{ */

    const std::string& get_instance_name() const { return instance_name_; }
    const fmi::model_description* get_model_description() const { return model_description_.get(); }
    double get_step_size_hint() const { return step_size_hint_; }

    /** @} */

    /** @name 变量引用查找 @{ */

    /**
     * @brief 根据变量名查找值引用
     * @param variable_name 变量名称
     * @return 值引用，未找到返回-1
     */
    fmi::value_reference get_value_reference(const std::string& variable_name) const;

    /** @} */
};

/**
 * @brief 模拟模型实例（用于测试和演示）
 *
 * 内部维护变量状态，不依赖真实FMU库。
 * 支持通过代码设置变量的初始值和步进行为。
 */
class mock_model_instance : public model_instance
{
public:
    /**
     * @brief 构造函数
     * @param instance_name 实例名称
     * @param model_desc 模型描述
     * @param step_size_hint 建议步长
     */
    mock_model_instance(const std::string& instance_name,
                        std::unique_ptr<fmi::model_description> model_desc,
                        double step_size_hint = -1.0);

    fmi::fmi_status enter_initialization_mode(double start_time = 0.0) override;
    fmi::fmi_status exit_initialization_mode() override;
    fmi::fmi_status do_step(double current_time, double step_size) override;
    fmi::fmi_status terminate() override;
    fmi::fmi_status reset() override;

    fmi::fmi_status get_real(fmi::value_reference vr, fmi::fmi_real& value) override;
    fmi::fmi_status get_integer(fmi::value_reference vr, fmi::fmi_integer& value) override;
    fmi::fmi_status get_boolean(fmi::value_reference vr, fmi::fmi_boolean& value) override;
    fmi::fmi_status get_string(fmi::value_reference vr, fmi::fmi_string& value) override;

    fmi::fmi_status set_real(fmi::value_reference vr, fmi::fmi_real value) override;
    fmi::fmi_status set_integer(fmi::value_reference vr, fmi::fmi_integer value) override;
    fmi::fmi_status set_boolean(fmi::value_reference vr, fmi::fmi_boolean value) override;
    fmi::fmi_status set_string(fmi::value_reference vr, const fmi::fmi_string& value) override;

    /** @name 模拟行为配置 @{ */

    /**
     * @brief 设置步进回调
     *
     * 每次do_step时调用此回调，可用于模拟动态行为。
     */
    void set_step_callback(std::function<void(double, double)> callback) {
        step_callback_ = callback;
    }

    /**
     * @brief 设置步进返回状态
     *
     * 默认返回fmi_ok，可设置为其他状态模拟异常。
     */
    void set_step_return_status(fmi::fmi_status status) {
        step_return_status_ = status;
    }

    /** @} */

private:
    // 变量存储
    std::map<fmi::value_reference, fmi::fmi_real> real_values_;
    std::map<fmi::value_reference, fmi::fmi_integer> integer_values_;
    std::map<fmi::value_reference, fmi::fmi_boolean> boolean_values_;
    std::map<fmi::value_reference, fmi::fmi_string> string_values_;

    // 状态追踪
    bool inited_;
    bool terminated_;

    // 模拟行为
    std::function<void(double, double)> step_callback_;
    fmi::fmi_status step_return_status_;

    // 根据模型描述初始化默认变量值
    void init_default_values();
};

/**
 * @brief 创建模拟模型实例
 */
std::unique_ptr<model_instance> create_model_instance(
    const std::string& instance_name,
    std::unique_ptr<fmi::model_description> model_desc,
    double step_size_hint = -1.0);

} // namespace simulation
} // namespace cosmo

#endif // COSMO_SIMULATION_MODEL_INSTANCE_HPP
