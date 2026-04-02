/**
 * @file cosmo_api_test.cpp
 * @brief C API 全面功能测试
 *
 * 测试C API的所有已实现功能，包括仿真结构创建、
 * 仿真执行、变量访问、监听器管理和错误处理。
 */

#include "cosmo/cosmo.h"
#include "cosmo/test/test_framework.hpp"
#include "cosmo/logger/logger.hpp"
#include "cosmo/fmi/fmi_types.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

// ============================================================================
// 版本信息测试
// ============================================================================

TEST_CASE(c_api_version) {
    cosmo_version v = cosmo_library_version();
    REQUIRE_EQ(v.major, 0);
    REQUIRE_EQ(v.minor, 1);
    REQUIRE_EQ(v.patch, 0);

    const char* vs = cosmo_library_version_string();
    REQUIRE(vs != nullptr);
    REQUIRE_EQ(std::string(vs), std::string("0.1.0"));
}

// ============================================================================
// 日志测试
// ============================================================================

TEST_CASE(c_api_log_level) {
    // 设置各种日志级别，不应崩溃
    cosmo_set_log_level("trace");
    cosmo_set_log_level("debug");
    cosmo_set_log_level("info");
    cosmo_set_log_level("warn");
    cosmo_set_log_level("err");
    cosmo_set_log_level("off");

    // 恢复默认
    cosmo_set_log_level("warn");
}

TEST_CASE(c_api_log_functions) {
    cosmo_set_log_level("info");
    cosmo_trace("trace test");
    cosmo_debug("debug test");
    cosmo_info("info test");
    cosmo_warn("warn test");
    cosmo_error("error test");
    cosmo_set_log_level("off");
}

// ============================================================================
// 错误处理测试
// ============================================================================

TEST_CASE(c_api_last_error) {
    cosmo_clear_error();
    REQUIRE(std::string(cosmo_last_error_msg()).empty());

    // 传入nullptr应设置错误
    cosmo_simulation_t* sim = cosmo_simulation_create_from_structure(nullptr, 0.1);
    REQUIRE(sim == nullptr);
    REQUIRE(std::string(cosmo_last_error_msg()).length() > 0);
    cosmo_clear_error();
}

// ============================================================================
// 参数集测试
// ============================================================================

TEST_CASE(c_api_parameter_set) {
    cosmo_parameter_set_t* pps = cosmo_parameter_set_create();
    REQUIRE(pps != nullptr);

    cosmo_parameter_set_add_real(pps, "mass", 2.5);
    cosmo_parameter_set_add_integer(pps, "count", 10);
    cosmo_parameter_set_add_boolean(pps, "enabled", 1);
    cosmo_parameter_set_add_string(pps, "name", "test");

    // 销毁不应崩溃
    cosmo_parameter_set_destroy(pps);

    // 销毁nullptr应安全
    cosmo_parameter_set_destroy(nullptr);
}

TEST_CASE(c_api_parameter_set_null_checks) {
    // 传入nullptr不应崩溃
    cosmo_parameter_set_add_real(nullptr, "x", 1.0);
    cosmo_parameter_set_add_integer(nullptr, "x", 1);
    cosmo_parameter_set_add_boolean(nullptr, "x", 1);
    cosmo_parameter_set_add_string(nullptr, "x", "v");
    cosmo_parameter_set_destroy(nullptr);
}

// ============================================================================
// 仿真结构测试
// ============================================================================

TEST_CASE(c_api_simulation_structure_create_destroy) {
    cosmo_simulation_structure_t* ss = cosmo_simulation_structure_create();
    REQUIRE(ss != nullptr);
    cosmo_simulation_structure_destroy(ss);

    // 销毁nullptr应安全
    cosmo_simulation_structure_destroy(nullptr);
}

TEST_CASE(c_api_simulation_structure_add_model) {
    cosmo_simulation_structure_t* ss = cosmo_simulation_structure_create();

    int ret = cosmo_simulation_structure_add_model(ss, "m1", "/path/to/model.fmu");
    REQUIRE_EQ(ret, 0);

    // nullptr参数
    ret = cosmo_simulation_structure_add_model(nullptr, "m2", "x.fmu");
    REQUIRE(ret != 0);

    ret = cosmo_simulation_structure_add_model(ss, nullptr, "x.fmu");
    REQUIRE(ret != 0);

    cosmo_simulation_structure_destroy(ss);
}

TEST_CASE(c_api_simulation_structure_connections) {
    cosmo_simulation_structure_t* ss = cosmo_simulation_structure_create();
    cosmo_simulation_structure_add_model(ss, "m1", "a.fmu");
    cosmo_simulation_structure_add_model(ss, "m2", "b.fmu");

    // 各种类型的连接不应崩溃
    cosmo_simulation_structure_make_real_connection(ss, "m1::out", "m2::in");
    cosmo_simulation_structure_make_integer_connection(ss, "m1::status", "m2::mode");
    cosmo_simulation_structure_make_boolean_connection(ss, "m1::flag", "m2::en");
    cosmo_simulation_structure_make_string_connection(ss, "m1::name", "m2::label");

    // nullptr不应崩溃
    cosmo_simulation_structure_make_real_connection(nullptr, "a", "b");
    cosmo_simulation_structure_make_real_connection(ss, nullptr, "b");

    cosmo_simulation_structure_destroy(ss);
}

TEST_CASE(c_api_simulation_structure_add_parameter_set) {
    cosmo_simulation_structure_t* ss = cosmo_simulation_structure_create();

    cosmo_parameter_set_t* pps = cosmo_parameter_set_create();
    cosmo_parameter_set_add_real(pps, "x", 1.0);

    int ret = cosmo_simulation_structure_add_parameter_set(ss, "params", pps);
    REQUIRE_EQ(ret, 0);

    ret = cosmo_simulation_structure_add_parameter_set(nullptr, "params", pps);
    REQUIRE(ret != 0);

    cosmo_parameter_set_destroy(pps);
    cosmo_simulation_structure_destroy(ss);
}

// ============================================================================
// 仿真创建和执行测试
// ============================================================================

TEST_CASE(c_api_create_simulation_from_structure) {
    cosmo_simulation_structure_t* ss = cosmo_simulation_structure_create();
    cosmo_simulation_structure_add_model(ss, "model1", "test.fmu");
    cosmo_simulation_structure_make_real_connection(ss, "model1::out", "model1::in");

    cosmo_simulation_t* sim = cosmo_simulation_create_from_structure(ss, 0.01);
    REQUIRE(sim != nullptr);

    // 清理
    cosmo_simulation_destroy(sim);
    cosmo_simulation_structure_destroy(ss);
}

TEST_CASE(c_api_null_structure_returns_null) {
    cosmo_simulation_t* sim = cosmo_simulation_create_from_structure(nullptr, 0.1);
    REQUIRE(sim == nullptr);
}

TEST_CASE(c_api_ssp_not_implemented) {
    cosmo_simulation_t* sim = cosmo_simulation_create_from_ssp("nonexistent.ssp", 0.1);
    REQUIRE(sim == nullptr);
    REQUIRE(std::string(cosmo_last_error_msg()).length() > 0);
    cosmo_clear_error();
}

TEST_CASE(c_api_simulation_lifecycle) {
    cosmo_simulation_structure_t* ss = cosmo_simulation_structure_create();
    cosmo_simulation_structure_add_model(ss, "m1", "model.fmu");

    cosmo_simulation_t* sim = cosmo_simulation_create_from_structure(ss, 0.1);
    REQUIRE(sim != nullptr);

    // 初始化
    int ret = cosmo_simulation_initialize(sim, 0.0);
    REQUIRE_EQ(ret, 0);

    // 执行步进
    ret = cosmo_simulation_step(sim);
    REQUIRE_EQ(ret, 0);

    // 检查时间
    double t = cosmo_simulation_get_time(sim);
    REQUIRE(fabs(t - 0.1) < 1e-12);

    // 再执行一步
    cosmo_simulation_step(sim);
    t = cosmo_simulation_get_time(sim);
    REQUIRE(fabs(t - 0.2) < 1e-12);

    // 终止
    ret = cosmo_simulation_terminate(sim);
    REQUIRE_EQ(ret, 0);

    // 销毁
    cosmo_simulation_destroy(sim);
    cosmo_simulation_structure_destroy(ss);
}

TEST_CASE(c_api_step_until_and_for) {
    cosmo_simulation_structure_t* ss = cosmo_simulation_structure_create();
    cosmo_simulation_structure_add_model(ss, "m1", "model.fmu");

    cosmo_simulation_t* sim = cosmo_simulation_create_from_structure(ss, 0.1);
    cosmo_simulation_initialize(sim, 0.0);

    cosmo_simulation_step_until(sim, 0.5);
    REQUIRE(fabs(cosmo_simulation_get_time(sim) - 0.5) < 1e-12);

    cosmo_simulation_step_for(sim, 0.3);
    REQUIRE(fabs(cosmo_simulation_get_time(sim) - 0.8) < 1e-12);

    cosmo_simulation_terminate(sim);
    cosmo_simulation_destroy(sim);
    cosmo_simulation_structure_destroy(ss);
}

TEST_CASE(c_api_null_sim_operations) {
    // 传入nullptr不应崩溃
    cosmo_simulation_initialize(nullptr, 0.0);
    cosmo_simulation_step(nullptr);
    cosmo_simulation_step_until(nullptr, 1.0);
    cosmo_simulation_step_for(nullptr, 1.0);
    cosmo_simulation_terminate(nullptr);
    cosmo_simulation_reset(nullptr);
    REQUIRE(fabs(cosmo_simulation_get_time(nullptr) - 0.0) < 1e-12);
    cosmo_simulation_destroy(nullptr);
}

// ============================================================================
// 变量访问测试
// ============================================================================

TEST_CASE(c_api_get_set_real) {
    cosmo_simulation_structure_t* ss = cosmo_simulation_structure_create();
    cosmo_simulation_structure_add_model(ss, "m1", "model.fmu");
    cosmo_simulation_structure_register_variable(ss, "m1", "real", "x");

    cosmo_simulation_t* sim = cosmo_simulation_create_from_structure(ss, 0.01);
    cosmo_simulation_initialize(sim, 0.0);

    int ret = cosmo_simulation_set_real(sim, "m1::x", 42.5);
    REQUIRE_EQ(ret, 0);

    double value = 0.0;
    ret = cosmo_simulation_get_real(sim, "m1::x", &value);
    REQUIRE_EQ(ret, 0);
    REQUIRE(fabs(value - 42.5) < 1e-12);

    cosmo_simulation_terminate(sim);
    cosmo_simulation_destroy(sim);
    cosmo_simulation_structure_destroy(ss);
}

TEST_CASE(c_api_get_set_integer) {
    cosmo_simulation_structure_t* ss = cosmo_simulation_structure_create();
    cosmo_simulation_structure_add_model(ss, "m1", "model.fmu");
    cosmo_simulation_structure_register_variable(ss, "m1", "integer", "count");

    cosmo_simulation_t* sim = cosmo_simulation_create_from_structure(ss, 0.01);
    cosmo_simulation_initialize(sim, 0.0);

    REQUIRE_EQ(cosmo_simulation_set_integer(sim, "m1::count", 99), 0);

    int value = 0;
    REQUIRE_EQ(cosmo_simulation_get_integer(sim, "m1::count", &value), 0);
    REQUIRE_EQ(value, 99);

    cosmo_simulation_terminate(sim);
    cosmo_simulation_destroy(sim);
    cosmo_simulation_structure_destroy(ss);
}

TEST_CASE(c_api_get_set_boolean) {
    cosmo_simulation_structure_t* ss = cosmo_simulation_structure_create();
    cosmo_simulation_structure_add_model(ss, "m1", "model.fmu");
    cosmo_simulation_structure_register_variable(ss, "m1", "boolean", "flag");

    cosmo_simulation_t* sim = cosmo_simulation_create_from_structure(ss, 0.01);
    cosmo_simulation_initialize(sim, 0.0);

    REQUIRE_EQ(cosmo_simulation_set_boolean(sim, "m1::flag", 1), 0);

    int value = 0;
    REQUIRE_EQ(cosmo_simulation_get_boolean(sim, "m1::flag", &value), 0);
    REQUIRE_EQ(value, 1);

    cosmo_simulation_terminate(sim);
    cosmo_simulation_destroy(sim);
    cosmo_simulation_structure_destroy(ss);
}

TEST_CASE(c_api_variable_null_checks) {
    // nullptr参数检查不应崩溃
    double d = 0.0;
    int i = 0;
    cosmo_simulation_get_real(nullptr, "x", &d);
    cosmo_simulation_set_real(nullptr, "x", 1.0);
    cosmo_simulation_get_integer(nullptr, "x", &i);
    cosmo_simulation_set_integer(nullptr, "x", 1);
    cosmo_simulation_get_boolean(nullptr, "x", &i);
    cosmo_simulation_set_boolean(nullptr, "x", 1);
}

TEST_CASE(c_api_variable_invalid_identifier) {
    cosmo_simulation_structure_t* ss = cosmo_simulation_structure_create();
    cosmo_simulation_structure_add_model(ss, "m1", "model.fmu");

    cosmo_simulation_t* sim = cosmo_simulation_create_from_structure(ss, 0.01);
    cosmo_simulation_initialize(sim, 0.0);

    double d = 0.0;
    REQUIRE_EQ(cosmo_simulation_get_real(sim, "no_separator", &d), cosmo::fmi::fmi_error);
    REQUIRE_EQ(cosmo_simulation_get_real(sim, "nonexistent::x", &d), cosmo::fmi::fmi_error);

    cosmo_simulation_terminate(sim);
    cosmo_simulation_destroy(sim);
    cosmo_simulation_structure_destroy(ss);
}

// ============================================================================
// 监听器测试
// ============================================================================

TEST_CASE(c_api_listener_create_destroy) {
    cosmo_event_callbacks cb = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    cosmo_observer_t* obs = cosmo_observer_new(cb);
    REQUIRE(obs != nullptr);
    cosmo_observer_free(obs);
    cosmo_observer_free(nullptr);
}

TEST_CASE(c_api_listener_callbacks) {
    static int setup_called = 0;
    static int post_step_called = 0;
    static double last_step_time = -999;

    cosmo_event_callbacks cb = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    cb.on_setup = [](double) { setup_called++; };
    cb.on_post_step = [](double t, double) { post_step_called++; last_step_time = t; };

    cosmo_simulation_structure_t* ss = cosmo_simulation_structure_create();
    cosmo_simulation_structure_add_model(ss, "m1", "model.fmu");

    cosmo_simulation_t* sim = cosmo_simulation_create_from_structure(ss, 0.1);

    cosmo_observer_t* obs = cosmo_observer_new(cb);
    cosmo_simulation_add_listener(sim, "cb_obs", obs);

    cosmo_simulation_initialize(sim, 0.0);
    REQUIRE_EQ(setup_called, 1);

    cosmo_simulation_step(sim);
    REQUIRE_EQ(post_step_called, 1);
    REQUIRE(fabs(last_step_time - 0.1) < 1e-12);

    cosmo_simulation_terminate(sim);
    cosmo_simulation_destroy(sim);
    cosmo_simulation_structure_destroy(ss);

    setup_called = 0;
    post_step_called = 0;
}

// ============================================================================
// CSV写入器测试
// ============================================================================

TEST_CASE(c_api_csv_writer_create) {
    cosmo_observer_t* writer = cosmo_csv_new("/tmp/test_cosmo.csv");
    REQUIRE(writer != nullptr);
    cosmo_observer_free(writer);
}

TEST_CASE(c_api_csv_writer_null_filename) {
    cosmo_observer_t* writer = cosmo_csv_new(nullptr);
    REQUIRE(writer == nullptr);
}

// ============================================================================
// 场景测试
// ============================================================================

TEST_CASE(c_api_scenario_create_destroy) {
    cosmo_scenario_t* scen = cosmo_scenario_create();
    REQUIRE(scen != nullptr);
    cosmo_scenario_destroy(scen);
    cosmo_scenario_destroy(nullptr);
}

TEST_CASE(c_api_scenario_add_actions) {
    cosmo_scenario_t* scen = cosmo_scenario_create();
    REQUIRE(scen != nullptr);

    cosmo_scenario_add_real_action(scen, 1.0, "m1::x", 5.0);
    cosmo_scenario_add_integer_action(scen, 2.0, "m1::count", 10);
    cosmo_scenario_add_boolean_action(scen, 3.0, "m1::flag", 1);
    cosmo_scenario_add_string_action(scen, 4.0, "m1::name", "hello");

    // nullptr不应崩溃
    cosmo_scenario_add_real_action(nullptr, 0.0, "x", 1.0);
    cosmo_scenario_add_integer_action(nullptr, 0.0, "x", 1);
    cosmo_scenario_add_boolean_action(nullptr, 0.0, "x", 1);
    cosmo_scenario_add_string_action(nullptr, 0.0, "x", "v");
    cosmo_scenario_add_string_action(scen, 0.0, nullptr, "v");
    cosmo_scenario_add_string_action(scen, 0.0, "x", nullptr);

    cosmo_scenario_destroy(scen);
}

TEST_CASE(c_api_scenario_load_nonexistent_file) {
    cosmo_scenario_t* scen = cosmo_scenario_load_from_file("/nonexistent/path.xml");
    REQUIRE(scen == nullptr);
    REQUIRE(std::string(cosmo_last_error_msg()).length() > 0);
    cosmo_clear_error();
}

TEST_CASE(c_api_scenario_null_path) {
    cosmo_scenario_t* scen = cosmo_scenario_load_from_file(nullptr);
    REQUIRE(scen == nullptr);
}

// ============================================================================
// 仿真运行器测试
// ============================================================================

TEST_CASE(c_api_runner_null_sim) {
    cosmo_simulation_runner_t* runner = cosmo_simulation_runner_create(nullptr);
    REQUIRE(runner == nullptr);
}

TEST_CASE(c_api_runner_create_destroy) {
    cosmo_simulation_structure_t* ss = cosmo_simulation_structure_create();
    cosmo_simulation_structure_add_model(ss, "m1", "model.fmu");
    cosmo_simulation_t* sim = cosmo_simulation_create_from_structure(ss, 0.01);
    cosmo_simulation_initialize(sim, 0.0);

    cosmo_simulation_runner_t* runner = cosmo_simulation_runner_create(sim);
    REQUIRE(runner != nullptr);

    // 空指针操作不应崩溃
    cosmo_simulation_runner_start(nullptr);
    cosmo_simulation_runner_stop(nullptr);
    cosmo_simulation_runner_pause(nullptr);
    cosmo_simulation_runner_resume(nullptr);
    REQUIRE_EQ(cosmo_simulation_runner_is_running(nullptr), 0);
    cosmo_simulation_runner_set_real_time_factor(nullptr, 1.0);
    REQUIRE(fabs(cosmo_simulation_runner_get_real_time_factor(nullptr) - (-1.0)) < 1e-12);

    cosmo_simulation_terminate(sim);
    cosmo_simulation_runner_destroy(runner);
    cosmo_simulation_destroy(sim);
    cosmo_simulation_structure_destroy(ss);
}

// ============================================================================
// SSP加载测试
// ============================================================================

TEST_CASE(c_api_load_ssp_not_implemented) {
    cosmo_simulation_structure_t* ss = cosmo_load_ssp(nullptr);
    REQUIRE(ss == nullptr);

    ss = cosmo_load_ssp("nonexistent.ssp");
    REQUIRE(ss == nullptr);
    cosmo_clear_error();
}

// ============================================================================
// 重置测试
// ============================================================================

TEST_CASE(c_api_simulation_reset) {
    cosmo_simulation_structure_t* ss = cosmo_simulation_structure_create();
    cosmo_simulation_structure_add_model(ss, "m1", "model.fmu");

    cosmo_simulation_t* sim = cosmo_simulation_create_from_structure(ss, 0.1);
    cosmo_simulation_initialize(sim, 0.0);
    cosmo_simulation_step(sim);
    cosmo_simulation_step(sim);

    int ret = cosmo_simulation_reset(sim);
    REQUIRE_EQ(ret, 0);

    // 重置后可以重新初始化
    cosmo_simulation_initialize(sim, 0.0);
    REQUIRE(fabs(cosmo_simulation_get_time(sim)) < 1e-12);

    cosmo_simulation_terminate(sim);
    cosmo_simulation_destroy(sim);
    cosmo_simulation_structure_destroy(ss);
}

// ============================================================================
// 多模型连接数据传输测试
// ============================================================================

TEST_CASE(c_api_multi_model_connection) {
    cosmo_simulation_structure_t* ss = cosmo_simulation_structure_create();
    cosmo_simulation_structure_add_model(ss, "sender", "a.fmu");
    cosmo_simulation_structure_add_model(ss, "receiver", "b.fmu");
    cosmo_simulation_structure_make_real_connection(ss, "sender::output", "receiver::input");

    cosmo_simulation_t* sim = cosmo_simulation_create_from_structure(ss, 0.01);
    cosmo_simulation_initialize(sim, 0.0);

    // 设置发送端的输出值
    double test_val = 123.456;
    cosmo_simulation_set_real(sim, "sender::output", test_val);

    // 执行步进触发连接传输
    cosmo_simulation_step(sim);

    // 检查接收端是否收到值
    double received = 0.0;
    int ret = cosmo_simulation_get_real(sim, "receiver::input", &received);
    REQUIRE_EQ(ret, 0);
    REQUIRE(fabs(received - test_val) < 1e-12);

    cosmo_simulation_terminate(sim);
    cosmo_simulation_destroy(sim);
    cosmo_simulation_structure_destroy(ss);
}

int main()
{
    return RUN_ALL_TESTS();
}
