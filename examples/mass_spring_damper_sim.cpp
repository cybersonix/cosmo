/**
 * @file mass_spring_damper_sim.cpp
 * @brief 质量弹簧阻尼器联合仿真实例
 *
 * 使用三个FMU（Spring, Mass, Damper）进行联合仿真。
 * 仿真步长 1/100 秒，仿真到 80 秒。
 * 输出CSV文件包含所有连接变量的时间序列。
 *
 * @copyright Copyright (c) 2026 COSMO Development Team
 */

#include "cosmo/simulation/simulation_structure.hpp"
#include "cosmo/simulation/simulation.hpp"
#include "cosmo/simulation/model_instance.hpp"
#include "cosmo/simulation/fmi_model_instance.hpp"
#include "cosmo/fmi/model_description.hpp"
#include "cosmo/logger/logger.hpp"
#include "cosmo/util/filesystem.hpp"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <stdexcept>
#include <cmath>

using cosmo::simulation::create_simulation_structure;
using cosmo::simulation::fixed_step_algorithm;
using cosmo::simulation::simulation;
using cosmo::simulation::simulation_structure;
using cosmo::simulation::fmi_model_instance;
using cosmo::fmi::fmi_real;

static std::string get_fmu_dir()
{
    std::vector<std::string> search_paths = {
        "./data/fmus/mass_spring_damper",
        "../data/fmus/mass_spring_damper",
        "../../data/fmus/mass_spring_damper",
        "/home/sun/cosmo/cosmo/data/fmus/mass_spring_damper",
    };

    for (const auto& p : search_paths) {
        if (cosmo::util::directory_exists(p)) {
            return p;
        }
    }

    const char* env = std::getenv("COSMO_FMU_DIR");
    if (env) {
        return std::string(env);
    }

    throw std::runtime_error("Cannot find FMU directory");
}

// CSV output tracking
struct CsvTracker {
    std::ofstream file;
    std::string filename;
    bool header_written;

    // Variables to track
    std::vector<std::string> variables;

    CsvTracker(const std::string& fn) : filename(fn), header_written(false) {}

    void open() {
        file.open(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open CSV file: " + filename);
        }
    }

    void write_header() {
        if (!header_written) {
            file << "time";
            for (const auto& v : variables) {
                file << "," << v;
            }
            file << "\n";
            header_written = true;
        }
    }

    void write_row(double time, const std::vector<double>& values) {
        file << std::fixed << std::setprecision(6) << time;
        for (const auto& v : values) {
            file << "," << v;
        }
        file << "\n";
    }

    void close() {
        if (file.is_open()) file.close();
    }
};

static void setup_simulation(simulation_structure& ss, const std::string& dir)
{
    // Add models
    ss.add_model("spring", dir + "/Spring.fmu");
    ss.add_model("mass", dir + "/Mass.fmu");
    ss.add_model("damper", dir + "/Damper.fmu");

    // Add 8 connections (spring-mass and damper-mass loops)
    ss.make_connection("spring::for_xx", "mass::in_l_u");
    ss.make_connection("spring::for_yx", "mass::in_l_w");
    ss.make_connection("mass::out_l_u", "spring::dis_xx");
    ss.make_connection("mass::out_l_w", "spring::dis_yx");
    ss.make_connection("damper::df_0", "mass::in_f_u");
    ss.make_connection("damper::df_1", "mass::in_f_w");
    ss.make_connection("mass::out_f_u", "damper::lv_0");
    ss.make_connection("mass::out_f_w", "damper::lv_1");
}

int main(int argc, char* argv[])
{
    try {
        std::string csv_path = "mass_spring_damper_results.csv";
        double end_time = 10.0;  // Default: 10 seconds

        // Parse command line arguments
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--output" && i + 1 < argc) {
                csv_path = argv[++i];
            } else if (arg == "--time" && i + 1 < argc) {
                end_time = std::stod(argv[++i]);
            } else if (arg == "--help") {
                std::cout << "Usage: " << argv[0] << " [--output FILE] [--time T] [--help]\n"
                          << "  --output FILE  CSV output file (default: mass_spring_damper_results.csv)\n"
                          << "  --time T       Simulation end time in seconds (default: 10.0)\n";
                return 0;
            }
        }

        cosmo::log::info("========================================");
        cosmo::log::info("Mass-Spring-Damper Co-Simulation");
        cosmo::log::info("========================================");

        std::string dir = get_fmu_dir();
        cosmo::log::info("FMU directory: " + dir);

        // Create simulation structure
        auto ss = create_simulation_structure();
        setup_simulation(*ss, dir);

        // Load simulation
        double step_size = 1.0 / 100.0;
        auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
            new cosmo::simulation::fixed_step_algorithm(step_size));
        auto sim = ss->load(std::move(algo));

        if (!sim) {
            throw std::runtime_error("Failed to load simulation");
        }

        cosmo::log::info("Simulation loaded successfully");

        // Check if FMUs loaded (not mock)
        bool using_fmu = true;
        for (const auto& inst : sim->get_instances()) {
            auto* fmi_inst = dynamic_cast<fmi_model_instance*>(inst.get());
            if (!fmi_inst || !fmi_inst->is_fmu_loaded()) {
                using_fmu = false;
                break;
            }
        }
        cosmo::log::info(std::string("Using real FMUs: ") + (using_fmu ? "YES" : "NO (mock fallback)"));

        // Set parameters BEFORE initialization
        cosmo::log::info("Setting parameters...");
        sim->set_real("spring::springStiffness", 5.0);
        sim->set_real("spring::zeroForceLength", 5.0);
        sim->set_real("damper::dampingCoefficient", 2.0);
        sim->set_real("mass::initialPositionX", 6.0);
        sim->set_real("mass::mediumDensity", 1.0);
        sim->set_real("mass::gravity", -9.81);
        sim->set_real("mass::mass", 10.0);

        // Initialize
        auto init_status = sim->initialize(0.0);
        if (init_status != cosmo::fmi::fmi_ok) {
            throw std::runtime_error("Simulation initialization failed");
        }
        cosmo::log::info("Simulation initialized");

        // Do initial data transfer: set spring/damper inputs from mass initial outputs
        // This ensures the first step has correct inputs
        {
            double posX = 0, posY = 0, velX = 0, velY = 0;
            sim->get_real("mass::out_l_u", posX);
            sim->get_real("mass::out_l_w", posY);
            sim->get_real("mass::out_f_u", velX);
            sim->get_real("mass::out_f_w", velY);
            sim->set_real("spring::dis_xx", posX);
            sim->set_real("spring::dis_yx", posY);
            sim->set_real("damper::lv_0", velX);
            sim->set_real("damper::lv_1", velY);
        }

        // Setup CSV output
        CsvTracker csv(csv_path);
        csv.variables = {
            "spring::for_xx", "spring::for_yx", "spring::dis_xx", "spring::dis_yx",
            "mass::in_l_u", "mass::in_l_w", "mass::in_f_u", "mass::in_f_w",
            "mass::out_l_u", "mass::out_l_w", "mass::out_f_u", "mass::out_f_w",
            "damper::df_0", "damper::df_1", "damper::lv_0", "damper::lv_1"
        };

        csv.open();
        csv.write_header();

        // Write initial state
        {
            std::vector<double> vals;
            vals.reserve(csv.variables.size());
            for (const auto& var : csv.variables) {
                double v = 0;
                sim->get_real(var, v);
                vals.push_back(v);
            }
            csv.write_row(0.0, vals);
        }

        cosmo::log::info("Running simulation to t=" + std::to_string(end_time) + "s (dt=" + std::to_string(step_size) + ")...");

        // Step loop
        size_t total_steps = static_cast<size_t>(end_time / step_size);
        size_t report_interval = std::max(static_cast<size_t>(total_steps / 10), static_cast<size_t>(1));

        for (size_t i = 0; i < total_steps; i++) {
            auto status = sim->step();
            if (status != cosmo::fmi::fmi_ok) {
                cosmo::log::error("Step failed at step " + std::to_string(i));
                break;
            }

            // Write to CSV (every step)
            {
                std::vector<double> vals;
                vals.reserve(csv.variables.size());
                for (const auto& var : csv.variables) {
                    double v = 0;
                    sim->get_real(var, v);
                    vals.push_back(v);
                }
                csv.write_row(sim->get_current_time(), vals);
            }

            // Progress reporting
            if ((i + 1) % report_interval == 0) {
                double pct = 100.0 * (i + 1) / total_steps;
                cosmo::log::info("  Progress: " + std::to_string(static_cast<int>(pct)) + "% (t="
                    + std::to_string(sim->get_current_time()) + "s)");
            }
        }

        // Terminate
        sim->terminate();
        csv.close();

        cosmo::log::info("========================================");
        cosmo::log::info("Simulation completed!");
        cosmo::log::info("  Total steps: " + std::to_string(sim->get_step_count()));
        cosmo::log::info("  End time: " + std::to_string(sim->get_current_time()) + "s");
        cosmo::log::info("  CSV output: " + csv_path);
        cosmo::log::info("  Using FMUs: " + std::string(using_fmu ? "YES" : "NO (mock - results not physically valid)"));
        cosmo::log::info("========================================");

        // Print final values summary
        if (using_fmu) {
            std::cout << "\nFinal values at t=" << sim->get_current_time() << "s:\n";
            for (const auto& var : csv.variables) {
                double v = 0;
                sim->get_real(var, v);
                std::cout << "  " << var << " = " << std::fixed << std::setprecision(6) << v << "\n";
            }
        }

        return 0;

    } catch (const std::exception& e) {
        cosmo::log::error(std::string("Simulation error: ") + e.what());
        return 1;
    }
}
