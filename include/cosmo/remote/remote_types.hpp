#ifndef COSMO_REMOTE_REMOTE_TYPES_HPP
#define COSMO_REMOTE_REMOTE_TYPES_HPP

#include <string>
#include <vector>
#include <memory>

namespace cosmo {
namespace remote {

// 远程主机信息
struct remote_host
{
    std::string host;
    int port;
    std::string username;
    std::string password;
    
    remote_host(const std::string& h = "localhost", int p = 8080)
        : host(h), port(p) {}
};

// 远程进程信息
struct remote_process
{
    int pid;
    std::string command;
    std::string working_directory;
    bool is_running;
    
    remote_process(int id = -1, const std::string& cmd = "", const std::string& wd = "")
        : pid(id), command(cmd), working_directory(wd), is_running(false) {}
};

// 远程通信协议枚举
enum class protocol
{
    tcp,
    udp,
    unix_domain_socket
};

// 远程通信端点
class endpoint
{
private:
    std::string address_;
    int port_;
    protocol protocol_;
    
public:
    endpoint(const std::string& address, int port, protocol proto = protocol::tcp)
        : address_(address), port_(port), protocol_(proto) {}
    
    const std::string& get_address() const { return address_; }
    int get_port() const { return port_; }
    protocol get_protocol() const { return protocol_; }
};

} // namespace remote
} // namespace cosmo

#endif // COSMO_REMOTE_REMOTE_TYPES_HPP