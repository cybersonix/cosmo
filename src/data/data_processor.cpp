#include "cosmo/data/data_processor.hpp"
#include "cosmo/logger/logger.hpp"
#include "cosmo/util/filesystem.hpp"

#include <cmath>
#include <limits>
#include <sstream>
#include <fstream>

namespace cosmo {
namespace data {

// time_series实现
time_series::time_series(const std::string& name)
    : name_(name)
{
}

void time_series::add_point(double time, double value)
{
    points_.emplace_back(time, value);
}

void time_series::add_point(const data_point& point)
{
    points_.push_back(point);
}

const std::vector<data_point>& time_series::get_points() const
{
    return points_;
}

const std::string& time_series::get_name() const
{
    return name_;
}

void time_series::set_name(const std::string& name)
{
    name_ = name;
}

size_t time_series::size() const
{
    return points_.size();
}

void time_series::clear()
{
    points_.clear();
}

// data_processor实现
void data_processor::add_time_series(std::unique_ptr<time_series> series)
{
    if (series) {
        std::string name = series->get_name();
        series_[name] = std::move(series);
        cosmo::log::info("Added time series: " + name);
    }
}

time_series* data_processor::get_time_series(const std::string& name)
{
    auto it = series_.find(name);
    if (it != series_.end()) {
        return it->second.get();
    }
    return nullptr;
}

const time_series* data_processor::get_time_series(const std::string& name) const
{
    auto it = series_.find(name);
    if (it != series_.end()) {
        return it->second.get();
    }
    return nullptr;
}

void data_processor::remove_time_series(const std::string& name)
{
    auto it = series_.find(name);
    if (it != series_.end()) {
        series_.erase(it);
        cosmo::log::info("Removed time series: " + name);
    }
}

processing_result data_processor::process_series(const std::string& name)
{
    processing_result result;
    result.name = name;
    
    const time_series* series = get_time_series(name);
    if (!series) {
        cosmo::log::error("Time series not found: " + name);
        return result;
    }
    
    const std::vector<data_point>& points = series->get_points();
    if (points.empty()) {
        cosmo::log::warn("Time series is empty: " + name);
        return result;
    }
    
    // 计算最小值、最大值和平均值
    double min_val = std::numeric_limits<double>::max();
    double max_val = std::numeric_limits<double>::lowest();
    double sum = 0.0;
    
    for (const auto& point : points) {
        if (point.value < min_val) min_val = point.value;
        if (point.value > max_val) max_val = point.value;
        sum += point.value;
    }
    
    result.min_value = min_val;
    result.max_value = max_val;
    result.mean_value = sum / points.size();
    
    // 计算标准差
    double sum_sq_diff = 0.0;
    for (const auto& point : points) {
        double diff = point.value - result.mean_value;
        sum_sq_diff += diff * diff;
    }
    
    result.stddev_value = std::sqrt(sum_sq_diff / points.size());
    
    return result;
}

std::unique_ptr<time_series> data_processor::merge_series(const std::string& name1, const std::string& name2, const std::string& result_name)
{
    const time_series* series1 = get_time_series(name1);
    const time_series* series2 = get_time_series(name2);
    
    if (!series1 || !series2) {
        cosmo::log::error("One or both time series not found");
        return nullptr;
    }
    
    auto result_series = std::unique_ptr<time_series>(new time_series(result_name));
    
    // 简化的合并逻辑：交替添加点
    const auto& points1 = series1->get_points();
    const auto& points2 = series2->get_points();
    
    size_t max_size = std::max(points1.size(), points2.size());
    for (size_t i = 0; i < max_size; ++i) {
        if (i < points1.size()) {
            result_series->add_point(points1[i]);
        }
        if (i < points2.size()) {
            result_series->add_point(points2[i]);
        }
    }
    
    return result_series;
}

bool data_processor::export_to_csv(const std::string& series_name, const std::string& filename)
{
    const time_series* series = get_time_series(series_name);
    if (!series) {
        cosmo::log::error("Time series not found: " + series_name);
        return false;
    }
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        cosmo::log::error("Failed to open file for writing: " + filename);
        return false;
    }
    
    // 写入CSV头部
    file << "time,value\n";
    
    // 写入数据点
    const auto& points = series->get_points();
    for (const auto& point : points) {
        file << point.time << "," << point.value << "\n";
    }
    
    file.close();
    cosmo::log::info("Exported time series '" + series_name + "' to CSV file: " + filename);
    return true;
}

std::unique_ptr<time_series> data_processor::import_from_csv(const std::string& filename, const std::string& series_name)
{
    if (!cosmo::util::file_exists(filename)) {
        cosmo::log::error("CSV file not found: " + filename);
        return nullptr;
    }
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        cosmo::log::error("Failed to open file for reading: " + filename);
        return nullptr;
    }
    
    auto series = std::unique_ptr<time_series>(new time_series(series_name));
    
    std::string line;
    // 跳过头部行
    std::getline(file, line);
    
    // 读取数据点
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string time_str, value_str;
        
        if (std::getline(iss, time_str, ',') && std::getline(iss, value_str)) {
            try {
                double time = std::stod(time_str);
                double value = std::stod(value_str);
                series->add_point(time, value);
            } catch (const std::exception& e) {
                cosmo::log::warn("Failed to parse CSV line: " + line);
            }
        }
    }
    
    file.close();
    cosmo::log::info("Imported time series '" + series_name + "' from CSV file: " + filename);
    return series;
}

// 工厂函数实现
std::unique_ptr<data_processor> create_data_processor()
{
    return std::unique_ptr<data_processor>(new data_processor());
}

} // namespace data
} // namespace cosmo