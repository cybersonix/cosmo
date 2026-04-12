#include "cosmo/listeners/csv_writer.hpp"
#include "cosmo/simulation/simulation.hpp"
#include "cosmo/simulation/model_instance.hpp"
#include "cosmo/logger/logger.hpp"
#include <sstream>
#include <iomanip>

namespace cosmo {
namespace listeners {

// csv_config implementation
csv_config::csv_config()
    : downsample_factor_(1)
    , clear_on_reset_(false)
{
}

void csv_config::register_variable(const std::string& variable_name)
{
    variables_.push_back(variable_name);
}

void csv_config::register_variable(const std::string& model_name, const std::string& variable_name)
{
    std::string full_name = model_name + "::" + variable_name;
    variables_.push_back(full_name);
}

void csv_config::set_downsample_factor(int factor)
{
    downsample_factor_ = factor > 0 ? factor : 1;
}

int csv_config::get_downsample_factor() const
{
    return downsample_factor_;
}

void csv_config::set_clear_on_reset(bool clear)
{
    clear_on_reset_ = clear;
}

bool csv_config::get_clear_on_reset() const
{
    return clear_on_reset_;
}

const std::vector<std::string>& csv_config::get_registered_variables() const
{
    return variables_;
}

// csv_writer implementation
csv_writer::csv_writer(const std::string& filename, const csv_config& config)
    : filename_(filename)
    , config_(config)
    , step_counter_(0)
    , header_written_(false)
    , bound_sim_(nullptr)
{
    variables_ = config_.get_registered_variables();
}

csv_writer::~csv_writer()
{
    if (file_.is_open()) {
        file_.close();
    }
}

void csv_writer::on_pre_init(double start_time)
{
    cosmo::log::info("CSV writer: Pre-init at time " + std::to_string(start_time));
    
    // 如果文件已经打开，先关闭
    if (file_.is_open()) {
        file_.close();
    }
    
    // 打开文件（追加模式或截断模式取决于配置）
    std::ios_base::openmode mode = std::ios::out;
    if (!config_.get_clear_on_reset() && header_written_) {
        mode |= std::ios::app; // 追加模式
    } else {
        mode |= std::ios::trunc; // 截断模式
        header_written_ = false;
    }
    
    file_.open(filename_, mode);
    if (!file_.is_open()) {
        cosmo::log::error("Failed to open CSV file: " + filename_);
        return;
    }
    
    // 写入头部（仅在需要时）
    if (!header_written_) {
        write_header();
    }
}

void csv_writer::on_post_init(double start_time)
{
    cosmo::log::info("CSV writer: Post-init at time " + std::to_string(start_time));

    // 写入初始数据行（时间0）
    if (file_.is_open()) {
        std::map<std::string, double> values;
        if (bound_sim_) {
            for (const auto& var : variables_) {
                if (var == "time") {
                    values[var] = start_time;
                } else {
                    fmi::fmi_real val = 0.0;
                    if (bound_sim_->get_real(var, val) == fmi::fmi_ok) {
                        values[var] = val;
                    } else {
                        fmi::fmi_integer int_val = 0;
                        if (bound_sim_->get_integer(var, int_val) == fmi::fmi_ok) {
                            values[var] = static_cast<double>(int_val);
                        } else {
                            values[var] = 0.0;
                        }
                    }
                }
            }
        } else {
            for (const auto& var : variables_) {
                values[var] = 0.0;
            }
        }
        write_row(start_time, values);
    }
}

void csv_writer::on_pre_step(double /*current_time*/, double /*step_size*/)
{
    // 步进前不做特殊处理
}

void csv_writer::on_post_step(double current_time, double /*step_size*/)
{
    step_counter_++;
    
    // 检查降采样 - 当降采样因子为1时写入每一步，否则按因子降采样
    int factor = config_.get_downsample_factor();
    if (factor > 1 && (step_counter_ % factor != 0)) {
        return;
    }
    
    // 写入数据行
    std::map<std::string, double> values;
    if (bound_sim_) {
        for (const auto& var : variables_) {
            if (var == "time") {
                values[var] = current_time;
            } else {
                fmi::fmi_real val = 0.0;
                if (bound_sim_->get_real(var, val) == fmi::fmi_ok) {
                    values[var] = val;
                } else {
                    fmi::fmi_integer int_val = 0;
                    if (bound_sim_->get_integer(var, int_val) == fmi::fmi_ok) {
                        values[var] = static_cast<double>(int_val);
                    } else {
                        values[var] = 0.0;
                    }
                }
            }
        }
    } else {
        for (const auto& var : variables_) {
            values[var] = current_time;
        }
    }

    write_row(current_time, values);
}

void csv_writer::on_pre_terminate()
{
    cosmo::log::info("CSV writer: Pre-terminate");
}

void csv_writer::on_post_terminate()
{
    cosmo::log::info("CSV writer: Post-terminate");
    
    if (file_.is_open()) {
        file_.close();
    }
}

void csv_writer::on_reset()
{
    cosmo::log::info("CSV writer: Reset");
    
    step_counter_ = 0;
    last_values_.clear();
    
    if (config_.get_clear_on_reset() && file_.is_open()) {
        file_.close();
        file_.open(filename_);
        if (file_.is_open()) {
            header_written_ = false;
            write_header();
        }
    }
}

void csv_writer::set_simulation(simulation::simulation* sim)
{
    bound_sim_ = sim;
}

void csv_writer::set_owner(simulation::simulation* owner)
{
    set_simulation(owner);
}

std::string csv_writer::get_name() const
{
    return "csv_writer";
}

const std::string& csv_writer::get_output_path() const
{
    return filename_;
}

void csv_writer::write_header()
{
    if (!file_.is_open() || header_written_) {
        return;
    }
    
    // 写入变量名作为头部
    for (size_t i = 0; i < variables_.size(); ++i) {
        if (i > 0) {
            file_ << ",";
        }
        file_ << variables_[i];
    }
    file_ << "\n";
    
    header_written_ = true;
}

void csv_writer::write_row(double time, const std::map<std::string, double>& values)
{
    if (!file_.is_open()) {
        return;
    }
    
    // 写入变量值
    for (size_t i = 0; i < variables_.size(); ++i) {
        if (i > 0) {
            file_ << ",";
        }
        
        const auto& var = variables_[i];
        if (var == "time") {
            file_ << std::fixed << std::setprecision(6) << time;
        } else {
            auto it = values.find(var);
            if (it != values.end()) {
                file_ << std::fixed << std::setprecision(6) << it->second;
            } else {
                file_ << "0.000000";
            }
        }
    }
    file_ << "\n";
    
    // 更新最后的值
    last_values_ = values;
}

std::pair<std::string, std::string> csv_writer::parse_variable_name(const std::string& full_name) const
{
    size_t pos = full_name.find("::");
    if (pos != std::string::npos) {
        return {full_name.substr(0, pos), full_name.substr(pos + 2)};
    }
    return {"", full_name};
}

// 工厂函数
std::unique_ptr<csv_writer> create_csv_writer(const std::string& filename, const csv_config& config)
{
    return std::unique_ptr<csv_writer>(new csv_writer(filename, config));
}

} // namespace listeners
} // namespace cosmo