#ifndef COSMO_DATA_DATA_PROCESSOR_HPP
#define COSMO_DATA_DATA_PROCESSOR_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace cosmo {
namespace data {

// 数据点结构
struct data_point
{
    double time;
    double value;
    
    data_point(double t = 0.0, double v = 0.0) : time(t), value(v) {}
};

// 时间序列数据
class time_series
{
private:
    std::string name_;
    std::vector<data_point> points_;
    
public:
    explicit time_series(const std::string& name = "");
    
    // 添加数据点
    void add_point(double time, double value);
    void add_point(const data_point& point);
    
    // 获取数据点
    const std::vector<data_point>& get_points() const;
    
    // 获取时间序列名称
    const std::string& get_name() const;
    
    // 设置时间序列名称
    void set_name(const std::string& name);
    
    // 获取数据点数量
    size_t size() const;
    
    // 清空数据
    void clear();
};

// 数据处理结果
struct processing_result
{
    std::string name;
    double min_value;
    double max_value;
    double mean_value;
    double stddev_value;
    
    processing_result()
        : min_value(0.0), max_value(0.0), mean_value(0.0), stddev_value(0.0) {}
};

// 数据处理器类
class data_processor
{
private:
    std::map<std::string, std::unique_ptr<time_series>> series_;
    
public:
    // 添加时间序列
    void add_time_series(std::unique_ptr<time_series> series);
    
    // 获取时间序列
    time_series* get_time_series(const std::string& name);
    const time_series* get_time_series(const std::string& name) const;
    
    // 移除时间序列
    void remove_time_series(const std::string& name);
    
    // 处理数据
    processing_result process_series(const std::string& name);
    
    // 合并时间序列
    std::unique_ptr<time_series> merge_series(const std::string& name1, const std::string& name2, const std::string& result_name);
    
    // 导出数据到CSV
    bool export_to_csv(const std::string& series_name, const std::string& filename);
    
    // 从CSV导入数据
    std::unique_ptr<time_series> import_from_csv(const std::string& filename, const std::string& series_name);
};

// 工厂函数
std::unique_ptr<data_processor> create_data_processor();

} // namespace data
} // namespace cosmo

#endif // COSMO_DATA_DATA_PROCESSOR_HPP