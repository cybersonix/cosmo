#ifndef COSMO_SSP_SSP_LOADER_HPP
#define COSMO_SSP_SSP_LOADER_HPP

#include "cosmo/ssp/ssp_types.hpp"
#include "cosmo/util/filesystem.hpp"
#include "cosmo/util/xml_parser.hpp"

#include <string>
#include <memory>

namespace cosmo {
namespace ssp {

// SSP加载器类
class ssp_loader
{
public:
    // 从文件加载SSP描述
    static std::unique_ptr<system_structure_description> load_from_file(const std::string& file_path);
    
    // 从目录加载SSP描述
    static std::unique_ptr<system_structure_description> load_from_directory(const std::string& dir_path);
    
private:
    // 解析SSP文件内容
    static std::unique_ptr<system_structure_description> parse_ssp_content(const std::string& content);
    
    // 解析组件节点
    static std::unique_ptr<component> parse_component_node(std::shared_ptr<util::xml_node> node);
    
    // 解析连接器节点
    static std::unique_ptr<connector> parse_connector_node(std::shared_ptr<util::xml_node> node);
    
    // 解析连接节点
    static std::unique_ptr<connection> parse_connection_node(std::shared_ptr<util::xml_node> node);
    
    // 解析组件（字符串版本）
    static std::unique_ptr<component> parse_component(const std::string& component_xml);
    
    // 解析连接器（字符串版本）
    static std::unique_ptr<connector> parse_connector(const std::string& connector_xml);
    
    // 解析连接（字符串版本）
    static std::unique_ptr<connection> parse_connection(const std::string& connection_xml);
};

// 工厂函数
std::unique_ptr<system_structure_description> load_ssp(const std::string& path);

} // namespace ssp
} // namespace cosmo

#endif // COSMO_SSP_SSP_LOADER_HPP
