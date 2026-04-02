#ifndef COSMO_REMOTE_REMOTE_EXECUTOR_HPP
#define COSMO_REMOTE_REMOTE_EXECUTOR_HPP

#include "cosmo/remote/remote_types.hpp"
#include "cosmo/logger/logger.hpp"

#include <string>
#include <memory>
#include <vector>

namespace cosmo {
namespace remote {

// 远程执行器类
class remote_executor
{
private:
    remote_host host_;
    std::vector<std::unique_ptr<remote_process>> processes_;
    
public:
    explicit remote_executor(const remote_host& host = remote_host());
    
    // 连接到远程主机
    bool connect();
    
    // 断开连接
    void disconnect();
    
    // 在远程主机上执行命令
    std::unique_ptr<remote_process> execute(const std::string& command, const std::string& working_dir = "");
    
    // 终止远程进程
    bool terminate_process(remote_process& process);
    
    // 检查进程是否仍在运行
    bool is_process_running(const remote_process& process);
    
    // 获取远程主机信息
    const remote_host& get_host() const { return host_; }
    
    ~remote_executor();
};

// 工厂函数
std::unique_ptr<remote_executor> create_remote_executor(const remote_host& host);

} // namespace remote
} // namespace cosmo

#endif // COSMO_REMOTE_REMOTE_EXECUTOR_HPP