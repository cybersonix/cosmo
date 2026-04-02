#ifndef COSMO_SIMULATION_SIMULATION_LISTENER_HPP
#define COSMO_SIMULATION_SIMULATION_LISTENER_HPP

#include "cosmo/fmi/fmi_types.hpp"
#include <string>
#include <memory>

namespace cosmo {
namespace simulation {

// 仿真事件类型
enum class simulation_event
{
    pre_init,
    post_init,
    pre_step,
    post_step,
    pre_terminate,
    post_terminate,
    on_reset
};

// 仿真监听器接口
class simulation_listener
{
public:
    virtual ~simulation_listener() = default;
    
    // 初始化前事件
    virtual void on_pre_init(double /*start_time*/) {}
    
    // 初始化后事件
    virtual void on_post_init(double /*start_time*/) {}
    
    // 步进前事件
    virtual void on_pre_step(double /*current_time*/, double /*step_size*/) {}
    
    // 步进后事件
    virtual void on_post_step(double /*current_time*/, double /*step_size*/) {}
    
    // 终止前事件
    virtual void on_pre_terminate() {}
    
    // 终止后事件
    virtual void on_post_terminate() {}
    
    // 重置事件
    virtual void on_reset() {}
    
    // 获取监听器名称
    virtual std::string get_name() const = 0;

    // 设置所属仿真实例（由 simulation::add_listener 自动调用）
    virtual void set_owner(class simulation* /*owner*/) {}
};

// 监听器工厂函数
std::unique_ptr<simulation_listener> create_simulation_listener(const std::string& name);

} // namespace simulation
} // namespace cosmo

#endif // COSMO_SIMULATION_SIMULATION_LISTENER_HPP