/**
 * @file boundary_test.cpp
 * @brief 边界条件测试
 * 
 * 测试各种边界条件和异常情况，确保代码的健壮性。
 * 
 * @copyright Copyright (c) 2024
 */

#include "cosmo/test/test_framework.hpp"
#include "cosmo/logger/logger.hpp"
#include "cosmo/util/xml_parser.hpp"
#include "cosmo/fmi/model_description.hpp"
#include "cosmo/simulation/simulation_structure.hpp"
#include "cosmo/data/data_processor.hpp"

#include <limits>
#include <cmath>

using namespace cosmo;

/**
 * @brief 测试XML解析器边界条件
 */
TEST_CASE(xml_parser_boundary)
{
    // 空字符串解析应该返回false
    {
        auto doc = util::create_xml_document();
        bool result = doc->load_string("");
        REQUIRE(!result);
    }
    
    // 无效XML格式
    {
        auto doc = util::create_xml_document();
        bool result = doc->load_string("<invalid>");
        REQUIRE(!result);
    }
    
    // 只包含空白字符应该返回false
    {
        auto doc = util::create_xml_document();
        bool result = doc->load_string("   \n\t  ");
        REQUIRE(!result);
    }
    
    // 空节点访问
    {
        auto doc = util::create_xml_document();
        bool result = doc->load_string("<root></root>");
        REQUIRE(result);
        
        auto root = doc->get_root();
        REQUIRE(root != nullptr);
        
        // 获取不存在的子节点
        auto child = root->get_child("nonexistent");
        REQUIRE(child == nullptr);
        
        // 获取不存在的属性
        auto attr = root->get_attribute("nonexistent", "default");
        REQUIRE_EQ(attr, std::string("default"));
    }
}

/**
 * @brief 测试模型描述边界条件
 */
TEST_CASE(model_description_boundary)
{
    // 创建模型描述
    auto md = fmi::create_model_description();
    REQUIRE(md != nullptr);
    
    // 初始状态应为空
    REQUIRE(md->get_model_name().empty());
    REQUIRE(md->get_guid().empty());
}

/**
 * @brief 测试变量描述边界条件
 */
TEST_CASE(variable_boundary)
{
    // 最小变量引用
    {
        fmi::variable_description var;
        var.name = "var0";
        var.type = fmi::variable_type::real;
        var.vr = 0;
        REQUIRE_EQ(var.vr, 0);
    }
    
    // 最大变量引用
    {
        fmi::variable_description var;
        var.name = "var_max";
        var.type = fmi::variable_type::integer;
        var.vr = 0xFFFFFFFF;
        REQUIRE_EQ(var.vr, 0xFFFFFFFF);
    }
    
    // 空变量名
    {
        fmi::variable_description var;
        var.name = "";
        var.type = fmi::variable_type::boolean;
        var.vr = 0;
        REQUIRE_EQ(var.name, std::string(""));
    }
}

/**
 * @brief 测试仿真结构边界条件
 */
TEST_CASE(simulation_structure_boundary)
{
    simulation::simulation_structure structure;
    
    // 空实例名
    {
        auto desc = fmi::create_model_description();
        auto instance = simulation::create_model_instance("", std::move(desc), 0.01);
        REQUIRE(instance != nullptr);
        REQUIRE_EQ(instance->get_instance_name(), std::string(""));
    }
    
    // 零步长提示
    {
        auto desc = fmi::create_model_description();
        auto instance = simulation::create_model_instance("test", std::move(desc), 0.0);
        REQUIRE(instance != nullptr);
        REQUIRE_EQ(instance->get_step_size_hint(), 0.0);
    }
    
    // 极大步长
    {
        auto desc = fmi::create_model_description();
        auto instance = simulation::create_model_instance("test", std::move(desc), 1e10);
        REQUIRE(instance != nullptr);
        REQUIRE_EQ(instance->get_step_size_hint(), 1e10);
    }
}

/**
 * @brief 测试变量连接边界条件
 */
TEST_CASE(variable_connection_boundary)
{
    // 空变量名
    {
        simulation::variable_connection conn("", "", "", "");
        REQUIRE_EQ(conn.get_source_model(), std::string(""));
        REQUIRE_EQ(conn.get_source_variable(), std::string(""));
    }
    
    // 相同源和目标
    {
        simulation::variable_connection conn("model1", "var1", "model1", "var1");
        REQUIRE_EQ(conn.get_source_model(), conn.get_target_model());
        REQUIRE_EQ(conn.get_source_variable(), conn.get_target_variable());
    }
}

/**
 * @brief 测试日志级别边界
 */
TEST_CASE(logger_level_boundary)
{
    // 验证设置日志级别后，级别确实生效
    log::set_logging_level(log::debug_level);
    REQUIRE_EQ(static_cast<int>(log::get_logging_level()), static_cast<int>(log::debug_level));

    log::set_logging_level(log::err_level);
    REQUIRE_EQ(static_cast<int>(log::get_logging_level()), static_cast<int>(log::err_level));

    log::set_logging_level(log::off_level);
    REQUIRE_EQ(static_cast<int>(log::get_logging_level()), static_cast<int>(log::off_level));

    log::set_logging_level(log::trace_level);
    REQUIRE_EQ(static_cast<int>(log::get_logging_level()), static_cast<int>(log::trace_level));

    // 恢复默认
    log::set_logging_level(log::warn_level);
}

/**
 * @brief 测试时间序列边界条件
 */
TEST_CASE(time_series_boundary)
{
    data::time_series series("test");
    
    // 空时间序列
    {
        REQUIRE_EQ(series.size(), 0);
    }
    
    // 单个数据点
    {
        series.add_point(0.0, 1.0);
        REQUIRE_EQ(series.size(), 1);
    }
    
    // 清除数据
    {
        series.clear();
        REQUIRE_EQ(series.size(), 0);
    }
}

/**
 * @brief 测试CSV导入导出边界条件
 */
TEST_CASE(csv_boundary)
{
    // 创建数据处理器
    data::data_processor processor;
    
    // 空序列名称导出
    {
        bool result = processor.export_to_csv("", "test.csv");
        REQUIRE(!result);
    }
    
    // 空文件名导出
    {
        auto series = std::unique_ptr<data::time_series>(new data::time_series("test_series"));
        series->add_point(0.0, 1.0);
        processor.add_time_series(std::move(series));
        bool result = processor.export_to_csv("test_series", "");
        REQUIRE(!result);
    }
}

/**
 * @brief 测试数值边界 - 仿真中使用
 */
TEST_CASE(numeric_boundary)
{
    // 极大步长不应导致溢出
    {
        auto desc = fmi::create_model_description();
        desc->set_version(fmi::fmi_version::fmi_2_0);
        auto instance = simulation::create_model_instance("test", std::move(desc), 1e10);
        REQUIRE(instance != nullptr);
    }
    
    // 极小正步长应被接受
    {
        auto desc = fmi::create_model_description();
        desc->set_version(fmi::fmi_version::fmi_2_0);
        auto instance = simulation::create_model_instance("test", std::move(desc), 1e-15);
        REQUIRE(instance != nullptr);
    }
    
    // NaN和Inf作为时间值不应导致崩溃
    {
        double nan_val = std::numeric_limits<double>::quiet_NaN();
        double inf_val = std::numeric_limits<double>::infinity();
        REQUIRE(std::isnan(nan_val));
        REQUIRE(std::isinf(inf_val));
        
        // NaN不应等于自身
        REQUIRE(!(nan_val == nan_val));
        // Inf大于任何有限值
        REQUIRE(inf_val > 1e300);
    }
}

/**
 * @brief 测试字符串边界 - 变量名和模型名中的特殊字符
 */
TEST_CASE(string_boundary)
{
    // 空字符串作为变量名
    {
        fmi::variable_description var;
        var.name = "";
        var.type = fmi::variable_type::real;
        var.vr = 0;
        REQUIRE_EQ(var.name, std::string(""));
    }
    
    // 包含特殊字符的变量名
    {
        fmi::variable_description var;
        var.name = "var_with.special-char_123";
        var.type = fmi::variable_type::real;
        var.vr = 42;
        REQUIRE_EQ(var.name, std::string("var_with.special-char_123"));
        REQUIRE_EQ(var.vr, 42u);
    }
    
    // 很长的变量名
    {
        std::string long_name(1000, 'a');
        fmi::variable_description var;
        var.name = long_name;
        var.type = fmi::variable_type::integer;
        REQUIRE_EQ(var.name.length(), 1000);
    }
    
    // Unicode字符在XML中的处理
    {
        auto doc = util::create_xml_document();
        bool result = doc->load_string("<root attr=\"\xC3\xA9\">\xC3\xA9</root>");
        REQUIRE(result);
    }
}

int main()
{
    cosmo::log::info("Starting boundary condition tests");
    return RUN_ALL_TESTS();
}
