#ifndef COSMO_CLI_CLI_HPP
#define COSMO_CLI_CLI_HPP

#include <string>
#include <vector>
#include <memory>

namespace cosmo {
namespace cli {

// CLI参数结构
struct cli_args
{
    std::string command;
    std::vector<std::string> arguments;
    std::vector<std::pair<std::string, std::string>> options;
};

// CLI命令处理器基类
class command_handler
{
public:
    virtual ~command_handler() = default;
    
    // 处理命令
    virtual int handle_command(const cli_args& args) = 0;
    
    // 获取命令名称
    virtual std::string get_command_name() const = 0;
    
    // 获取命令描述
    virtual std::string get_command_description() const = 0;
};

// 主CLI类
class cli
{
private:
    std::vector<std::unique_ptr<command_handler>> handlers_;
    std::string program_name_;
    
public:
    explicit cli(const std::string& program_name = "cosmo");
    
    // 注册命令处理器
    void register_command_handler(std::unique_ptr<command_handler> handler);
    
    // 解析命令行参数
    cli_args parse_arguments(int argc, char* argv[]);
    
    // 执行命令
    int execute(const cli_args& args);
    
    // 显示帮助信息
    void show_help();
    
    // 运行CLI
    int run(int argc, char* argv[]);
};

// 工厂函数
std::unique_ptr<cli> create_cli(const std::string& program_name = "cosmo");

} // namespace cli
} // namespace cosmo

#endif // COSMO_CLI_CLI_HPP