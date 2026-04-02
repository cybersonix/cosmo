#include "cosmo/remote/remote_executor.hpp"
#include "cosmo/remote/remote_types.hpp"
#include "cosmo/test/test_framework.hpp"
#include "cosmo/logger/logger.hpp"

using namespace cosmo::test;
using namespace cosmo::remote;

TEST_CASE(remote_types_basic)
{
    // 测试远程主机
    remote_host host("example.com", 8080);
    REQUIRE_EQ(host.host, "example.com");
    REQUIRE_EQ(host.port, 8080);
    REQUIRE_EQ(host.username, std::string(""));
    REQUIRE_EQ(host.password, std::string(""));

    // 测试远程进程
    remote_process process(1234, "test_command", "/tmp");
    REQUIRE_EQ(process.pid, 1234);
    REQUIRE_EQ(process.command, "test_command");
    REQUIRE_EQ(process.working_directory, "/tmp");
    REQUIRE_FALSE(process.is_running);

    // 测试端点
    endpoint ep("192.168.1.1", 9090, protocol::tcp);
    REQUIRE_EQ(ep.get_address(), "192.168.1.1");
    REQUIRE_EQ(ep.get_port(), 9090);
    REQUIRE_EQ(ep.get_protocol(), protocol::tcp);
}

TEST_CASE(remote_types_defaults)
{
    // 默认构造 - 参数构造函数有默认值
    remote_host default_host;
    REQUIRE_EQ(default_host.host, std::string("localhost"));
    REQUIRE_EQ(default_host.port, 8080);

    remote_process default_process;
    REQUIRE_EQ(default_process.pid, -1);
    REQUIRE_EQ(default_process.command, std::string(""));
    REQUIRE_FALSE(default_process.is_running);
}

TEST_CASE(remote_executor_creation)
{
    remote_host host("localhost", 8080);
    auto executor = create_remote_executor(host);

    REQUIRE(executor != nullptr);
    REQUIRE_EQ(executor->get_host().host, "localhost");
    REQUIRE_EQ(executor->get_host().port, 8080);
}

TEST_CASE(remote_executor_connect)
{
    remote_host host("localhost", 8080);
    auto executor = create_remote_executor(host);

    // stub 实现中 connect 总是返回 true
    bool connected = executor->connect();
    REQUIRE(connected);

    executor->disconnect();
}

TEST_CASE(remote_executor_execute)
{
    remote_host host("localhost", 8080);
    auto executor = create_remote_executor(host);
    executor->connect();

    // stub 实现返回模拟进程
    auto process = executor->execute("echo test", "/tmp");
    REQUIRE(process != nullptr);
    REQUIRE_FALSE(process->command.empty());
    REQUIRE(process->pid > 0);
    REQUIRE(process->is_running);

    executor->disconnect();
}

TEST_CASE(remote_executor_terminate)
{
    remote_host host("localhost", 8080);
    auto executor = create_remote_executor(host);
    executor->connect();

    auto process = executor->execute("echo test", "/tmp");
    REQUIRE(process->is_running);

    bool terminated = executor->terminate_process(*process);
    REQUIRE(terminated);
    REQUIRE_FALSE(executor->is_process_running(*process));

    executor->disconnect();
}

int main()
{
    cosmo::log::info("Starting remote execution test");
    return RUN_ALL_TESTS();
}
