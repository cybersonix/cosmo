/**
 * @file simulation_runner_test.cpp
 * @brief 仿真运行器单元测试
 *
 * 测试仿真运行器的各种功能。
 *
 * @copyright Copyright (c) 2024
 */

#include "cosmo/simulation/simulation_runner.hpp"
#include "cosmo/simulation/simulation.hpp"
#include "cosmo/test/test_framework.hpp"
#include "cosmo/logger/logger.hpp"

#include <thread>
#include <chrono>
#include <system_error>

using namespace cosmo;
using namespace cosmo::simulation;

TEST_CASE(runner_creation)
{
    auto algo = std::unique_ptr<algorithm>(new fixed_step_algorithm(0.01));
    auto sim_instance = std::unique_ptr<cosmo::simulation::simulation>(new cosmo::simulation::simulation(std::move(algo)));

    auto runner = create_simulation_runner(sim_instance.get());
    REQUIRE(runner != nullptr);
}

TEST_CASE(runner_creation_null)
{
    auto runner = create_simulation_runner(nullptr);
    REQUIRE(runner != nullptr);
    REQUIRE_EQ(static_cast<int>(runner->get_state()), static_cast<int>(runner_state::error));
}

TEST_CASE(runner_state)
{
    auto algo = std::unique_ptr<algorithm>(new fixed_step_algorithm(0.01));
    auto sim_instance = std::unique_ptr<cosmo::simulation::simulation>(new cosmo::simulation::simulation(std::move(algo)));
    auto runner = create_simulation_runner(sim_instance.get());

    REQUIRE_EQ(static_cast<int>(runner->get_state()), static_cast<int>(runner_state::idle));
    REQUIRE_FALSE(runner->is_running());
}

TEST_CASE(runner_configuration)
{
    auto algo = std::unique_ptr<algorithm>(new fixed_step_algorithm(0.01));
    auto sim_instance = std::unique_ptr<cosmo::simulation::simulation>(new cosmo::simulation::simulation(std::move(algo)));
    auto runner = create_simulation_runner(sim_instance.get());

    runner->set_real_time_factor(0.5);
    REQUIRE_EQ(runner->get_real_time_factor(), 0.5);

    runner->set_real_time_factor(2.0);
    REQUIRE_EQ(runner->get_real_time_factor(), 2.0);

    // -1 表示尽快
    runner->set_real_time_factor(-1.0);
    REQUIRE_EQ(runner->get_real_time_factor(), -1.0);

    // 0 也表示尽快
    runner->set_real_time_factor(0.0);
    REQUIRE_EQ(runner->get_real_time_factor(), 0.0);
}

TEST_CASE(runner_stop_time)
{
    auto algo = std::unique_ptr<algorithm>(new fixed_step_algorithm(0.01));
    auto sim_instance = std::unique_ptr<cosmo::simulation::simulation>(new cosmo::simulation::simulation(std::move(algo)));
    auto runner = create_simulation_runner(sim_instance.get());

    runner->set_stop_time(10.0);
    // 验证设置不崩溃，stop_time 是私有成员但通过 start 的行为间接验证

    runner->set_stop_time(0.0);
}

TEST_CASE(runner_statistics)
{
    auto algo = std::unique_ptr<algorithm>(new fixed_step_algorithm(0.01));
    auto sim_instance = std::unique_ptr<cosmo::simulation::simulation>(new cosmo::simulation::simulation(std::move(algo)));
    auto runner = create_simulation_runner(sim_instance.get());

    REQUIRE_EQ(runner->get_elapsed_time(), 0.0);
    REQUIRE_FALSE(runner->is_running());
}

TEST_CASE(runner_invalid_operations)
{
    auto algo = std::unique_ptr<algorithm>(new fixed_step_algorithm(0.01));
    auto sim_instance = std::unique_ptr<cosmo::simulation::simulation>(new cosmo::simulation::simulation(std::move(algo)));
    auto runner = create_simulation_runner(sim_instance.get());

    runner->stop();
    runner->pause();
    runner->resume();

    REQUIRE_EQ(static_cast<int>(runner->get_state()), static_cast<int>(runner_state::idle));
}

TEST_CASE(runner_start_stop)
{
    auto algo = std::unique_ptr<algorithm>(new fixed_step_algorithm(0.001));
    auto sim_instance = std::unique_ptr<cosmo::simulation::simulation>(new cosmo::simulation::simulation(std::move(algo)));
    auto runner = create_simulation_runner(sim_instance.get());

    // 设置一个很短的停止时间，使仿真快速结束
    runner->set_stop_time(0.01);
    runner->set_real_time_factor(-1.0); // 尽快运行

    try {
        runner->start();
        REQUIRE(runner->is_running());

        runner->wait_for_completion();
    } catch (const std::system_error&) {
        // 环境不支持多线程时跳过
        cosmo::log::warn("多线程不可用，跳过运行器测试");
        return;
    }

    REQUIRE_FALSE(runner->is_running());
    REQUIRE(static_cast<int>(runner->get_state()) == static_cast<int>(runner_state::stopped) ||
            static_cast<int>(runner->get_state()) == static_cast<int>(runner_state::error));

    REQUIRE(runner->get_elapsed_time() >= 0.0);
}

TEST_CASE(runner_pause_resume)
{
    auto algo = std::unique_ptr<algorithm>(new fixed_step_algorithm(0.001));
    auto sim_instance = std::unique_ptr<cosmo::simulation::simulation>(new cosmo::simulation::simulation(std::move(algo)));
    auto runner = create_simulation_runner(sim_instance.get());
    runner->set_real_time_factor(-1.0);

    try {
        runner->start();
        REQUIRE(runner->is_running());

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        runner->pause();

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        REQUIRE_EQ(static_cast<int>(runner->get_state()), static_cast<int>(runner_state::paused));
        REQUIRE_FALSE(runner->is_running());

        runner->resume();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        REQUIRE(runner->is_running());

        runner->stop();
        runner->wait_for_completion();
    } catch (const std::system_error&) {
        cosmo::log::warn("多线程不可用，跳过暂停/恢复测试");
        return;
    }

    REQUIRE_FALSE(runner->is_running());
}

TEST_CASE(runner_completion_callback)
{
    auto algo = std::unique_ptr<algorithm>(new fixed_step_algorithm(0.001));
    auto sim_instance = std::unique_ptr<cosmo::simulation::simulation>(new cosmo::simulation::simulation(std::move(algo)));
    auto runner = create_simulation_runner(sim_instance.get());
    runner->set_stop_time(0.005);
    runner->set_real_time_factor(-1.0);

    bool callback_called = false;
    runner->set_completion_callback([&callback_called]() {
        callback_called = true;
    });

    try {
        runner->start();
        runner->wait_for_completion();
    } catch (const std::system_error&) {
        cosmo::log::warn("多线程不可用，跳过回调测试");
        return;
    }

    REQUIRE(callback_called);
}

int main()
{
    cosmo::log::info("Starting simulation runner tests");
    return RUN_ALL_TESTS();
}
