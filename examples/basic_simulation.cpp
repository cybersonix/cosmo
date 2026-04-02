/**
 * @file basic_simulation.cpp
 * @brief COSMO基本仿真示例
 *
 * 演示如何使用C++ API创建和执行多模型联合仿真。
 * 本示例使用模拟模型实例（无需实际FMU文件），
 * 在实际使用中替换为FMU加载即可。
 */

#include "cosmo/simulation/simulation.hpp"
#include "cosmo/simulation/simulation_structure.hpp"
#include "cosmo/simulation/scenario.hpp"
#include "cosmo/listeners/csv_writer.hpp"
#include "cosmo/fmi/model_description.hpp"
#include "cosmo/fmi/fmi_types.hpp"
#include "cosmo/logger/logger.hpp"

#include <iostream>
#include <memory>

int main()
{
    cosmo::log::set_logging_level(cosmo::log::info_level);
    cosmo::log::info("=== COSMO基本联合仿真示例 ===");

    // 1. 创建仿真结构
    auto ss = cosmo::simulation::create_simulation_structure();

    // 2. 添加模型（使用模拟FMU路径）
    ss->add_model("controller", "controller.fmu");
    ss->add_model("plant", "plant.fmu");

    // 3. 为模型注册变量（实际使用中由FMU自动提供）
    ss->register_variable("controller", "real", "output");
    ss->register_variable("controller", "real", "reference");
    ss->register_variable("plant", "real", "input");
    ss->register_variable("plant", "real", "position");
    ss->register_variable("plant", "real", "velocity");

    // 4. 建立模型间连接：controller.output -> plant.input
    ss->make_connection("controller::output", "plant::input");

    // 5. 创建仿真引擎（固定步长0.001秒）
    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.001));
    auto sim = ss->load(std::move(algo));

    if (!sim) {
        cosmo::log::error("创建仿真失败");
        return 1;
    }

    // 6. 添加CSV结果写入器
    cosmo::listeners::csv_config csv_conf;
    csv_conf.register_variable("time");
    csv_conf.register_variable("controller", "output");
    csv_conf.register_variable("plant", "position");
    csv_conf.register_variable("plant", "velocity");

    auto writer = cosmo::listeners::create_csv_writer("results.csv", csv_conf);
    sim->add_listener(std::move(writer));

    // 7. 添加场景（在特定时间点修改参数）
    auto scen = cosmo::simulation::make_scenario();
    scen->bind(sim.get());
    scen->schedule_real(0.0, "controller::reference", 1.0);
    scen->schedule_real(2.0, "controller::reference", 2.0);
    scen->schedule_real(5.0, "controller::reference", 0.5);
    sim->add_listener(std::move(scen));

    // 8. 初始化仿真
    cosmo::log::info("初始化仿真...");
    cosmo::fmi::fmi_status status = sim->initialize(0.0);
    if (status != cosmo::fmi::fmi_ok) {
        cosmo::log::error("初始化失败");
        return 1;
    }

    // 9. 执行仿真（10秒）
    cosmo::log::info("开始仿真（10秒）...");
    sim->step_until(10.0);

    // 10. 输出最终状态
    cosmo::log::info("仿真完成:");
    cosmo::log::info("  当前时间: " + std::to_string(sim->get_current_time()));
    cosmo::log::info("  总步数: " + std::to_string(sim->get_step_count()));
    cosmo::log::info("  模型数: " + std::to_string(sim->get_instances().size()));
    cosmo::log::info("  连接数: " + std::to_string(sim->get_connection_count()));

    // 11. 读取最终变量值
    cosmo::fmi::fmi_real pos = 0.0;
    sim->get_real("plant::position", pos);
    cosmo::log::info("  plant::position = " + std::to_string(pos));

    // 12. 终止仿真
    sim->terminate();
    cosmo::log::info("=== 仿真结束 ===");

    return 0;
}
