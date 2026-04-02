/**
 * @file scenario_test.cpp
 * @brief 场景调度系统全面测试
 *
 * 测试variable_command管理、时间轴排序、仿真执行、
 * XML加载和生命周期行为。
 */

#include "cosmo/test/test_framework.hpp"
#include "cosmo/simulation/scenario.hpp"
#include "cosmo/simulation/simulation.hpp"
#include "cosmo/simulation/model_instance.hpp"
#include "cosmo/simulation/simulation_types.hpp"
#include "cosmo/fmi/model_description.hpp"
#include "cosmo/logger/logger.hpp"

#include <fstream>
#include <cstdio>
#include <cmath>

namespace {

auto make_test_desc() -> std::unique_ptr<cosmo::fmi::model_description>
{
    auto d = cosmo::fmi::create_model_description();
    d->set_model_name("test_model");

    cosmo::fmi::variable_description v;

    v.name = "x"; v.vr = 0; v.type = cosmo::fmi::variable_type::real;
    v.default_real = 0.0; v.causality_type = cosmo::fmi::causality::local;
    d->add_variable(v);

    v.name = "y"; v.vr = 1; v.type = cosmo::fmi::variable_type::real;
    v.default_real = 0.0;
    d->add_variable(v);

    v.name = "z"; v.vr = 2; v.type = cosmo::fmi::variable_type::real;
    v.default_real = 0.0;
    d->add_variable(v);

    v.name = "n"; v.vr = 3; v.type = cosmo::fmi::variable_type::integer;
    v.default_integer = 0;
    d->add_variable(v);

    v.name = "mode"; v.vr = 4; v.type = cosmo::fmi::variable_type::integer;
    v.default_integer = 0;
    d->add_variable(v);

    v.name = "count"; v.vr = 5; v.type = cosmo::fmi::variable_type::integer;
    v.default_integer = 0;
    d->add_variable(v);

    v.name = "flag"; v.vr = 6; v.type = cosmo::fmi::variable_type::boolean;
    v.default_boolean = false;
    d->add_variable(v);

    v.name = "enable"; v.vr = 7; v.type = cosmo::fmi::variable_type::boolean;
    v.default_boolean = false;
    d->add_variable(v);

    v.name = "label"; v.vr = 8; v.type = cosmo::fmi::variable_type::string;
    v.default_string = "";
    d->add_variable(v);

    return d;
}

auto make_test_sim() -> std::unique_ptr<cosmo::simulation::simulation>
{
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.1));
    auto sim = std::unique_ptr<cosmo::simulation::simulation>(
        new cosmo::simulation::simulation(std::move(algo)));
    sim->add_model_instance(
        cosmo::simulation::create_model_instance("m1", make_test_desc()));
    return sim;
}

} // anonymous namespace

// ============================================================================
// 场景创建和基本操作
// ============================================================================

TEST_CASE(scenario_create) {
    auto scen = cosmo::simulation::make_scenario();
    REQUIRE(scen != nullptr);
    REQUIRE_EQ(scen->command_count(), 0u);
    REQUIRE_EQ(scen->get_name(), std::string("scenario"));
}

TEST_CASE(scenario_schedule_real_commands) {
    auto scen = cosmo::simulation::make_scenario();
    scen->schedule_real(1.0, "m1::x", 5.0);
    scen->schedule_real(2.0, "m1::y", 10.0);
    scen->schedule_real(0.5, "m1::z", 15.0);

    REQUIRE_EQ(scen->command_count(), 3u);

    // schedule() 内部使用 upper_bound 插入，自动保持有序
    const auto* a = scen->peek_command(0);
    REQUIRE(a != nullptr);
    REQUIRE_EQ(static_cast<int>(a->kind), static_cast<int>(cosmo::simulation::command_kind::assign_real));
    REQUIRE(fabs(a->scheduled_time - 0.5) < 1e-12);
}

TEST_CASE(scenario_schedule_integer_commands) {
    auto scen = cosmo::simulation::make_scenario();
    scen->schedule_integer(1.0, "m1::mode", 3);
    scen->schedule_integer(2.0, "m1::count", 100);

    REQUIRE_EQ(scen->command_count(), 2u);
    REQUIRE_EQ(static_cast<int>(scen->peek_command(0)->kind),
               static_cast<int>(cosmo::simulation::command_kind::assign_integer));
}

TEST_CASE(scenario_schedule_boolean_commands) {
    auto scen = cosmo::simulation::make_scenario();
    scen->schedule_boolean(0.0, "m1::enable", true);
    scen->schedule_boolean(5.0, "m1::reset", false);

    REQUIRE_EQ(scen->command_count(), 2u);
    REQUIRE_EQ(static_cast<int>(scen->peek_command(0)->kind),
               static_cast<int>(cosmo::simulation::command_kind::assign_boolean));
}

TEST_CASE(scenario_schedule_string_commands) {
    auto scen = cosmo::simulation::make_scenario();
    scen->schedule_string(1.0, "m1::name", std::string("hello"));

    REQUIRE_EQ(scen->command_count(), 1u);
    REQUIRE_EQ(static_cast<int>(scen->peek_command(0)->kind),
               static_cast<int>(cosmo::simulation::command_kind::assign_string));
}

// ============================================================================
// 场景排序测试
// ============================================================================

TEST_CASE(scenario_auto_sort) {
    auto scen = cosmo::simulation::make_scenario();
    scen->schedule_real(10.0, "m1::a", 1.0);
    scen->schedule_real(1.0, "m1::b", 2.0);
    scen->schedule_real(5.0, "m1::c", 3.0);

    // schedule() 内部使用 upper_bound 插入，自动保持有序
    REQUIRE(fabs(scen->peek_command(0)->scheduled_time - 1.0) < 1e-12);
    REQUIRE(fabs(scen->peek_command(1)->scheduled_time - 5.0) < 1e-12);
    REQUIRE(fabs(scen->peek_command(2)->scheduled_time - 10.0) < 1e-12);
}

TEST_CASE(scenario_same_time_order_preserved) {
    auto scen = cosmo::simulation::make_scenario();
    scen->schedule_real(1.0, "m1::first", 1.0);
    scen->schedule_real(1.0, "m1::second", 2.0);
    scen->schedule_real(1.0, "m1::third", 3.0);

    // upper_bound 插入保证同时间元素保持插入顺序
    REQUIRE_EQ(scen->peek_command(0)->target, std::string("m1::first"));
    REQUIRE_EQ(scen->peek_command(1)->target, std::string("m1::second"));
    REQUIRE_EQ(scen->peek_command(2)->target, std::string("m1::third"));
}

// ============================================================================
// 场景获取测试
// ============================================================================

TEST_CASE(scenario_peek_command) {
    auto scen = cosmo::simulation::make_scenario();
    scen->schedule_real(0.0, "m1::x", 1.0);

    const auto* a = scen->peek_command(0);
    REQUIRE(a != nullptr);
    REQUIRE_EQ(a->target, std::string("m1::x"));

    REQUIRE(scen->peek_command(1) == nullptr);
    REQUIRE(scen->peek_command(999) == nullptr);
}

TEST_CASE(scenario_purge) {
    auto scen = cosmo::simulation::make_scenario();
    scen->schedule_real(1.0, "m1::x", 1.0);
    scen->schedule_integer(2.0, "m1::y", 2);
    scen->schedule_boolean(3.0, "m1::z", true);

    REQUIRE_EQ(scen->command_count(), 3u);
    scen->purge();
    REQUIRE_EQ(scen->command_count(), 0u);
}

// ============================================================================
// 场景执行测试
// ============================================================================

TEST_CASE(scenario_execute_with_simulation) {
    auto sim = make_test_sim();

    auto scen = cosmo::simulation::make_scenario();
    scen->bind(sim.get());
    scen->schedule_real(0.0, "m1::x", 42.0);
    scen->schedule_real(0.2, "m1::x", 99.0);

    sim->add_listener(std::move(scen));
    sim->initialize(0.0);

    cosmo::fmi::fmi_real val = 0.0;
    sim->get_real("m1::x", val);
    REQUIRE(fabs(val - 42.0) < 1e-12);

    sim->step();  // t=0.1
    sim->step();  // t=0.2
    sim->get_real("m1::x", val);
    REQUIRE(fabs(val - 99.0) < 1e-12);

    sim->terminate();
}

TEST_CASE(scenario_without_simulation_pointer) {
    auto scen = cosmo::simulation::make_scenario();
    scen->schedule_real(0.0, "m1::x", 1.0);

    scen->on_pre_init(0.0);
    scen->on_post_init(0.0);
    scen->on_pre_step(0.0, 0.1);
    // 不应崩溃
}

TEST_CASE(scenario_invalid_target_format) {
    auto sim = make_test_sim();

    auto scen = cosmo::simulation::make_scenario();
    scen->bind(sim.get());
    // 无效格式（使用 '.' 但目标不存在）
    scen->schedule_real(0.0, "invalid_var", 1.0);

    sim->add_listener(std::move(scen));
    sim->initialize(0.0);

    // 无分隔符的格式应被跳过但不崩溃    sim->step();
    sim->terminate();
}

// ============================================================================
// 场景生命周期回调测试
// ============================================================================

TEST_CASE(scenario_reset) {
    auto scen = cosmo::simulation::make_scenario();
    scen->schedule_real(1.0, "m1::x", 1.0);
    scen->schedule_real(2.0, "m1::y", 2.0);

    scen->on_pre_init(0.0);
    REQUIRE_EQ(scen->peek_command(0)->target, std::string("m1::x"));

    scen->on_reset();
    // 重置后 cursor 归零
}

TEST_CASE(scenario_multiple_types_commands) {
    auto sim = make_test_sim();

    auto scen = cosmo::simulation::make_scenario();
    scen->bind(sim.get());
    scen->schedule_real(0.0, "m1::x", 1.0);
    scen->schedule_integer(0.0, "m1::n", 10);
    scen->schedule_boolean(0.0, "m1::flag", true);
    scen->schedule_string(0.0, "m1::label", std::string("test"));

    sim->add_listener(std::move(scen));
    sim->initialize(0.0);

    cosmo::fmi::fmi_real rval = 0.0;
    sim->get_real("m1::x", rval);
    REQUIRE(fabs(rval - 1.0) < 1e-12);

    sim->terminate();
}

// ============================================================================
// XML加载测试
// ============================================================================

TEST_CASE(scenario_load_from_xml_content) {
    const char* xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<Scenario>"
        "  <RealAction time=\"1.0\" variable=\"m1::x\" value=\"3.14\"/>"
        "  <IntegerAction time=\"2.0\" variable=\"m1::mode\" value=\"5\"/>"
        "  <BooleanAction time=\"0.5\" variable=\"m1::enabled\" value=\"true\"/>"
        "  <StringAction time=\"3.0\" variable=\"m1::name\" value=\"hello\"/>"
        "</Scenario>";

    auto scen = cosmo::simulation::scenario::from_xml(xml);
    REQUIRE(scen != nullptr);
    REQUIRE_EQ(scen->command_count(), 4u);
}

TEST_CASE(scenario_load_empty_xml) {
    auto scen = cosmo::simulation::scenario::from_xml("");
    REQUIRE(scen == nullptr);
}

TEST_CASE(scenario_load_invalid_xml) {
    auto scen = cosmo::simulation::scenario::from_xml("not valid xml <><>");
    REQUIRE(scen == nullptr);
}

TEST_CASE(scenario_load_from_file) {
    const std::string filename = "/tmp/test_scenario.xml";
    std::ofstream ofs(filename);
    ofs << "<?xml version=\"1.0\"?>"
        << "<Scenario>"
        << "<RealAction time=\"0.0\" variable=\"m1::x\" value=\"42.0\"/>"
        << "</Scenario>";
    ofs.close();

    auto scen = cosmo::simulation::scenario::from_file(filename);
    REQUIRE(scen != nullptr);
    REQUIRE_EQ(scen->command_count(), 1u);

    std::remove(filename.c_str());
}

TEST_CASE(scenario_load_from_nonexistent_file) {
    auto scen = cosmo::simulation::scenario::from_file("/nonexistent/path.xml");
    REQUIRE(scen == nullptr);
}

TEST_CASE(scenario_skip_empty_variables) {
    const char* xml =
        "<?xml version=\"1.0\"?>"
        "<Scenario>"
        "  <RealAction time=\"1.0\" variable=\"\" value=\"0\"/>"
        "  <RealAction time=\"2.0\" variable=\"m1::x\" value=\"5.0\"/>"
        "</Scenario>";

    auto scen = cosmo::simulation::scenario::from_xml(xml);
    REQUIRE(scen != nullptr);
    REQUIRE_EQ(scen->command_count(), 1u);
}

// ============================================================================
// 变量命令构造测试
// ============================================================================

TEST_CASE(variable_command_factory_methods) {
    auto rc = cosmo::simulation::variable_command::make_real(1.5, "m::x", 3.14);
    REQUIRE(fabs(rc.scheduled_time - 1.5) < 1e-12);
    REQUIRE_EQ(rc.kind, cosmo::simulation::command_kind::assign_real);
    REQUIRE(fabs(rc.real_val - 3.14) < 1e-12);

    auto ic = cosmo::simulation::variable_command::make_integer(2.0, "m::n", 42);
    REQUIRE_EQ(ic.int_val, 42);

    auto bc = cosmo::simulation::variable_command::make_boolean(3.0, "m::f", true);
    REQUIRE_EQ(bc.bool_val, true);

    auto sc = cosmo::simulation::variable_command::make_string(4.0, "m::s", "hi");
    REQUIRE_EQ(sc.str_val, std::string("hi"));
}

int main()
{
    return RUN_ALL_TESTS();
}
