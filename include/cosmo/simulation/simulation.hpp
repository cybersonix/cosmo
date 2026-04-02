/**
 * @file simulation.hpp
 * @brief 仿真引擎核心类定义
 *
 * 本文件定义了COSMO共仿真引擎的核心仿真类。
 * 提供仿真生命周期管理、模型实例管理、算法接口和监听器通知。
 *
 * @copyright Copyright (c) 2026 COSMO Development Team
 */

#ifndef COSMO_SIMULATION_SIMULATION_HPP
#define COSMO_SIMULATION_SIMULATION_HPP

#include "cosmo/simulation/simulation_types.hpp"
#include "cosmo/simulation/simulation_listener.hpp"

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <functional>

namespace cosmo {
namespace simulation {

// 前向声明
class model_instance;

/**
 * @brief 仿真算法抽象接口
 *
 * 定义仿真算法的基本接口。
 * 用户可以实现此接口来自定义仿真算法（如变步长算法）。
 */
class algorithm
{
public:
    virtual ~algorithm() = default;

    /** @brief 初始化算法 */
    virtual fmi::fmi_status initialize() = 0;

    /** @brief 执行算法步进 */
    virtual fmi::fmi_status step(double current_time, double step_size) = 0;

    /** @brief 终止算法 */
    virtual fmi::fmi_status terminate() = 0;

    /** @brief 获取算法步长 */
    virtual double get_step_size() const = 0;
};

/**
 * @brief 固定步长算法实现
 *
 * 使用固定仿真步长的最简算法实现。
 */
class fixed_step_algorithm : public algorithm
{
private:
    double step_size_;

public:
    explicit fixed_step_algorithm(double step_size);

    fmi::fmi_status initialize() override;
    fmi::fmi_status step(double current_time, double step_size) override;
    fmi::fmi_status terminate() override;
    double get_step_size() const override { return step_size_; }
};

/**
 * @brief 变量连接信息
 *
 * 描述模型间变量的连接关系和数据传输通道。
 */
struct connection_binding
{
    std::string source_model;
    std::string source_variable;
    std::string target_model;
    std::string target_variable;
    std::function<fmi::fmi_real(fmi::fmi_real)> modifier;

    connection_binding(const std::string& src_m, const std::string& src_v,
                       const std::string& tgt_m, const std::string& tgt_v,
                       std::function<fmi::fmi_real(fmi::fmi_real)> mod = nullptr)
        : source_model(src_m), source_variable(src_v)
        , target_model(tgt_m), target_variable(tgt_v)
        , modifier(mod)
    {}
};

/**
 * @brief 仿真引擎类
 *
 * 管理多模型联合仿真的核心类。
 * 负责模型实例生命周期管理、仿真算法执行、数据传输和监听器通知。
 */
class simulation
{
private:
    std::vector<std::unique_ptr<model_instance>> instances_;
    std::map<std::string, std::unique_ptr<simulation_listener>> listeners_;
    std::vector<connection_binding> connections_;
    std::unique_ptr<algorithm> algorithm_;
    double current_time_;
    size_t step_count_;
    bool initialized_;
    bool terminated_;

    void notify_pre_init(double start_time);
    void notify_post_init(double start_time);
    void notify_pre_step(double current_time, double step_size);
    void notify_post_step(double current_time, double step_size);
    void notify_pre_terminate();
    void notify_post_terminate();
    void notify_on_reset();

    fmi::fmi_status transfer_connection_data();

public:
    explicit simulation(std::unique_ptr<algorithm> algo);
    ~simulation();

    // 禁用拷贝
    simulation(const simulation&) = delete;
    simulation& operator=(const simulation&) = delete;

    // 允许移动
    simulation(simulation&&) = default;
    simulation& operator=(simulation&&) = default;

    // 生命周期管理
    fmi::fmi_status initialize(double start_time = 0.0);
    fmi::fmi_status step();
    fmi::fmi_status step_until(double time_point);
    fmi::fmi_status step_for(double duration);
    fmi::fmi_status terminate();
    fmi::fmi_status reset();

    // 模型实例管理
    void add_model_instance(std::unique_ptr<model_instance> instance);
    model_instance* get_model_instance(const std::string& name) const;

    // 监听器管理
    void add_listener(std::unique_ptr<simulation_listener> listener);
    bool remove_listener(const std::string& name);

    // 变量连接管理
    void add_connection(const connection_binding& conn);
    size_t get_connection_count() const { return connections_.size(); }

    // 变量访问
    fmi::fmi_status get_real(const std::string& identifier, fmi::fmi_real& value);
    fmi::fmi_status set_real(const std::string& identifier, fmi::fmi_real value);
    fmi::fmi_status get_integer(const std::string& identifier, fmi::fmi_integer& value);
    fmi::fmi_status set_integer(const std::string& identifier, fmi::fmi_integer value);
    fmi::fmi_status get_boolean(const std::string& identifier, fmi::fmi_boolean& value);
    fmi::fmi_status set_boolean(const std::string& identifier, fmi::fmi_boolean value);
    fmi::fmi_status get_string(const std::string& identifier, fmi::fmi_string& value);
    fmi::fmi_status set_string(const std::string& identifier, const fmi::fmi_string& value);

    // 访问器
    double get_current_time() const { return current_time_; }
    double get_step_size() const;
    size_t get_step_count() const { return step_count_; }
    bool is_initialized() const { return initialized_; }
    bool is_terminated() const { return terminated_; }
    const std::vector<std::unique_ptr<model_instance>>& get_instances() const { return instances_; }
    const std::vector<connection_binding>& get_connections() const { return connections_; }
};

} // namespace simulation
} // namespace cosmo

#endif // COSMO_SIMULATION_SIMULATION_HPP
