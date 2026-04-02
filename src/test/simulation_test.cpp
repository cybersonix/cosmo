/**
 * @file simulation_test.cpp
 * @brief 仿真引擎核心功能测试
 *
 * 全面测试仿真引擎的生命周期管理、模型实例协调、
 * 监听器通知、变量连接数据传输和变量访问功能。
 */

#include "cosmo/test/test_framework.hpp"
#include "cosmo/simulation/simulation.hpp"
#include "cosmo/simulation/simulation_structure.hpp"
#include "cosmo/simulation/model_instance.hpp"
#include "cosmo/simulation/simulation_listener.hpp"
#include "cosmo/simulation/simulation_types.hpp"
#include "cosmo/simulation/scenario.hpp"
#include "cosmo/fmi/model_description.hpp"
#include "cosmo/fmi/fmi_types.hpp"
#include "cosmo/logger/logger.hpp"

#include <cmath>
#include <memory>

// 辅助：创建带变量的模型描述
static std::unique_ptr<cosmo::fmi::model_description> make_model_desc(
    const std::string& name,
    std::vector<std::pair<std::string, cosmo::fmi::value_reference>> real_vars
        = std::vector<std::pair<std::string, cosmo::fmi::value_reference>>(),
    std::vector<std::pair<std::string, cosmo::fmi::value_reference>> int_vars
        = std::vector<std::pair<std::string, cosmo::fmi::value_reference>>(),
    std::vector<std::pair<std::string, cosmo::fmi::value_reference>> bool_vars
        = std::vector<std::pair<std::string, cosmo::fmi::value_reference>>(),
    std::vector<std::pair<std::string, cosmo::fmi::value_reference>> str_vars
        = std::vector<std::pair<std::string, cosmo::fmi::value_reference>>())
{
    auto desc = cosmo::fmi::create_model_description();
    desc->set_model_name(name);

    for (const auto& p : real_vars) {
        cosmo::fmi::variable_description var;
        var.name = p.first;
        var.vr = p.second;
        var.type = cosmo::fmi::variable_type::real;
        var.default_real = 0.0;
        var.causality_type = cosmo::fmi::causality::local;
        desc->add_variable(var);
    }
    for (const auto& p : int_vars) {
        cosmo::fmi::variable_description var;
        var.name = p.first;
        var.vr = p.second;
        var.type = cosmo::fmi::variable_type::integer;
        var.default_integer = 0;
        var.causality_type = cosmo::fmi::causality::local;
        desc->add_variable(var);
    }
    for (const auto& p : bool_vars) {
        cosmo::fmi::variable_description var;
        var.name = p.first;
        var.vr = p.second;
        var.type = cosmo::fmi::variable_type::boolean;
        var.default_boolean = false;
        var.causality_type = cosmo::fmi::causality::local;
        desc->add_variable(var);
    }
    for (const auto& p : str_vars) {
        cosmo::fmi::variable_description var;
        var.name = p.first;
        var.vr = p.second;
        var.type = cosmo::fmi::variable_type::string;
        var.default_string = "";
        var.causality_type = cosmo::fmi::causality::local;
        desc->add_variable(var);
    }
    return desc;
}

// ============================================================================
// 算法测试
// ============================================================================

TEST_CASE(algorithm_construction) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.01));
    REQUIRE_EQ(algo->get_step_size(), 0.01);
}

TEST_CASE(algorithm_step_size_validation) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.001));
    REQUIRE(fabs(algo->get_step_size() - 0.001) < 1e-12);
}

// ============================================================================
// 仿真生命周期测试
// ============================================================================

TEST_CASE(simulation_create_and_destroy) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    auto sim = std::unique_ptr<cosmo::simulation::simulation>(
        new cosmo::simulation::simulation(std::move(algo)));
    REQUIRE(!sim->is_initialized());
    REQUIRE(!sim->is_terminated());
}

TEST_CASE(simulation_initialize_without_models) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));

    cosmo::fmi::fmi_status status = sim.initialize(0.0);
    REQUIRE_EQ(status, cosmo::fmi::fmi_ok);
    REQUIRE(sim.is_initialized());
    REQUIRE_EQ(sim.get_current_time(), 0.0);
    REQUIRE_EQ(sim.get_step_count(), 0u);
}

TEST_CASE(simulation_double_initialize_fails) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));

    sim.initialize(0.0);
    cosmo::fmi::fmi_status status = sim.initialize(0.0);
    REQUIRE_EQ(status, cosmo::fmi::fmi_warning);
}

TEST_CASE(simulation_step_without_initialize_fails) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));

    cosmo::fmi::fmi_status status = sim.step();
    REQUIRE_EQ(status, cosmo::fmi::fmi_error);
}

TEST_CASE(simulation_step_advances_time) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));
    sim.initialize(0.0);

    sim.step();
    REQUIRE(fabs(sim.get_current_time() - 0.1) < 1e-12);
    REQUIRE_EQ(sim.get_step_count(), 1u);

    sim.step();
    REQUIRE(fabs(sim.get_current_time() - 0.2) < 1e-12);
    REQUIRE_EQ(sim.get_step_count(), 2u);
}

TEST_CASE(simulation_step_until) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));
    sim.initialize(0.0);

    sim.step_until(0.5);
    REQUIRE(fabs(sim.get_current_time() - 0.5) < 1e-12);
    REQUIRE_EQ(sim.get_step_count(), 5u);
}

TEST_CASE(simulation_step_for) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));
    sim.initialize(0.0);

    sim.step_for(0.3);
    REQUIRE(fabs(sim.get_current_time() - 0.3) < 1e-12);
    REQUIRE_EQ(sim.get_step_count(), 3u);
}

TEST_CASE(simulation_terminate) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));
    sim.initialize(0.0);
    sim.step();

    cosmo::fmi::fmi_status status = sim.terminate();
    REQUIRE_EQ(status, cosmo::fmi::fmi_ok);
    REQUIRE(sim.is_terminated());
}

TEST_CASE(simulation_terminate_without_init_warns) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));

    cosmo::fmi::fmi_status status = sim.terminate();
    REQUIRE_EQ(status, cosmo::fmi::fmi_warning);
}

TEST_CASE(simulation_step_after_terminate_fails) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));
    sim.initialize(0.0);
    sim.terminate();

    cosmo::fmi::fmi_status status = sim.step();
    REQUIRE_EQ(status, cosmo::fmi::fmi_error);
}

TEST_CASE(simulation_reset) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));
    sim.initialize(0.0);
    sim.step();
    sim.step();
    REQUIRE_EQ(sim.get_step_count(), 2u);

    sim.reset();
    REQUIRE(!sim.is_initialized());
    REQUIRE(!sim.is_terminated());
    REQUIRE_EQ(sim.get_step_count(), 0u);
    REQUIRE_EQ(sim.get_current_time(), 0.0);

    // 重置后可以重新初始化
    sim.initialize(0.0);
    sim.step();
    REQUIRE_EQ(sim.get_step_count(), 1u);
}

TEST_CASE(simulation_different_step_sizes) {
    // 步长0.05, 跑0.2秒 = 4步
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.05));
    cosmo::simulation::simulation sim(std::move(algo));
    sim.initialize(0.0);
    sim.step_until(0.2);
    REQUIRE_EQ(sim.get_step_count(), 4u);
    REQUIRE(fabs(sim.get_current_time() - 0.2) < 1e-12);
}

// ============================================================================
// 模型实例管理测试
// ============================================================================

TEST_CASE(add_and_get_model_instance) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));

    auto desc = make_model_desc("test_model", {{"x", 0}, {"y", 1}});
    auto inst = cosmo::simulation::create_model_instance("inst1", std::move(desc));
    sim.add_model_instance(std::move(inst));

    REQUIRE(sim.get_model_instance("inst1") != nullptr);
    REQUIRE(sim.get_model_instance("nonexistent") == nullptr);
    REQUIRE_EQ(sim.get_instances().size(), 1u);
}

TEST_CASE(add_null_model_instance_ignored) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));

    sim.add_model_instance(nullptr);
    REQUIRE_EQ(sim.get_instances().size(), 0u);
}

TEST_CASE(duplicate_model_name_ignored) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));

    auto desc1 = make_model_desc("m", {{"x", 0}});
    auto inst1 = cosmo::simulation::create_model_instance("dup", std::move(desc1));
    sim.add_model_instance(std::move(inst1));

    auto desc2 = make_model_desc("m", {{"y", 0}});
    auto inst2 = cosmo::simulation::create_model_instance("dup", std::move(desc2));
    sim.add_model_instance(std::move(inst2));

    REQUIRE_EQ(sim.get_instances().size(), 1u);
}

TEST_CASE(simulation_step_with_model) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));

    auto desc = make_model_desc("test", {{"x", 0}});
    auto inst = cosmo::simulation::create_model_instance("m1", std::move(desc));
    sim.add_model_instance(std::move(inst));

    cosmo::fmi::fmi_status status = sim.initialize(0.0);
    REQUIRE_EQ(status, cosmo::fmi::fmi_ok);

    status = sim.step();
    REQUIRE_EQ(status, cosmo::fmi::fmi_ok);
    REQUIRE_EQ(sim.get_step_count(), 1u);
}

// ============================================================================
// 变量访问测试
// ============================================================================

TEST_CASE(get_set_real_variable) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));

    auto desc = make_model_desc("test", {{"x", 0}, {"y", 1}});
    auto inst = cosmo::simulation::create_model_instance("m1", std::move(desc));
    sim.add_model_instance(std::move(inst));
    sim.initialize(0.0);

    // 设置值
    cosmo::fmi::fmi_status status = sim.set_real("m1::x", 3.14);
    REQUIRE_EQ(status, cosmo::fmi::fmi_ok);

    // 读取值
    cosmo::fmi::fmi_real value = 0.0;
    status = sim.get_real("m1::x", value);
    REQUIRE_EQ(status, cosmo::fmi::fmi_ok);
    REQUIRE(fabs(value - 3.14) < 1e-12);
}

TEST_CASE(get_variable_invalid_identifier) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));

    auto desc = make_model_desc("test", {{"x", 0}});
    auto inst = cosmo::simulation::create_model_instance("m1", std::move(desc));
    sim.add_model_instance(std::move(inst));
    sim.initialize(0.0);

    cosmo::fmi::fmi_real value = 0.0;
    REQUIRE_EQ(sim.get_real("invalid_no_separator", value), cosmo::fmi::fmi_error);
    REQUIRE_EQ(sim.get_real("nonexistent::x", value), cosmo::fmi::fmi_error);
}

TEST_CASE(get_set_integer_variable) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));

    auto desc = make_model_desc("test", {}, {{"count", 0}});
    auto inst = cosmo::simulation::create_model_instance("m1", std::move(desc));
    sim.add_model_instance(std::move(inst));
    sim.initialize(0.0);

    REQUIRE_EQ(sim.set_integer("m1::count", 42), cosmo::fmi::fmi_ok);

    cosmo::fmi::fmi_integer value = 0;
    REQUIRE_EQ(sim.get_integer("m1::count", value), cosmo::fmi::fmi_ok);
    REQUIRE_EQ(value, 42);
}

TEST_CASE(get_set_boolean_variable) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));

    std::vector<std::pair<std::string, cosmo::fmi::value_reference>> bool_vars;
    bool_vars.push_back(std::make_pair(std::string("flag"), cosmo::fmi::value_reference(0)));
    auto desc = make_model_desc("test", std::vector<std::pair<std::string, cosmo::fmi::value_reference>>(),
        std::vector<std::pair<std::string, cosmo::fmi::value_reference>>(), bool_vars);
    auto inst = cosmo::simulation::create_model_instance("m1", std::move(desc));
    sim.add_model_instance(std::move(inst));
    sim.initialize(0.0);

    REQUIRE_EQ(sim.set_boolean("m1::flag", true), cosmo::fmi::fmi_ok);

    cosmo::fmi::fmi_boolean value = false;
    REQUIRE_EQ(sim.get_boolean("m1::flag", value), cosmo::fmi::fmi_ok);
    REQUIRE_EQ(value, true);
}

// ============================================================================
// 监听器测试
// ============================================================================

namespace {

// 测试用监听器：记录回调调用次数
class counting_listener : public cosmo::simulation::simulation_listener
{
public:
    int pre_init_count = 0;
    int post_init_count = 0;
    int pre_step_count = 0;
    int post_step_count = 0;
    double last_pre_step_time = -999;
    double last_post_step_time = -999;
    double last_post_step_size = -999;
    int pre_terminate_count = 0;
    int post_terminate_count = 0;
    int reset_count = 0;

    std::string get_name() const override { return "counting_listener"; }

    void on_pre_init(double) override { pre_init_count++; }
    void on_post_init(double) override { post_init_count++; }
    void on_pre_step(double t, double /*s*/) override {
        pre_step_count++;
        last_pre_step_time = t;
    }
    void on_post_step(double t, double s) override {
        post_step_count++;
        last_post_step_time = t;
        last_post_step_size = s;
    }
    void on_pre_terminate() override { pre_terminate_count++; }
    void on_post_terminate() override { post_terminate_count++; }
    void on_reset() override { reset_count++; }
};

} // anonymous namespace

TEST_CASE(listener_receives_init_notifications) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));

    auto listener = std::unique_ptr<counting_listener>(new counting_listener());
    counting_listener* ptr = listener.get();
    sim.add_listener(std::move(listener));

    sim.initialize(1.0);
    REQUIRE_EQ(ptr->pre_init_count, 1);
    REQUIRE_EQ(ptr->post_init_count, 1);
}

TEST_CASE(listener_receives_step_notifications) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));

    auto listener = std::unique_ptr<counting_listener>(new counting_listener());
    counting_listener* ptr = listener.get();
    sim.add_listener(std::move(listener));

    sim.initialize(0.0);
    sim.step();
    REQUIRE_EQ(ptr->pre_step_count, 1);
    REQUIRE_EQ(ptr->post_step_count, 1);
    REQUIRE(fabs(ptr->last_post_step_time - 0.1) < 1e-12);

    sim.step();
    REQUIRE_EQ(ptr->pre_step_count, 2);
    REQUIRE_EQ(ptr->post_step_count, 2);
    REQUIRE(fabs(ptr->last_post_step_time - 0.2) < 1e-12);
}

TEST_CASE(listener_receives_terminate_notifications) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));

    auto listener = std::unique_ptr<counting_listener>(new counting_listener());
    counting_listener* ptr = listener.get();
    sim.add_listener(std::move(listener));

    sim.initialize(0.0);
    sim.terminate();
    REQUIRE_EQ(ptr->pre_terminate_count, 1);
    REQUIRE_EQ(ptr->post_terminate_count, 1);
}

TEST_CASE(listener_receives_reset_notification) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));

    auto listener = std::unique_ptr<counting_listener>(new counting_listener());
    counting_listener* ptr = listener.get();
    sim.add_listener(std::move(listener));

    sim.initialize(0.0);
    sim.step();
    sim.reset();
    REQUIRE_EQ(ptr->reset_count, 1);
}

TEST_CASE(remove_listener) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));

    auto listener = std::unique_ptr<counting_listener>(new counting_listener());
    sim.add_listener(std::move(listener));

    sim.initialize(0.0);
    REQUIRE_EQ(sim.remove_listener("counting_listener"), true);
    REQUIRE_EQ(sim.remove_listener("nonexistent"), false);

    // 移除后不再收到通知
    sim.step();
    // 没有崩溃即为成功
}

TEST_CASE(multiple_listeners) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));

    auto l1 = std::unique_ptr<counting_listener>(new counting_listener());
    auto l1_raw = l1.get();

    class listener2 : public counting_listener {
        std::string get_name() const override { return "listener_2"; }
    };
    auto l2 = std::unique_ptr<counting_listener>(new listener2());
    auto l2_raw = l2.get();

    sim.add_listener(std::move(l1));
    sim.add_listener(std::move(l2));

    sim.initialize(0.0);
    sim.step();
    sim.terminate();

    REQUIRE_EQ(l1_raw->pre_init_count, 1);
    REQUIRE_EQ(l2_raw->pre_init_count, 1);
    REQUIRE_EQ(l1_raw->post_step_count, 1);
    REQUIRE_EQ(l2_raw->post_step_count, 1);
    REQUIRE_EQ(l1_raw->pre_terminate_count, 1);
    REQUIRE_EQ(l2_raw->pre_terminate_count, 1);
}

TEST_CASE(add_null_listener_ignored) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));

    sim.add_listener(nullptr);
    sim.initialize(0.0);
    sim.step();
    // 无崩溃即为成功
}

// ============================================================================
// 连接数据传输测试
// ============================================================================

TEST_CASE(connection_data_transfer) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));

    std::vector<std::pair<std::string, cosmo::fmi::value_reference>> src_vars;
    src_vars.push_back(std::make_pair(std::string("out"), cosmo::fmi::value_reference(0)));
    auto src_desc = make_model_desc("src", src_vars);
    auto src_inst = cosmo::simulation::create_model_instance("source", std::move(src_desc));

    std::vector<std::pair<std::string, cosmo::fmi::value_reference>> tgt_vars;
    tgt_vars.push_back(std::make_pair(std::string("in"), cosmo::fmi::value_reference(0)));
    auto tgt_desc = make_model_desc("tgt", tgt_vars);
    auto tgt_inst = cosmo::simulation::create_model_instance("target", std::move(tgt_desc));

    sim.add_model_instance(std::move(src_inst));
    sim.add_model_instance(std::move(tgt_inst));

    // 设置源变量值
    sim.set_real("source::out", 7.5);

    // 建立连接：source::out -> target::in
    sim.add_connection(cosmo::simulation::connection_binding("source", "out", "target", "in"));

    sim.initialize(0.0);

    // 步进后数据应通过连接传输
    sim.step();

    cosmo::fmi::fmi_real target_val = 0.0;
    sim.get_real("target::in", target_val);
    REQUIRE(fabs(target_val - 7.5) < 1e-12);
}

TEST_CASE(connection_with_modifier) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));

    std::vector<std::pair<std::string, cosmo::fmi::value_reference>> src_vars;
    src_vars.push_back(std::make_pair(std::string("out"), cosmo::fmi::value_reference(0)));
    auto src_desc = make_model_desc("src", src_vars);

    std::vector<std::pair<std::string, cosmo::fmi::value_reference>> tgt_vars;
    tgt_vars.push_back(std::make_pair(std::string("in"), cosmo::fmi::value_reference(0)));
    auto tgt_desc = make_model_desc("tgt", tgt_vars);

    sim.add_model_instance(cosmo::simulation::create_model_instance("source", std::move(src_desc)));
    sim.add_model_instance(cosmo::simulation::create_model_instance("target", std::move(tgt_desc)));

    sim.set_real("source::out", 10.0);

    // 乘以2的修饰器
    sim.add_connection(cosmo::simulation::connection_binding(
        "source", "out", "target", "in",
        [](cosmo::fmi::fmi_real v) { return v * 2.0; }
    ));

    sim.initialize(0.0);
    sim.step();

    cosmo::fmi::fmi_real target_val = 0.0;
    sim.get_real("target::in", target_val);
    REQUIRE(fabs(target_val - 20.0) < 1e-12);
}

TEST_CASE(connection_missing_model_ignored) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));

    sim.add_connection(cosmo::simulation::connection_binding(
        "nonexistent", "x", "also_nonexistent", "y"
    ));

    sim.initialize(0.0);
    cosmo::fmi::fmi_status status = sim.step();
    REQUIRE_EQ(status, cosmo::fmi::fmi_ok);
}

// ============================================================================
// 多步仿真完整流程测试
// ============================================================================

TEST_CASE(full_simulation_workflow) {
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.01));
    cosmo::simulation::simulation sim(std::move(algo));

    std::vector<std::pair<std::string, cosmo::fmi::value_reference>> vars;
    vars.push_back(std::make_pair(std::string("pos"), cosmo::fmi::value_reference(0)));
    vars.push_back(std::make_pair(std::string("vel"), cosmo::fmi::value_reference(1)));
    auto desc = make_model_desc("oscillator", vars);
    sim.add_model_instance(cosmo::simulation::create_model_instance("osc", std::move(desc)));

    auto listener = std::unique_ptr<counting_listener>(new counting_listener());
    counting_listener* lptr = listener.get();
    sim.add_listener(std::move(listener));

    // 初始化
    REQUIRE_EQ(sim.initialize(0.0), cosmo::fmi::fmi_ok);
    REQUIRE_EQ(lptr->pre_init_count, 1);

    // 设置初始条件
    sim.set_real("osc::pos", 1.0);
    sim.set_real("osc::vel", 0.0);

    // 运行100步
    for (int i = 0; i < 100; i++) {
        REQUIRE_EQ(sim.step(), cosmo::fmi::fmi_ok);
    }

    REQUIRE_EQ(sim.get_step_count(), 100u);
    REQUIRE(fabs(sim.get_current_time() - 1.0) < 1e-12);
    REQUIRE_EQ(lptr->post_step_count, 100);

    // 终止
    REQUIRE_EQ(sim.terminate(), cosmo::fmi::fmi_ok);
    REQUIRE(sim.is_terminated());
}

TEST_CASE(step_until_exact_alignment) {
    // 步长0.1, 运行到1.0 = 恰好10步
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));
    sim.initialize(0.0);
    sim.step_until(1.0);
    REQUIRE_EQ(sim.get_step_count(), 10u);
}

TEST_CASE(step_until_non_exact_alignment) {
    // 步长0.1, 运行到0.95 = 需要10步（最后一步到1.0但目标0.95）
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    cosmo::simulation::simulation sim(std::move(algo));
    sim.initialize(0.0);
    sim.step_until(0.95);
    // 0.0 -> 0.1 -> 0.2 -> ... -> 0.9 -> 1.0 (但 1.0 > 0.95，所以9步就到0.9时0.9 < 0.95，继续到1.0)
    REQUIRE_EQ(sim.get_step_count(), 10u);
}

int main()
{
    return RUN_ALL_TESTS();
}
