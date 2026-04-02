/**
 * @file cli_main.cpp
 * @brief COSMO命令行工具入口
 *
 * 提供COSMO仿真引擎的命令行交互界面。
 * 支持从配置文件加载仿真结构并执行联合仿真。
 */

#include "cosmo/cosmo.h"
#include "cosmo/logger/logger.hpp"

#include <iostream>
#include <string>
#include <cstdlib>

/**
 * @brief COSMO 启动横幅（ASCII Art）
 */
static void print_banner()
{
    const char* banner =
        "\n"
        "    ╔═══════════════════════════════════════════════════════╗\n"
        "    ║                                                       ║\n"
        "    ║      ██████╗ ██████╗ ███████╗███╗   ███╗ ██████╗      ║\n"
        "    ║     ██╔════╝██╔═══██╗██╔════╝████╗ ████║██╔═══██╗     ║\n"
        "    ║     ██║     ██║   ██║███████╗██╔████╔██║██║   ██║     ║\n"
        "    ║     ██║     ██║   ██║╚════██║██║╚██╔╝██║██║   ██║     ║\n"
        "    ║     ╚██████╗╚██████╔╝███████║██║ ╚═╝ ██║╚██████╔╝     ║\n"
        "    ║      ╚═════╝ ╚═════╝ ╚══════╝╚═╝     ╚═╝ ╚═════╝      ║\n"
        "    ║                                                       ║\n"
        "    ║    Collaborative Open-Source Simulation & Modeling    ║\n"
        "    ║   多物理场联合仿真平台                                ║\n"
        "    ║                                                       ║\n"
        "    ╚═══════════════════════════════════════════════════════╝\n";

    std::cout << "\033[36m" << banner << "\033[0m";
}

/**
 * @brief 显示项目信息摘要
 */
static void print_info()
{
    cosmo_version v = cosmo_library_version();

    std::cout << "\033[1m  版  本: \033[0m\033[33m"
              << v.major << "." << v.minor << "." << v.patch
              << "\033[0m\n"
              << "\033[1m  协  议: \033[0mMIT\n"
              << "\033[1m  FMI 支持: \033[0m1.0 / 2.0 / 3.0\n"
              << std::endl;
}

/**
 * @brief 显示快速命令提示
 */
static void print_quick_commands(const char* program)
{
    std::cout << "\033[1m  常用命令:\033[0m\n"
              << "    " << program << " -h              显示帮助信息\n"
              << "    " << program << " -v              显示版本信息\n"
              << "    " << program << " --step 0.001 --duration 60   运行仿真\n"
              << "    " << program << " --log debug     启用调试日志\n"
              << std::endl;
}

/**
 * @brief 显示欢迎信息（完整启动画面）
 */
static void print_welcome(const char* program)
{
    print_banner();
    print_info();
    print_quick_commands(program);

    std::cout << "\033[90m  ─────────────────────────────────────────────\033[0m\n"
              << std::endl;
}

static void print_usage(const char* program)
{
    std::cout << "\n"
              << "\033[1mCOSMO\033[0m - 多物理场联合仿真平台\n"
              << "版本 " << cosmo_library_version_string() << "\n\n"
              << "\033[1m用法:\033[0m " << program << " [选项]\n\n"
              << "\033[1m选项:\033[0m\n"
              << "  -h, --help            显示帮助信息\n"
              << "  -v, --version         显示版本信息\n"
              << "  --log <级别>          设置日志级别 (trace/debug/info/warn/error/off)\n"
              << "  --structure <文件>    从仿真结构文件创建仿真（预留）\n"
              << "  --ssp <路径>          从SSP文件创建仿真（预留）\n"
              << "  --step <步长>         设置仿真步长（默认0.01）\n"
              << "  --duration <秒数>     设置仿真时长（默认1.0）\n"
              << std::endl;
}

static void print_version()
{
    cosmo_version v = cosmo_library_version();
    print_banner();
    std::cout << "  \033[1m版  本:\033[0m " << v.major << "." << v.minor << "." << v.patch << "\n"
              << "  \033[1m描  述:\033[0m 多FMU联合仿真引擎\n"
              << "  \033[1m版  权:\033[0m (c) 2026 COSMO 开发团队\n"
              << "  \033[1m协  议:\033[0m MIT\n"
              << std::endl;
}

/**
 * @brief 解析命令行参数并执行仿真
 */
int main(int argc, char* argv[])
{
    // 无参数时显示欢迎画面后退出
    if (argc <= 1) {
        print_welcome(argv[0]);
        return 0;
    }

    double step_size = 0.01;
    double duration = 1.0;
    std::string log_level = "info";

    // 简单的命令行参数解析
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            print_version();
            return 0;
        } else if (arg == "--log" && i + 1 < argc) {
            log_level = argv[++i];
        } else if (arg == "--step" && i + 1 < argc) {
            step_size = std::stod(argv[++i]);
        } else if (arg == "--duration" && i + 1 < argc) {
            duration = std::stod(argv[++i]);
        } else {
            std::cerr << "\033[31m错误: 未知选项: \033[0m" << arg << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }

    // 设置日志级别
    cosmo_set_log_level(log_level.c_str());

    cosmo::log::info("COSMO 联合仿真引擎 v" +
                     std::string(cosmo_library_version_string()));
    cosmo::log::info("仿真步长: " + std::to_string(step_size) +
                     ", 仿真时长: " + std::to_string(duration));

    // 创建仿真结构
    cosmo_simulation_structure_t* ss = cosmo_simulation_structure_create();
    if (!ss) {
        std::cerr << "创建仿真结构失败" << std::endl;
        return 1;
    }

    // 添加示例模型
    cosmo_simulation_structure_add_model(ss, "model1", "example.fmu");
    cosmo_simulation_structure_register_variable(ss, "model1", "real", "output");
    cosmo_simulation_structure_register_variable(ss, "model1", "real", "state");

    // 创建仿真
    cosmo_simulation_t* sim = cosmo_simulation_create_from_structure(ss, step_size);
    if (!sim) {
        std::cerr << "创建仿真失败: " << cosmo_last_error_msg() << std::endl;
        cosmo_simulation_structure_destroy(ss);
        return 1;
    }

    // 执行仿真
    int ret = cosmo_simulation_initialize(sim, 0.0);
    if (ret != 0) {
        std::cerr << "初始化仿真失败" << std::endl;
        cosmo_simulation_destroy(sim);
        cosmo_simulation_structure_destroy(ss);
        return 1;
    }

    cosmo::log::info("开始仿真...");
    cosmo_simulation_step_until(sim, duration);
    cosmo::log::info("仿真完成, 当前时间: " +
                     std::to_string(cosmo_simulation_get_time(sim)));

    cosmo_simulation_terminate(sim);

    // 清理
    cosmo_simulation_destroy(sim);
    cosmo_simulation_structure_destroy(ss);

    cosmo::log::info("仿真运行结束");
    return 0;
}
