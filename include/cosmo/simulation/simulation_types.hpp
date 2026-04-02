/**
 * @file simulation_types.hpp
 * @brief 仿真模块共享类型定义
 *
 * 定义仿真模块中各组件共享的类型，避免循环依赖。
 *
 * @copyright Copyright (c) 2026 COSMO Development Team
 */

#ifndef COSMO_SIMULATION_TYPES_HPP
#define COSMO_SIMULATION_TYPES_HPP

#include "cosmo/fmi/fmi_types.hpp"

#include <string>
#include <map>

namespace cosmo {
namespace simulation {

/**
 * @brief 变量标识符结构体
 *
 * 唯一标识多模型仿真中的变量。
 * 格式：model_name::variable_name
 */
struct variable_identifier
{
    std::string model_name;     ///< 模型实例名称
    std::string variable_name;  ///< 变量名称

    variable_identifier() = default;

    variable_identifier(const std::string& model, const std::string& var)
        : model_name(model), variable_name(var) {}

    std::string to_string() const
    {
        return model_name + "::" + variable_name;
    }

    bool operator<(const variable_identifier& other) const
    {
        if (model_name != other.model_name) {
            return model_name < other.model_name;
        }
        return variable_name < other.variable_name;
    }
};

/**
 * @brief 标量值结构体
 *
 * 存储不同类型的标量值。通过value_type字段指示当前存储的实际类型。
 */
struct scalar_value
{
    fmi::fmi_real real_value;
    fmi::fmi_integer integer_value;
    fmi::fmi_boolean boolean_value;
    fmi::fmi_string string_value;

    enum class type { real, integer, boolean, string } value_type;

    scalar_value() : real_value(0.0), value_type(type::real) {}
    explicit scalar_value(fmi::fmi_real r) : real_value(r), value_type(type::real) {}
    explicit scalar_value(fmi::fmi_integer i) : integer_value(i), value_type(type::integer) {}
    explicit scalar_value(fmi::fmi_boolean b) : boolean_value(b), value_type(type::boolean) {}
    explicit scalar_value(const fmi::fmi_string& s) : string_value(s), value_type(type::string) {}
};

/**
 * @brief 参数集类型
 *
 * 存储一组变量的参数值，用于模型配置。
 */
using parameter_set = std::map<variable_identifier, scalar_value>;

} // namespace simulation
} // namespace cosmo

#endif // COSMO_SIMULATION_TYPES_HPP
