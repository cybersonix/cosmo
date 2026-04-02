/**
 * @file data_router.hpp
 * @brief 数据路由器 - 用于多模型间的数据传输管理
 * 
 * 数据路由器负责管理仿真中多个模型实例之间的数据流，
 * 支持一对一、一对多、多对一的变量连接模式。
 * 
 * @author COSMO Development Team
 * @version 0.1.0
 * @date 2026-03-27
 */

#ifndef COSMO_DATA_DATA_ROUTER_HPP
#define COSMO_DATA_DATA_ROUTER_HPP

#include "cosmo/fmi/fmi_types.hpp"
#include "cosmo/simulation/simulation_structure.hpp"

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace cosmo {
namespace data {

// 前向声明
class data_channel;
class data_router;

/**
 * @brief 数据传输类型枚举
 */
enum class transfer_type
{
    direct,       ///< 直接传输，无转换
    linear,       ///< 线性转换 (y = kx + b)
    scaling,      ///< 比例缩放 (y = kx)
    offset,       ///< 偏移转换 (y = x + b)
    custom        ///< 自定义转换函数
};

/**
 * @brief 数据通道端点
 * 
 * 描述数据通道的一个端点，包含模型实例和变量信息。
 */
struct channel_endpoint
{
    std::string model_instance;   ///< 模型实例名称
    std::string variable_name;    ///< 变量名称
    fmi::value_reference vr;      ///< 值引用
    
    channel_endpoint() : vr(0) {}
    channel_endpoint(const std::string& model, const std::string& var, fmi::value_reference ref = 0)
        : model_instance(model), variable_name(var), vr(ref) {}
};

/**
 * @brief 数据转换参数
 * 
 * 用于线性转换的参数结构。
 */
struct transform_params
{
    double scale = 1.0;      ///< 比例因子 k
    double offset = 0.0;     ///< 偏移量 b
    
    transform_params() = default;
    transform_params(double s, double o) : scale(s), offset(o) {}
};

/**
 * @brief 自定义转换函数类型
 */
using transform_function = std::function<double(double)>;

/**
 * @brief 数据通道类
 * 
 * 表示一个从源到目标的数据传输通道。
 * 支持多种传输类型和数据转换。
 */
class data_channel
{
public:
    /**
     * @brief 构造函数
     * @param source 源端点
     * @param target 目标端点
     * @param type 传输类型
     */
    data_channel(const channel_endpoint& source, const channel_endpoint& target,
                 transfer_type type = transfer_type::direct);
    
    /**
     * @brief 析构函数
     */
    ~data_channel() = default;
    
    // 禁用拷贝，允许移动
    data_channel(const data_channel&) = delete;
    data_channel& operator=(const data_channel&) = delete;
    data_channel(data_channel&&) = default;
    data_channel& operator=(data_channel&&) = default;
    
    /**
     * @brief 设置线性转换参数
     * @param scale 比例因子
     * @param offset 偏移量
     */
    void set_linear_transform(double scale, double offset);
    
    /**
     * @brief 设置自定义转换函数
     * @param func 转换函数
     */
    void set_custom_transform(transform_function func);
    
    /**
     * @brief 应用数据转换
     * @param value 输入值
     * @return 转换后的值
     */
    double apply_transform(double value) const;
    
    /**
     * @brief 获取源端点
     * @return 源端点的const引用
     */
    const channel_endpoint& get_source() const { return source_; }
    
    /**
     * @brief 获取目标端点
     * @return 目标端点的const引用
     */
    const channel_endpoint& get_target() const { return target_; }
    
    /**
     * @brief 获取传输类型
     * @return 传输类型枚举
     */
    transfer_type get_transfer_type() const { return type_; }
    
    /**
     * @brief 检查通道是否有效
     * @return 有效返回true
     */
    bool is_valid() const;
    
    /**
     * @brief 启用/禁用通道
     * @param enabled 是否启用
     */
    void set_enabled(bool enabled) { enabled_ = enabled; }
    
    /**
     * @brief 检查通道是否启用
     * @return 启用返回true
     */
    bool is_enabled() const { return enabled_; }
    
    /**
     * @brief 获取通道描述字符串
     * @return 描述字符串
     */
    std::string to_string() const;

private:
    channel_endpoint source_;           ///< 源端点
    channel_endpoint target_;           ///< 目标端点
    transfer_type type_;                ///< 传输类型
    transform_params params_;           ///< 转换参数
    transform_function custom_func_;    ///< 自定义转换函数
    bool enabled_ = true;               ///< 是否启用
};

/**
 * @brief 数据路由器类
 * 
 * 管理仿真中的所有数据传输通道，
 * 负责路由调度和数据传递。
 */
class data_router
{
public:
    /**
     * @brief 构造函数
     */
    data_router();
    
    /**
     * @brief 析构函数
     */
    ~data_router() = default;
    
    // 禁用拷贝，允许移动
    data_router(const data_router&) = delete;
    data_router& operator=(const data_router&) = delete;
    data_router(data_router&&) = default;
    data_router& operator=(data_router&&) = default;
    
    /**
     * @brief 添加数据通道
     * @param source 源端点
     * @param target 目标端点
     * @param type 传输类型
     * @return 通道ID，失败返回-1
     */
    int add_channel(const channel_endpoint& source, const channel_endpoint& target,
                    transfer_type type = transfer_type::direct);
    
    /**
     * @brief 从仿真结构添加连接
     * @param structure 仿真结构
     * @return 添加的通道数量
     */
    int add_channels_from_structure(const simulation::simulation_structure& structure);
    
    /**
     * @brief 移除数据通道
     * @param channel_id 通道ID
     * @return 成功返回true
     */
    bool remove_channel(int channel_id);
    
    /**
     * @brief 获取通道
     * @param channel_id 通道ID
     * @return 通道指针，不存在返回nullptr
     */
    data_channel* get_channel(int channel_id);
    
    /**
     * @brief 获取所有通道
     * @return 通道映射的const引用
     */
    const std::map<int, std::unique_ptr<data_channel>>& get_all_channels() const { return channels_; }
    
    /**
     * @brief 获取通道数量
     * @return 通道数量
     */
    size_t get_channel_count() const { return channels_.size(); }
    
    /**
     * @brief 清除所有通道
     */
    void clear_all_channels();
    
    /**
     * @brief 启用/禁用通道
     * @param channel_id 通道ID
     * @param enabled 是否启用
     * @return 成功返回true
     */
    bool set_channel_enabled(int channel_id, bool enabled);
    
    /**
     * @brief 获取源模型的所有输出通道
     * @param model_instance 模型实例名称
     * @return 通道ID列表
     */
    std::vector<int> get_output_channels(const std::string& model_instance) const;
    
    /**
     * @brief 获取目标模型的所有输入通道
     * @param model_instance 模型实例名称
     * @return 通道ID列表
     */
    std::vector<int> get_input_channels(const std::string& model_instance) const;
    
    /**
     * @brief 验证所有通道
     * @return 无效通道数量
     */
    int validate_all_channels();
    
    /**
     * @brief 获取路由器状态信息
     * @return 状态描述字符串
     */
    std::string get_status() const;
    
    /**
     * @brief 打印路由表
     */
    void print_routing_table() const;

private:
    std::map<int, std::unique_ptr<data_channel>> channels_;  ///< 通道映射
    int next_channel_id_ = 0;                                 ///< 下一个通道ID
    
    // 模型到通道的索引
    std::map<std::string, std::vector<int>> model_outputs_;   ///< 模型输出通道
    std::map<std::string, std::vector<int>> model_inputs_;    ///< 模型输入通道
    
    void update_model_indices(const data_channel& channel, int channel_id);
    void remove_model_indices(int channel_id);
};

/**
 * @brief 创建数据路由器实例（工厂函数）
 * @return 新的数据路由器实例
 */
std::unique_ptr<data_router> create_data_router();

/**
 * @brief 创建数据通道（工厂函数）
 * @param source 源端点
 * @param target 目标端点
 * @param type 传输类型
 * @return 新的数据通道实例
 */
std::unique_ptr<data_channel> create_data_channel(const channel_endpoint& source,
                                                   const channel_endpoint& target,
                                                   transfer_type type = transfer_type::direct);

} // namespace data
} // namespace cosmo

#endif // COSMO_DATA_DATA_ROUTER_HPP
