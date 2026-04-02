/**
 * @file simulation_listener_test.cpp
 * @brief 仿真监听器功能全面测试
 *
 * 测试监听器接口的各种实现、回调行为、
 * 场景集成以及CSV写入器功能。
 */

#include "cosmo/test/test_framework.hpp"
#include "cosmo/simulation/simulation_listener.hpp"
#include "cosmo/simulation/simulation.hpp"
#include "cosmo/simulation/model_instance.hpp"
#include "cosmo/simulation/scenario.hpp"
#include "cosmo/simulation/simulation_types.hpp"
#include "cosmo/listeners/csv_writer.hpp"
#include "cosmo/fmi/model_description.hpp"
#include "cosmo/fmi/fmi_types.hpp"
#include "cosmo/logger/logger.hpp"

#include <fstream>
#include <sstream>
#include <cstdio>
#include <cmath>

namespace {

// 记录所有回调的监听器
class full_tracking_listener : public cosmo::simulation::simulation_listener
{
public:
    std::string name_;
    int pre_init_count = 0;
    int post_init_count = 0;
    int pre_step_count = 0;
    int post_step_count = 0;
    int pre_terminate_count = 0;
    int post_terminate_count = 0;
    int reset_count = 0;
    double last_init_time = -999;
    double last_step_time = -999;
    double last_step_size = -999;

    explicit full_tracking_listener(const std::string& name = "tracker") : name_(name) {}
    std::string get_name() const override { return name_; }

    void on_pre_init(double t) override { pre_init_count++; last_init_time = t; }
    void on_post_init(double /*t*/) override { post_init_count++; }
    void on_pre_step(double t, double s) override { pre_step_count++; last_step_time = t; last_step_size = s; }
    void on_post_step(double /*t*/, double /*s*/) override { post_step_count++; }
    void on_pre_terminate() override { pre_terminate_count++; }
    void on_post_terminate() override { post_terminate_count++; }
    void on_reset() override { reset_count++; }
};

auto make_desc() -> std::unique_ptr<cosmo::fmi::model_description>
{
    auto d = cosmo::fmi::create_model_description();
    d->set_model_name("test");
    cosmo::fmi::variable_description v;
    v.name = "x"; v.vr = 0; v.type = cosmo::fmi::variable_type::real;
    v.causality_type = cosmo::fmi::causality::local;
    d->add_variable(v);
    return d;
}

auto make_sim(double step_size = 0.1) -> std::unique_ptr<cosmo::simulation::simulation>
{
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(step_size));
    return std::unique_ptr<cosmo::simulation::simulation>(
        new cosmo::simulation::simulation(std::move(algo)));
}

} // anonymous namespace

// ============================================================================
// 基础监听器回调测试
// ============================================================================

TEST_CASE(listener_init_callbacks) {
    auto sim = make_sim();
    auto listener = std::unique_ptr<full_tracking_listener>(new full_tracking_listener());
    auto* ptr = listener.get();
    sim->add_listener(std::move(listener));

    sim->initialize(2.5);
    REQUIRE_EQ(ptr->pre_init_count, 1);
    REQUIRE_EQ(ptr->post_init_count, 1);
    REQUIRE(fabs(ptr->last_init_time - 2.5) < 1e-12);
}

TEST_CASE(listener_step_callbacks) {
    auto sim = make_sim();
    auto listener = std::unique_ptr<full_tracking_listener>(new full_tracking_listener());
    auto* ptr = listener.get();
    sim->add_listener(std::move(listener));

    sim->initialize(0.0);
    sim->step();
    sim->step();
    sim->step();

    REQUIRE_EQ(ptr->pre_step_count, 3);
    REQUIRE_EQ(ptr->post_step_count, 3);
    REQUIRE(fabs(ptr->last_step_time - 0.2) < 1e-12);
    REQUIRE(fabs(ptr->last_step_size - 0.1) < 1e-12);
}

TEST_CASE(listener_terminate_callbacks) {
    auto sim = make_sim();
    auto listener = std::unique_ptr<full_tracking_listener>(new full_tracking_listener());
    auto* ptr = listener.get();
    sim->add_listener(std::move(listener));

    sim->initialize(0.0);
    sim->terminate();

    REQUIRE_EQ(ptr->pre_terminate_count, 1);
    REQUIRE_EQ(ptr->post_terminate_count, 1);
}

TEST_CASE(listener_reset_callback) {
    auto sim = make_sim();
    auto listener = std::unique_ptr<full_tracking_listener>(new full_tracking_listener());
    auto* ptr = listener.get();
    sim->add_listener(std::move(listener));

    sim->initialize(0.0);
    sim->step();
    sim->reset();

    REQUIRE_EQ(ptr->reset_count, 1);
}

TEST_CASE(listener_full_lifecycle_sequence) {
    auto sim = make_sim();
    auto listener = std::unique_ptr<full_tracking_listener>(new full_tracking_listener());
    auto* ptr = listener.get();
    sim->add_listener(std::move(listener));

    sim->initialize(0.0);    // pre_init + post_init
    sim->step();              // pre_step + post_step
    sim->step();              // pre_step + post_step
    sim->terminate();          // pre_terminate + post_terminate
    sim->reset();             // reset
    sim->initialize(0.0);    // pre_init + post_init
    sim->step();              // pre_step + post_step

    REQUIRE_EQ(ptr->pre_init_count, 2);
    REQUIRE_EQ(ptr->post_init_count, 2);
    REQUIRE_EQ(ptr->pre_step_count, 3);
    REQUIRE_EQ(ptr->post_step_count, 3);
    REQUIRE_EQ(ptr->pre_terminate_count, 1);
    REQUIRE_EQ(ptr->post_terminate_count, 1);
    REQUIRE_EQ(ptr->reset_count, 1);
}

// ============================================================================
// 场景与监听器集成测试
// ============================================================================

TEST_CASE(scenario_as_listener) {
    auto sim = make_sim(0.1);
    auto desc = make_desc();
    sim->add_model_instance(
        cosmo::simulation::create_model_instance("m1", std::move(desc)));

    auto scen = cosmo::simulation::make_scenario();
    scen->bind(sim.get());
    scen->schedule_real(0.0, "m1::x", 100.0);
    scen->schedule_real(0.2, "m1::x", 200.0);

    sim->add_listener(std::move(scen));

    sim->initialize(0.0);
    cosmo::fmi::fmi_real val = 0.0;
    sim->get_real("m1::x", val);
    REQUIRE(fabs(val - 100.0) < 1e-12);

    sim->step();  // t=0.1
    sim->step();  // t=0.2

    sim->get_real("m1::x", val);
    REQUIRE(fabs(val - 200.0) < 1e-12);

    sim->terminate();
}

TEST_CASE(scenario_action_ordering) {
    auto scen = cosmo::simulation::make_scenario();
    scen->schedule_real(5.0, "m::x", 1.0);
    scen->schedule_real(2.0, "m::y", 2.0);
    scen->schedule_real(1.0, "m::z", 3.0);

    REQUIRE_EQ(scen->command_count(), 3u);

    // schedule() 内部使用 upper_bound 插入，自动保持有序
    const auto* a0 = scen->peek_command(0);
    REQUIRE(fabs(a0->scheduled_time - 1.0) < 1e-12);

    const auto* a1 = scen->peek_command(1);
    REQUIRE(fabs(a1->scheduled_time - 2.0) < 1e-12);

    const auto* a2 = scen->peek_command(2);
    REQUIRE(fabs(a2->scheduled_time - 5.0) < 1e-12);
}

TEST_CASE(scenario_clear) {
    auto scen = cosmo::simulation::make_scenario();
    scen->schedule_real(1.0, "m::x", 1.0);
    scen->schedule_integer(2.0, "m::y", 2);
    REQUIRE_EQ(scen->command_count(), 2u);

    scen->purge();
    REQUIRE_EQ(scen->command_count(), 0u);
}

TEST_CASE(scenario_command_types) {
    auto scen = cosmo::simulation::make_scenario();

    scen->schedule_real(1.0, "m::x", 3.14);
    const auto* cmd0 = scen->peek_command(0);
    REQUIRE(cmd0 != nullptr);
    REQUIRE_EQ(static_cast<int>(cmd0->kind), static_cast<int>(cosmo::simulation::command_kind::assign_real));
    REQUIRE(fabs(cmd0->real_val - 3.14) < 1e-12);
    REQUIRE_EQ(cmd0->target, std::string("m::x"));

    scen->schedule_integer(2.0, "m::y", 42);
    const auto* cmd1 = scen->peek_command(1);
    REQUIRE_EQ(static_cast<int>(cmd1->kind), static_cast<int>(cosmo::simulation::command_kind::assign_integer));
    REQUIRE_EQ(cmd1->int_val, 42);

    scen->schedule_boolean(3.0, "m::z", true);
    const auto* cmd2 = scen->peek_command(2);
    REQUIRE_EQ(static_cast<int>(cmd2->kind), static_cast<int>(cosmo::simulation::command_kind::assign_boolean));
    REQUIRE_EQ(cmd2->bool_val, true);

    scen->schedule_string(4.0, "m::w", std::string("hello"));
    const auto* cmd3 = scen->peek_command(3);
    REQUIRE_EQ(static_cast<int>(cmd3->kind), static_cast<int>(cosmo::simulation::command_kind::assign_string));
    REQUIRE_EQ(cmd3->str_val, std::string("hello"));
}

TEST_CASE(scenario_load_from_nonexistent_file) {
    auto scen = cosmo::simulation::scenario::from_file("/nonexistent/file.xml");
    REQUIRE(scen == nullptr);
}

// ============================================================================
// CSV写入器测试
// ============================================================================

TEST_CASE(csv_writer_basic) {
    const std::string filename = "/tmp/test_cosmo_csv_basic.csv";

    cosmo::listeners::csv_config config;
    config.register_variable("time");
    config.register_variable("m1", "x");

    auto writer = cosmo::listeners::create_csv_writer(filename, config);
    REQUIRE_EQ(writer->get_name(), std::string("csv_writer"));

    // 触发生命周期
    writer->on_pre_init(0.0);
    writer->on_post_init(0.0);
    writer->on_post_step(0.1, 0.1);
    writer->on_post_step(0.2, 0.1);
    writer->on_pre_terminate();
    writer->on_post_terminate();

    // 检查文件是否存在
    std::ifstream file(filename);
    REQUIRE(file.is_open());

    std::string line;
    std::getline(file, line);  // 头部
    REQUIRE(line.find("time") != std::string::npos);
    REQUIRE(line.find("m1::x") != std::string::npos);

    file.close();
    std::remove(filename.c_str());
}

TEST_CASE(csv_writer_downsample) {
    const std::string filename = "/tmp/test_cosmo_csv_downsample.csv";

    cosmo::listeners::csv_config config;
    config.set_downsample_factor(2);

    auto writer = cosmo::listeners::create_csv_writer(filename, config);
    writer->on_pre_init(0.0);
    writer->on_post_init(0.0);

    // 4步，但降采样因子为2，只应记录2行
    writer->on_post_step(0.1, 0.1);  // step 1: 记录
    writer->on_post_step(0.2, 0.1);  // step 2: 跳过
    writer->on_post_step(0.3, 0.1);  // step 3: 记录
    writer->on_post_step(0.4, 0.1);  // step 4: 跳过

    writer->on_post_terminate();

    // 验证行数
    std::ifstream file(filename);
    std::string line;
    int line_count = 0;
    while (std::getline(file, line)) {
        line_count++;
    }
    file.close();

    // 头部1行 + 初始行1行 + 记录2行 = 4行
    REQUIRE_EQ(line_count, 4);
    std::remove(filename.c_str());
}

TEST_CASE(csv_writer_config) {
    cosmo::listeners::csv_config config;
    REQUIRE_EQ(config.get_downsample_factor(), 1);
    REQUIRE_FALSE(config.get_clear_on_reset());

    config.set_downsample_factor(5);
    REQUIRE_EQ(config.get_downsample_factor(), 5);

    config.set_downsample_factor(0);  // 非法值应被限制为1
    REQUIRE_EQ(config.get_downsample_factor(), 1);

    config.set_clear_on_reset(true);
    REQUIRE(config.get_clear_on_reset());

    config.register_variable("time");
    config.register_variable("m1", "y");

    const auto& vars = config.get_registered_variables();
    REQUIRE_EQ(vars.size(), 2u);
    REQUIRE_EQ(vars[0], std::string("time"));
    REQUIRE_EQ(vars[1], std::string("m1::y"));
}

// ============================================================================
// 边界条件测试
// ============================================================================

TEST_CASE(listener_with_zero_step_size) {
    // 步长为0.001很小但合法
    auto sim = make_sim(0.001);
    auto listener = std::unique_ptr<full_tracking_listener>(new full_tracking_listener());
    auto* ptr = listener.get();
    sim->add_listener(std::move(listener));

    sim->initialize(0.0);
    for (int i = 0; i < 10; i++) {
        sim->step();
    }
    REQUIRE_EQ(ptr->post_step_count, 10);
    REQUIRE(fabs(sim->get_current_time() - 0.01) < 1e-12);
}

TEST_CASE(listener_removed_mid_simulation) {
    auto sim = make_sim();
    auto listener = std::unique_ptr<full_tracking_listener>(new full_tracking_listener("removable"));
    auto* ptr = listener.get();
    sim->add_listener(std::move(listener));

    sim->initialize(0.0);
    sim->step();  // 1次通知
    REQUIRE_EQ(ptr->post_step_count, 1);

    sim->remove_listener("removable");
    sim->step();  // 不会再通知
    REQUIRE_EQ(ptr->post_step_count, 1);

    sim->terminate();
}

TEST_CASE(csv_writer_reset_behavior) {
    const std::string filename = "/tmp/test_cosmo_csv_reset.csv";

    cosmo::listeners::csv_config config;
    config.set_clear_on_reset(true);
    config.register_variable("time");

    auto writer = cosmo::listeners::create_csv_writer(filename, config);
    writer->on_pre_init(0.0);
    writer->on_post_init(0.0);
    writer->on_post_step(0.1, 0.1);

    // 重置后清空文件
    writer->on_reset();
    writer->on_pre_init(0.0);
    writer->on_post_init(0.0);
    writer->on_post_step(0.1, 0.1);
    writer->on_post_terminate();

    std::ifstream file(filename);
    REQUIRE(file.is_open());
    file.close();
    std::remove(filename.c_str());
}

int main()
{
    return RUN_ALL_TESTS();
}
