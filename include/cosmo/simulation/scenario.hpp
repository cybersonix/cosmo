#ifndef COSMO_SIMULATION_SCENARIO_HPP
#define COSMO_SIMULATION_SCENARIO_HPP

#include "cosmo/simulation/simulation_listener.hpp"
#include "cosmo/fmi/fmi_types.hpp"

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <limits>

namespace cosmo {
namespace simulation {

// 前向声明
class simulation;

// 动作类型标签
enum class command_kind
{
    assign_real,
    assign_integer,
    assign_boolean,
    assign_string
};

// 变量设定值 - 使用 tagged union 风格的单结构体替代继承层次
struct variable_command
{
    // 时刻
    double scheduled_time;
    // 目标变量（格式："instance_name::variable_name"，与仿真引擎 API 一致）
    std::string target;
    // 动作类别
    command_kind kind;
    // 带类型的值存储
    double real_val;
    int int_val;
    bool bool_val;
    std::string str_val;

    variable_command()
        : scheduled_time(0.0)
        , kind(command_kind::assign_real)
        , real_val(0.0)
        , int_val(0)
        , bool_val(false)
    {}

    // 工厂方法
    static variable_command make_real(double t, const std::string& tgt, double v)
    {
        variable_command cmd;
        cmd.scheduled_time = t;
        cmd.target = tgt;
        cmd.kind = command_kind::assign_real;
        cmd.real_val = v;
        return cmd;
    }

    static variable_command make_integer(double t, const std::string& tgt, int v)
    {
        variable_command cmd;
        cmd.scheduled_time = t;
        cmd.target = tgt;
        cmd.kind = command_kind::assign_integer;
        cmd.int_val = v;
        return cmd;
    }

    static variable_command make_boolean(double t, const std::string& tgt, bool v)
    {
        variable_command cmd;
        cmd.scheduled_time = t;
        cmd.target = tgt;
        cmd.kind = command_kind::assign_boolean;
        cmd.bool_val = v;
        return cmd;
    }

    static variable_command make_string(double t, const std::string& tgt, const std::string& v)
    {
        variable_command cmd;
        cmd.scheduled_time = t;
        cmd.target = tgt;
        cmd.kind = command_kind::assign_string;
        cmd.str_val = v;
        return cmd;
    }
};

// 场景调度器 - 管理时间轴上的变量设定命令
// 使用扁平的 command 列表 + 二分查找插入 + 前向 cursor 扫描
class scenario : public simulation_listener
{
public:
    // 时间容差：用于浮点时间比较
    static const double DEFAULT_TOLERANCE;

    scenario();
    ~scenario() override = default;

    // ---- 命令管理 ----

    // 注册命令（自动维护时间排序）
    void schedule(const variable_command& cmd);

    // 便捷注册方法
    void schedule_real(double time, const std::string& target, double value);
    void schedule_integer(double time, const std::string& target, int value);
    void schedule_boolean(double time, const std::string& target, bool value);
    void schedule_string(double time, const std::string& target, const std::string& value);

    // 查询
    size_t command_count() const;
    const variable_command* peek_command(size_t idx) const;

    // 清除所有已注册的命令
    void purge();

    // ---- 关联仿真实例 ----

    /**
     * 绑定仿真实例，使调度器能够在正确时刻修改变量。
     * 通常由 simulation::add_listener 自动调用，也可手动调用。
     */
    void bind(simulation* sim);

    // ---- 监听器回调 ----

    void on_pre_init(double start_time) override;
    void on_post_init(double start_time) override;
    void on_pre_step(double current_time, double step_size) override;
    void on_post_step(double current_time, double step_size) override;
    void on_pre_terminate() override;
    void on_post_terminate() override;
    void on_reset() override;
    std::string get_name() const override;
    void set_owner(simulation* owner) override;

    // ---- 序列化 ----

    static std::unique_ptr<scenario> from_xml(const std::string& xml_content);
    static std::unique_ptr<scenario> from_file(const std::string& path);

private:
    // 按时间排序的命令队列
    std::vector<variable_command> timeline_;
    // 已调度位置标记（用于前向扫描）
    size_t dispatch_cursor_;
    // 绑定的仿真指针
    simulation* bound_sim_;

    // 执行单条命令
    bool fire_command(const variable_command& cmd);

    // 解析 "instance_name::variable_name" 格式的目标字符串
    bool resolve_target(const std::string& target,
                        std::string& out_instance,
                        std::string& out_variable) const;
};

// 工厂函数
std::unique_ptr<scenario> make_scenario();

} // namespace simulation
} // namespace cosmo

#endif // COSMO_SIMULATION_SCENARIO_HPP
