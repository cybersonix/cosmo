/**
 * @file fmu_processor_test.cpp
 * @brief FMU处理器单元测试
 *
 * 测试FMU处理器的各种功能。
 *
 * @copyright Copyright (c) 2024
 */

#include "cosmo/fmi/fmu_processor.hpp"
#include "cosmo/test/test_framework.hpp"
#include "cosmo/logger/logger.hpp"

using namespace cosmo;
using namespace cosmo::fmi;

/**
 * @brief 测试FMU处理器创建
 */
TEST_CASE(fmu_processor_creation)
{
    fmu_processor processor;

    // 验证初始状态：未加载时没有条目
    auto entries = processor.get_entry_names();
    REQUIRE(entries.empty());

    // 验证初始状态：未加载时modelDescription为空
    auto md = processor.parse_model_description();
    REQUIRE(md == nullptr);

    // 验证加载不存在的路径返回false
    REQUIRE(!processor.load_fmu("/nonexistent/model.fmu"));

    // 加载后仍然没有条目（因为路径无效）
    entries = processor.get_entry_names();
    REQUIRE(entries.empty());
}

/**
 * @brief 测试加载无效FMU路径
 */
TEST_CASE(fmu_processor_invalid_path)
{
    fmu_processor processor;

    // 加载不存在的FMU
    bool result = processor.load_fmu("/nonexistent/path/model.fmu");
    REQUIRE(!result);

    // 加载空路径
    REQUIRE(!processor.load_fmu(""));
}

/**
 * @brief 测试获取条目列表（空FMU）
 */
TEST_CASE(fmu_processor_empty_entries)
{
    fmu_processor processor;
    
    auto entries = processor.get_entry_names();
    REQUIRE(entries.empty());
}

/**
 * @brief 测试解析modelDescription（空FMU）
 */
TEST_CASE(fmu_processor_parse_empty)
{
    fmu_processor processor;
    
    auto md = processor.parse_model_description();
    // 空FMU解析应返回nullptr
    REQUIRE(md == nullptr);
}

int main()
{
    cosmo::log::info("Starting FMU processor tests");
    return RUN_ALL_TESTS();
}
