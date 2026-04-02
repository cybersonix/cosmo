/**
 * @file data_router_test.cpp
 * @brief 数据路由器单元测试
 *
 * 测试数据路由器和数据通道的所有功能。
 *
 * @copyright Copyright (c) 2024
 */

#include "cosmo/data/data_router.hpp"
#include "cosmo/test/test_framework.hpp"
#include "cosmo/logger/logger.hpp"

#include <algorithm>

using namespace cosmo;
using namespace cosmo::data;

/**
 * @brief 测试数据通道创建
 */
TEST_CASE(data_channel_creation)
{
    channel_endpoint source("model1", "var1", 1);
    channel_endpoint target("model2", "var2", 2);

    data_channel channel(source, target, transfer_type::direct);

    REQUIRE_EQ(channel.get_source().model_instance, std::string("model1"));
    REQUIRE_EQ(channel.get_source().variable_name, std::string("var1"));
    REQUIRE_EQ(channel.get_target().model_instance, std::string("model2"));
    REQUIRE_EQ(channel.get_target().variable_name, std::string("var2"));
    REQUIRE(channel.is_valid());
    REQUIRE(channel.is_enabled());
}

/**
 * @brief 测试数据通道线性转换
 */
TEST_CASE(data_channel_linear_transform)
{
    channel_endpoint source("model1", "var1");
    channel_endpoint target("model2", "var2");

    data_channel channel(source, target, transfer_type::linear);
    channel.set_linear_transform(2.0, 1.0);  // y = 2x + 1

    REQUIRE_EQ(channel.get_transfer_type(), transfer_type::linear);
    REQUIRE_EQ(channel.apply_transform(0.0), 1.0);   // 2*0 + 1 = 1
    REQUIRE_EQ(channel.apply_transform(1.0), 3.0);   // 2*1 + 1 = 3
    REQUIRE_EQ(channel.apply_transform(5.0), 11.0);  // 2*5 + 1 = 11
}

/**
 * @brief 测试数据通道禁用
 */
TEST_CASE(data_channel_disable)
{
    channel_endpoint source("model1", "var1");
    channel_endpoint target("model2", "var2");

    data_channel channel(source, target);
    channel.set_linear_transform(2.0, 1.0);

    // 禁用后转换应返回原值
    channel.set_enabled(false);
    REQUIRE(!channel.is_enabled());
    REQUIRE_EQ(channel.apply_transform(5.0), 5.0);  // 禁用时不转换

    channel.set_enabled(true);
    REQUIRE(channel.is_enabled());
    REQUIRE_EQ(channel.apply_transform(5.0), 11.0);  // 重新启用后转换
}

/**
 * @brief 测试无效通道
 */
TEST_CASE(data_channel_invalid)
{
    // 空端点
    {
        channel_endpoint empty("", "");
        channel_endpoint valid("model1", "var1");

        data_channel channel1(empty, valid);
        REQUIRE(!channel1.is_valid());

        data_channel channel2(valid, empty);
        REQUIRE(!channel2.is_valid());
    }

    // 相同模型（不允许自环）
    {
        channel_endpoint source("model1", "var1");
        channel_endpoint target("model1", "var2");

        data_channel channel(source, target);
        REQUIRE(!channel.is_valid());
    }
}

/**
 * @brief 测试自定义转换函数
 */
TEST_CASE(data_channel_custom_transform)
{
    channel_endpoint source("model1", "var1");
    channel_endpoint target("model2", "var2");

    data_channel channel(source, target);

    // 设置自定义转换：平方
    channel.set_custom_transform([](double x) { return x * x; });

    REQUIRE_EQ(channel.get_transfer_type(), transfer_type::custom);
    REQUIRE_EQ(channel.apply_transform(2.0), 4.0);
    REQUIRE_EQ(channel.apply_transform(3.0), 9.0);
    REQUIRE_EQ(channel.apply_transform(-2.0), 4.0);
}

/**
 * @brief 测试数据路由器创建
 */
TEST_CASE(data_router_creation)
{
    auto router = create_data_router();
    REQUIRE(router != nullptr);
    REQUIRE_EQ(router->get_channel_count(), 0);
}

/**
 * @brief 测试添加和移除通道
 */
TEST_CASE(data_router_add_remove)
{
    auto router = create_data_router();

    channel_endpoint source("model1", "var1");
    channel_endpoint target("model2", "var2");

    int id = router->add_channel(source, target);
    REQUIRE(id >= 0);
    REQUIRE_EQ(router->get_channel_count(), 1);

    // 获取通道
    auto* channel = router->get_channel(id);
    REQUIRE(channel != nullptr);
    REQUIRE_EQ(channel->get_source().model_instance, std::string("model1"));

    // 移除通道
    bool result = router->remove_channel(id);
    REQUIRE(result);
    REQUIRE_EQ(router->get_channel_count(), 0);

    // 再次移除应失败
    result = router->remove_channel(id);
    REQUIRE(!result);
}

/**
 * @brief 测试添加无效通道
 */
TEST_CASE(data_router_invalid_channel)
{
    auto router = create_data_router();

    // 相同模型的通道应被拒绝
    channel_endpoint source("model1", "var1");
    channel_endpoint target("model1", "var2");

    int id = router->add_channel(source, target);
    REQUIRE(id < 0);  // 应返回无效ID
    REQUIRE_EQ(router->get_channel_count(), 0);
}

/**
 * @brief 测试通道启用/禁用
 */
TEST_CASE(data_router_enable_disable)
{
    auto router = create_data_router();

    channel_endpoint source("model1", "var1");
    channel_endpoint target("model2", "var2");

    int id = router->add_channel(source, target);
    REQUIRE(id >= 0);

    // 禁用通道
    bool result = router->set_channel_enabled(id, false);
    REQUIRE(result);

    auto* channel = router->get_channel(id);
    REQUIRE(!channel->is_enabled());

    // 重新启用
    result = router->set_channel_enabled(id, true);
    REQUIRE(result);
    REQUIRE(channel->is_enabled());

    // 无效通道ID
    result = router->set_channel_enabled(999, false);
    REQUIRE(!result);
}

/**
 * @brief 测试模型输入输出通道查询
 */
TEST_CASE(data_router_model_channels)
{
    auto router = create_data_router();

    // 添加多个通道
    channel_endpoint source1("model1", "var1");
    channel_endpoint target1("model2", "var2");
    int id1 = router->add_channel(source1, target1);

    channel_endpoint source2("model1", "var3");
    channel_endpoint target2("model3", "var4");
    int id2 = router->add_channel(source2, target2);

    channel_endpoint source3("model2", "var5");
    channel_endpoint target3("model1", "var6");
    int id3 = router->add_channel(source3, target3);

    // 查询model1的输出通道
    auto outputs = router->get_output_channels("model1");
    REQUIRE_EQ(outputs.size(), 2);
    REQUIRE(std::find(outputs.begin(), outputs.end(), id1) != outputs.end());
    REQUIRE(std::find(outputs.begin(), outputs.end(), id2) != outputs.end());

    // 查询model1的输入通道
    auto inputs = router->get_input_channels("model1");
    REQUIRE_EQ(inputs.size(), 1);
    REQUIRE_EQ(inputs[0], id3);

    // 查询不存在的模型
    auto empty = router->get_output_channels("nonexistent");
    REQUIRE(empty.empty());
}

/**
 * @brief 测试验证所有通道
 */
TEST_CASE(data_router_validate)
{
    auto router = create_data_router();

    // 空路由器验证应通过
    int invalid = router->validate_all_channels();
    REQUIRE_EQ(invalid, 0);

    // 添加有效通道
    channel_endpoint source("model1", "var1");
    channel_endpoint target("model2", "var2");
    router->add_channel(source, target);

    invalid = router->validate_all_channels();
    REQUIRE_EQ(invalid, 0);
}

/**
 * @brief 测试清除所有通道
 */
TEST_CASE(data_router_clear)
{
    auto router = create_data_router();

    // 添加多个通道
    for (int i = 0; i < 5; ++i) {
        channel_endpoint source("model" + std::to_string(i), "var" + std::to_string(i));
        channel_endpoint target("target" + std::to_string(i), "var" + std::to_string(i));
        router->add_channel(source, target);
    }

    REQUIRE_EQ(router->get_channel_count(), 5);

    router->clear_all_channels();

    REQUIRE_EQ(router->get_channel_count(), 0);
    REQUIRE(router->get_output_channels("model0").empty());
    REQUIRE(router->get_input_channels("target0").empty());
}

/**
 * @brief 测试通道字符串表示
 */
TEST_CASE(data_channel_to_string)
{
    channel_endpoint source("model1", "var1");
    channel_endpoint target("model2", "var2");

    data_channel channel(source, target, transfer_type::linear);
    channel.set_linear_transform(2.5, 1.5);

    std::string str = channel.to_string();
    REQUIRE(str.find("model1::var1") != std::string::npos);
    REQUIRE(str.find("model2::var2") != std::string::npos);
    REQUIRE(str.find("linear") != std::string::npos);
}

/**
 * @brief 测试路由器状态信息
 */
TEST_CASE(data_router_status)
{
    auto router = create_data_router();

    std::string status = router->get_status();
    REQUIRE(status.find("数据路由器状态") != std::string::npos);
    REQUIRE(status.find("总通道数: 0") != std::string::npos);

    channel_endpoint source("model1", "var1");
    channel_endpoint target("model2", "var2");
    router->add_channel(source, target);

    status = router->get_status();
    REQUIRE(status.find("总通道数: 1") != std::string::npos);
}

int main()
{
    cosmo::log::info("Starting data router tests");
    return RUN_ALL_TESTS();
}
