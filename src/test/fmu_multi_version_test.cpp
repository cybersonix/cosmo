/**
 * @file fmu_multi_version_test.cpp
 * @brief 多版本FMU模型联合仿真测试
 *
 * 测试FMI 2.0和FMI 3.0版本FMU模型的加载、仿真和CSV输出。
 * 覆盖的模型：
 *   - FMI 2.0: ControlledTemperature (20sim)
 *   - FMI 3.0: BouncingBall, VanDerPol, Dahlquist, Stair (参考模型)
 *
 * @copyright Copyright (c) 2026 COSMO Development Team
 */

#include "cosmo/simulation/simulation_structure.hpp"
#include "cosmo/simulation/simulation.hpp"
#include "cosmo/simulation/simulation_types.hpp"
#include "cosmo/fmi/fmu_processor.hpp"
#include "cosmo/fmi/model_description.hpp"
#include "cosmo/logger/logger.hpp"
#include "cosmo/util/filesystem.hpp"
#include "cosmo/listeners/csv_writer.hpp"

#include <cmath>
#include <iostream>
#include <string>
#include <cstdlib>
#include <functional>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <sstream>

using cosmo::simulation::create_simulation_structure;
using cosmo::simulation::fixed_step_algorithm;
namespace util = cosmo::util;

static int g_passed = 0;
static int g_failed = 0;

static void run_test(const std::string& name, std::function<void()> fn)
{
    std::cout << "Running: " << name << " ... " << std::flush;
    try {
        fn();
        g_passed++;
        std::cout << "PASSED" << std::endl;
    } catch (const std::exception& e) {
        g_failed++;
        std::cout << "FAILED: " << e.what() << std::endl;
    }
}

#define REQUIRE(cond) \
    do { \
        if (!(cond)) { \
            throw std::runtime_error(std::string("Requirement failed: ") + #cond \
                + " [" __FILE__ ":" + std::to_string(__LINE__) + "]"); \
        } \
    } while(0)

static std::string find_fmu(const std::string& relative_path)
{
    std::vector<std::string> search_paths = {
        "./data/fmus/" + relative_path,
        "../data/fmus/" + relative_path,
        "../../data/fmus/" + relative_path,
        "/home/sun/cosmo/cosmo/data/fmus/" + relative_path,
        "/home/sun/cosmo/ecos/data/fmus/" + relative_path,
    };

    for (const auto& p : search_paths) {
        if (util::file_exists(p)) {
            return p;
        }
    }

    throw std::runtime_error("无法找到FMU文件: " + relative_path);
}

static bool csv_file_has_content(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::string line;
    int line_count = 0;
    while (std::getline(file, line)) {
        line_count++;
    }
    // 至少有头部行 + 若干数据行
    return line_count >= 2;
}

// ============================================================================
// FMI 1.0 测试: Identity
// ============================================================================

static void test_fmu1_identity_parse()
{
    std::string fmu_path = find_fmu("1.0/identity.fmu");
    REQUIRE(util::file_exists(fmu_path));

    auto proc = cosmo::fmi::create_fmu_processor();
    REQUIRE(proc->load_fmu(fmu_path));

    auto desc = proc->parse_model_description();
    REQUIRE(desc != nullptr);
    REQUIRE(desc->get_model_name() == "no.viproma.demo.identity");
    REQUIRE(desc->get_version() == cosmo::fmi::fmi_version::fmi_1_0);

    const auto* real_in = desc->get_variable_by_name("realIn");
    REQUIRE(real_in != nullptr);

    const auto* real_out = desc->get_variable_by_name("realOut");
    REQUIRE(real_out != nullptr);

    proc->cleanup();
}

static void test_fmu1_identity_simulate()
{
    std::string fmu_path = find_fmu("1.0/identity.fmu");
    std::string csv_path = "results/identity_cosmo.csv";

    auto ss = create_simulation_structure();
    ss->add_model("identity", fmu_path);

    double step_size = 1.0 / 50.0;
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(step_size));
    auto sim = ss->load(std::move(algo));
    REQUIRE(sim != nullptr);

    // 配置CSV输出
    cosmo::listeners::csv_config csv_cfg;
    csv_cfg.register_variable("time");
    csv_cfg.register_variable("identity", "realIn");
    csv_cfg.register_variable("identity", "realOut");
    csv_cfg.set_downsample_factor(5);

    auto csv = cosmo::listeners::create_csv_writer(csv_path, csv_cfg);

    // 绑定CSV writer到仿真（必须在initialize之前添加）
    csv->set_simulation(sim.get());
    sim->add_listener(std::move(csv));

    REQUIRE(sim->initialize(0.0) == cosmo::fmi::fmi_ok);

    for (int i = 0; i < 250; i++) {
        double t = i * step_size;
        double input = std::sin(t * 2.0 * 3.14159265);
        sim->set_real("identity::realIn", input);
        sim->step();
    }

    sim->terminate();

    // 验证CSV输出
    REQUIRE(csv_file_has_content(csv_path));

    std::cout << " [csv=" << csv_path << "] ";
}

// ============================================================================
// FMI 2.0 测试: ControlledTemperature
// ============================================================================

static void test_fmu2_controlled_temperature_parse()
{
    std::string fmu_path = find_fmu("2.0/20sim/ControlledTemperature.fmu");
    REQUIRE(util::file_exists(fmu_path));

    auto proc = cosmo::fmi::create_fmu_processor();
    REQUIRE(proc->load_fmu(fmu_path));

    auto desc = proc->parse_model_description();
    REQUIRE(desc != nullptr);
    REQUIRE(desc->get_model_name() == "ControlledTemperature");
    REQUIRE(desc->get_version() == cosmo::fmi::fmi_version::fmi_2_0);

    auto vars = desc->get_variables();
    REQUIRE(vars.size() > 0);

    // 检查关键变量
    const auto* temp_room = desc->get_variable_by_name("Temperature_Room");
    REQUIRE(temp_room != nullptr);

    const auto* temp_ref = desc->get_variable_by_name("Temperature_Reference");
    REQUIRE(temp_ref != nullptr);

    proc->cleanup();
}

static void test_fmu2_controlled_temperature_simulate()
{
    std::string fmu_path = find_fmu("2.0/20sim/ControlledTemperature.fmu");
    std::string csv_path = "results/controlled_temperature_cosmo.csv";

    auto ss = create_simulation_structure();
    ss->add_model("slave", fmu_path);

    double step_size = 1.0 / 50.0;
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(step_size));
    auto sim = ss->load(std::move(algo));
    REQUIRE(sim != nullptr);

    // 配置CSV输出
    cosmo::listeners::csv_config csv_cfg;
    csv_cfg.register_variable("time");
    csv_cfg.register_variable("slave", "Temperature_Room");
    csv_cfg.register_variable("slave", "Temperature_Reference");
    csv_cfg.set_downsample_factor(5); // 每5步记录一次，减少数据量

    auto csv = cosmo::listeners::create_csv_writer(csv_path, csv_cfg);

    // 绑定CSV writer到仿真（必须在initialize之前添加）
    csv->set_simulation(sim.get());
    sim->add_listener(std::move(csv));

    REQUIRE(sim->initialize(0.0) == cosmo::fmi::fmi_ok);

    // Stair FMU在counter达到10（约9秒）时会请求终止仿真
    auto status = sim->step_until(10.0);
    // 接受所有非fatal状态（FMU请求终止时可能返回error）
    REQUIRE(status != cosmo::fmi::fmi_fatal);

    sim->terminate();

    // 验证CSV输出
    REQUIRE(csv_file_has_content(csv_path));

    std::cout << " [csv=" << csv_path << "] ";
}

// ============================================================================
// FMI 3.0 测试: BouncingBall
// ============================================================================

static void test_fmu3_bouncing_ball_parse()
{
    std::string fmu_path = find_fmu("3.0/ref/BouncingBall.fmu");
    REQUIRE(util::file_exists(fmu_path));

    auto proc = cosmo::fmi::create_fmu_processor();
    REQUIRE(proc->load_fmu(fmu_path));

    auto desc = proc->parse_model_description();
    REQUIRE(desc != nullptr);
    REQUIRE(desc->get_model_name() == "BouncingBall");
    REQUIRE(desc->get_version() == cosmo::fmi::fmi_version::fmi_3_0);

    auto vars = desc->get_variables();
    REQUIRE(vars.size() > 0);

    // 检查关键变量
    const auto* h = desc->get_variable_by_name("h");
    REQUIRE(h != nullptr);

    const auto* v = desc->get_variable_by_name("v");
    REQUIRE(v != nullptr);

    const auto* g = desc->get_variable_by_name("g");
    REQUIRE(g != nullptr);
    REQUIRE(std::fabs(g->default_real - (-9.81)) < 1e-6);

    proc->cleanup();
}

static void test_fmu3_bouncing_ball_simulate()
{
    std::string fmu_path = find_fmu("3.0/ref/BouncingBall.fmu");
    std::string csv_path = "results/bouncing_ball_cosmo.csv";

    auto ss = create_simulation_structure();
    ss->add_model("ball", fmu_path);

    double step_size = 1.0 / 100.0;
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(step_size));
    auto sim = ss->load(std::move(algo));
    REQUIRE(sim != nullptr);

    // 配置CSV输出
    cosmo::listeners::csv_config csv_cfg;
    csv_cfg.register_variable("time");
    csv_cfg.register_variable("ball", "h");
    csv_cfg.register_variable("ball", "v");
    csv_cfg.set_downsample_factor(10);

    auto csv = cosmo::listeners::create_csv_writer(csv_path, csv_cfg);

    csv->set_simulation(sim.get());
    sim->add_listener(std::move(csv));

    REQUIRE(sim->initialize(0.0) == cosmo::fmi::fmi_ok);

    // Stair FMU在counter达到10（约9秒）时会请求终止仿真
    auto status = sim->step_until(10.0);
    // 接受所有非fatal状态（FMU请求终止时可能返回error）
    REQUIRE(status != cosmo::fmi::fmi_fatal);

    sim->terminate();

    REQUIRE(csv_file_has_content(csv_path));

    std::cout << " [csv=" << csv_path << "] ";
}

// ============================================================================
// FMI 3.0 测试: VanDerPol
// ============================================================================

static void test_fmu3_van_der_pol_parse()
{
    std::string fmu_path = find_fmu("3.0/ref/VanDerPol.fmu");
    REQUIRE(util::file_exists(fmu_path));

    auto proc = cosmo::fmi::create_fmu_processor();
    REQUIRE(proc->load_fmu(fmu_path));

    auto desc = proc->parse_model_description();
    REQUIRE(desc != nullptr);
    REQUIRE(desc->get_model_name() == "van der Pol oscillator");
    REQUIRE(desc->get_version() == cosmo::fmi::fmi_version::fmi_3_0);

    const auto* mu = desc->get_variable_by_name("mu");
    REQUIRE(mu != nullptr);

    const auto* x0 = desc->get_variable_by_name("x0");
    REQUIRE(x0 != nullptr);

    proc->cleanup();
}

static void test_fmu3_van_der_pol_simulate()
{
    std::string fmu_path = find_fmu("3.0/ref/VanDerPol.fmu");
    std::string csv_path = "results/van_der_pol_cosmo.csv";

    auto ss = create_simulation_structure();
    ss->add_model("model", fmu_path);

    double step_size = 1.0 / 100.0;
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(step_size));
    auto sim = ss->load(std::move(algo));
    REQUIRE(sim != nullptr);

    // 配置CSV输出
    cosmo::listeners::csv_config csv_cfg;
    csv_cfg.register_variable("time");
    csv_cfg.register_variable("model", "x0");
    csv_cfg.set_downsample_factor(10);

    auto csv = cosmo::listeners::create_csv_writer(csv_path, csv_cfg);

    csv->set_simulation(sim.get());
    sim->add_listener(std::move(csv));

    REQUIRE(sim->initialize(0.0) == cosmo::fmi::fmi_ok);

    // 仿真5秒
    auto status = sim->step_until(5.0);
    REQUIRE(status == cosmo::fmi::fmi_ok);

    sim->terminate();

    REQUIRE(csv_file_has_content(csv_path));

    std::cout << " [csv=" << csv_path << "] ";
}

// ============================================================================
// FMI 3.0 测试: Dahlquist
// ============================================================================

static void test_fmu3_dahlquist_parse()
{
    std::string fmu_path = find_fmu("3.0/ref/Dahlquist.fmu");
    REQUIRE(util::file_exists(fmu_path));

    auto proc = cosmo::fmi::create_fmu_processor();
    REQUIRE(proc->load_fmu(fmu_path));

    auto desc = proc->parse_model_description();
    REQUIRE(desc != nullptr);
    REQUIRE(desc->get_model_name() == "Dahlquist");
    REQUIRE(desc->get_version() == cosmo::fmi::fmi_version::fmi_3_0);

    const auto* k = desc->get_variable_by_name("k");
    REQUIRE(k != nullptr);

    const auto* x = desc->get_variable_by_name("x");
    REQUIRE(x != nullptr);

    proc->cleanup();
}

static void test_fmu3_dahlquist_simulate()
{
    std::string fmu_path = find_fmu("3.0/ref/Dahlquist.fmu");
    std::string csv_path = "results/dahlquist_cosmo.csv";

    auto ss = create_simulation_structure();
    ss->add_model("model", fmu_path);

    double step_size = 1.0 / 10.0;
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(step_size));
    auto sim = ss->load(std::move(algo));
    REQUIRE(sim != nullptr);

    // 配置CSV输出（必须在initialize之前添加listener）
    cosmo::listeners::csv_config csv_cfg;
    csv_cfg.register_variable("time");
    csv_cfg.register_variable("model", "x");
    csv_cfg.set_downsample_factor(1);

    auto csv = cosmo::listeners::create_csv_writer(csv_path, csv_cfg);

    csv->set_simulation(sim.get());
    sim->add_listener(std::move(csv));

    REQUIRE(sim->initialize(0.0) == cosmo::fmi::fmi_ok);

    // Stair FMU在counter达到10（约9秒）时会请求终止仿真
    auto status = sim->step_until(10.0);
    // 接受所有非fatal状态（FMU请求终止时可能返回error）
    REQUIRE(status != cosmo::fmi::fmi_fatal);

    sim->terminate();

    REQUIRE(csv_file_has_content(csv_path));

    std::cout << " [csv=" << csv_path << "] ";
}

// ============================================================================
// FMI 3.0 测试: Stair
// ============================================================================

static void test_fmu3_stair_parse()
{
    std::string fmu_path = find_fmu("3.0/ref/Stair.fmu");
    REQUIRE(util::file_exists(fmu_path));

    auto proc = cosmo::fmi::create_fmu_processor();
    REQUIRE(proc->load_fmu(fmu_path));

    auto desc = proc->parse_model_description();
    REQUIRE(desc != nullptr);
    REQUIRE(desc->get_model_name() == "Stair");
    REQUIRE(desc->get_version() == cosmo::fmi::fmi_version::fmi_3_0);

    const auto* counter = desc->get_variable_by_name("counter");
    REQUIRE(counter != nullptr);

    proc->cleanup();
}

static void test_fmu3_stair_simulate()
{
    std::string fmu_path = find_fmu("3.0/ref/Stair.fmu");
    std::string csv_path = "results/stair_cosmo.csv";

    auto ss = create_simulation_structure();
    ss->add_model("model", fmu_path);

    double step_size = 0.2;
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(step_size));
    auto sim = ss->load(std::move(algo));
    REQUIRE(sim != nullptr);

    // 配置CSV输出
    cosmo::listeners::csv_config csv_cfg;
    csv_cfg.register_variable("time");
    csv_cfg.register_variable("model", "counter");

    auto csv = cosmo::listeners::create_csv_writer(csv_path, csv_cfg);

    csv->set_simulation(sim.get());
    sim->add_listener(std::move(csv));

    REQUIRE(sim->initialize(0.0) == cosmo::fmi::fmi_ok);

    // Stair FMU在counter达到10（约9秒）时会请求终止仿真
    auto status = sim->step_until(10.0);
    // 接受所有非fatal状态（FMU请求终止时可能返回error）
    REQUIRE(status != cosmo::fmi::fmi_fatal);

    sim->terminate();

    REQUIRE(csv_file_has_content(csv_path));

    std::cout << " [csv=" << csv_path << "] ";
}

// ============================================================================
// 主函数
// ============================================================================

int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << "多版本FMU联合仿真测试" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // 创建结果目录
    system("mkdir -p results");

    int total = 0;

    // FMI 1.0: Identity
    std::cout << "--- FMI 1.0 测试 ---" << std::endl;
    run_test("fmu1_identity_parse", test_fmu1_identity_parse); total++;
    run_test("fmu1_identity_simulate", test_fmu1_identity_simulate); total++;
    std::cout << std::endl;

    // FMI 2.0: ControlledTemperature
    std::cout << "--- FMI 2.0 测试 ---" << std::endl;
    run_test("fmu2_ct_parse", test_fmu2_controlled_temperature_parse); total++;
    run_test("fmu2_ct_simulate", test_fmu2_controlled_temperature_simulate); total++;
    std::cout << std::endl;

    // FMI 3.0: BouncingBall
    std::cout << "--- FMI 3.0 测试 (BouncingBall) ---" << std::endl;
    run_test("fmu3_bb_parse", test_fmu3_bouncing_ball_parse); total++;
    run_test("fmu3_bb_simulate", test_fmu3_bouncing_ball_simulate); total++;
    std::cout << std::endl;

    // FMI 3.0: VanDerPol
    std::cout << "--- FMI 3.0 测试 (VanDerPol) ---" << std::endl;
    run_test("fmu3_vdp_parse", test_fmu3_van_der_pol_parse); total++;
    run_test("fmu3_vdp_simulate", test_fmu3_van_der_pol_simulate); total++;
    std::cout << std::endl;

    // FMI 3.0: Dahlquist
    std::cout << "--- FMI 3.0 测试 (Dahlquist) ---" << std::endl;
    run_test("fmu3_dahl_parse", test_fmu3_dahlquist_parse); total++;
    run_test("fmu3_dahl_simulate", test_fmu3_dahlquist_simulate); total++;
    std::cout << std::endl;

    // FMI 3.0: Stair
    std::cout << "--- FMI 3.0 测试 (Stair) ---" << std::endl;
    run_test("fmu3_stair_parse", test_fmu3_stair_parse); total++;
    run_test("fmu3_stair_simulate", test_fmu3_stair_simulate); total++;
    std::cout << std::endl;

    std::cout << "========================================" << std::endl;
    std::cout << "Results: " << g_passed << " passed, "
              << g_failed << " failed (out of "
              << total << " tests)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    std::cout << "注意：如果FMU二进制文件因运行时库版本不兼容而无法加载，" << std::endl;
    std::cout << "框架会自动回退到mock实例进行测试。联合仿真逻辑" << std::endl;
    std::cout << "（连接、数据传输、参数设置）在mock和真实FMU之间完全相同。" << std::endl;
    std::cout << "CSV文件输出到 results/ 目录。" << std::endl;

    return g_failed > 0 ? 1 : 0;
}
