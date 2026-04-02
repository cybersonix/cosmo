#include "cosmo/cosmo.h"
#include "cosmo/logger/logger.hpp"
#include "cosmo/util/xml_parser.hpp"
#include "cosmo/util/filesystem.hpp"
#include "cosmo/util/dynamic_library.hpp"
#include "cosmo/fmi/fmu_processor.hpp"
#include "cosmo/fmi/fmi_interface.hpp"
#include "cosmo/ssp/ssp_loader.hpp"
#include "cosmo/simulation/simulation.hpp"

#include "cosmo/test/test_framework.hpp"

#include <iostream>
#include <cassert>
#include <memory>
#include <cstdio>

using namespace cosmo::test;

// 测试XML解析器
TEST_CASE(xml_parser_basic)
{
    std::string xml_content = R"(<?xml version="1.0" encoding="UTF-8"?>
<root>
    <element name="test" value="123">
        <child>Text content</child>
    </element>
</root>)";

    auto doc = cosmo::util::create_xml_document();
    bool loaded = doc->load_string(xml_content);
    REQUIRE(loaded);

    auto root = doc->get_root();
    REQUIRE(root != nullptr);
    REQUIRE_EQ(root->get_name(), std::string("root"));

    auto element = root->get_child("element");
    REQUIRE(element != nullptr);
    REQUIRE_EQ(element->get_attribute("name"), std::string("test"));
    REQUIRE_EQ(element->get_attribute("value"), std::string("123"));

    auto child = element->get_child("child");
    REQUIRE(child != nullptr);
    REQUIRE_EQ(child->get_text(), std::string("Text content"));
}

// 测试XML解析器空内容
TEST_CASE(xml_parser_empty)
{
    auto doc = cosmo::util::create_xml_document();
    REQUIRE_FALSE(doc->load_string(""));
}

// 测试XML序列化（含特殊字符转义）
TEST_CASE(xml_parser_special_chars)
{
    std::string xml_content = R"(<?xml version="1.0"?>
<root attr="a&amp;b">
    <child>a&lt;b &amp; c&gt;d</child>
</root>)";

    auto doc = cosmo::util::create_xml_document();
    REQUIRE(doc->load_string(xml_content));

    auto root = doc->get_root();
    REQUIRE(root != nullptr);
    REQUIRE_EQ(root->get_attribute("attr"), std::string("a&b"));

    auto child = root->get_child("child");
    REQUIRE(child != nullptr);
    REQUIRE_EQ(child->get_text(), std::string("a<b & c>d"));
}

// 测试文件系统工具
TEST_CASE(filesystem_basic)
{
    // 测试文件扩展名获取
    std::string ext = cosmo::util::file_extension("test.fmu");
    REQUIRE_EQ(ext, std::string(".fmu"));

    ext = cosmo::util::file_extension("no_extension");
    REQUIRE_EQ(ext, std::string(""));

    ext = cosmo::util::file_extension("multiple.dots.xml");
    REQUIRE_EQ(ext, std::string(".xml"));
}

TEST_CASE(filesystem_read_write)
{
    const std::string test_file = "/tmp/cosmo_test_fs.txt";
    const std::string content = "Hello COSMO";

    REQUIRE(cosmo::util::write_file(test_file, content));
    REQUIRE(cosmo::util::file_exists(test_file));

    std::string read_back = cosmo::util::read_file(test_file);
    REQUIRE_EQ(read_back, content);

    std::remove(test_file.c_str());
    REQUIRE_FALSE(cosmo::util::file_exists(test_file));
}

TEST_CASE(filesystem_nonexistent)
{
    REQUIRE_FALSE(cosmo::util::file_exists("/tmp/cosmo_nonexistent_file_12345.txt"));

    std::string content = cosmo::util::read_file("/tmp/cosmo_nonexistent_file_12345.txt");
    REQUIRE_EQ(content, std::string(""));
}

// 测试日志系统
TEST_CASE(logging_levels)
{
    // 只验证不崩溃
    cosmo::log::trace("Trace message");
    cosmo::log::debug("Debug message");
    cosmo::log::info("Info message");
    cosmo::log::warn("Warn message");
    cosmo::log::error("Error message");

    // 测试C API
    cosmo_trace("C API trace");
    cosmo_debug("C API debug");
    cosmo_info("C API info");
    cosmo_warn("C API warn");
    cosmo_error("C API error");
}

TEST_CASE(log_level_setting)
{
    cosmo::log::set_logging_level(cosmo::log::info_level);
    REQUIRE_EQ(static_cast<int>(cosmo::log::get_logging_level()), static_cast<int>(cosmo::log::info_level));

    cosmo::log::set_logging_level(cosmo::log::err_level);
    REQUIRE_EQ(static_cast<int>(cosmo::log::get_logging_level()), static_cast<int>(cosmo::log::err_level));

    cosmo::log::set_logging_level(cosmo::log::trace_level);
}

// 测试版本信息
TEST_CASE(version_info)
{
    cosmo_version_info v = cosmo_get_version();
    REQUIRE_EQ(v.major, 0);
    REQUIRE_EQ(v.minor, 1);
    REQUIRE_EQ(v.patch, 0);

    const char* ver_str = cosmo_get_version_str();
    REQUIRE(ver_str != nullptr);
    REQUIRE_EQ(std::string(ver_str), std::string("0.1.0"));
}

// 测试FMU处理器 - 无效路径
TEST_CASE(fmu_processor_invalid_path)
{
    auto processor = cosmo::fmi::create_fmu_processor();
    REQUIRE(processor != nullptr);

    bool loaded = processor->load_fmu("nonexistent.fmu");
    REQUIRE_FALSE(loaded);

    loaded = processor->load_fmu("");
    REQUIRE_FALSE(loaded);
}

// 测试动态库加载器 - 无效路径
TEST_CASE(dynamic_library_invalid)
{
    auto lib = cosmo::util::create_dynamic_library();
    REQUIRE(lib != nullptr);
    REQUIRE_FALSE(lib->is_loaded());

    bool loaded = lib->load("nonexistent.so");
    REQUIRE_FALSE(loaded);
    REQUIRE_FALSE(lib->is_loaded());

    REQUIRE_FALSE(lib->get_error().empty());
}

// 测试FMI接口 - 无效路径
TEST_CASE(fmi_interface_invalid)
{
    auto fmi = cosmo::fmi::create_fmi_interface();
    REQUIRE(fmi != nullptr);
    REQUIRE_FALSE(fmi->is_loaded());

    bool loaded = fmi->load_library("nonexistent.so");
    REQUIRE_FALSE(loaded);
    REQUIRE_FALSE(fmi->is_loaded());
}

// 测试SSP加载器 - 无效路径
TEST_CASE(ssp_loader_invalid)
{
    auto ssp = cosmo::ssp::load_ssp("nonexistent.ssp");
    REQUIRE(ssp == nullptr);

    ssp = cosmo::ssp::load_ssp("");
    REQUIRE(ssp == nullptr);
}

// 测试仿真引擎完整生命周期
TEST_CASE(simulation_lifecycle)
{
    auto algorithm = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(0.01)
    );
    auto sim = std::unique_ptr<cosmo::simulation::simulation>(
        new cosmo::simulation::simulation(std::move(algorithm))
    );

    REQUIRE_FALSE(sim->is_initialized());
    REQUIRE_FALSE(sim->is_terminated());

    // 初始化
    auto status = sim->initialize(0.0);
    REQUIRE_EQ(static_cast<int>(status), 0); // fmi_ok
    REQUIRE(sim->is_initialized());

    // 步进
    REQUIRE_EQ(sim->get_step_count(), 0u);
    status = sim->step();
    REQUIRE_EQ(static_cast<int>(status), 0);
    REQUIRE_EQ(sim->get_current_time(), 0.01);
    REQUIRE_EQ(sim->get_step_count(), 1u);

    // 多次步进
    sim->step();
    sim->step();
    REQUIRE_EQ(sim->get_step_count(), 3u);
    REQUIRE_EQ(sim->get_current_time(), 0.03);

    // step_until
    sim->step_until(0.05);
    REQUIRE_EQ(sim->get_current_time(), 0.05);

    // step_for
    sim->step_for(0.03);
    REQUIRE_EQ(sim->get_current_time(), 0.08);

    // 终止
    status = sim->terminate();
    REQUIRE_EQ(static_cast<int>(status), 0);
    REQUIRE(sim->is_terminated());

    // 重置
    status = sim->reset();
    REQUIRE_EQ(static_cast<int>(status), 0);
    REQUIRE_FALSE(sim->is_initialized());
    REQUIRE_FALSE(sim->is_terminated());
    REQUIRE_EQ(sim->get_current_time(), 0.0);
    REQUIRE_EQ(sim->get_step_count(), 0u);
}

// 测试错误处理
TEST_CASE(error_handling)
{
    cosmo_set_log_level("error");
    REQUIRE_EQ(static_cast<int>(cosmo::log::get_logging_level()), static_cast<int>(cosmo::log::err_level));

    cosmo_set_log_level("invalid_level");
    // 无效级别应被忽略，保持原值
    REQUIRE_EQ(static_cast<int>(cosmo::log::get_logging_level()), static_cast<int>(cosmo::log::err_level));

    // 测试 last_error
    cosmo_clear_error();
    REQUIRE(cosmo_get_last_error() != nullptr);
    REQUIRE_EQ(std::string(cosmo_get_last_error()), std::string(""));
}

int main()
{
    cosmo::log::set_logging_level(cosmo::log::warn_level);
    cosmo::log::info("Starting COSMO test suite");
    return RUN_ALL_TESTS();
}
