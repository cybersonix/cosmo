#include "cosmo/ssp/ssp_loader.hpp"
#include "cosmo/util/xml_parser.hpp"
#include "cosmo/logger/logger.hpp"

#include <fstream>
#include <sstream>

namespace cosmo {
namespace ssp {

std::unique_ptr<system_structure_description> ssp_loader::load_from_file(const std::string& file_path)
{
    cosmo::log::info("Loading SSP from file: " + file_path);
    
    // 检查文件是否存在
    if (!cosmo::util::file_exists(file_path)) {
        cosmo::log::error("SSP file not found: " + file_path);
        return nullptr;
    }
    
    // 读取文件内容
    std::string content = cosmo::util::read_file(file_path);
    if (content.empty()) {
        cosmo::log::error("Failed to read SSP file: " + file_path);
        return nullptr;
    }
    
    // 解析内容
    return parse_ssp_content(content);
}

std::unique_ptr<system_structure_description> ssp_loader::load_from_directory(const std::string& dir_path)
{
    cosmo::log::info("Loading SSP from directory: " + dir_path);
    
    // 检查目录是否存在
    if (!cosmo::util::directory_exists(dir_path)) {
        cosmo::log::error("SSP directory not found: " + dir_path);
        return nullptr;
    }
    
    // 查找SSP描述文件
    std::string ssp_file = dir_path + "/SystemStructure.ssd";
    if (!cosmo::util::file_exists(ssp_file)) {
        cosmo::log::error("SSP description file not found: " + ssp_file);
        return nullptr;
    }
    
    // 加载文件
    return load_from_file(ssp_file);
}

std::unique_ptr<system_structure_description> ssp_loader::parse_ssp_content(const std::string& content)
{
    cosmo::log::info("Parsing SSP content");
    
    // 使用XML解析器解析SSP内容
    auto doc = util::create_xml_document();
    if (!doc->load_string(content)) {
        cosmo::log::error("Failed to parse SSP XML: " + doc->get_error());
        return nullptr;
    }
    
    auto root = doc->get_root();
    if (!root) {
        cosmo::log::error("Invalid SSP: missing root element");
        return nullptr;
    }
    
    // 创建系统结构描述对象
    auto ssp_desc = std::unique_ptr<system_structure_description>(new system_structure_description());
    
    // 解析元数据
    auto metadata_node = root->get_child("ssd:Metadata");
    if (metadata_node) {
        metadata meta;
        meta.author = metadata_node->get_attribute("author", "");
        meta.file_version = metadata_node->get_attribute("fileVersion", "");
        meta.copyright = metadata_node->get_attribute("copyright", "");
        meta.license = metadata_node->get_attribute("license", "");
        meta.description = metadata_node->get_attribute("description", "");
        ssp_desc->set_metadata(meta);
    }
    
    // 解析系统（支持带和不带命名空间前缀的标签）
    auto system_node = root->get_child("ssd:System");
    if (!system_node) {
        system_node = root->get_child("System");
    }
    if (system_node) {
        // 解析元素
        auto elements_node = system_node->get_child("ssd:Elements");
        if (!elements_node) {
            elements_node = system_node->get_child("Elements");
        }
        if (elements_node) {
            // 解析组件（尝试两种命名方式）
            auto component_nodes = elements_node->get_children("ssd:Component");
            if (component_nodes.empty()) {
                component_nodes = elements_node->get_children("Component");
            }
            for (const auto& comp_node : component_nodes) {
                auto comp = parse_component_node(comp_node);
                if (comp) {
                    ssp_desc->add_component(std::move(comp));
                }
            }
        }
        
        // 解析连接
        auto connections_node = system_node->get_child("ssd:Connections");
        if (!connections_node) {
            connections_node = system_node->get_child("Connections");
        }
        if (connections_node) {
            auto connection_nodes = connections_node->get_children("ssd:Connection");
            if (connection_nodes.empty()) {
                connection_nodes = connections_node->get_children("Connection");
            }
            for (const auto& conn_node : connection_nodes) {
                auto conn = parse_connection_node(conn_node);
                if (conn) {
                    ssp_desc->add_connection(std::move(conn));
                }
            }
        }
    }
    
    return ssp_desc;
}

std::unique_ptr<component> ssp_loader::parse_component_node(std::shared_ptr<util::xml_node> node)
{
    if (!node) {
        return nullptr;
    }
    
    std::string name = node->get_attribute("name", "");
    std::string source = node->get_attribute("source", "");
    
    if (name.empty() || source.empty()) {
        cosmo::log::warn("Component missing name or source attribute");
        return nullptr;
    }
    
    auto comp = std::unique_ptr<component>(new component(name, source));
    
    // 解析参数（支持带和不带命名空间前缀）
    auto parameter_nodes = node->get_children("ssd:Parameter");
    if (parameter_nodes.empty()) {
        parameter_nodes = node->get_children("Parameter");
    }
    for (const auto& param_node : parameter_nodes) {
        std::string param_name = param_node->get_attribute("name", "");
        std::string param_value = param_node->get_attribute("value", "");
        if (!param_name.empty()) {
            comp->set_parameter(param_name, param_value);
        }
    }
    
    return comp;
}

std::unique_ptr<connector> ssp_loader::parse_connector_node(std::shared_ptr<util::xml_node> node)
{
    if (!node) {
        return nullptr;
    }
    
    std::string name = node->get_attribute("name", "");
    std::string direction_str = node->get_attribute("kind", "input");
    
    connector::direction dir = connector::direction::input;
    if (direction_str == "output") {
        dir = connector::direction::output;
    }
    
    return std::unique_ptr<connector>(new connector(name, dir));
}

std::unique_ptr<connection> ssp_loader::parse_connection_node(std::shared_ptr<util::xml_node> node)
{
    if (!node) {
        return nullptr;
    }
    
    std::string start_element = node->get_attribute("startElement", "");
    std::string start_connector = node->get_attribute("startConnector", "");
    std::string end_element = node->get_attribute("endElement", "");
    std::string end_connector = node->get_attribute("endConnector", "");
    
    if (start_element.empty() || start_connector.empty() || 
        end_element.empty() || end_connector.empty()) {
        cosmo::log::warn("Connection missing required attributes");
        return nullptr;
    }
    
    return std::unique_ptr<connection>(new connection(start_element, start_connector, 
                                                     end_element, end_connector));
}

std::unique_ptr<component> ssp_loader::parse_component(const std::string& /*component_xml*/)
{
    // 简化的组件解析逻辑
    cosmo::log::info("Parsing component");
    return std::unique_ptr<component>(new component("default_component", "default.fmu"));
}

std::unique_ptr<connector> ssp_loader::parse_connector(const std::string& /*connector_xml*/)
{
    // 简化的连接器解析逻辑
    cosmo::log::info("Parsing connector");
    return std::unique_ptr<connector>(new connector("default_connector", connector::direction::input));
}

std::unique_ptr<connection> ssp_loader::parse_connection(const std::string& /*connection_xml*/)
{
    // 简化的连接解析逻辑
    cosmo::log::info("Parsing connection");
    return std::unique_ptr<connection>(new connection("default_start", "default_start_conn", "default_end", "default_end_conn"));
}

// 工厂函数实现
std::unique_ptr<system_structure_description> load_ssp(const std::string& path)
{
    // 检查路径是文件还是目录
    if (cosmo::util::file_exists(path)) {
        return ssp_loader::load_from_file(path);
    } else if (cosmo::util::directory_exists(path)) {
        return ssp_loader::load_from_directory(path);
    } else {
        cosmo::log::error("Invalid SSP path: " + path);
        return nullptr;
    }
}

} // namespace ssp
} // namespace cosmo
