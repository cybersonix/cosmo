#include "cosmo/remote/remote_executor.hpp"
#include "cosmo/util/filesystem.hpp"

#include <cstdlib>
#include <cstdio>
#include <memory>
#include <atomic>

namespace cosmo {
namespace remote {

remote_executor::remote_executor(const remote_host& host)
    : host_(host)
{
    cosmo::log::info("Creating remote executor for host: " + host.host + ":" + std::to_string(host.port));
}

bool remote_executor::connect()
{
    cosmo::log::info("Connecting to remote host: " + host_.host + ":" + std::to_string(host_.port));
    
    // 简化的连接逻辑 - 在实际实现中，这里会建立网络连接
    // 为了演示目的，我们假设连接总是成功的
    
    cosmo::log::info("Connected to remote host successfully");
    return true;
}

void remote_executor::disconnect()
{
    cosmo::log::info("Disconnecting from remote host: " + host_.host + ":" + std::to_string(host_.port));
    
    // 终止所有远程进程
    for (auto& process : processes_) {
        if (process && process->is_running) {
            terminate_process(*process);
        }
    }
    
    processes_.clear();
}

std::unique_ptr<remote_process> remote_executor::execute(const std::string& command, const std::string& working_dir)
{
    cosmo::log::info("Executing command on remote host: " + command);
    
    // 简化的执行逻辑 - 在实际实现中，这里会通过网络发送命令
    // 为了演示目的，我们创建一个模拟的进程对象
    
    auto process = std::unique_ptr<remote_process>(new remote_process());
    process->command = command;
    process->working_directory = working_dir;
    // 使用原子计数器生成模拟PID，避免rand()的线程安全问题
    static std::atomic<int> next_pid{1000};
    process->pid = next_pid.fetch_add(1);
    process->is_running = true;
    
    // 添加到进程列表
    processes_.push_back(std::move(process));
    
    // 返回最后一个添加的进程的指针
    return std::unique_ptr<remote_process>(new remote_process(*processes_.back()));
}

bool remote_executor::terminate_process(remote_process& process)
{
    cosmo::log::info("Terminating remote process with PID: " + std::to_string(process.pid));
    
    // 简化的终止逻辑 - 在实际实现中，这里会发送终止信号
    // 为了演示目的，我们直接标记进程为已终止
    
    process.is_running = false;
    
    // 在进程列表中查找并标记为已终止
    for (auto& proc : processes_) {
        if (proc && proc->pid == process.pid) {
            proc->is_running = false;
            break;
        }
    }
    
    cosmo::log::info("Remote process terminated successfully");
    return true;
}

bool remote_executor::is_process_running(const remote_process& process)
{
    // 在进程列表中查找进程状态
    for (const auto& proc : processes_) {
        if (proc && proc->pid == process.pid) {
            return proc->is_running;
        }
    }
    
    // 如果找不到进程，默认认为不在运行
    return false;
}

remote_executor::~remote_executor()
{
    disconnect();
}

// 工厂函数实现
std::unique_ptr<remote_executor> create_remote_executor(const remote_host& host)
{
    return std::unique_ptr<remote_executor>(new remote_executor(host));
}

} // namespace remote
} // namespace cosmo