#include "cosmo/simulation/scenario.hpp"
#include "cosmo/simulation/simulation.hpp"
#include "cosmo/simulation/model_instance.hpp"
#include "cosmo/logger/logger.hpp"
#include "cosmo/util/xml_parser.hpp"
#include "cosmo/util/filesystem.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>

namespace cosmo {
namespace simulation {

const double scenario::DEFAULT_TOLERANCE = 1e-9;

// ============================================================================
// scenario 实现
// ============================================================================

scenario::scenario()
    : dispatch_cursor_(0)
    , bound_sim_(nullptr)
{
}

void scenario::bind(simulation* sim)
{
    bound_sim_ = sim;
}

void scenario::schedule(const variable_command& cmd)
{
    // 使用 upper_bound 保持同时间元素的插入顺序（稳定排序）
    auto it = std::upper_bound(timeline_.begin(), timeline_.end(), cmd,
        [](const variable_command& a, const variable_command& b) {
            return a.scheduled_time < b.scheduled_time;
        });
    timeline_.insert(it, cmd);
    // 如果新命令插入在 cursor 之前，需要回退 cursor
    size_t pos = static_cast<size_t>(it - timeline_.begin());
    if (pos < dispatch_cursor_) {
        dispatch_cursor_ = pos;
    }
}

void scenario::schedule_real(double time, const std::string& target, double value)
{
    schedule(variable_command::make_real(time, target, value));
}

void scenario::schedule_integer(double time, const std::string& target, int value)
{
    schedule(variable_command::make_integer(time, target, value));
}

void scenario::schedule_boolean(double time, const std::string& target, bool value)
{
    schedule(variable_command::make_boolean(time, target, value));
}

void scenario::schedule_string(double time, const std::string& target, const std::string& value)
{
    schedule(variable_command::make_string(time, target, value));
}

size_t scenario::command_count() const
{
    return timeline_.size();
}

const variable_command* scenario::peek_command(size_t idx) const
{
    if (idx < timeline_.size()) {
        return &timeline_[idx];
    }
    return nullptr;
}

void scenario::purge()
{
    timeline_.clear();
    dispatch_cursor_ = 0;
}

bool scenario::resolve_target(const std::string& target,
                              std::string& out_instance,
                              std::string& out_variable) const
{
    // 支持 "instance_name::variable_name" 格式（与仿真引擎核心 API 一致）
    size_t sep = target.find("::");
    if (sep == std::string::npos) {
        cosmo::log::error("无效的目标格式(需要 '实例名::变量名'): " + target);
        return false;
    }
    out_instance = target.substr(0, sep);
    out_variable = target.substr(sep + 2);
    return true;
}

bool scenario::fire_command(const variable_command& cmd)
{
    if (!bound_sim_) {
        cosmo::log::warn("场景命令已跳过: 无绑定的仿真实例");
        return false;
    }

    std::string instance_name;
    std::string var_name;
    if (!resolve_target(cmd.target, instance_name, var_name)) {
        return false;
    }

    model_instance* inst = bound_sim_->get_model_instance(instance_name);
    if (!inst) {
        cosmo::log::error("场景目标实例未找到: " + instance_name);
        return false;
    }

    fmi::value_reference vr = inst->get_value_reference(var_name);
    if (vr == static_cast<fmi::value_reference>(-1)) {
        cosmo::log::error("场景目标变量未找到: " + var_name);
        return false;
    }

    switch (cmd.kind) {
        case command_kind::assign_real:
            inst->set_real(vr, cmd.real_val);
            cosmo::log::debug("Scenario: " + cmd.target + " = " + std::to_string(cmd.real_val));
            break;
        case command_kind::assign_integer:
            inst->set_integer(vr, cmd.int_val);
            cosmo::log::debug("Scenario: " + cmd.target + " = " + std::to_string(cmd.int_val));
            break;
        case command_kind::assign_boolean:
            inst->set_boolean(vr, cmd.bool_val);
            cosmo::log::debug("Scenario: " + cmd.target + " = " + std::to_string(cmd.bool_val));
            break;
        case command_kind::assign_string:
            inst->set_string(vr, cmd.str_val);
            cosmo::log::debug("Scenario: " + cmd.target + " = " + cmd.str_val);
            break;
    }
    return true;
}

// ---- 监听器回调实现 ----

void scenario::on_pre_init(double /*start_time*/)
{
    dispatch_cursor_ = 0;
    cosmo::log::info("场景已就绪: " + std::to_string(timeline_.size()) +
                     " 条命令, 容差=" + std::to_string(DEFAULT_TOLERANCE));
}

void scenario::on_post_init(double start_time)
{
    // 初始化后立即触发 t≈0 的命令
    while (dispatch_cursor_ < timeline_.size()) {
        const variable_command& cmd = timeline_[dispatch_cursor_];
        if (std::abs(cmd.scheduled_time - start_time) <= DEFAULT_TOLERANCE) {
            fire_command(cmd);
            ++dispatch_cursor_;
        } else {
            break;
        }
    }
}

void scenario::on_pre_step(double /*current_time*/, double /*step_size*/)
{
    // 前置步骤：不执行命令（保持空实现）
}

void scenario::on_post_step(double current_time, double /*step_size*/)
{
    // 在步进完成后扫描并触发所有 scheduled_time <= current_time + eps 的命令
    while (dispatch_cursor_ < timeline_.size()) {
        const variable_command& cmd = timeline_[dispatch_cursor_];
        if (cmd.scheduled_time <= current_time + DEFAULT_TOLERANCE) {
            fire_command(cmd);
            ++dispatch_cursor_;
        } else {
            break;
        }
    }
}

void scenario::on_pre_terminate()
{
    cosmo::log::info("场景完成: 已执行 " + std::to_string(dispatch_cursor_) +
                     " / " + std::to_string(timeline_.size()) + " 条命令");
}

void scenario::on_post_terminate()
{
}

void scenario::on_reset()
{
    dispatch_cursor_ = 0;
    cosmo::log::info("场景已重置: 所有命令重新就绪");
}

std::string scenario::get_name() const
{
    return "scenario";
}

void scenario::set_owner(simulation* owner)
{
    bind(owner);
}

// ---- 序列化 ----

std::unique_ptr<scenario> scenario::from_xml(const std::string& xml_content)
{
    auto scen = make_scenario();

    auto doc = util::create_xml_document();
    if (!doc->load_string(xml_content)) {
        cosmo::log::error("Scenario XML 解析失败");
        return nullptr;
    }

    auto root = doc->get_root();
    if (!root) {
        cosmo::log::error("Scenario XML 根元素缺失");
        return nullptr;
    }

    // 解析各类动作节点
    struct action_info {
        const char* tag;
        command_kind kind;
    };
    const action_info kinds[] = {
        {"RealAction", command_kind::assign_real},
        {"IntegerAction", command_kind::assign_integer},
        {"BooleanAction", command_kind::assign_boolean},
        {"StringAction", command_kind::assign_string}
    };

    for (const auto& info : kinds) {
        auto nodes = root->get_children(info.tag);
        for (const auto& node : nodes) {
            double t = std::stod(node->get_attribute("time", "0"));
            std::string tgt = node->get_attribute("variable", "");
            if (tgt.empty()) continue;

            variable_command cmd;
            cmd.scheduled_time = t;
            cmd.target = tgt;
            cmd.kind = info.kind;

            switch (info.kind) {
                case command_kind::assign_real:
                    cmd.real_val = std::stod(node->get_attribute("value", "0"));
                    break;
                case command_kind::assign_integer:
                    cmd.int_val = std::stoi(node->get_attribute("value", "0"));
                    break;
                case command_kind::assign_boolean:
                    cmd.bool_val = (node->get_attribute("value", "false") == "true");
                    break;
                case command_kind::assign_string:
                    cmd.str_val = node->get_attribute("value", "");
                    break;
            }
            scen->schedule(cmd);
        }
    }

    cosmo::log::info("从XML加载了 " + std::to_string(scen->command_count()) + " 条场景命令");
    return scen;
}

std::unique_ptr<scenario> scenario::from_file(const std::string& path)
{
    if (!util::file_exists(path)) {
        cosmo::log::error("场景文件未找到: " + path);
        return nullptr;
    }

    std::string content = util::read_file(path);
    if (content.empty()) {
        cosmo::log::error("读取场景文件失败: " + path);
        return nullptr;
    }

    return from_xml(content);
}

// 工厂函数
std::unique_ptr<scenario> make_scenario()
{
    return std::unique_ptr<scenario>(new scenario());
}

} // namespace simulation
} // namespace cosmo
