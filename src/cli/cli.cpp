#include "cosmo/cli/cli.hpp"
#include "cosmo/logger/logger.hpp"
#include "cosmo/simulation/simulation.hpp"
#include "cosmo/ssp/ssp_loader.hpp"
#include "cosmo/util/filesystem.hpp"

#include <iostream>
#include <algorithm>
#include <memory>

namespace cosmo {
namespace cli {

// simulate命令处理器
class simulate_command_handler : public command_handler
{
public:
    std::string get_command_name() const override
    {
        return "simulate";
    }
    
    std::string get_command_description() const override
    {
        return "Run co-simulation with FMU/SSP models";
    }
    
    int handle_command(const cli_args& args) override
    {
        // 解析参数
        std::string path;
        double start_time = 0.0;
        double stop_time = 10.0;
        double step_size = 0.01;
        
        for (const auto& option : args.options) {
            if (option.first == "--path" || option.first == "-p") {
                path = option.second;
            } else if (option.first == "--start-time" || option.first == "-s") {
                start_time = std::stod(option.second);
            } else if (option.first == "--stop-time" || option.first == "-t") {
                stop_time = std::stod(option.second);
            } else if (option.first == "--step-size" || option.first == "-d") {
                step_size = std::stod(option.second);
            }
        }
        
        if (path.empty()) {
            std::cerr << "Error: --path is required" << std::endl;
            return 1;
        }
        
        cosmo::log::info("Starting simulation");
        cosmo::log::info("Path: " + path);
        cosmo::log::info("Start time: " + std::to_string(start_time));
        cosmo::log::info("Stop time: " + std::to_string(stop_time));
        cosmo::log::info("Step size: " + std::to_string(step_size));
        
        try {
            // 创建仿真
            auto algorithm = std::unique_ptr<simulation::algorithm>(new simulation::fixed_step_algorithm(step_size));
            auto sim = std::unique_ptr<simulation::simulation>(new simulation::simulation(std::move(algorithm)));
            
            // 根据文件扩展名判断是FMU还是SSP
            std::string ext = util::file_extension(path);
            if (ext == ".ssp") {
                // 加载SSP
                auto ssp_desc = ssp::load_ssp(path);
                if (!ssp_desc) {
                    std::cerr << "Failed to load SSP: " << path << std::endl;
                    return 1;
                }
                
                cosmo::log::info("Loaded SSP with " + std::to_string(ssp_desc->get_components().size()) + " components");
            } else {
                // 单个FMU
                cosmo::log::info("Processing FMU: " + path);
            }
            
            // 初始化
            sim->initialize(start_time);
            
            // 运行仿真
            cosmo::log::info("Running simulation...");
            sim->step_until(stop_time);
            
            // 终止
            sim->terminate();
            
            cosmo::log::info("Simulation completed successfully");
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Simulation failed: " << e.what() << std::endl;
            return 1;
        }
    }
};

cli::cli(const std::string& program_name)
    : program_name_(program_name)
{
    cosmo::log::info("Creating CLI with program name: " + program_name);
    
    // 注册默认命令处理器
    register_command_handler(std::unique_ptr<command_handler>(new simulate_command_handler()));
}

void cli::register_command_handler(std::unique_ptr<command_handler> handler)
{
    if (handler) {
        handlers_.push_back(std::move(handler));
        cosmo::log::info("Registered command handler: " + handlers_.back()->get_command_name());
    }
}

cli_args cli::parse_arguments(int argc, char* argv[])
{
    cli_args args;
    
    // 如果没有参数，返回空参数
    if (argc <= 1) {
        return args;
    }
    
    // 第一个参数是命令
    args.command = argv[1];
    
    // 解析其余参数
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        
        // 检查是否是选项（以-开头）
        if (arg.length() > 1 && arg[0] == '-') {
            // 处理选项
            std::string option_name = arg;
            std::string option_value = "";
            
            // 检查是否有值（下一个参数不是选项）
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                option_value = argv[i + 1];
                ++i; // 跳过下一个参数
            }
            
            args.options.push_back({option_name, option_value});
        } else {
            // 普通参数
            args.arguments.push_back(arg);
        }
    }
    
    return args;
}

int cli::execute(const cli_args& args)
{
    // 如果没有命令，显示帮助
    if (args.command.empty()) {
        show_help();
        return 0;
    }
    
    // 查找匹配的命令处理器
    for (const auto& handler : handlers_) {
        if (handler->get_command_name() == args.command) {
            return handler->handle_command(args);
        }
    }
    
    // 如果没有找到匹配的命令处理器，显示错误
    std::cerr << "Unknown command: " << args.command << std::endl;
    show_help();
    return 1;
}

void cli::show_help()
{
    std::cout << "Usage: " << program_name_ << " <command> [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Available commands:" << std::endl;
    
    for (const auto& handler : handlers_) {
        std::cout << "  " << handler->get_command_name() << " - " << handler->get_command_description() << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "Global options:" << std::endl;
    std::cout << "  -h, --help     Show this help message" << std::endl;
    std::cout << "  -v, --version  Show version information" << std::endl;
}

int cli::run(int argc, char* argv[])
{
    // 检查帮助选项
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            show_help();
            return 0;
        }
        
        if (arg == "-v" || arg == "--version") {
            std::cout << program_name_ << " version 0.1.0" << std::endl;
            return 0;
        }
    }
    
    // 解析参数
    cli_args args = parse_arguments(argc, argv);
    
    // 执行命令
    return execute(args);
}

// 工厂函数实现
std::unique_ptr<cli> create_cli(const std::string& program_name)
{
    return std::unique_ptr<cli>(new cli(program_name));
}

} // namespace cli
} // namespace cosmo
