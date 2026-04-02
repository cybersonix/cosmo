/**
 * @file simulation_structure_test.cpp
 * @brief 仿真结构配置功能全面测试
 *
 * 测试仿真结构的模型管理、连接创建、
 * 参数集管理、工厂函数和加载功能。
 */

#include "cosmo/test/test_framework.hpp"
#include "cosmo/simulation/simulation_structure.hpp"
#include "cosmo/simulation/simulation.hpp"
#include "cosmo/simulation/simulation_types.hpp"
#include "cosmo/simulation/model_instance.hpp"
#include "cosmo/fmi/model_description.hpp"
#include "cosmo/logger/logger.hpp"

#include <memory>
#include <cmath>

namespace {

auto create_algo(double step = 0.1) -> std::unique_ptr<cosmo::simulation::algorithm>
{
    return std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(step));
}

} // anonymous namespace

// ============================================================================
// 基本创建和销毁
// ============================================================================

TEST_CASE(structure_create_and_destroy) {
    auto ss = cosmo::simulation::create_simulation_structure();
    REQUIRE(ss != nullptr);
    REQUIRE_EQ(ss->get_model_count(), 0u);
    REQUIRE_EQ(ss->get_connection_count(), 0u);
    REQUIRE_EQ(ss->get_parameter_set_count(), 0u);
}

TEST_CASE(structure_factory_function) {
    auto ss = cosmo::simulation::create_simulation_structure();
    REQUIRE(ss.get() != nullptr);
}

// ============================================================================
// 模型管理测试
// ============================================================================

TEST_CASE(add_single_model) {
    auto ss = cosmo::simulation::create_simulation_structure();
    ss->add_model("m1", "/path/to/model.fmu");

    REQUIRE_EQ(ss->get_model_count(), 1u);
    REQUIRE(ss->has_model("m1"));
    REQUIRE_FALSE(ss->has_model("nonexistent"));
    REQUIRE_EQ(ss->get_model_fmu_path("m1"), std::string("/path/to/model.fmu"));
}

TEST_CASE(add_multiple_models) {
    auto ss = cosmo::simulation::create_simulation_structure();
    ss->add_model("m1", "a.fmu");
    ss->add_model("m2", "b.fmu");
    ss->add_model("m3", "c.fmu");

    REQUIRE_EQ(ss->get_model_count(), 3u);
    REQUIRE(ss->has_model("m1"));
    REQUIRE(ss->has_model("m2"));
    REQUIRE(ss->has_model("m3"));
}

TEST_CASE(duplicate_model_name_ignored) {
    auto ss = cosmo::simulation::create_simulation_structure();
    ss->add_model("dup", "first.fmu");
    ss->add_model("dup", "second.fmu");

    REQUIRE_EQ(ss->get_model_count(), 1u);
    REQUIRE_EQ(ss->get_model_fmu_path("dup"), std::string("first.fmu"));
}

TEST_CASE(add_model_with_parameters) {
    auto ss = cosmo::simulation::create_simulation_structure();

    std::map<std::string, cosmo::simulation::scalar_value> params;
    params["mass"] = cosmo::simulation::scalar_value(2.5);
    params["stiffness"] = cosmo::simulation::scalar_value(100.0);

    ss->add_model("spring", "spring.fmu", params);
    REQUIRE_EQ(ss->get_model_count(), 1u);
    REQUIRE(ss->has_model("spring"));
}

TEST_CASE(get_fmu_path_nonexistent) {
    auto ss = cosmo::simulation::create_simulation_structure();
    REQUIRE_EQ(ss->get_model_fmu_path("nonexistent"), std::string(""));
}

// ============================================================================
// 连接管理测试
// ============================================================================

TEST_CASE(make_real_connection) {
    auto ss = cosmo::simulation::create_simulation_structure();
    ss->add_model("src", "a.fmu");
    ss->add_model("tgt", "b.fmu");

    ss->make_connection("src::out", "tgt::in");
    REQUIRE_EQ(ss->get_connection_count(), 1u);
}

TEST_CASE(make_connection_with_modifier) {
    auto ss = cosmo::simulation::create_simulation_structure();
    ss->add_model("src", "a.fmu");
    ss->add_model("tgt", "b.fmu");

    auto modifier = [](cosmo::fmi::fmi_real v) { return v * 2.0; };
    ss->make_connection("src::out", "tgt::in", modifier);
    REQUIRE_EQ(ss->get_connection_count(), 1u);
}

TEST_CASE(make_multiple_connections) {
    auto ss = cosmo::simulation::create_simulation_structure();
    ss->add_model("m1", "a.fmu");
    ss->add_model("m2", "b.fmu");

    ss->make_connection("m1::x", "m2::x");
    ss->make_connection("m1::y", "m2::y");
    ss->make_connection("m2::out", "m1::feedback");
    REQUIRE_EQ(ss->get_connection_count(), 3u);
}

TEST_CASE(invalid_connection_identifier) {
    auto ss = cosmo::simulation::create_simulation_structure();
    ss->add_model("m1", "a.fmu");

    // 无分隔符的标识符应被忽略
    ss->make_connection("invalid", "also_invalid");
    REQUIRE_EQ(ss->get_connection_count(), 0u);
}

TEST_CASE(connection_parse_identifier) {
    auto ss = cosmo::simulation::create_simulation_structure();

    // 通过make_connection测试标识符解析
    ss->add_model("m1", "a.fmu");
    ss->add_model("m2", "b.fmu");
    ss->make_connection("m1::variable_x", "m2::variable_y");

    // 连接成功创建说明解析正确
    REQUIRE_EQ(ss->get_connection_count(), 1u);

    // 无分隔符的标识符应被忽略
    auto ss2 = cosmo::simulation::create_simulation_structure();
    ss2->add_model("m1", "a.fmu");
    ss2->make_connection("no_separator", "also_invalid");
    REQUIRE_EQ(ss2->get_connection_count(), 0u);
}

// ============================================================================
// 参数集测试
// ============================================================================

TEST_CASE(add_parameter_set) {
    auto ss = cosmo::simulation::create_simulation_structure();

    std::map<std::string, cosmo::simulation::scalar_value> params;
    params["p1"] = cosmo::simulation::scalar_value(1.0);
    params["p2"] = cosmo::simulation::scalar_value(2);

    ss->add_parameter_set("default", params);
    REQUIRE_EQ(ss->get_parameter_set_count(), 1u);
}

TEST_CASE(add_multiple_parameter_sets) {
    auto ss = cosmo::simulation::create_simulation_structure();

    std::map<std::string, cosmo::simulation::scalar_value> p1;
    p1["x"] = cosmo::simulation::scalar_value(1.0);
    ss->add_parameter_set("set1", p1);

    std::map<std::string, cosmo::simulation::scalar_value> p2;
    p2["y"] = cosmo::simulation::scalar_value(2.0);
    ss->add_parameter_set("set2", p2);

    REQUIRE_EQ(ss->get_parameter_set_count(), 2u);
}

// ============================================================================
// 加载测试
// ============================================================================

TEST_CASE(load_empty_structure) {
    auto ss = cosmo::simulation::create_simulation_structure();
    auto sim = ss->load(create_algo());

    REQUIRE(sim != nullptr);
    REQUIRE(!sim->is_initialized());
    REQUIRE_EQ(sim->get_instances().size(), 0u);
    REQUIRE_EQ(sim->get_connection_count(), 0u);
}

TEST_CASE(load_with_models) {
    auto ss = cosmo::simulation::create_simulation_structure();
    ss->add_model("m1", "model1.fmu");
    ss->add_model("m2", "model2.fmu");

    auto sim = ss->load(create_algo());

    REQUIRE(sim != nullptr);
    REQUIRE_EQ(sim->get_instances().size(), 2u);
    REQUIRE(sim->get_model_instance("m1") != nullptr);
    REQUIRE(sim->get_model_instance("m2") != nullptr);
}

TEST_CASE(load_with_connections) {
    auto ss = cosmo::simulation::create_simulation_structure();
    ss->add_model("src", "a.fmu");
    ss->add_model("tgt", "b.fmu");
    ss->make_connection("src::output", "tgt::input");

    auto sim = ss->load(create_algo());

    REQUIRE_EQ(sim->get_connection_count(), 1u);
    const auto& conns = sim->get_connections();
    REQUIRE_EQ(conns[0].source_model, std::string("src"));
    REQUIRE_EQ(conns[0].source_variable, std::string("output"));
    REQUIRE_EQ(conns[0].target_model, std::string("tgt"));
    REQUIRE_EQ(conns[0].target_variable, std::string("input"));
}

TEST_CASE(load_with_modifier_connection) {
    auto ss = cosmo::simulation::create_simulation_structure();
    ss->add_model("src", "a.fmu");
    ss->add_model("tgt", "b.fmu");

    ss->make_connection("src::out", "tgt::in",
        [](cosmo::fmi::fmi_real v) { return v + 1.0; });

    auto sim = ss->load(create_algo());
    const auto& conns = sim->get_connections();
    REQUIRE_EQ(conns.size(), 1u);
    REQUIRE(conns[0].modifier != nullptr);
}

TEST_CASE(load_and_run_simulation) {
    auto ss = cosmo::simulation::create_simulation_structure();
    ss->add_model("m1", "model.fmu");
    ss->make_connection("m1::out", "m1::in");

    auto sim = ss->load(create_algo(0.01));
    REQUIRE_EQ(sim->initialize(0.0), cosmo::fmi::fmi_ok);

    for (int i = 0; i < 100; i++) {
        REQUIRE_EQ(sim->step(), cosmo::fmi::fmi_ok);
    }

    REQUIRE_EQ(sim->get_step_count(), 100u);
    REQUIRE(fabs(sim->get_current_time() - 1.0) < 1e-12);

    sim->terminate();
}

TEST_CASE(load_with_parameters) {
    auto ss = cosmo::simulation::create_simulation_structure();

    std::map<std::string, cosmo::simulation::scalar_value> params;
    params["mass"] = cosmo::simulation::scalar_value(5.0);
    params["damping"] = cosmo::simulation::scalar_value(0.1);

    ss->add_model("damper", "damper.fmu", params);
    ss->add_parameter_set("high_damping", params);

    auto sim = ss->load(create_algo());
    REQUIRE_EQ(sim->get_instances().size(), 1u);
}

// ============================================================================
// variable_connection 测试
// ============================================================================

TEST_CASE(variable_connection_properties) {
    cosmo::simulation::variable_connection conn("src_m", "src_v", "tgt_m", "tgt_v");

    REQUIRE_EQ(conn.get_source_model(), std::string("src_m"));
    REQUIRE_EQ(conn.get_source_variable(), std::string("src_v"));
    REQUIRE_EQ(conn.get_target_model(), std::string("tgt_m"));
    REQUIRE_EQ(conn.get_target_variable(), std::string("tgt_v"));
    REQUIRE_FALSE(conn.has_modifier());

    double result = conn.apply_modifier(5.0);
    REQUIRE(fabs(result - 5.0) < 1e-12);
}

TEST_CASE(variable_connection_with_modifier) {
    auto mod = [](cosmo::fmi::fmi_real v) { return v * 3.0; };
    cosmo::simulation::variable_connection conn("s", "v", "t", "w", mod);

    REQUIRE(conn.has_modifier());
    REQUIRE(fabs(conn.apply_modifier(2.0) - 6.0) < 1e-12);
    REQUIRE(fabs(conn.apply_modifier(0.0) - 0.0) < 1e-12);
    REQUIRE(fabs(conn.apply_modifier(-1.0) - (-3.0)) < 1e-12);
}

// ============================================================================
// 完整集成测试
// ============================================================================

TEST_CASE(full_structure_workflow) {
    // 构建仿真结构
    auto ss = cosmo::simulation::create_simulation_structure();

    ss->add_model("controller", "pid.fmu");
    ss->add_model("plant", "motor.fmu");

    std::map<std::string, cosmo::simulation::scalar_value> pid_params;
    pid_params["kp"] = cosmo::simulation::scalar_value(1.0);
    pid_params["ki"] = cosmo::simulation::scalar_value(0.1);
    ss->add_model("controller", "pid.fmu", pid_params);

    ss->make_connection("controller::output", "plant::input");
    ss->make_connection("plant::sensor", "controller::feedback");

    std::map<std::string, cosmo::simulation::scalar_value> scenario_params;
    scenario_params["ref"] = cosmo::simulation::scalar_value(10.0);
    ss->add_parameter_set("nominal", scenario_params);

    REQUIRE_EQ(ss->get_model_count(), 2u);  // "controller"只保留一个（重名忽略）
    REQUIRE_EQ(ss->get_connection_count(), 2u);
    REQUIRE_EQ(ss->get_parameter_set_count(), 1u);

    // 加载并运行
    auto sim = ss->load(create_algo(0.005));
    REQUIRE_EQ(sim->get_instances().size(), 2u);
    REQUIRE_EQ(sim->get_connection_count(), 2u);

    REQUIRE_EQ(sim->initialize(0.0), cosmo::fmi::fmi_ok);
    REQUIRE_EQ(sim->step(), cosmo::fmi::fmi_ok);
    REQUIRE_EQ(sim->step(), cosmo::fmi::fmi_ok);

    REQUIRE_EQ(sim->get_step_count(), 2u);
    sim->terminate();
}

int main()
{
    return RUN_ALL_TESTS();
}
