#ifndef COSMO_SIMULATION_SIMULATION_STRUCTURE_HPP
#define COSMO_SIMULATION_SIMULATION_STRUCTURE_HPP

#include "cosmo/simulation/simulation.hpp"
#include "cosmo/simulation/model_instance.hpp"
#include "cosmo/fmi/model_description.hpp"
#include "cosmo/fmi/fmi_types.hpp"

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <functional>

namespace cosmo {
namespace simulation {

// 变量连接修饰器类型
using connection_modifier = std::function<fmi::fmi_real(fmi::fmi_real)>;

// 变量连接类
class variable_connection
{
public:
    variable_connection(const std::string& source_model, const std::string& source_variable,
                       const std::string& target_model, const std::string& target_variable,
                       connection_modifier modifier = nullptr);
    
    const std::string& get_source_model() const;
    const std::string& get_source_variable() const;
    const std::string& get_target_model() const;
    const std::string& get_target_variable() const;
    
    fmi::fmi_real apply_modifier(fmi::fmi_real value) const;
    bool has_modifier() const;
    
private:
    std::string source_model_;
    std::string source_variable_;
    std::string target_model_;
    std::string target_variable_;
    connection_modifier modifier_;
};

// 仿真结构配置类
class simulation_structure
{
public:
    simulation_structure();
    ~simulation_structure() = default;
    
    // 添加模型实例
    void add_model(const std::string& instance_name, const std::string& fmu_path);
    void add_model(const std::string& instance_name, const std::string& fmu_path, 
                   const std::map<std::string, scalar_value>& parameters);

    /**
     * @brief 为指定模型注册变量
     * @param model_name 模型实例名称
     * @param variable_type 变量数据类型（"real"/"integer"/"boolean"/"string"）
     * @param variable_name 变量名称
     *
     * 在没有实际FMU的情况下（如测试环境），需要预先声明模型具有哪些变量。
     * 这些变量会被添加到模型描述中，使得set/get操作可以正常工作。
     */
    void register_variable(const std::string& model_name,
                           const std::string& variable_type,
                           const std::string& variable_name);
    
    // 创建变量连接
    void make_connection(const std::string& source, const std::string& target);
    void make_connection(const std::string& source, const std::string& target, 
                        connection_modifier modifier);
    
    // 添加参数集
    void add_parameter_set(const std::string& name, 
                          const std::map<std::string, scalar_value>& parameters);
    
    // 加载仿真
    std::unique_ptr<simulation> load(std::unique_ptr<algorithm> algo);
    
    // 获取模型实例数量
    size_t get_model_count() const;
    
    // 获取连接数量
    size_t get_connection_count() const;
    
    // 获取参数集数量
    size_t get_parameter_set_count() const;
    
    // 检查模型是否存在
    bool has_model(const std::string& instance_name) const;
    
    // 获取模型FMU路径
    const std::string& get_model_fmu_path(const std::string& instance_name) const;
    
    // 获取连接列表
    const std::vector<variable_connection>& get_connections() const { return connections_; }
    
private:
    struct model_info
    {
        std::string instance_name;
        std::string fmu_path;
        std::map<std::string, scalar_value> parameters;
        std::string parameter_set_name;
    };
    
    std::map<std::string, model_info> models_;
    std::vector<variable_connection> connections_;
    std::map<std::string, std::map<std::string, scalar_value>> parameter_sets_;
    // 手动注册的变量：模型名称 -> (变量类型, 变量名称)
    std::map<std::string, std::vector<std::pair<std::string, std::string>>> registered_variables_;
    
    // 解析变量标识符
    std::pair<std::string, std::string> parse_variable_identifier(const std::string& identifier) const;

};

// 工厂函数
std::unique_ptr<simulation_structure> create_simulation_structure();

} // namespace simulation
} // namespace cosmo

#endif // COSMO_SIMULATION_SIMULATION_STRUCTURE_HPP