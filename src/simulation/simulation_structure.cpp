/**
 * @file simulation_structure.cpp
 * @brief 仿真结构配置实现
 *
 * 实现仿真结构的模型和连接管理，
 * 以及从结构定义创建实际仿真实例的加载逻辑。
 *
 * @copyright Copyright (c) 2026 COSMO Development Team
 */

#include "cosmo/simulation/simulation_structure.hpp"
#include "cosmo/simulation/model_instance.hpp"
#include "cosmo/simulation/fmi_model_instance.hpp"
#include "cosmo/simulation/simulation.hpp"
#include "cosmo/fmi/model_description.hpp"
#include "cosmo/fmi/fmi_types.hpp"
#include "cosmo/logger/logger.hpp"
#include "cosmo/util/filesystem.hpp"

#include <algorithm>

namespace cosmo {
namespace simulation {

// ============================================================================
// variable_connection
// ============================================================================

variable_connection::variable_connection(const std::string& source_model,
                                         const std::string& source_variable,
                                         const std::string& target_model,
                                         const std::string& target_variable,
                                         connection_modifier modifier)
    : source_model_(source_model)
    , source_variable_(source_variable)
    , target_model_(target_model)
    , target_variable_(target_variable)
    , modifier_(modifier)
{
}

const std::string& variable_connection::get_source_model() const { return source_model_; }
const std::string& variable_connection::get_source_variable() const { return source_variable_; }
const std::string& variable_connection::get_target_model() const { return target_model_; }
const std::string& variable_connection::get_target_variable() const { return target_variable_; }

fmi::fmi_real variable_connection::apply_modifier(fmi::fmi_real value) const
{
    return modifier_ ? modifier_(value) : value;
}

bool variable_connection::has_modifier() const
{
    return modifier_ != nullptr;
}

// ============================================================================
// simulation_structure
// ============================================================================

simulation_structure::simulation_structure()
{
}

void simulation_structure::add_model(const std::string& instance_name, const std::string& fmu_path)
{
    if (has_model(instance_name)) {
        cosmo::log::warn("模型实例 '" + instance_name + "' 已存在，已忽略");
        return;
    }

    model_info info;
    info.instance_name = instance_name;
    info.fmu_path = fmu_path;
    models_[instance_name] = info;

    cosmo::log::info("添加模型: " + instance_name + " -> " + fmu_path);
}

void simulation_structure::add_model(const std::string& instance_name,
                                     const std::string& fmu_path,
                                     const std::map<std::string, scalar_value>& parameters)
{
    if (has_model(instance_name)) {
        cosmo::log::warn("模型实例 '" + instance_name + "' 已存在，已忽略");
        return;
    }

    model_info info;
    info.instance_name = instance_name;
    info.fmu_path = fmu_path;
    info.parameters = parameters;
    models_[instance_name] = info;

    cosmo::log::info("添加模型: " + instance_name + " -> " + fmu_path +
                     "（含 " + std::to_string(parameters.size()) + " 个参数）");
}

void simulation_structure::make_connection(const std::string& source, const std::string& target)
{
    auto src = parse_variable_identifier(source);
    auto tgt = parse_variable_identifier(target);

    if (src.first.empty() || src.second.empty() ||
        tgt.first.empty() || tgt.second.empty()) {
        cosmo::log::error("变量标识符格式无效: " + source + " -> " + target);
        return;
    }

    connections_.emplace_back(src.first, src.second, tgt.first, tgt.second, nullptr);
    cosmo::log::info("创建连接: " + source + " -> " + target);
}

void simulation_structure::make_connection(const std::string& source,
                                           const std::string& target,
                                           connection_modifier modifier)
{
    auto src = parse_variable_identifier(source);
    auto tgt = parse_variable_identifier(target);

    if (src.first.empty() || src.second.empty() ||
        tgt.first.empty() || tgt.second.empty()) {
        cosmo::log::error("变量标识符格式无效: " + source + " -> " + target);
        return;
    }

    connections_.emplace_back(src.first, src.second, tgt.first, tgt.second, modifier);
    cosmo::log::info("创建连接（含修饰器）: " + source + " -> " + target);
}

void simulation_structure::add_parameter_set(const std::string& name,
                                            const std::map<std::string, scalar_value>& parameters)
{
    parameter_sets_[name] = parameters;
    cosmo::log::info("添加参数集: " + name + "（" +
                     std::to_string(parameters.size()) + " 个参数）");
}

void simulation_structure::register_variable(const std::string& model_name,
                                              const std::string& variable_type,
                                              const std::string& variable_name)
{
    registered_variables_[model_name].push_back(
        std::make_pair(variable_type, variable_name));
    cosmo::log::info("注册变量: " + model_name + "::" + variable_name +
                     "（类型: " + variable_type + "）");
}

std::unique_ptr<simulation> simulation_structure::load(std::unique_ptr<algorithm> algo)
{
    cosmo::log::info("开始加载仿真结构...");

    auto sim = std::unique_ptr<simulation>(new simulation(std::move(algo)));

    // 第一遍：收集所有连接中引用的变量，按模型分组
    std::map<std::string, std::set<std::string>> model_variables;
    for (const auto& conn : connections_) {
        model_variables[conn.get_source_model()].insert(conn.get_source_variable());
        model_variables[conn.get_target_model()].insert(conn.get_target_variable());
    }

    // 收集手动注册的变量
    for (const auto& pair : registered_variables_) {
        const std::string& model_name = pair.first;
        for (const auto& var_info : pair.second) {
            model_variables[model_name].insert(var_info.second);
        }
    }

    // 为每个模型创建模拟实例
    for (const auto& pair : models_) {
        const model_info& info = pair.second;

        std::unique_ptr<model_instance> inst;

        // 尝试使用真实FMU加载
        if (util::file_exists(info.fmu_path) &&
            info.fmu_path.find(".fmu") != std::string::npos) {
            cosmo::log::info("检测到FMU文件，尝试真实加载: " + info.fmu_path);
            inst = create_fmi_model_instance(info.instance_name, info.fmu_path);
            if (!inst) {
                cosmo::log::warn("FMU加载失败，回退到mock实例: " + info.fmu_path);
            }
        }

        // 回退到mock实例
        if (!inst) {
            // 创建模型描述
            auto model_desc = cosmo::fmi::create_model_description();
            model_desc->set_model_name(info.instance_name);

            // 建立变量名称到类型的映射（来自手动注册）
            std::map<std::string, std::string> var_type_map;
            for (const auto& var_info : registered_variables_[info.instance_name]) {
                var_type_map[var_info.second] = var_info.first;
            }

            // 从连接和注册中收集到的变量添加到模型描述中
            const auto& vars = model_variables[info.instance_name];
            fmi::value_reference vr = 0;
            for (const auto& var_name : vars) {
                cosmo::fmi::variable_description var;
                var.name = var_name;
                var.vr = vr++;

                // 根据注册的类型设置变量类型，默认为real
                auto type_it = var_type_map.find(var_name);
                if (type_it != var_type_map.end()) {
                    const std::string& type_str = type_it->second;
                    if (type_str == "integer") {
                        var.type = cosmo::fmi::variable_type::integer;
                        var.default_integer = 0;
                    } else if (type_str == "boolean") {
                        var.type = cosmo::fmi::variable_type::boolean;
                        var.default_boolean = false;
                    } else if (type_str == "string") {
                        var.type = cosmo::fmi::variable_type::string;
                        var.default_string = "";
                    } else {
                        var.type = cosmo::fmi::variable_type::real;
                        var.default_real = 0.0;
                    }
                } else {
                    var.type = cosmo::fmi::variable_type::real;
                    var.default_real = 0.0;
                }

                var.causality_type = cosmo::fmi::causality::local;
                model_desc->add_variable(var);
            }

            inst = create_model_instance(info.instance_name, std::move(model_desc));
        }

        sim->add_model_instance(std::move(inst));
    }

    // 将连接添加到仿真中
    for (const auto& conn : connections_) {
        std::function<fmi::fmi_real(fmi::fmi_real)> mod_fn;
        if (conn.has_modifier()) {
            // 安全：mod_fn 在当前迭代内创建并传给 connection_binding，
            // connections_ 在整个函数生命周期内有效
            mod_fn = [&conn](fmi::fmi_real v) { return conn.apply_modifier(v); };
        }
        connection_binding binding(
            conn.get_source_model(), conn.get_source_variable(),
            conn.get_target_model(), conn.get_target_variable(),
            mod_fn
        );
        sim->add_connection(binding);
    }

    cosmo::log::info("仿真结构加载完成: " + std::to_string(models_.size()) +
                     " 个模型, " + std::to_string(connections_.size()) + " 个连接");
    return sim;
}

size_t simulation_structure::get_model_count() const
{
    return models_.size();
}

size_t simulation_structure::get_connection_count() const
{
    return connections_.size();
}

size_t simulation_structure::get_parameter_set_count() const
{
    return parameter_sets_.size();
}

bool simulation_structure::has_model(const std::string& instance_name) const
{
    return models_.find(instance_name) != models_.end();
}

const std::string& simulation_structure::get_model_fmu_path(const std::string& instance_name) const
{
    auto it = models_.find(instance_name);
    if (it != models_.end()) {
        return it->second.fmu_path;
    }
    static std::string empty;
    return empty;
}

std::pair<std::string, std::string> simulation_structure::parse_variable_identifier(
    const std::string& identifier) const
{
    size_t pos = identifier.find("::");
    if (pos != std::string::npos) {
        return {identifier.substr(0, pos), identifier.substr(pos + 2)};
    }
    return {"", ""};
}

// ============================================================================
// 工厂函数
// ============================================================================

std::unique_ptr<simulation_structure> create_simulation_structure()
{
    return std::unique_ptr<simulation_structure>(new simulation_structure());
}

} // namespace simulation
} // namespace cosmo
