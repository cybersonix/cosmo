#include "cosmo/data/data_processor.hpp"
#include "cosmo/test/test_framework.hpp"
#include "cosmo/logger/logger.hpp"

using namespace cosmo::test;
using namespace cosmo::data;

TEST_CASE(time_series_basic)
{
    // 创建时间序列
    time_series series("test_series");
    REQUIRE_EQ(series.get_name(), "test_series");
    REQUIRE_EQ(series.size(), 0);
    
    // 添加数据点
    series.add_point(0.0, 1.0);
    series.add_point(1.0, 2.0);
    series.add_point(2.0, 3.0);
    
    REQUIRE_EQ(series.size(), 3);
    
    // 检查数据点
    const auto& points = series.get_points();
    REQUIRE_EQ(points[0].time, 0.0);
    REQUIRE_EQ(points[0].value, 1.0);
    REQUIRE_EQ(points[1].time, 1.0);
    REQUIRE_EQ(points[1].value, 2.0);
    REQUIRE_EQ(points[2].time, 2.0);
    REQUIRE_EQ(points[2].value, 3.0);
    
    // 修改名称
    series.set_name("new_name");
    REQUIRE_EQ(series.get_name(), "new_name");
    
    // 清空数据
    series.clear();
    REQUIRE_EQ(series.size(), 0);
}

TEST_CASE(data_processor_basic)
{
    // 创建数据处理器
    auto processor = create_data_processor();
    REQUIRE(processor != nullptr);
    
    // 创建时间序列
    auto series1 = std::unique_ptr<time_series>(new time_series("series1"));
    series1->add_point(0.0, 1.0);
    series1->add_point(1.0, 2.0);
    series1->add_point(2.0, 3.0);
    
    auto series2 = std::unique_ptr<time_series>(new time_series("series2"));
    series2->add_point(0.0, 4.0);
    series2->add_point(1.0, 5.0);
    series2->add_point(2.0, 6.0);
    
    // 添加时间序列到处理器
    processor->add_time_series(std::move(series1));
    processor->add_time_series(std::move(series2));
    
    // 检查时间序列是否添加成功
    time_series* retrieved_series1 = processor->get_time_series("series1");
    REQUIRE(retrieved_series1 != nullptr);
    REQUIRE_EQ(retrieved_series1->size(), 3);
    
    time_series* retrieved_series2 = processor->get_time_series("series2");
    REQUIRE(retrieved_series2 != nullptr);
    REQUIRE_EQ(retrieved_series2->size(), 3);
    
    // 处理数据
    processing_result result1 = processor->process_series("series1");
    REQUIRE_EQ(result1.name, "series1");
    REQUIRE_EQ(result1.min_value, 1.0);
    REQUIRE_EQ(result1.max_value, 3.0);
    REQUIRE_EQ(result1.mean_value, 2.0);
    
    processing_result result2 = processor->process_series("series2");
    REQUIRE_EQ(result2.name, "series2");
    REQUIRE_EQ(result2.min_value, 4.0);
    REQUIRE_EQ(result2.max_value, 6.0);
    REQUIRE_EQ(result2.mean_value, 5.0);
    
    // 合并时间序列
    auto merged_series = processor->merge_series("series1", "series2", "merged");
    REQUIRE(merged_series != nullptr);
    REQUIRE_EQ(merged_series->size(), 6);
    
    // 添加合并后的时间序列
    processor->add_time_series(std::move(merged_series));
    
    time_series* retrieved_merged = processor->get_time_series("merged");
    REQUIRE(retrieved_merged != nullptr);
    REQUIRE_EQ(retrieved_merged->size(), 6);
}

TEST_CASE(csv_export_import)
{
    // 创建数据处理器
    auto processor = create_data_processor();
    REQUIRE(processor != nullptr);
    
    // 创建时间序列
    auto series = std::unique_ptr<time_series>(new time_series("test_series"));
    series->add_point(0.0, 1.0);
    series->add_point(1.0, 2.0);
    series->add_point(2.0, 3.0);
    
    // 添加时间序列到处理器
    processor->add_time_series(std::move(series));
    
    // 导出到CSV
    std::string filename = "test_data.csv";
    REQUIRE(processor->export_to_csv("test_series", filename));
    
    // 从CSV导入
    auto imported_series = processor->import_from_csv(filename, "imported_series");
    REQUIRE(imported_series != nullptr);
    REQUIRE_EQ(imported_series->size(), 3);
    
    const auto& points = imported_series->get_points();
    REQUIRE_EQ(points[0].time, 0.0);
    REQUIRE_EQ(points[0].value, 1.0);
    REQUIRE_EQ(points[1].time, 1.0);
    REQUIRE_EQ(points[1].value, 2.0);
    REQUIRE_EQ(points[2].time, 2.0);
    REQUIRE_EQ(points[2].value, 3.0);
    
    // 清理测试文件
    remove(filename.c_str());
}

int main()
{
    cosmo::log::info("Starting data processing test");
    return RUN_ALL_TESTS();
}