/**
 * @file data_router.cpp
 * @brief 数据路由器实现
 * 
 * 实现数据路由器和数据通道的所有功能。
 * 
 * @copyright Copyright (c) 2026 COSMO Development Team
 */

#include "cosmo/data/data_router.hpp"
#include "cosmo/logger/logger.hpp"

#include <algorithm>
#include <sstream>
#include <iomanip>

namespace cosmo {
namespace data {

// data_channel implementation

data_channel::data_channel(const channel_endpoint& source, const channel_endpoint& target,
                           transfer_type type)
    : source_(source)
    , target_(target)
    , type_(type)
    , enabled_(true)
{
    cosmo::log::debug("创建数据通道: " + source_.model_instance + "::" + source_.variable_name +
                      " -> " + target_.model_instance + "::" + target_.variable_name);
}

void data_channel::set_linear_transform(double scale, double offset)
{
    params_.scale = scale;
    params_.offset = offset;
    type_ = transfer_type::linear;
    cosmo::log::debug("设置线性变换: scale=" + std::to_string(scale) + 
                      ", offset=" + std::to_string(offset));
}

void data_channel::set_custom_transform(transform_function func)
{
    custom_func_ = func;
    if (func) {
        type_ = transfer_type::custom;
    }
}

double data_channel::apply_transform(double value) const
{
    if (!enabled_) {
        return value;
    }
    
    switch (type_) {
        case transfer_type::direct:
            return value;
            
        case transfer_type::linear:
            return params_.scale * value + params_.offset;
            
        case transfer_type::scaling:
            return params_.scale * value;
            
        case transfer_type::offset:
            return value + params_.offset;
            
        case transfer_type::custom:
            if (custom_func_) {
                return custom_func_(value);
            }
            return value;
            
        default:
            return value;
    }
}

bool data_channel::is_valid() const
{
    return !source_.model_instance.empty() &&
           !source_.variable_name.empty() &&
           !target_.model_instance.empty() &&
           !target_.variable_name.empty() &&
           source_.model_instance != target_.model_instance;
}

std::string data_channel::to_string() const
{
    std::stringstream ss;
    ss << source_.model_instance << "::" << source_.variable_name
       << " -> " << target_.model_instance << "::" << target_.variable_name;
    
    switch (type_) {
        case transfer_type::direct:
            ss << " [direct]";
            break;
        case transfer_type::linear:
            ss << " [linear: " << params_.scale << "x + " << params_.offset << "]";
            break;
        case transfer_type::scaling:
            ss << " [scale: " << params_.scale << "]";
            break;
        case transfer_type::offset:
            ss << " [offset: " << params_.offset << "]";
            break;
        case transfer_type::custom:
            ss << " [custom]";
            break;
    }
    
    if (!enabled_) {
        ss << " (disabled)";
    }
    
    return ss.str();
}

// data_router implementation

data_router::data_router()
    : next_channel_id_(0)
{
    cosmo::log::info("数据路由器已创建");
}

int data_router::add_channel(const channel_endpoint& source, const channel_endpoint& target,
                             transfer_type type)
{
    // 创建通道
    auto channel = std::unique_ptr<data_channel>(new data_channel(source, target, type));
    
    // 验证通道
    if (!channel->is_valid()) {
        cosmo::log::error("无效的数据通道: " + source.model_instance + "::" + source.variable_name +
                         " -> " + target.model_instance + "::" + target.variable_name);
        return -1;
    }
    
    int id = next_channel_id_++;
    channels_[id] = std::move(channel);
    
    // 更新索引
    update_model_indices(*channels_[id], id);
    
    cosmo::log::info("添加数据通道 " + std::to_string(id) + ": " + 
                    channels_[id]->to_string());
    
    return id;
}

int data_router::add_channels_from_structure(const simulation::simulation_structure& structure)
{
    int count = 0;
    
    const auto& connections = structure.get_connections();
    for (const auto& conn : connections) {
        channel_endpoint source(conn.get_source_model(), conn.get_source_variable());
        channel_endpoint target(conn.get_target_model(), conn.get_target_variable());
        
        transfer_type type = transfer_type::direct;
        if (conn.has_modifier()) {
            type = transfer_type::custom;
        }
        
        int id = add_channel(source, target, type);
        if (id >= 0) {
            if (conn.has_modifier()) {
                // 将修饰器桥接为自定义转换函数
                auto* channel = get_channel(id);
                if (channel) {
                    auto mod = [&conn](double v) { return conn.apply_modifier(v); };
                    channel->set_custom_transform(mod);
                }
            }
            count++;
        }
    }
    
    cosmo::log::info("从仿真结构添加了 " + std::to_string(count) + " 个数据通道");
    return count;
}

bool data_router::remove_channel(int channel_id)
{
    auto it = channels_.find(channel_id);
    if (it == channels_.end()) {
        cosmo::log::warn("通道未找到: " + std::to_string(channel_id));
        return false;
    }
    
    // 移除索引
    remove_model_indices(channel_id);
    
    cosmo::log::info("移除数据通道 " + std::to_string(channel_id));
    channels_.erase(it);
    return true;
}

data_channel* data_router::get_channel(int channel_id)
{
    auto it = channels_.find(channel_id);
    if (it != channels_.end()) {
        return it->second.get();
    }
    return nullptr;
}

void data_router::clear_all_channels()
{
    channels_.clear();
    model_outputs_.clear();
    model_inputs_.clear();
    next_channel_id_ = 0;
    
    cosmo::log::info("所有数据通道已清除");
}

bool data_router::set_channel_enabled(int channel_id, bool enabled)
{
    auto* channel = get_channel(channel_id);
    if (!channel) {
        cosmo::log::warn("通道未找到: " + std::to_string(channel_id));
        return false;
    }
    
    channel->set_enabled(enabled);
    cosmo::log::info("通道 " + std::to_string(channel_id) + 
                    (enabled ? " enabled" : " disabled"));
    return true;
}

std::vector<int> data_router::get_output_channels(const std::string& model_instance) const
{
    auto it = model_outputs_.find(model_instance);
    if (it != model_outputs_.end()) {
        return it->second;
    }
    return {};
}

std::vector<int> data_router::get_input_channels(const std::string& model_instance) const
{
    auto it = model_inputs_.find(model_instance);
    if (it != model_inputs_.end()) {
        return it->second;
    }
    return {};
}

int data_router::validate_all_channels()
{
    int invalid_count = 0;
    
    for (const auto& pair : channels_) {
        if (!pair.second->is_valid()) {
            cosmo::log::warn("Invalid channel detected: " + std::to_string(pair.first));
            invalid_count++;
        }
    }
    
    if (invalid_count > 0) {
        cosmo::log::warn("发现 " + std::to_string(invalid_count) + " 个无效通道");
    } else {
        cosmo::log::info("所有通道验证通过");
    }
    
    return invalid_count;
}

std::string data_router::get_status() const
{
    std::stringstream ss;
    ss << "数据路由器状态:\n";
    ss << "  总通道数: " << channels_.size() << "\n";
    ss << "  输出模型数: " << model_outputs_.size() << "\n";
    ss << "  输入模型数: " << model_inputs_.size() << "\n";
    
    size_t enabled_count = 0;
    for (const auto& pair : channels_) {
        if (pair.second->is_enabled()) {
            enabled_count++;
        }
    }
    ss << "  启用通道: " << enabled_count << "\n";
    ss << "  禁用通道: " << (channels_.size() - enabled_count);
    
    return ss.str();
}

void data_router::print_routing_table() const
{
    cosmo::log::info("=== 数据路由表 ===");
    
    if (channels_.empty()) {
        cosmo::log::info("未配置数据通道");
        return;
    }
    
    for (const auto& pair : channels_) {
        cosmo::log::info("  [" + std::to_string(pair.first) + "] " + pair.second->to_string());
    }
    
    cosmo::log::info("==========================");
}

void data_router::update_model_indices(const data_channel& channel, int channel_id)
{
    const auto& source = channel.get_source();
    const auto& target = channel.get_target();
    
    model_outputs_[source.model_instance].push_back(channel_id);
    model_inputs_[target.model_instance].push_back(channel_id);
}

void data_router::remove_model_indices(int channel_id)
{
    auto* channel = get_channel(channel_id);
    if (!channel) {
        return;
    }
    
    const auto& source = channel->get_source();
    const auto& target = channel->get_target();
    
    // 从输出索引中移除
    auto out_it = model_outputs_.find(source.model_instance);
    if (out_it != model_outputs_.end()) {
        auto& vec = out_it->second;
        vec.erase(std::remove(vec.begin(), vec.end(), channel_id), vec.end());
        if (vec.empty()) {
            model_outputs_.erase(out_it);
        }
    }
    
    // 从输入索引中移除
    auto in_it = model_inputs_.find(target.model_instance);
    if (in_it != model_inputs_.end()) {
        auto& vec = in_it->second;
        vec.erase(std::remove(vec.begin(), vec.end(), channel_id), vec.end());
        if (vec.empty()) {
            model_inputs_.erase(in_it);
        }
    }
}

// 工厂函数

std::unique_ptr<data_router> create_data_router()
{
    return std::unique_ptr<data_router>(new data_router());
}

std::unique_ptr<data_channel> create_data_channel(const channel_endpoint& source,
                                                   const channel_endpoint& target,
                                                   transfer_type type)
{
    return std::unique_ptr<data_channel>(new data_channel(source, target, type));
}

} // namespace data
} // namespace cosmo
