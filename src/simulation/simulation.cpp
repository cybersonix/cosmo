/**
 * @file simulation.cpp
 * @brief 仿真引擎核心实现
 *
 * 实现仿真引擎的生命周期管理、模型实例协调、
 * 监听器通知和变量连接数据传输。
 *
 * @copyright Copyright (c) 2026 COSMO Development Team
 */

#include "cosmo/simulation/simulation.hpp"
#include "cosmo/simulation/model_instance.hpp"
#include "cosmo/logger/logger.hpp"

#include <algorithm>

namespace cosmo {
namespace simulation {

// ============================================================================
// 固定步长算法
// ============================================================================

fixed_step_algorithm::fixed_step_algorithm(double step_size)
    : step_size_(step_size)
{
}

fmi::fmi_status fixed_step_algorithm::initialize()
{
    if (step_size_ <= 0.0) {
        cosmo::log::error("固定步长必须为正数，当前值: " + std::to_string(step_size_));
        return fmi::fmi_error;
    }
    cosmo::log::info("固定步长算法初始化完成，步长: " + std::to_string(step_size_));
    return fmi::fmi_ok;
}

fmi::fmi_status fixed_step_algorithm::step(double /*current_time*/, double /*step_size*/)
{
    // 固定步长算法忽略外部传入的步长参数
    return fmi::fmi_ok;
}

fmi::fmi_status fixed_step_algorithm::terminate()
{
    return fmi::fmi_ok;
}

// ============================================================================
// 仿真引擎
// ============================================================================

simulation::simulation(std::unique_ptr<algorithm> algo)
    : algorithm_(std::move(algo))
    , current_time_(0.0)
    , step_count_(0)
    , initialized_(false)
    , terminated_(false)
{
    if (!algorithm_) {
        cosmo::log::error("仿真引擎创建失败：算法实例为空");
    }
}

simulation::~simulation()
{
    if (initialized_ && !terminated_) {
        try {
            terminate();
        } catch (...) {
            // 析构函数中不抛出异常
        }
    }
}

double simulation::get_step_size() const
{
    if (algorithm_) {
        return algorithm_->get_step_size();
    }
    return 0.0;
}

// ---------------------------------------------------------------------------
// 监听器通知
// ---------------------------------------------------------------------------

void simulation::notify_pre_init(double start_time)
{
    for (auto& pair : listeners_) {
        pair.second->on_pre_init(start_time);
    }
}

void simulation::notify_post_init(double start_time)
{
    for (auto& pair : listeners_) {
        pair.second->on_post_init(start_time);
    }
}

void simulation::notify_pre_step(double current_time, double step_size)
{
    for (auto& pair : listeners_) {
        pair.second->on_pre_step(current_time, step_size);
    }
}

void simulation::notify_post_step(double current_time, double step_size)
{
    for (auto& pair : listeners_) {
        pair.second->on_post_step(current_time, step_size);
    }
}

void simulation::notify_pre_terminate()
{
    for (auto& pair : listeners_) {
        pair.second->on_pre_terminate();
    }
}

void simulation::notify_post_terminate()
{
    for (auto& pair : listeners_) {
        pair.second->on_post_terminate();
    }
}

void simulation::notify_on_reset()
{
    for (auto& pair : listeners_) {
        pair.second->on_reset();
    }
}

// ---------------------------------------------------------------------------
// 变量连接数据传输
// ---------------------------------------------------------------------------

fmi::fmi_status simulation::transfer_connection_data()
{
    fmi::fmi_status worst_status = fmi::fmi_ok;

    for (const auto& conn : connections_) {
        // 查找源模型实例
        model_instance* src = get_model_instance(conn.source_model);
        if (!src) {
            cosmo::log::warn("连接传输跳过：源模型 '" + conn.source_model + "' 不存在");
            continue;
        }

        // 查找目标模型实例
        model_instance* tgt = get_model_instance(conn.target_model);
        if (!tgt) {
            cosmo::log::warn("连接传输跳过：目标模型 '" + conn.target_model + "' 不存在");
            continue;
        }

        // 查找源变量的值引用
        fmi::value_reference src_vr = src->get_value_reference(conn.source_variable);
        if (src_vr == static_cast<fmi::value_reference>(-1)) {
            cosmo::log::warn("连接传输跳过：源变量 '" + conn.source_variable + "' 未找到");
            continue;
        }

        // 查找目标变量的值引用
        fmi::value_reference tgt_vr = tgt->get_value_reference(conn.target_variable);
        if (tgt_vr == static_cast<fmi::value_reference>(-1)) {
            cosmo::log::warn("连接传输跳过：目标变量 '" + conn.target_variable + "' 未找到");
            continue;
        }

        // 读取源变量的实数值
        fmi::fmi_real value = 0.0;
        fmi::fmi_status read_status = src->get_real(src_vr, value);
        if (read_status != fmi::fmi_ok) {
            worst_status = std::max(worst_status, read_status);
            cosmo::log::warn("连接传输：读取源变量 '" + conn.source_variable + "' 失败");
            continue;
        }

        // 应用修饰器（如果有）
        if (conn.modifier) {
            value = conn.modifier(value);
        }

        // 写入目标变量
        fmi::fmi_status write_status = tgt->set_real(tgt_vr, value);
        if (write_status != fmi::fmi_ok) {
            worst_status = std::max(worst_status, write_status);
            cosmo::log::warn("连接传输：写入目标变量 '" + conn.target_variable + "' 失败");
        }
    }

    return worst_status;
}

// ---------------------------------------------------------------------------
// 生命周期管理
// ---------------------------------------------------------------------------

fmi::fmi_status simulation::initialize(double start_time)
{
    if (initialized_) {
        cosmo::log::warn("仿真已经初始化，请勿重复调用");
        return fmi::fmi_warning;
    }

    if (terminated_) {
        cosmo::log::error("已终止的仿真无法重新初始化，请先调用reset()");
        return fmi::fmi_error;
    }

    // 通知监听器：初始化前
    notify_pre_init(start_time);

    current_time_ = start_time;
    fmi::fmi_status status = fmi::fmi_ok;

    // 初始化算法
    if (algorithm_) {
        fmi::fmi_status algo_status = algorithm_->initialize();
        if (algo_status > status) {
            status = algo_status;
        }
    }

    // 初始化所有模型实例
    for (auto& inst : instances_) {
        fmi::fmi_status inst_status = inst->enter_initialization_mode(start_time);
        if (inst_status > status) {
            status = inst_status;
        }

        // 立即退出初始化模式，进入仿真就绪状态
        inst_status = inst->exit_initialization_mode();
        if (inst_status > status) {
            status = inst_status;
        }
    }

    if (status == fmi::fmi_ok) {
        initialized_ = true;
        step_count_ = 0;

        // 通知监听器：初始化后（仅在成功时）
        notify_post_init(start_time);

        cosmo::log::info("仿真初始化完成，开始时间: " + std::to_string(start_time) +
                         "，模型数: " + std::to_string(instances_.size()) +
                         "，连接数: " + std::to_string(connections_.size()));
    } else {
        cosmo::log::error("仿真初始化失败");
    }
    return status;
}

fmi::fmi_status simulation::step()
{
    if (!initialized_) {
        cosmo::log::error("仿真未初始化，无法执行步进");
        return fmi::fmi_error;
    }

    if (terminated_) {
        cosmo::log::error("仿真已终止，无法执行步进");
        return fmi::fmi_error;
    }

    // 获取步长
    double step_size = 0.1;  // 默认步长
    if (algorithm_) {
        step_size = algorithm_->get_step_size();
    }

    // 通知监听器：步进前
    notify_pre_step(current_time_, step_size);

    fmi::fmi_status status = fmi::fmi_ok;

    // 步进所有模型实例
    for (auto& inst : instances_) {
        fmi::fmi_status inst_status = inst->do_step(current_time_, step_size);
        if (inst_status > status) {
            status = inst_status;
        }
    }

    if (status == fmi::fmi_ok || status == fmi::fmi_warning) {
        // 执行变量连接的数据传输
        fmi::fmi_status transfer_status = transfer_connection_data();
        if (transfer_status > status) {
            status = transfer_status;
        }

        // 执行算法步进
        if (algorithm_) {
            fmi::fmi_status algo_status = algorithm_->step(current_time_, step_size);
            if (algo_status > status) {
                status = algo_status;
            }
        }

        // 推进时间
        current_time_ += step_size;
        step_count_++;
    }

    // 通知监听器：步进后
    notify_post_step(current_time_, step_size);

    return status;
}

fmi::fmi_status simulation::step_until(double time_point)
{
    if (!initialized_) {
        cosmo::log::error("仿真未初始化");
        return fmi::fmi_error;
    }

    if (terminated_) {
        cosmo::log::error("仿真已终止");
        return fmi::fmi_error;
    }

    fmi::fmi_status status = fmi::fmi_ok;
    const size_t max_steps = 10000000;  // 防止无限循环的安全阈值
    size_t step_count = 0;

    while (current_time_ < time_point - 1e-12) {
        if (++step_count > max_steps) {
            cosmo::log::warn("步进达到最大迭代次数限制(" + std::to_string(max_steps) + ")，已停止");
            break;
        }

        fmi::fmi_status step_status = step();
        if (step_status > status) {
            status = step_status;
        }

        if (step_status == fmi::fmi_error || step_status == fmi::fmi_fatal) {
            cosmo::log::error("步进过程中发生错误，已停止");
            break;
        }
    }

    return status;
}

fmi::fmi_status simulation::step_for(double duration)
{
    if (!initialized_) {
        cosmo::log::error("仿真未初始化");
        return fmi::fmi_error;
    }

    if (duration <= 0.0) {
        cosmo::log::warn("步进时长必须为正数: " + std::to_string(duration));
        return fmi::fmi_warning;
    }

    return step_until(current_time_ + duration);
}

fmi::fmi_status simulation::terminate()
{
    if (!initialized_) {
        cosmo::log::warn("仿真未初始化，无法终止");
        return fmi::fmi_warning;
    }

    if (terminated_) {
        cosmo::log::warn("仿真已经终止");
        return fmi::fmi_warning;
    }

    // 通知监听器：终止前
    notify_pre_terminate();

    fmi::fmi_status status = fmi::fmi_ok;

    // 终止所有模型实例
    for (auto& inst : instances_) {
        fmi::fmi_status inst_status = inst->terminate();
        if (inst_status > status) {
            status = inst_status;
        }
    }

    // 终止算法
    if (algorithm_) {
        fmi::fmi_status algo_status = algorithm_->terminate();
        if (algo_status > status) {
            status = algo_status;
        }
    }

    if (status == fmi::fmi_ok) {
        terminated_ = true;
    }

    // 通知监听器：终止后
    notify_post_terminate();

    cosmo::log::info("仿真终止完成，总步数: " + std::to_string(step_count_) +
                     "，结束时间: " + std::to_string(current_time_));
    return status;
}

fmi::fmi_status simulation::reset()
{
    if (!initialized_) {
        cosmo::log::warn("仿真未初始化，无需重置");
        return fmi::fmi_warning;
    }

    // 通知监听器：重置
    notify_on_reset();

    fmi::fmi_status status = fmi::fmi_ok;

    // 重置所有模型实例
    for (auto& inst : instances_) {
        fmi::fmi_status inst_status = inst->reset();
        if (inst_status > status) {
            status = inst_status;
        }
    }

    // 重置内部状态
    current_time_ = 0.0;
    step_count_ = 0;
    initialized_ = false;
    terminated_ = false;

    cosmo::log::info("仿真已重置");
    return status;
}

// ---------------------------------------------------------------------------
// 模型实例管理
// ---------------------------------------------------------------------------

void simulation::add_model_instance(std::unique_ptr<model_instance> instance)
{
    if (!instance) {
        cosmo::log::warn("尝试添加空模型实例，已忽略");
        return;
    }

    // 检查重名
    const std::string& name = instance->get_instance_name();
    if (get_model_instance(name) != nullptr) {
        cosmo::log::warn("模型实例名称 '" + name + "' 已存在，已忽略");
        return;
    }

    cosmo::log::info("添加模型实例: " + name);
    instances_.push_back(std::move(instance));
}

model_instance* simulation::get_model_instance(const std::string& name) const
{
    auto it = std::find_if(instances_.begin(), instances_.end(),
        [&name](const std::unique_ptr<model_instance>& inst) {
            return inst->get_instance_name() == name;
        });

    return (it != instances_.end()) ? it->get() : nullptr;
}

// ---------------------------------------------------------------------------
// 监听器管理
// ---------------------------------------------------------------------------

void simulation::add_listener(std::unique_ptr<simulation_listener> listener)
{
    if (!listener) {
        cosmo::log::warn("尝试添加空监听器，已忽略");
        return;
    }

    const std::string& name = listener->get_name();
    listener->set_owner(this);
    listeners_[name] = std::move(listener);
    cosmo::log::info("添加监听器: " + name);
}

bool simulation::remove_listener(const std::string& name)
{
    auto it = listeners_.find(name);
    if (it != listeners_.end()) {
        listeners_.erase(it);
        cosmo::log::info("移除监听器: " + name);
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// 变量连接管理
// ---------------------------------------------------------------------------

void simulation::add_connection(const connection_binding& conn)
{
    // 检查重复连接
    for (const auto& existing : connections_) {
        if (existing.source_model == conn.source_model &&
            existing.source_variable == conn.source_variable &&
            existing.target_model == conn.target_model &&
            existing.target_variable == conn.target_variable) {
            cosmo::log::warn("忽略重复连接: " + conn.source_model + "::" + conn.source_variable +
                             " -> " + conn.target_model + "::" + conn.target_variable);
            return;
        }
    }
    connections_.push_back(conn);
    cosmo::log::info("添加连接: " + conn.source_model + "::" + conn.source_variable +
                     " -> " + conn.target_model + "::" + conn.target_variable);
}

// ---------------------------------------------------------------------------
// 变量访问便捷方法
// ---------------------------------------------------------------------------

fmi::fmi_status simulation::get_real(const std::string& identifier, fmi::fmi_real& value)
{
    size_t pos = identifier.find("::");
    if (pos == std::string::npos) {
        cosmo::log::error("无效的变量标识符格式: " + identifier);
        return fmi::fmi_error;
    }

    std::string model_name = identifier.substr(0, pos);
    std::string var_name = identifier.substr(pos + 2);

    model_instance* inst = get_model_instance(model_name);
    if (!inst) {
        cosmo::log::error("模型实例未找到: " + model_name);
        return fmi::fmi_error;
    }

    fmi::value_reference vr = inst->get_value_reference(var_name);
    if (vr == static_cast<fmi::value_reference>(-1)) {
        cosmo::log::error("变量未找到: " + var_name);
        return fmi::fmi_error;
    }

    return inst->get_real(vr, value);
}

fmi::fmi_status simulation::set_real(const std::string& identifier, fmi::fmi_real value)
{
    size_t pos = identifier.find("::");
    if (pos == std::string::npos) {
        cosmo::log::error("无效的变量标识符格式: " + identifier);
        return fmi::fmi_error;
    }

    std::string model_name = identifier.substr(0, pos);
    std::string var_name = identifier.substr(pos + 2);

    model_instance* inst = get_model_instance(model_name);
    if (!inst) {
        cosmo::log::error("模型实例未找到: " + model_name);
        return fmi::fmi_error;
    }

    fmi::value_reference vr = inst->get_value_reference(var_name);
    if (vr == static_cast<fmi::value_reference>(-1)) {
        cosmo::log::error("变量未找到: " + var_name);
        return fmi::fmi_error;
    }

    return inst->set_real(vr, value);
}

fmi::fmi_status simulation::get_integer(const std::string& identifier, fmi::fmi_integer& value)
{
    size_t pos = identifier.find("::");
    if (pos == std::string::npos) {
        return fmi::fmi_error;
    }

    std::string model_name = identifier.substr(0, pos);
    std::string var_name = identifier.substr(pos + 2);

    model_instance* inst = get_model_instance(model_name);
    if (!inst) return fmi::fmi_error;

    fmi::value_reference vr = inst->get_value_reference(var_name);
    if (vr == static_cast<fmi::value_reference>(-1)) return fmi::fmi_error;

    return inst->get_integer(vr, value);
}

fmi::fmi_status simulation::set_integer(const std::string& identifier, fmi::fmi_integer value)
{
    size_t pos = identifier.find("::");
    if (pos == std::string::npos) return fmi::fmi_error;

    std::string model_name = identifier.substr(0, pos);
    std::string var_name = identifier.substr(pos + 2);

    model_instance* inst = get_model_instance(model_name);
    if (!inst) return fmi::fmi_error;

    fmi::value_reference vr = inst->get_value_reference(var_name);
    if (vr == static_cast<fmi::value_reference>(-1)) return fmi::fmi_error;

    return inst->set_integer(vr, value);
}

fmi::fmi_status simulation::get_boolean(const std::string& identifier, fmi::fmi_boolean& value)
{
    size_t pos = identifier.find("::");
    if (pos == std::string::npos) return fmi::fmi_error;

    std::string model_name = identifier.substr(0, pos);
    std::string var_name = identifier.substr(pos + 2);

    model_instance* inst = get_model_instance(model_name);
    if (!inst) return fmi::fmi_error;

    fmi::value_reference vr = inst->get_value_reference(var_name);
    if (vr == static_cast<fmi::value_reference>(-1)) return fmi::fmi_error;

    return inst->get_boolean(vr, value);
}

fmi::fmi_status simulation::set_boolean(const std::string& identifier, fmi::fmi_boolean value)
{
    size_t pos = identifier.find("::");
    if (pos == std::string::npos) return fmi::fmi_error;

    std::string model_name = identifier.substr(0, pos);
    std::string var_name = identifier.substr(pos + 2);

    model_instance* inst = get_model_instance(model_name);
    if (!inst) return fmi::fmi_error;

    fmi::value_reference vr = inst->get_value_reference(var_name);
    if (vr == static_cast<fmi::value_reference>(-1)) return fmi::fmi_error;

    return inst->set_boolean(vr, value);
}

fmi::fmi_status simulation::get_string(const std::string& identifier, fmi::fmi_string& value)
{
    size_t pos = identifier.find("::");
    if (pos == std::string::npos) return fmi::fmi_error;

    std::string model_name = identifier.substr(0, pos);
    std::string var_name = identifier.substr(pos + 2);

    model_instance* inst = get_model_instance(model_name);
    if (!inst) return fmi::fmi_error;

    fmi::value_reference vr = inst->get_value_reference(var_name);
    if (vr == static_cast<fmi::value_reference>(-1)) return fmi::fmi_error;

    return inst->get_string(vr, value);
}

fmi::fmi_status simulation::set_string(const std::string& identifier, const fmi::fmi_string& value)
{
    size_t pos = identifier.find("::");
    if (pos == std::string::npos) return fmi::fmi_error;

    std::string model_name = identifier.substr(0, pos);
    std::string var_name = identifier.substr(pos + 2);

    model_instance* inst = get_model_instance(model_name);
    if (!inst) return fmi::fmi_error;

    fmi::value_reference vr = inst->get_value_reference(var_name);
    if (vr == static_cast<fmi::value_reference>(-1)) return fmi::fmi_error;

    return inst->set_string(vr, value);
}

} // namespace simulation
} // namespace cosmo
