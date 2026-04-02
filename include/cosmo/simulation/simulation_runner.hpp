#ifndef COSMO_SIMULATION_SIMULATION_RUNNER_HPP
#define COSMO_SIMULATION_SIMULATION_RUNNER_HPP

#include "cosmo/simulation/simulation.hpp"

#include <atomic>
#include <thread>
#include <functional>
#include <chrono>
#include <mutex>

namespace cosmo {
namespace simulation {

// 仿真运行器状态
enum class runner_state
{
    idle,       // 空闲
    running,    // 运行中
    paused,     // 暂停
    stopped,    // 已停止
    error       // 错误
};

// 仿真运行器 - 支持实时仿真控制
class simulation_runner
{
public:
    explicit simulation_runner(simulation* sim);
    ~simulation_runner();

    // 禁用拷贝
    simulation_runner(const simulation_runner&) = delete;
    simulation_runner& operator=(const simulation_runner&) = delete;

    // 启动仿真
    void start();

    // 停止仿真
    void stop();

    // 暂停仿真
    void pause();

    // 恢复仿真
    void resume();

    // 设置实时因子 (1.0 = 实时, 2.0 = 2倍速, 0.5 = 半速, -1 = 尽可能快)
    void set_real_time_factor(double factor);

    // 获取当前状态
    runner_state get_state() const;

    // 获取当前实时因子
    double get_real_time_factor() const;

    // 检查是否正在运行
    bool is_running() const;

    // 获取已运行时间（秒）
    double get_elapsed_time() const;

    // 设置停止时间（可选）
    void set_stop_time(double stop_time);

    // 等待仿真完成（阻塞）
    void wait_for_completion();

    // 设置完成回调
    void set_completion_callback(std::function<void()> callback);

private:
    simulation* simulation_;
    std::atomic<runner_state> state_;
    std::atomic<double> real_time_factor_;
    std::atomic<double> stop_time_;
    std::atomic<double> elapsed_time_;
    std::thread runner_thread_;
    std::function<void()> completion_callback_;
    std::mutex callback_mutex_;

    std::chrono::steady_clock::time_point start_time_point_;

    // 运行循环
    void run_loop();

    // 休眠以维持实时因子
    void sleep_for_real_time(double sim_time_delta, 
                             std::chrono::steady_clock::time_point& last_wall_time);
};

// 工厂函数
std::unique_ptr<simulation_runner> create_simulation_runner(simulation* sim);

} // namespace simulation
} // namespace cosmo

#endif // COSMO_SIMULATION_SIMULATION_RUNNER_HPP
