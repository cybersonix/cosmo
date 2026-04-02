/**
 * @file model_instance.cpp
 * @brief 模型实例实现
 *
 * 实现模型实例基类和模拟模型实例。
 * 模拟模型实例维护内部变量状态，支持测试和演示场景。
 *
 * @copyright Copyright (c) 2026 COSMO Development Team
 */

#include "cosmo/simulation/model_instance.hpp"
#include "cosmo/logger/logger.hpp"

namespace cosmo {
namespace simulation {

// ============================================================================
// model_instance 基类
// ============================================================================

model_instance::model_instance(const std::string& instance_name,
                               std::unique_ptr<fmi::model_description> model_desc,
                               double step_size_hint)
    : instance_name_(instance_name)
    , model_description_(std::move(model_desc))
    , step_size_hint_(step_size_hint)
{
}

void model_instance::add_parameter_set(const std::string& name, const parameter_set& params)
{
    parameter_sets_[name] = params;
}

bool model_instance::apply_parameter_set(const std::string& name)
{
    auto it = parameter_sets_.find(name);
    if (it == parameter_sets_.end()) {
        cosmo::log::warn("参数集 '" + name + "' 未找到");
        return false;
    }

    cosmo::log::info("已应用参数集 '" + name + "' 到模型实例 '" + instance_name_ + "'");

    // 基类只记录日志，子类可以覆写以实际设置参数值
    return true;
}

fmi::value_reference model_instance::get_value_reference(const std::string& variable_name) const
{
    if (model_description_) {
        return model_description_->get_vr_by_name(variable_name);
    }
    return static_cast<fmi::value_reference>(-1);
}

// ============================================================================
// mock_model_instance
// ============================================================================

mock_model_instance::mock_model_instance(const std::string& instance_name,
                                         std::unique_ptr<fmi::model_description> model_desc,
                                         double step_size_hint)
    : model_instance(instance_name, std::move(model_desc), step_size_hint)
    , inited_(false)
    , terminated_(false)
    , step_return_status_(fmi::fmi_ok)
{
    init_default_values();
}

void mock_model_instance::init_default_values()
{
    if (!model_description_) {
        return;
    }

    // 根据模型描述中的变量列表初始化默认值
    const auto& vars = model_description_->get_variables();
    for (const auto& var : vars) {
        switch (var.type) {
            case fmi::variable_type::real:
                real_values_[var.vr] = var.default_real;
                break;
            case fmi::variable_type::integer:
                integer_values_[var.vr] = var.default_integer;
                break;
            case fmi::variable_type::boolean:
                boolean_values_[var.vr] = var.default_boolean;
                break;
            case fmi::variable_type::string:
                string_values_[var.vr] = var.default_string;
                break;
            default:
                break;
        }
    }
}

fmi::fmi_status mock_model_instance::enter_initialization_mode(double start_time)
{
    if (inited_) {
        cosmo::log::warn("模型实例 '" + instance_name_ + "' 已经进入过初始化模式");
        return fmi::fmi_warning;
    }
    cosmo::log::info("模型实例 '" + instance_name_ + "' 进入初始化模式，start_time=" +
                     std::to_string(start_time));
    return fmi::fmi_ok;
}

fmi::fmi_status mock_model_instance::exit_initialization_mode()
{
    inited_ = true;
    cosmo::log::info("模型实例 '" + instance_name_ + "' 退出初始化模式");
    return fmi::fmi_ok;
}

fmi::fmi_status mock_model_instance::do_step(double current_time, double step_size)
{
    if (!inited_) {
        cosmo::log::error("模型实例 '" + instance_name_ + "' 未初始化，无法步进");
        return fmi::fmi_error;
    }

    if (terminated_) {
        cosmo::log::error("模型实例 '" + instance_name_ + "' 已终止，无法步进");
        return fmi::fmi_error;
    }

    // 调用步进回调（如果有）
    if (step_callback_) {
        step_callback_(current_time, step_size);
    }

    return step_return_status_;
}

fmi::fmi_status mock_model_instance::terminate()
{
    if (!inited_) {
        cosmo::log::warn("模型实例 '" + instance_name_ + "' 未初始化，无需终止");
        return fmi::fmi_warning;
    }

    terminated_ = true;
    cosmo::log::info("模型实例 '" + instance_name_ + "' 已终止");
    return fmi::fmi_ok;
}

fmi::fmi_status mock_model_instance::reset()
{
    inited_ = false;
    terminated_ = false;

    // 恢复默认值
    real_values_.clear();
    integer_values_.clear();
    boolean_values_.clear();
    string_values_.clear();
    init_default_values();

    cosmo::log::info("模型实例 '" + instance_name_ + "' 已重置");
    return fmi::fmi_ok;
}

fmi::fmi_status mock_model_instance::get_real(fmi::value_reference vr, fmi::fmi_real& value)
{
    auto it = real_values_.find(vr);
    if (it != real_values_.end()) {
        value = it->second;
        return fmi::fmi_ok;
    }
    value = 0.0;
    return fmi::fmi_ok;
}

fmi::fmi_status mock_model_instance::get_integer(fmi::value_reference vr, fmi::fmi_integer& value)
{
    auto it = integer_values_.find(vr);
    if (it != integer_values_.end()) {
        value = it->second;
        return fmi::fmi_ok;
    }
    value = 0;
    return fmi::fmi_ok;
}

fmi::fmi_status mock_model_instance::get_boolean(fmi::value_reference vr, fmi::fmi_boolean& value)
{
    auto it = boolean_values_.find(vr);
    if (it != boolean_values_.end()) {
        value = it->second;
        return fmi::fmi_ok;
    }
    value = false;
    return fmi::fmi_ok;
}

fmi::fmi_status mock_model_instance::get_string(fmi::value_reference vr, fmi::fmi_string& value)
{
    auto it = string_values_.find(vr);
    if (it != string_values_.end()) {
        value = it->second;
        return fmi::fmi_ok;
    }
    value = "";
    return fmi::fmi_ok;
}

fmi::fmi_status mock_model_instance::set_real(fmi::value_reference vr, fmi::fmi_real value)
{
    real_values_[vr] = value;
    return fmi::fmi_ok;
}

fmi::fmi_status mock_model_instance::set_integer(fmi::value_reference vr, fmi::fmi_integer value)
{
    integer_values_[vr] = value;
    return fmi::fmi_ok;
}

fmi::fmi_status mock_model_instance::set_boolean(fmi::value_reference vr, fmi::fmi_boolean value)
{
    boolean_values_[vr] = value;
    return fmi::fmi_ok;
}

fmi::fmi_status mock_model_instance::set_string(fmi::value_reference vr, const fmi::fmi_string& value)
{
    string_values_[vr] = value;
    return fmi::fmi_ok;
}

// ============================================================================
// 工厂函数
// ============================================================================

std::unique_ptr<model_instance> create_model_instance(
    const std::string& instance_name,
    std::unique_ptr<fmi::model_description> model_desc,
    double step_size_hint)
{
    return std::unique_ptr<model_instance>(
        new mock_model_instance(instance_name, std::move(model_desc), step_size_hint));
}

} // namespace simulation
} // namespace cosmo
