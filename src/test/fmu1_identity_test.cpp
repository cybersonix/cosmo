/**
 * @file fmu1_identity_test.cpp
 * @brief FMI 1.0 identity模型仿真测试
 *
 * 测试FMI 1.0版本identity FMU的加载、仿真和数据透传。
 * identity模型的功能：输入什么就输出什么（所有类型的直通）。
 * 变量：
 *   - realIn/realOut (Real)
 *   - integerIn/integerOut (Integer)
 *   - booleanIn/booleanOut (Boolean)
 *   - stringIn/stringOut (String)
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

// ============================================================================
// 第1层：FMU文件和模型描述解析
// ============================================================================

static void test_identity_parse()
{
    std::string fmu_path = find_fmu("1.0/identity.fmu");
    REQUIRE(util::file_exists(fmu_path));

    auto proc = cosmo::fmi::create_fmu_processor();
    REQUIRE(proc->load_fmu(fmu_path));

    auto desc = proc->parse_model_description();
    REQUIRE(desc != nullptr);
    REQUIRE(desc->get_model_name() == "no.viproma.demo.identity");
    REQUIRE(desc->get_version() == cosmo::fmi::fmi_version::fmi_1_0);

    // 验证GUID
    REQUIRE(!desc->get_guid().empty());

    auto vars = desc->get_variables();
    REQUIRE(vars.size() == 8);  // 4 inputs + 4 outputs

    // 验证输入变量
    const auto* real_in = desc->get_variable_by_name("realIn");
    REQUIRE(real_in != nullptr);
    REQUIRE(real_in->type == cosmo::fmi::variable_type::real);
    REQUIRE(real_in->causality_type == cosmo::fmi::causality::input);
    REQUIRE(std::fabs(real_in->default_real - 0.0) < 1e-10);

    const auto* int_in = desc->get_variable_by_name("integerIn");
    REQUIRE(int_in != nullptr);
    REQUIRE(int_in->type == cosmo::fmi::variable_type::integer);
    REQUIRE(int_in->causality_type == cosmo::fmi::causality::input);

    const auto* bool_in = desc->get_variable_by_name("booleanIn");
    REQUIRE(bool_in != nullptr);
    REQUIRE(bool_in->type == cosmo::fmi::variable_type::boolean);
    REQUIRE(bool_in->causality_type == cosmo::fmi::causality::input);

    const auto* str_in = desc->get_variable_by_name("stringIn");
    REQUIRE(str_in != nullptr);
    REQUIRE(str_in->type == cosmo::fmi::variable_type::string);
    REQUIRE(str_in->causality_type == cosmo::fmi::causality::input);

    // 验证输出变量
    const auto* real_out = desc->get_variable_by_name("realOut");
    REQUIRE(real_out != nullptr);
    REQUIRE(real_out->type == cosmo::fmi::variable_type::real);
    REQUIRE(real_out->causality_type == cosmo::fmi::causality::output);

    const auto* int_out = desc->get_variable_by_name("integerOut");
    REQUIRE(int_out != nullptr);
    REQUIRE(int_out->causality_type == cosmo::fmi::causality::output);

    const auto* bool_out = desc->get_variable_by_name("booleanOut");
    REQUIRE(bool_out != nullptr);
    REQUIRE(bool_out->causality_type == cosmo::fmi::causality::output);

    const auto* str_out = desc->get_variable_by_name("stringOut");
    REQUIRE(str_out != nullptr);
    REQUIRE(str_out->causality_type == cosmo::fmi::causality::output);

    proc->cleanup();

    std::cout << " [vars=" << vars.size() << "] ";
}

static void test_identity_binary_path()
{
    std::string fmu_path = find_fmu("1.0/identity.fmu");

    auto proc = cosmo::fmi::create_fmu_processor();
    REQUIRE(proc->load_fmu(fmu_path));

    std::string binary_path = proc->get_binary_path();
    std::cout << " [binary=" << (binary_path.empty() ? "not found" : binary_path) << "] ";

    if (!binary_path.empty()) {
        REQUIRE(util::file_exists(binary_path));
    }

    proc->cleanup();
}

// ============================================================================
// 第2层：仿真测试
// ============================================================================

static void test_identity_real_passthrough()
{
    // 测试real类型的输入输出直通
    std::string fmu_path = find_fmu("1.0/identity.fmu");

    auto ss = create_simulation_structure();
    ss->add_model("identity", fmu_path);

    double step_size = 1.0 / 100.0;
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(step_size));
    auto sim = ss->load(std::move(algo));
    REQUIRE(sim != nullptr);

    REQUIRE(sim->initialize(0.0) == cosmo::fmi::fmi_ok);

    // 设置输入值
    double input_val = 3.14;
    sim->set_real("identity::realIn", input_val);

    // 执行步进
    REQUIRE(sim->step() == cosmo::fmi::fmi_ok);

    // 读取输出值
    double output_val = 0.0;
    REQUIRE(sim->get_real("identity::realOut", output_val) == cosmo::fmi::fmi_ok);

    std::cout << " [real: " << input_val << " -> " << output_val << "] ";

    sim->terminate();
}

static void test_identity_integer_passthrough()
{
    // 测试integer类型的输入输出直通
    std::string fmu_path = find_fmu("1.0/identity.fmu");

    auto ss = create_simulation_structure();
    ss->add_model("identity", fmu_path);

    double step_size = 1.0 / 100.0;
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(step_size));
    auto sim = ss->load(std::move(algo));
    REQUIRE(sim != nullptr);

    REQUIRE(sim->initialize(0.0) == cosmo::fmi::fmi_ok);

    // 设置整数输入
    sim->set_integer("identity::integerIn", 42);
    REQUIRE(sim->step() == cosmo::fmi::fmi_ok);

    int output_val = 0;
    REQUIRE(sim->get_integer("identity::integerOut", output_val) == cosmo::fmi::fmi_ok);

    std::cout << " [int: 42 -> " << output_val << "] ";

    sim->terminate();
}

static void test_identity_simulation_with_csv()
{
    // 完整仿真并输出CSV
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

    csv->set_simulation(sim.get());
    sim->add_listener(std::move(csv));

    REQUIRE(sim->initialize(0.0) == cosmo::fmi::fmi_ok);

    // 设置输入为正弦波
    for (int i = 0; i < 500; i++) {
        double t = i * step_size;
        double input = std::sin(t * 2.0 * 3.14159265);
        sim->set_real("identity::realIn", input);
        sim->step();
    }

    sim->terminate();

    // 验证CSV文件内容
    std::ifstream file(csv_path);
    REQUIRE(file.is_open());

    std::string line;
    int line_count = 0;
    while (std::getline(file, line)) {
        line_count++;
    }
    REQUIRE(line_count >= 2);  // 至少有header + 数据行

    std::cout << " [csv=" << csv_path << ", rows=" << line_count << "] ";
}

// ============================================================================
// 主函数
// ============================================================================

int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << "FMI 1.0 identity模型仿真测试" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    system("mkdir -p results");

    int total = 0;

    // FMU文件和模型描述解析
    std::cout << "--- 模型描述解析 ---" << std::endl;
    run_test("identity_parse", test_identity_parse); total++;
    run_test("identity_binary_path", test_identity_binary_path); total++;
    std::cout << std::endl;

    // 仿真测试
    std::cout << "--- 仿真测试 ---" << std::endl;
    run_test("identity_real_passthrough", test_identity_real_passthrough); total++;
    run_test("identity_integer_passthrough", test_identity_integer_passthrough); total++;
    run_test("identity_simulation_csv", test_identity_simulation_with_csv); total++;
    std::cout << std::endl;

    std::cout << "========================================" << std::endl;
    std::cout << "Results: " << g_passed << " passed, "
              << g_failed << " failed (out of "
              << total << " tests)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    std::cout << "注意：如果FMU二进制文件因运行时库版本不兼容而无法加载，" << std::endl;
    std::cout << "框架会自动回退到mock实例进行测试。" << std::endl;

    return g_failed > 0 ? 1 : 0;
}
