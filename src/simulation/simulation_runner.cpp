#include "cosmo/simulation/simulation_runner.hpp"
#include "cosmo/logger/logger.hpp"

#include <thread>
#include <chrono>

namespace cosmo {
namespace simulation {

simulation_runner::simulation_runner(simulation* sim)
    : simulation_(sim)
    , state_(runner_state::idle)
    , real_time_factor_(-1.0) // 默认尽可能快
    , stop_time_(-1.0) // 默认不限制
    , elapsed_time_(0.0)
{
    if (!simulation_) {
        cosmo::log::error("仿真运行器创建时传入了空指针");
        state_ = runner_state::error;
    }
}

simulation_runner::~simulation_runner()
{
    stop();
    if (runner_thread_.joinable()) {
        runner_thread_.join();
    }
}

void simulation_runner::start()
{
    if (state_ == runner_state::running) {
        cosmo::log::warn("仿真运行器已在运行中");
        return;
    }

    if (!simulation_) {
        cosmo::log::error("无法启动运行器: 仿真实例为空");
        state_ = runner_state::error;
        return;
    }

    // 如果之前停止过，重新初始化
    if (state_ == runner_state::stopped) {
        simulation_->reset();
        simulation_->initialize(0.0);
    }
    else if (!simulation_->is_initialized()) {
        simulation_->initialize(0.0);
    }

    state_ = runner_state::running;
    start_time_point_ = std::chrono::steady_clock::now();
    
    // 启动运行线程
    if (runner_thread_.joinable()) {
        runner_thread_.join();
    }
    runner_thread_ = std::thread(&simulation_runner::run_loop, this);

    cosmo::log::info("仿真运行器已启动");
}

void simulation_runner::stop()
{
    if (state_ != runner_state::running && state_ != runner_state::paused) {
        return;
    }

    state_ = runner_state::stopped;
    cosmo::log::info("仿真运行器已停止");
}

void simulation_runner::pause()
{
    if (state_ != runner_state::running) {
        cosmo::log::warn("无法暂停: 运行器未在运行中");
        return;
    }

    state_ = runner_state::paused;
    cosmo::log::info("仿真运行器已暂停");
}

void simulation_runner::resume()
{
    if (state_ != runner_state::paused) {
        cosmo::log::warn("无法恢复: 运行器未处于暂停状态");
        return;
    }

    state_ = runner_state::running;
    cosmo::log::info("仿真运行器已恢复运行");
}

void simulation_runner::set_real_time_factor(double factor)
{
    real_time_factor_ = factor;
    cosmo::log::info("实时因子设置为: " + std::to_string(factor));
}

runner_state simulation_runner::get_state() const
{
    return state_.load();
}

double simulation_runner::get_real_time_factor() const
{
    return real_time_factor_.load();
}

bool simulation_runner::is_running() const
{
    return state_.load() == runner_state::running;
}

double simulation_runner::get_elapsed_time() const
{
    return elapsed_time_.load();
}

void simulation_runner::set_stop_time(double stop_time)
{
    stop_time_ = stop_time;
    cosmo::log::info("停止时间设置为: " + std::to_string(stop_time));
}

void simulation_runner::wait_for_completion()
{
    if (runner_thread_.joinable()) {
        runner_thread_.join();
    }
}

void simulation_runner::set_completion_callback(std::function<void()> callback)
{
    std::lock_guard<std::mutex> lock(callback_mutex_);
    completion_callback_ = callback;
}

void simulation_runner::run_loop()
{
    auto last_wall_time = std::chrono::steady_clock::now();
    
    // 获取算法步长作为默认步长，避免硬编码
    double default_step_size = simulation_->get_step_size();
    if (default_step_size <= 0) {
        default_step_size = 0.01;  // 兜底默认值
    }

    while (state_.load() == runner_state::running || state_.load() == runner_state::paused) {
        // 处理暂停
        if (state_.load() == runner_state::paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            last_wall_time = std::chrono::steady_clock::now();
            continue;
        }

        // 检查停止时间
        double current_sim_time = simulation_->get_current_time();
        double stop_time = stop_time_.load();
        if (stop_time > 0 && current_sim_time >= stop_time) {
            cosmo::log::info("达到停止时间: " + std::to_string(stop_time));
            break;
        }

        // 执行仿真步
        fmi::fmi_status status = simulation_->step();
        
        if (status == fmi::fmi_error || status == fmi::fmi_fatal) {
            cosmo::log::error("仿真步进过程中发生错误");
            state_ = runner_state::error;
            break;
        }

        // 更新已运行时间
        auto now = std::chrono::steady_clock::now();
        elapsed_time_ = std::chrono::duration<double>(now - start_time_point_).count();

        // 实时控制 - 使用默认步长进行实时同步
        sleep_for_real_time(default_step_size, last_wall_time);
        last_wall_time = std::chrono::steady_clock::now();
    }

    // 仿真结束，调用回调
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        if (completion_callback_) {
            completion_callback_();
        }
    }

    if (state_.load() != runner_state::error) {
        state_ = runner_state::stopped;
    }
}

void simulation_runner::sleep_for_real_time(double sim_time_delta, 
                                            std::chrono::steady_clock::time_point& last_wall_time)
{
    double rtf = real_time_factor_.load();
    
    // rtf <= 0 表示尽可能快，不休眠
    if (rtf <= 0) {
        return;
    }

    // 计算期望的墙钟时间
    double target_wall_time_delta = sim_time_delta / rtf;
    auto target_duration = std::chrono::duration<double>(target_wall_time_delta);
    
    // 计算已经经过的墙钟时间
    auto now = std::chrono::steady_clock::now();
    auto elapsed = now - last_wall_time;
    
    // 如果需要，休眠剩余时间
    if (elapsed < target_duration) {
        auto sleep_time = target_duration - elapsed;
        std::this_thread::sleep_for(sleep_time);
    }
}

// 工厂函数
std::unique_ptr<simulation_runner> create_simulation_runner(simulation* sim)
{
    return std::unique_ptr<simulation_runner>(new simulation_runner(sim));
}

} // namespace simulation
} // namespace cosmo
