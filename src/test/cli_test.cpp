#include "cosmo/cli/cli.hpp"
#include "cosmo/test/test_framework.hpp"
#include "cosmo/logger/logger.hpp"

using namespace cosmo::test;
using namespace cosmo::cli;

// 简单的命令处理器实现
class test_command_handler : public command_handler
{
private:
    std::string name_;
    std::string description_;
    
public:
    test_command_handler(const std::string& name, const std::string& description)
        : name_(name), description_(description) {}
    
    int handle_command(const cli_args& /*args*/) override
    {
        cosmo::log::info("Handling command: " + name_);
        return 0;
    }
    
    std::string get_command_name() const override
    {
        return name_;
    }
    
    std::string get_command_description() const override
    {
        return description_;
    }
};

TEST_CASE(cli_basic)
{
    // 创建CLI
    auto cli_app = create_cli("test_cli");
    REQUIRE(cli_app != nullptr);
    
    // 注册命令处理器
    auto handler1 = std::unique_ptr<command_handler>(new test_command_handler("test1", "Test command 1"));
    auto handler2 = std::unique_ptr<command_handler>(new test_command_handler("test2", "Test command 2"));
    
    cli_app->register_command_handler(std::move(handler1));
    cli_app->register_command_handler(std::move(handler2));
    
    // 测试解析参数
    char* argv1[] = {(char*)"test_cli", (char*)"test1", (char*)"arg1", (char*)"arg2"};
    cli_args args1 = cli_app->parse_arguments(4, argv1);
    
    REQUIRE_EQ(args1.command, "test1");
    REQUIRE_EQ(args1.arguments.size(), 2);
    REQUIRE_EQ(args1.arguments[0], "arg1");
    REQUIRE_EQ(args1.arguments[1], "arg2");
    
    // 测试解析带选项的参数
    char* argv2[] = {(char*)"test_cli", (char*)"test2", (char*)"-o", (char*)"value", (char*)"arg1"};
    cli_args args2 = cli_app->parse_arguments(5, argv2);
    
    REQUIRE_EQ(args2.command, "test2");
    REQUIRE_EQ(args2.arguments.size(), 1);
    REQUIRE_EQ(args2.arguments[0], "arg1");
    REQUIRE_EQ(args2.options.size(), 1);
    REQUIRE_EQ(args2.options[0].first, "-o");
    REQUIRE_EQ(args2.options[0].second, "value");
}

TEST_CASE(cli_execution)
{
    // 创建CLI
    auto cli_app = create_cli("test_cli");
    REQUIRE(cli_app != nullptr);
    
    // 注册命令处理器
    auto handler = std::unique_ptr<command_handler>(new test_command_handler("test", "Test command"));
    cli_app->register_command_handler(std::move(handler));
    
    // 测试执行命令
    char* argv[] = {(char*)"test_cli", (char*)"test"};
    int result = cli_app->run(2, argv);
    REQUIRE_EQ(result, 0);
    
    // 测试执行未知命令
    char* argv2[] = {(char*)"test_cli", (char*)"unknown"};
    result = cli_app->run(2, argv2);
    REQUIRE_EQ(result, 1);
}

int main()
{
    cosmo::log::info("Starting CLI test");
    return RUN_ALL_TESTS();
}