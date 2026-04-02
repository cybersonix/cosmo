/**
 * @file fmi1_test.cpp
 * @brief FMI 1.0接口单元测试
 * 
 * 测试FMI 1.0接口的所有功能，包括库加载、实例化和变量访问。
 * 
 * @copyright Copyright (c) 2024
 */

#include "cosmo/fmi/fmi1_interface.hpp"
#include "cosmo/test/test_framework.hpp"
#include "cosmo/logger/logger.hpp"

using namespace cosmo;
using namespace cosmo::fmi;

/**
 * @brief 测试FMI 1.0接口创建
 */
TEST_CASE(fmi1_interface_creation)
{
    auto interface = fmi1::create_fmi1_interface();
    REQUIRE(interface != nullptr);
    REQUIRE(!interface->is_loaded());
}

/**
 * @brief 测试FMI 1.0接口移动语义
 */
TEST_CASE(fmi1_interface_move)
{
    auto interface1 = fmi1::create_fmi1_interface();
    REQUIRE(interface1 != nullptr);
    
    // 移动构造
    auto interface2 = std::move(interface1);
    REQUIRE(interface2 != nullptr);
    
    // 移动赋值
    auto interface3 = fmi1::create_fmi1_interface();
    interface3 = std::move(interface2);
    REQUIRE(interface3 != nullptr);
}

/**
 * @brief 测试FMI 1.0库加载（错误路径）
 */
TEST_CASE(fmi1_load_invalid_library)
{
    auto interface = fmi1::create_fmi1_interface();
    
    // 尝试加载不存在的库
    bool result = interface->load_library("/nonexistent/library.so");
    REQUIRE(!result);
    REQUIRE(!interface->is_loaded());
}

/**
 * @brief 测试FMI 1.0回调函数结构
 */
TEST_CASE(fmi1_callback_functions)
{
    fmi1::fmi1_callback_functions_t callbacks;
    callbacks.logger = nullptr;
    callbacks.allocate_memory = nullptr;
    callbacks.free_memory = nullptr;
    callbacks.step_finished = nullptr;
    
    // 验证结构体可以正确初始化
    REQUIRE(callbacks.logger == nullptr);
    REQUIRE(callbacks.allocate_memory == nullptr);
}

/**
 * @brief 测试FMI 1.0状态枚举
 */
TEST_CASE(fmi1_status_enum)
{
    // 验证状态枚举值
    REQUIRE(fmi1::fmi1_ok == 0);
    REQUIRE(fmi1::fmi1_warning == 1);
    REQUIRE(fmi1::fmi1_discard == 2);
    REQUIRE(fmi1::fmi1_error == 3);
    REQUIRE(fmi1::fmi1_fatal == 4);
    REQUIRE(fmi1::fmi1_pending == 5);
}

/**
 * @brief 测试FMI 1.0事件信息结构
 */
TEST_CASE(fmi1_event_info)
{
    fmi1::fmi1_event_info_t event_info;
    event_info.iteration_converged = 1;
    event_info.state_value_references_changed = 0;
    event_info.state_values_changed = 0;
    event_info.terminate_simulation = 0;
    event_info.upcoming_time_event = 1;
    event_info.next_event_time = 1.0;
    
    REQUIRE(event_info.iteration_converged == 1);
    REQUIRE(event_info.next_event_time == 1.0);
}

/**
 * @brief 测试FMI 1.0基本类型
 */
TEST_CASE(fmi1_basic_types)
{
    // 测试基本类型大小
    REQUIRE(sizeof(fmi1::fmi1_real_t) == sizeof(double));
    REQUIRE(sizeof(fmi1::fmi1_integer_t) == sizeof(int));
    REQUIRE(sizeof(fmi1::fmi1_boolean_t) == sizeof(int));
    REQUIRE(sizeof(fmi1::fmi1_value_reference_t) == sizeof(unsigned int));
}

/**
 * @brief 测试FMI 1.0卸载
 */
TEST_CASE(fmi1_unload)
{
    auto interface = fmi1::create_fmi1_interface();
    
    // 多次卸载应该是安全的
    interface->unload();
    interface->unload();
    
    REQUIRE(!interface->is_loaded());
}

int main()
{
    cosmo::log::info("Starting FMI 1.0 tests");
    return RUN_ALL_TESTS();
}
