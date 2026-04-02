#ifndef COSMO_SSP_SSP_TYPES_HPP
#define COSMO_SSP_SSP_TYPES_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace cosmo {
namespace ssp {

// SSP版本枚举
enum class ssp_version
{
    ssp_1_0
};

// 元数据结构
struct metadata
{
    std::string author;
    std::string file_version;
    std::string copyright;
    std::string license;
    std::string description;
};

// 系统结构元素基类
class system_structure_element
{
protected:
    std::string name_;
    std::string description_;
    
public:
    system_structure_element(const std::string& name, const std::string& description = "")
        : name_(name), description_(description) {}
    
    virtual ~system_structure_element() = default;
    
    const std::string& get_name() const { return name_; }
    const std::string& get_description() const { return description_; }
};

// 组件类
class component : public system_structure_element
{
private:
    std::string source_;
    std::map<std::string, std::string> parameters_;
    
public:
    component(const std::string& name, const std::string& source, const std::string& description = "")
        : system_structure_element(name, description), source_(source) {}
    
    const std::string& get_source() const { return source_; }
    void set_parameter(const std::string& name, const std::string& value) { parameters_[name] = value; }
    const std::map<std::string, std::string>& get_parameters() const { return parameters_; }
};

// 连接器类
class connector : public system_structure_element
{
public:
    enum class direction { input, output };
    
private:
    direction direction_;
    
public:
    connector(const std::string& name, direction dir, const std::string& description = "")
        : system_structure_element(name, description), direction_(dir) {}
    
    direction get_direction() const { return direction_; }
};

// 连接类
class connection
{
private:
    std::string start_element_;
    std::string start_connector_;
    std::string end_element_;
    std::string end_connector_;
    
public:
    connection(const std::string& start_element, const std::string& start_connector,
               const std::string& end_element, const std::string& end_connector)
        : start_element_(start_element), start_connector_(start_connector),
          end_element_(end_element), end_connector_(end_connector) {}
    
    const std::string& get_start_element() const { return start_element_; }
    const std::string& get_start_connector() const { return start_connector_; }
    const std::string& get_end_element() const { return end_element_; }
    const std::string& get_end_connector() const { return end_connector_; }
};

// 系统结构描述类
class system_structure_description
{
private:
    std::string version_;
    metadata metadata_;
    std::vector<std::unique_ptr<component>> components_;
    std::vector<std::unique_ptr<connector>> connectors_;
    std::vector<std::unique_ptr<connection>> connections_;
    
public:
    system_structure_description(const std::string& version = "1.0")
        : version_(version) {}
    
    void set_metadata(const metadata& meta) { metadata_ = meta; }
    const metadata& get_metadata() const { return metadata_; }
    
    void add_component(std::unique_ptr<component> comp) { components_.push_back(std::move(comp)); }
    void add_connector(std::unique_ptr<connector> conn) { connectors_.push_back(std::move(conn)); }
    void add_connection(std::unique_ptr<connection> conn) { connections_.push_back(std::move(conn)); }
    
    const std::vector<std::unique_ptr<component>>& get_components() const { return components_; }
    const std::vector<std::unique_ptr<connector>>& get_connectors() const { return connectors_; }
    const std::vector<std::unique_ptr<connection>>& get_connections() const { return connections_; }
};

} // namespace ssp
} // namespace cosmo

#endif // COSMO_SSP_SSP_TYPES_HPP