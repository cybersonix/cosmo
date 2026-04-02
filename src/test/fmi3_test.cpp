/**
 * @file fmi3_test.cpp
 * @brief FMI 3.0接口单元测试
 * 
 * 测试FMI 3.0接口的所有功能，包括库加载、实例化和变量访问。
 * 
 * @copyright Copyright (c) 2024
 */

#include "cosmo/fmi/fmi3_interface.hpp"
#include "cosmo/test/test_framework.hpp"
#include "cosmo/logger/logger.hpp"

using namespace cosmo;
using namespace cosmo::fmi;

/**
 * @brief 测试FMI 3.0接口创建
 */
TEST_CASE(fmi3_interface_creation)
{
    auto interface = fmi3::create_fmi3_interface();
    REQUIRE(interface != nullptr);
    REQUIRE(!interface->is_loaded());
}

/**
 * @brief 测试FMI 3.0接口移动语义
 */
TEST_CASE(fmi3_interface_move)
{
    auto interface1 = fmi3::create_fmi3_interface();
    REQUIRE(interface1 != nullptr);
    
    // 移动构造
    auto interface2 = std::move(interface1);
    REQUIRE(interface2 != nullptr);
    
    // 移动赋值
    auto interface3 = fmi3::create_fmi3_interface();
    interface3 = std::move(interface2);
    REQUIRE(interface3 != nullptr);
}

/**
 * @brief 测试FMI 3.0库加载（错误路径）
 */
TEST_CASE(fmi3_load_invalid_library)
{
    auto interface = fmi3::create_fmi3_interface();
    
    // 尝试加载不存在的库
    bool result = interface->load_library("/nonexistent/library.so");
    REQUIRE(!result);
    REQUIRE(!interface->is_loaded());
}

/**
 * @brief 测试FMI 3.0回调函数结构
 */
TEST_CASE(fmi3_callback_functions)
{
    fmi3::fmi3_callback_functions_t callbacks;
    callbacks.logger = nullptr;
    callbacks.allocate_memory = nullptr;
    callbacks.free_memory = nullptr;
    callbacks.instance_environment = nullptr;
    
    // 验证结构体可以正确初始化
    REQUIRE(callbacks.logger == nullptr);
    REQUIRE(callbacks.instance_environment == nullptr);
}

/**
 * @brief 测试FMI 3.0状态枚举
 */
TEST_CASE(fmi3_status_enum)
{
    // 验证状态枚举值
    REQUIRE(fmi3::fmi3_ok == 0);
    REQUIRE(fmi3::fmi3_warning == 1);
    REQUIRE(fmi3::fmi3_discard == 2);
    REQUIRE(fmi3::fmi3_error == 3);
    REQUIRE(fmi3::fmi3_fatal == 4);
}

/**
 * @brief 测试FMI 3.0基本类型
 */
TEST_CASE(fmi3_basic_types)
{
    // 测试基本类型大小
    REQUIRE(sizeof(fmi3::fmi3_float64_t) == sizeof(double));
    REQUIRE(sizeof(fmi3::fmi3_float32_t) == sizeof(float));
    REQUIRE(sizeof(fmi3::fmi3_int64_t) == sizeof(int64_t));
    REQUIRE(sizeof(fmi3::fmi3_int32_t) == sizeof(int32_t));
    REQUIRE(sizeof(fmi3::fmi3_boolean_t) == sizeof(int));
    REQUIRE(sizeof(fmi3::fmi3_value_reference_t) == sizeof(uint32_t));
}

/**
 * @brief 测试FMI 3.0卸载
 */
TEST_CASE(fmi3_unload)
{
    auto interface = fmi3::create_fmi3_interface();
    
    // 多次卸载应该是安全的
    interface->unload();
    interface->unload();
    
    REQUIRE(!interface->is_loaded());
}

/**
 * @brief 测试FMI 3.0实例管理（空接口）
 */
TEST_CASE(fmi3_instance_null)
{
    auto interface = fmi3::create_fmi3_interface();
    
    // 未加载库时操作应返回错误
    fmi3::fmi3_instance_t instance = nullptr;
    
    // 这些操作在空实例或未加载库时应返回错误
    fmi3::fmi3_status_t status = interface->exit_initialization_mode(instance);
    REQUIRE(status == fmi3::fmi3_error);
    
    status = interface->terminate(instance);
    REQUIRE(status == fmi3::fmi3_error);
    
    status = interface->reset(instance);
    REQUIRE(status == fmi3::fmi3_error);
}

int main()
{
    cosmo::log::info("Starting FMI 3.0 tests");
    return RUN_ALL_TESTS();
}
