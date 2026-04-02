#ifndef COSMO_LISTENERS_CSV_WRITER_HPP
#define COSMO_LISTENERS_CSV_WRITER_HPP

#include "cosmo/simulation/simulation_listener.hpp"
#include "cosmo/fmi/fmi_types.hpp"
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <memory>

namespace cosmo {

// 前向声明
namespace simulation { class simulation; }

namespace listeners {

// CSV配置类
class csv_config
{
public:
    csv_config();
    ~csv_config() = default;
    
    // 注册变量
    void register_variable(const std::string& variable_name);
    void register_variable(const std::string& model_name, const std::string& variable_name);
    
    // 设置降采样因子
    void set_downsample_factor(int factor);
    int get_downsample_factor() const;
    
    // 设置重置时清空文件
    void set_clear_on_reset(bool clear);
    bool get_clear_on_reset() const;
    
    // 获取注册的变量列表
    const std::vector<std::string>& get_registered_variables() const;
    
private:
    std::vector<std::string> variables_;
    int downsample_factor_;
    bool clear_on_reset_;
};

// CSV写入监听器
class csv_writer : public simulation::simulation_listener
{
public:
    csv_writer(const std::string& filename, const csv_config& config);
    ~csv_writer();
    
    // 监听器接口实现
    void on_pre_init(double start_time) override;
    void on_post_init(double start_time) override;
    void on_pre_step(double current_time, double step_size) override;
    void on_post_step(double current_time, double step_size) override;
    void on_pre_terminate() override;
    void on_post_terminate() override;
    void on_reset() override;
    
    std::string get_name() const override;
    void set_owner(simulation::simulation* owner) override;

    // 设置关联的仿真实例（必须在仿真 initialize 之前调用）
    void set_simulation(simulation::simulation* sim);

    // 获取输出文件路径
    const std::string& get_output_path() const;
    
private:
    std::string filename_;
    csv_config config_;
    std::ofstream file_;
    std::vector<std::string> variables_;
    std::map<std::string, double> last_values_;
    int step_counter_;
    bool header_written_;
    simulation::simulation* bound_sim_;
    
    // 写入CSV头部
    void write_header();
    
    // 写入数据行
    void write_row(double time, const std::map<std::string, double>& values);
    
    // 解析变量名
    std::pair<std::string, std::string> parse_variable_name(const std::string& full_name) const;
};

// 工厂函数
std::unique_ptr<csv_writer> create_csv_writer(const std::string& filename, const csv_config& config);

} // namespace listeners
} // namespace cosmo

#endif // COSMO_LISTENERS_CSV_WRITER_HPP