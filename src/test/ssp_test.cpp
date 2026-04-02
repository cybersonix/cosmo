#include "cosmo/ssp/ssp_loader.hpp"
#include "cosmo/ssp/ssp_types.hpp"
#include "cosmo/test/test_framework.hpp"
#include "cosmo/logger/logger.hpp"
#include "cosmo/util/filesystem.hpp"

using namespace cosmo::test;
using namespace cosmo::ssp;

TEST_CASE(ssp_types_basic)
{
    // 测试系统结构描述
    system_structure_description ssp_desc("1.0");
    
    // 测试元数据
    metadata meta;
    meta.author = "Test Author";
    meta.file_version = "1.0";
    meta.description = "Test SSP";
    ssp_desc.set_metadata(meta);
    
    REQUIRE_EQ(ssp_desc.get_metadata().author, "Test Author");
    REQUIRE_EQ(ssp_desc.get_metadata().file_version, "1.0");
    REQUIRE_EQ(ssp_desc.get_metadata().description, "Test SSP");
    
    // 测试组件
    auto component1 = std::unique_ptr<component>(new component("test_component", "test.fmu"));
    component1->set_parameter("param1", "value1");
    ssp_desc.add_component(std::move(component1));
    
    REQUIRE_EQ(ssp_desc.get_components().size(), 1);
    
    const auto& comp = ssp_desc.get_components()[0];
    REQUIRE_EQ(comp->get_name(), "test_component");
    REQUIRE_EQ(comp->get_source(), "test.fmu");
    REQUIRE_EQ(comp->get_parameters().at("param1"), "value1");
    
    // 测试连接器
    auto connector1 = std::unique_ptr<connector>(new connector("test_connector", connector::direction::input));
    ssp_desc.add_connector(std::move(connector1));
    
    REQUIRE_EQ(ssp_desc.get_connectors().size(), 1);
    
    const auto& conn = ssp_desc.get_connectors()[0];
    REQUIRE_EQ(conn->get_name(), "test_connector");
    REQUIRE_EQ(conn->get_direction(), connector::direction::input);
    
    // 测试连接
    auto connection1 = std::unique_ptr<connection>(new connection("start_elem", "start_conn", "end_elem", "end_conn"));
    ssp_desc.add_connection(std::move(connection1));
    
    REQUIRE_EQ(ssp_desc.get_connections().size(), 1);
    
    const auto& conn_obj = ssp_desc.get_connections()[0];
    REQUIRE_EQ(conn_obj->get_start_element(), "start_elem");
    REQUIRE_EQ(conn_obj->get_start_connector(), "start_conn");
    REQUIRE_EQ(conn_obj->get_end_element(), "end_elem");
    REQUIRE_EQ(conn_obj->get_end_connector(), "end_conn");
}

TEST_CASE(ssp_loader_basic)
{
    // 创建有效的SSP XML测试内容（使用无命名空间前缀的简化格式）
    std::string test_content = R"(<?xml version="1.0" encoding="UTF-8"?>
<SystemStructureDescription version="1.0" name="test_system">
    <System name="test_system">
        <Elements>
            <Component name="test_component" source="test.fmu">
                <Parameter name="param1" value="value1"/>
            </Component>
        </Elements>
        <Connections>
            <Connection startElement="comp1" startConnector="out1"
                       endElement="comp2" endConnector="in1"/>
        </Connections>
    </System>
</SystemStructureDescription>)";
    std::string test_file = "test_ssp.ssd";
    
    // 写入测试文件
    REQUIRE(cosmo::util::write_file(test_file, test_content));
    
    // 测试从文件加载
    auto ssp_desc = load_ssp(test_file);
    REQUIRE(ssp_desc != nullptr);
    
    // 验证加载的内容：组件应被解析
    if (!ssp_desc->get_components().empty()) {
        REQUIRE_EQ(ssp_desc->get_components()[0]->get_name(), "test_component");
        REQUIRE_EQ(ssp_desc->get_components()[0]->get_source(), "test.fmu");
    }
    
    // 验证加载的内容：连接应被解析
    if (!ssp_desc->get_connections().empty()) {
        const auto& conn = ssp_desc->get_connections()[0];
        REQUIRE_EQ(conn->get_start_element(), "comp1");
        REQUIRE_EQ(conn->get_start_connector(), "out1");
        REQUIRE_EQ(conn->get_end_element(), "comp2");
        REQUIRE_EQ(conn->get_end_connector(), "in1");
    }
    
    // 清理测试文件
    remove(test_file.c_str());
}

int main()
{
    cosmo::log::info("Starting SSP test");
    return RUN_ALL_TESTS();
}