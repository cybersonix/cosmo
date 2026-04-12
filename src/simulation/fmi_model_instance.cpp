/**
 * @file fmi_model_instance.cpp
 * @brief 基于真实FMI接口的模型实例实现
 *
 * 桥接FMI底层接口（fmu_processor、fmi1_interface、fmi2_interface）
 * 到仿真引擎的model_instance抽象接口。
 *
 * @copyright Copyright (c) 2026 COSMO Development Team
 */

#include "cosmo/simulation/fmi_model_instance.hpp"
#include "cosmo/fmi/fmi1_interface.hpp"
#include "cosmo/fmi/fmi_interface.hpp"
#include "cosmo/fmi/fmi3_interface.hpp"
#include "cosmo/logger/logger.hpp"

#include <cstdarg>
#include <fstream>

namespace cosmo {
namespace simulation {

// ============================================================================
// fmi_model_instance
// ============================================================================

fmi_model_instance::fmi_model_instance(const std::string& instance_name,
                                       const std::string& fmu_path,
                                       double step_size_hint)
    : model_instance(instance_name, nullptr, step_size_hint)
    , fmu_loaded_(false)
    , fmi_version_(fmi::fmi_version::fmi_2_0)
    , fmi1_component_(nullptr)
    , fmi2_component_(nullptr)
    , fmi3_component_(nullptr)
    , inited_(false)
    , terminated_(false)
{
    // 创建FMU处理器
    fmu_processor_ = fmi::create_fmu_processor();
    if (!fmu_processor_->load_fmu(fmu_path)) {
        last_error_ = "无法加载FMU文件: " + fmu_path;
        cosmo::log::error(last_error_);
        return;
    }

    // 解析模型描述
    auto model_desc = fmu_processor_->parse_model_description();
    if (!model_desc) {
        last_error_ = "无法解析模型描述";
        cosmo::log::error(last_error_);
        return;
    }

    // 使用FMU中的模型描述
    model_description_ = std::move(model_desc);
    fmi_version_ = model_description_->get_version();

    cosmo::log::info("模型实例 '" + instance_name + "' FMI版本: " +
                     (fmi_version_ == fmi::fmi_version::fmi_1_0 ? "1.0" :
                      fmi_version_ == fmi::fmi_version::fmi_2_0 ? "2.0" : "3.0"));

    // 根据FMI版本加载对应接口
    if (fmi_version_ == fmi::fmi_version::fmi_1_0) {
        fmu_loaded_ = load_fmi1(fmu_path);
    } else if (fmi_version_ == fmi::fmi_version::fmi_2_0) {
        fmu_loaded_ = load_fmi2(fmu_path);
    } else if (fmi_version_ == fmi::fmi_version::fmi_3_0) {
        fmu_loaded_ = load_fmi3(fmu_path);
    } else {
        last_error_ = "不支持的FMI版本";
        cosmo::log::error(last_error_);
    }

    if (fmu_loaded_) {
        cosmo::log::info("成功加载FMU模型: " + instance_name + " (" +
                         model_description_->get_model_name() + ")");
    }
}

fmi_model_instance::~fmi_model_instance()
{
    if (fmu_loaded_) {
        if (inited_ && !terminated_) {
            terminate();
        }

        if (fmi_version_ == fmi::fmi_version::fmi_1_0 && fmi1_component_) {
            fmi1_iface_->free_slave_instance(fmi1_component_);
            fmi1_component_ = nullptr;
        } else if (fmi_version_ == fmi::fmi_version::fmi_3_0 && fmi3_component_) {
            fmi3_iface_->free_instance(fmi3_component_);
            fmi3_component_ = nullptr;
        } else if (fmi2_component_) {
            fmi2_iface_->free_instance(fmi2_component_);
            fmi2_component_ = nullptr;
        }
    }

    // 显式重置接口对象，确保在删除临时目录前卸载动态库
    fmi1_iface_.reset();
    fmi2_iface_.reset();
    fmi3_iface_.reset();

    if (fmu_processor_) {
        fmu_processor_->cleanup();
    }
}

// ============================================================================
// FMI 1.0 加载
// ============================================================================

bool fmi_model_instance::load_fmi1(const std::string& /*fmu_path*/)
{
    std::string binary_path = fmu_processor_->get_binary_path();
    if (binary_path.empty()) {
        last_error_ = "无法找到FMU二进制文件";
        cosmo::log::error(last_error_);
        return false;
    }

    cosmo::log::info("加载FMI 1.0二进制: " + binary_path);

    fmi1_iface_ = fmi::fmi1::create_fmi1_interface();
    // FMI 1.0使用modelIdentifier作为符号前缀，而非modelName
    // 从modelDescription.xml中提取modelIdentifier
    std::string model_id;
    {
        std::string xml_path = fmu_processor_->get_extract_dir() + "/modelDescription.xml";
        std::ifstream xml_file(xml_path);
        if (xml_file.is_open()) {
            std::string line;
            while (std::getline(xml_file, line)) {
                auto pos = line.find("modelIdentifier");
                if (pos != std::string::npos) {
                    auto start = line.find("\"", pos + 15);
                    if (start != std::string::npos) {
                        auto end = line.find("\"", start + 1);
                        if (end != std::string::npos) {
                            model_id = line.substr(start + 1, end - start - 1);
                        }
                    }
                    break;
                }
            }
        }
    }
    if (model_id.empty()) {
        model_id = model_description_->get_model_name();
    }

    if (!fmi1_iface_->load_library(binary_path, model_id)) {
        last_error_ = "无法加载FMI 1.0共享库: " + fmi1_iface_->get_error();
        cosmo::log::error(last_error_);
        return false;
    }

    // 设置回调函数
    fmi::fmi1::fmi1_callback_functions_t callbacks;
    callbacks.logger = reinterpret_cast<fmi::fmi1::fmi1_callback_logger_ft>(fmi1_logger);
    callbacks.allocate_memory = fmi1_allocate_memory;
    callbacks.free_memory = fmi1_free_memory;
    callbacks.step_finished = nullptr;

    std::string fmu_location = fmu_processor_->get_extract_dir();
    std::string guid = model_description_->get_guid();

    // 实例化FMU slave
    fmi1_component_ = fmi1_iface_->instantiate_slave(
        instance_name_, guid, fmu_location,
        0.0,   // timeout
        false, // visible
        false, // interactive
        &callbacks,
        false  // logging_on
    );

    if (!fmi1_component_) {
        last_error_ = "无法实例化FMI 1.0 FMU: " + fmi1_iface_->get_error();
        cosmo::log::error(last_error_);
        return false;
    }

    cosmo::log::info("FMI 1.0实例创建成功: " + instance_name_);
    return true;
}

// ============================================================================
// FMI 2.0 加载
// ============================================================================

bool fmi_model_instance::load_fmi2(const std::string& /*fmu_path*/)
{
    std::string binary_path = fmu_processor_->get_binary_path();
    if (binary_path.empty()) {
        last_error_ = "无法找到FMU二进制文件";
        cosmo::log::error(last_error_);
        return false;
    }

    cosmo::log::info("加载FMI 2.0二进制: " + binary_path);

    fmi2_iface_ = fmi::create_fmi_interface();

    if (!fmi2_iface_->load_library(binary_path)) {
        last_error_ = "无法加载FMI 2.0共享库: " + fmi2_iface_->get_error();
        cosmo::log::error(last_error_);
        return false;
    }

    // 设置回调函数
    fmi::fmi2::fmi2_callback_functions_t callbacks;
    callbacks.logger = reinterpret_cast<fmi::fmi2::fmi2_callback_logger_ft>(fmi2_logger);
    callbacks.allocate_memory = fmi2_allocate_memory;
    callbacks.free_memory = fmi2_free_memory;
    callbacks.step_finished = nullptr;
    callbacks.component_environment = nullptr;

    std::string fmu_resource_location = "file://" + fmu_processor_->get_extract_dir() + "/resources";
    std::string guid = model_description_->get_guid();

    fmi2_component_ = fmi2_iface_->instantiate(
        instance_name_,
        1, // fmi2CoSimulation (0=ModelExchange, 1=CoSimulation)
        guid,
        fmu_resource_location,
        &callbacks,
        false, // visible
        false  // logging_on
    );

    if (!fmi2_component_) {
        last_error_ = "无法实例化FMI 2.0 FMU";
        cosmo::log::error(last_error_);
        return false;
    }

    cosmo::log::info("FMI 2.0实例创建成功: " + instance_name_);
    return true;
}

// ============================================================================
// FMI 3.0 加载
// ============================================================================

bool fmi_model_instance::load_fmi3(const std::string& /*fmu_path*/)
{
    std::string binary_path = fmu_processor_->get_binary_path();
    if (binary_path.empty()) {
        last_error_ = "无法找到FMU二进制文件";
        cosmo::log::error(last_error_);
        return false;
    }

    cosmo::log::info("加载FMI 3.0二进制: " + binary_path);

    fmi3_iface_ = fmi::fmi3::create_fmi3_interface();

    if (!fmi3_iface_->load_library(binary_path)) {
        last_error_ = "无法加载FMI 3.0共享库: " + fmi3_iface_->get_error();
        cosmo::log::error(last_error_);
        return false;
    }

    std::string resource_path = fmu_processor_->get_extract_dir() + "/resources";
    std::string instantiation_token = model_description_->get_guid();

    fmi3_component_ = fmi3_iface_->instantiate(
        instance_name_,
        instantiation_token,
        resource_path,
        false, // visible
        false, // logging_on
        false, // event_mode_used
        false, // early_return_allowed
        nullptr, // instance_environment
        fmi3_log_message_callback,
        nullptr  // intermediate_update
    );

    if (!fmi3_component_) {
        last_error_ = "无法实例化FMI 3.0 FMU: " + fmi3_iface_->get_error();
        cosmo::log::error(last_error_);
        return false;
    }

    cosmo::log::info("FMI 3.0实例创建成功: " + instance_name_);
    return true;
}

// ============================================================================
// 生命周期管理
// ============================================================================

fmi::fmi_status fmi_model_instance::enter_initialization_mode(double /*start_time*/)
{
    if (!fmu_loaded_) {
        cosmo::log::error("FMU未加载: " + instance_name_);
        return fmi::fmi_error;
    }

    cosmo::log::info("FMI模型 '" + instance_name_ + "' 进入初始化模式");

    // FMI 1.0: 不需要enterInitializationMode，直接在instantiate后设参数
    // FMI 2.0: 调用setupExperiment和enterInitializationMode
    // FMI 3.0: 调用enterInitializationMode
    if (fmi_version_ == fmi::fmi_version::fmi_2_0 && fmi2_component_) {
        auto status = fmi2_iface_->setup_experiment(
            fmi2_component_, false, 0.0, 0.0, false, 0.0);
        if (status != fmi::fmi2::fmi2_status_ok) {
            cosmo::log::error("FMI 2.0 setupExperiment失败");
            return static_cast<fmi::fmi_status>(status);
        }

        status = fmi2_iface_->enter_initialization_mode(fmi2_component_);
        if (status != fmi::fmi2::fmi2_status_ok) {
            cosmo::log::error("FMI 2.0 enterInitializationMode失败");
            return static_cast<fmi::fmi_status>(status);
        }
    } else if (fmi_version_ == fmi::fmi_version::fmi_3_0 && fmi3_component_) {
        auto status = fmi3_iface_->enter_initialization_mode(
            fmi3_component_, false, 0.0, 0.0, false, 0.0);
        if (status != fmi::fmi3::fmi3_ok) {
            cosmo::log::error("FMI 3.0 enterInitializationMode失败");
            return static_cast<fmi::fmi_status>(status);
        }
    }

    return fmi::fmi_ok;
}

fmi::fmi_status fmi_model_instance::exit_initialization_mode()
{
    if (!fmu_loaded_) {
        return fmi::fmi_error;
    }

    cosmo::log::info("FMI模型 '" + instance_name_ + "' 退出初始化模式");

    if (fmi_version_ == fmi::fmi_version::fmi_1_0 && fmi1_component_) {
        // FMI 1.0 CS: 调用fmiInitializeSlave
        auto status = fmi1_iface_->initialize_slave(fmi1_component_, 0.0, false, 0.0);
        if (status != fmi::fmi1::fmi1_ok) {
            cosmo::log::error("FMI 1.0 initializeSlave失败");
            return static_cast<fmi::fmi_status>(status);
        }
    } else if (fmi_version_ == fmi::fmi_version::fmi_3_0 && fmi3_component_) {
        auto status = fmi3_iface_->exit_initialization_mode(fmi3_component_);
        if (status != fmi::fmi3::fmi3_ok) {
            cosmo::log::error("FMI 3.0 exitInitializationMode失败");
            return static_cast<fmi::fmi_status>(status);
        }
    } else if (fmi2_component_) {
        auto status = fmi2_iface_->exit_initialization_mode(fmi2_component_);
        if (status != fmi::fmi2::fmi2_status_ok) {
            cosmo::log::error("FMI 2.0 exitInitializationMode失败");
            return static_cast<fmi::fmi_status>(status);
        }
    }

    inited_ = true;
    return fmi::fmi_ok;
}

fmi::fmi_status fmi_model_instance::do_step(double current_time, double step_size)
{
    if (!fmu_loaded_ || !inited_) {
        cosmo::log::error("FMU未初始化: " + instance_name_);
        return fmi::fmi_error;
    }

    if (terminated_) {
        cosmo::log::error("FMU已终止: " + instance_name_);
        return fmi::fmi_error;
    }

    if (fmi_version_ == fmi::fmi_version::fmi_1_0 && fmi1_component_) {
        auto status = fmi1_iface_->do_step(fmi1_component_, current_time, step_size, true);
        return static_cast<fmi::fmi_status>(status);
    } else if (fmi_version_ == fmi::fmi_version::fmi_3_0 && fmi3_component_) {
        auto status = fmi3_iface_->do_step(
            fmi3_component_, current_time, step_size, true,
            nullptr, nullptr, nullptr, nullptr);
        return static_cast<fmi::fmi_status>(status);
    } else if (fmi2_component_) {
        auto status = fmi2_iface_->do_step(fmi2_component_, current_time, step_size, true);
        return static_cast<fmi::fmi_status>(status);
    }

    return fmi::fmi_error;
}

fmi::fmi_status fmi_model_instance::terminate()
{
    if (!fmu_loaded_ || !inited_) {
        return fmi::fmi_warning;
    }

    cosmo::log::info("FMI模型 '" + instance_name_ + "' 终止");

    if (fmi_version_ == fmi::fmi_version::fmi_1_0 && fmi1_component_) {
        auto status = fmi1_iface_->terminate_slave(fmi1_component_);
        terminated_ = true;
        return static_cast<fmi::fmi_status>(status);
    } else if (fmi_version_ == fmi::fmi_version::fmi_3_0 && fmi3_component_) {
        auto status = fmi3_iface_->terminate(fmi3_component_);
        terminated_ = true;
        return static_cast<fmi::fmi_status>(status);
    } else if (fmi2_component_) {
        auto status = fmi2_iface_->terminate(fmi2_component_);
        terminated_ = true;
        return static_cast<fmi::fmi_status>(status);
    }

    return fmi::fmi_error;
}

fmi::fmi_status fmi_model_instance::reset()
{
    if (!fmu_loaded_) {
        return fmi::fmi_error;
    }

    if (fmi_version_ == fmi::fmi_version::fmi_1_0 && fmi1_component_) {
        auto status = fmi1_iface_->reset_slave(fmi1_component_);
        inited_ = false;
        terminated_ = false;
        return static_cast<fmi::fmi_status>(status);
    } else if (fmi_version_ == fmi::fmi_version::fmi_3_0 && fmi3_component_) {
        auto status = fmi3_iface_->reset(fmi3_component_);
        inited_ = false;
        terminated_ = false;
        return static_cast<fmi::fmi_status>(status);
    } else if (fmi2_component_) {
        auto status = fmi2_iface_->reset(fmi2_component_);
        inited_ = false;
        terminated_ = false;
        return static_cast<fmi::fmi_status>(status);
    }

    return fmi::fmi_error;
}

// ============================================================================
// 变量访问
// ============================================================================

fmi::fmi_status fmi_model_instance::get_real(fmi::value_reference vr, fmi::fmi_real& value)
{
    if (fmi_version_ == fmi::fmi_version::fmi_1_0 && fmi1_component_) {
        auto status = fmi1_iface_->get_real(fmi1_component_, &vr, 1, &value);
        return static_cast<fmi::fmi_status>(status);
    } else if (fmi_version_ == fmi::fmi_version::fmi_3_0 && fmi3_component_) {
        fmi::fmi3::fmi3_float64_t val = 0.0;
        auto status = fmi3_iface_->get_float64(fmi3_component_, &vr, 1, &val, 1);
        value = val;
        return static_cast<fmi::fmi_status>(status);
    } else if (fmi2_component_) {
        auto status = fmi2_iface_->get_real(fmi2_component_, &vr, 1, &value);
        return static_cast<fmi::fmi_status>(status);
    }
    return fmi::fmi_error;
}

fmi::fmi_status fmi_model_instance::get_integer(fmi::value_reference vr, fmi::fmi_integer& value)
{
    if (fmi_version_ == fmi::fmi_version::fmi_1_0 && fmi1_component_) {
        auto status = fmi1_iface_->get_integer(fmi1_component_, &vr, 1, &value);
        return static_cast<fmi::fmi_status>(status);
    } else if (fmi_version_ == fmi::fmi_version::fmi_3_0 && fmi3_component_) {
        fmi::fmi3::fmi3_int32_t val = 0;
        auto status = fmi3_iface_->get_int32(fmi3_component_, &vr, 1, &val, 1);
        value = static_cast<fmi::fmi_integer>(val);
        return static_cast<fmi::fmi_status>(status);
    } else if (fmi2_component_) {
        auto status = fmi2_iface_->get_integer(fmi2_component_, &vr, 1, &value);
        return static_cast<fmi::fmi_status>(status);
    }
    return fmi::fmi_error;
}

fmi::fmi_status fmi_model_instance::get_boolean(fmi::value_reference vr, fmi::fmi_boolean& value)
{
    if (fmi_version_ == fmi::fmi_version::fmi_1_0 && fmi1_component_) {
        fmi::fmi1::fmi1_boolean_t raw_value = 0;
        auto status = fmi1_iface_->get_boolean(fmi1_component_, &vr, 1, &raw_value);
        value = (raw_value != 0);
        return static_cast<fmi::fmi_status>(status);
    } else if (fmi_version_ == fmi::fmi_version::fmi_3_0 && fmi3_component_) {
        fmi::fmi3::fmi3_boolean_t raw_value = 0;
        auto status = fmi3_iface_->get_boolean(fmi3_component_, &vr, 1, &raw_value, 1);
        value = (raw_value != 0);
        return static_cast<fmi::fmi_status>(status);
    } else if (fmi2_component_) {
        fmi::fmi2::fmi2_boolean_t raw_value = 0;
        auto status = fmi2_iface_->get_boolean(fmi2_component_, &vr, 1, &raw_value);
        value = (raw_value != 0);
        return static_cast<fmi::fmi_status>(status);
    }
    return fmi::fmi_error;
}

fmi::fmi_status fmi_model_instance::get_string(fmi::value_reference vr, fmi::fmi_string& value)
{
    if (fmi_version_ == fmi::fmi_version::fmi_1_0 && fmi1_component_) {
        fmi::fmi1::fmi1_string_t raw_value = nullptr;
        auto status = fmi1_iface_->get_string(fmi1_component_, &vr, 1, &raw_value);
        if (raw_value) value = raw_value;
        return static_cast<fmi::fmi_status>(status);
    } else if (fmi_version_ == fmi::fmi_version::fmi_3_0 && fmi3_component_) {
        fmi::fmi3::fmi3_string_t raw_value = nullptr;
        auto status = fmi3_iface_->get_string(fmi3_component_, &vr, 1, &raw_value, 1);
        if (raw_value) value = raw_value;
        return static_cast<fmi::fmi_status>(status);
    } else if (fmi2_component_) {
        fmi::fmi2::fmi2_string_t raw_value = nullptr;
        auto status = fmi2_iface_->get_string(fmi2_component_, &vr, 1, &raw_value);
        if (raw_value) value = raw_value;
        return static_cast<fmi::fmi_status>(status);
    }
    return fmi::fmi_error;
}

fmi::fmi_status fmi_model_instance::set_real(fmi::value_reference vr, fmi::fmi_real value)
{
    if (fmi_version_ == fmi::fmi_version::fmi_1_0 && fmi1_component_) {
        auto status = fmi1_iface_->set_real(fmi1_component_, &vr, 1, &value);
        return static_cast<fmi::fmi_status>(status);
    } else if (fmi_version_ == fmi::fmi_version::fmi_3_0 && fmi3_component_) {
        fmi::fmi3::fmi3_float64_t val = value;
        auto status = fmi3_iface_->set_float64(fmi3_component_, &vr, 1, &val, 1);
        return static_cast<fmi::fmi_status>(status);
    } else if (fmi2_component_) {
        auto status = fmi2_iface_->set_real(fmi2_component_, &vr, 1, &value);
        return static_cast<fmi::fmi_status>(status);
    }
    return fmi::fmi_error;
}

fmi::fmi_status fmi_model_instance::set_integer(fmi::value_reference vr, fmi::fmi_integer value)
{
    if (fmi_version_ == fmi::fmi_version::fmi_1_0 && fmi1_component_) {
        auto status = fmi1_iface_->set_integer(fmi1_component_, &vr, 1, &value);
        return static_cast<fmi::fmi_status>(status);
    } else if (fmi_version_ == fmi::fmi_version::fmi_3_0 && fmi3_component_) {
        fmi::fmi3::fmi3_int32_t val = static_cast<fmi::fmi3::fmi3_int32_t>(value);
        auto status = fmi3_iface_->set_int32(fmi3_component_, &vr, 1, &val, 1);
        return static_cast<fmi::fmi_status>(status);
    } else if (fmi2_component_) {
        auto status = fmi2_iface_->set_integer(fmi2_component_, &vr, 1, &value);
        return static_cast<fmi::fmi_status>(status);
    }
    return fmi::fmi_error;
}

fmi::fmi_status fmi_model_instance::set_boolean(fmi::value_reference vr, fmi::fmi_boolean value)
{
    if (fmi_version_ == fmi::fmi_version::fmi_1_0 && fmi1_component_) {
        fmi::fmi1::fmi1_boolean_t raw_value = value ? 1 : 0;
        auto status = fmi1_iface_->set_boolean(fmi1_component_, &vr, 1, &raw_value);
        return static_cast<fmi::fmi_status>(status);
    } else if (fmi_version_ == fmi::fmi_version::fmi_3_0 && fmi3_component_) {
        fmi::fmi3::fmi3_boolean_t raw_value = value ? 1 : 0;
        auto status = fmi3_iface_->set_boolean(fmi3_component_, &vr, 1, &raw_value, 1);
        return static_cast<fmi::fmi_status>(status);
    } else if (fmi2_component_) {
        fmi::fmi2::fmi2_boolean_t raw_value = value ? 1 : 0;
        auto status = fmi2_iface_->set_boolean(fmi2_component_, &vr, 1, &raw_value);
        return static_cast<fmi::fmi_status>(status);
    }
    return fmi::fmi_error;
}

fmi::fmi_status fmi_model_instance::set_string(fmi::value_reference vr, const fmi::fmi_string& value)
{
    if (fmi_version_ == fmi::fmi_version::fmi_1_0 && fmi1_component_) {
        fmi::fmi1::fmi1_string_t raw_value = value.c_str();
        auto status = fmi1_iface_->set_string(fmi1_component_, &vr, 1, &raw_value);
        return static_cast<fmi::fmi_status>(status);
    } else if (fmi_version_ == fmi::fmi_version::fmi_3_0 && fmi3_component_) {
        fmi::fmi3::fmi3_string_t raw_value = value.c_str();
        auto status = fmi3_iface_->set_string(fmi3_component_, &vr, 1, &raw_value, 1);
        return static_cast<fmi::fmi_status>(status);
    } else if (fmi2_component_) {
        fmi::fmi2::fmi2_string_t raw_value = value.c_str();
        auto status = fmi2_iface_->set_string(fmi2_component_, &vr, 1, &raw_value);
        return static_cast<fmi::fmi_status>(status);
    }
    return fmi::fmi_error;
}

// ============================================================================
// 回调函数
// ============================================================================

void fmi_model_instance::fmi1_logger(void* /*component*/,
                                      const char* /*instance_name*/,
                                      int status,
                                      const char* /*category*/,
                                      const char* message, ...)
{
    char buffer[4096];
    va_list args;
    va_start(args, message);
    vsnprintf(buffer, sizeof(buffer), message, args);
    va_end(args);

    switch (status) {
        case fmi::fmi1::fmi1_ok:
            cosmo::log::debug(std::string("[FMI1] ") + buffer);
            break;
        case fmi::fmi1::fmi1_warning:
            cosmo::log::warn(std::string("[FMI1] ") + buffer);
            break;
        case fmi::fmi1::fmi1_error:
            cosmo::log::error(std::string("[FMI1] ") + buffer);
            break;
        default:
            cosmo::log::info(std::string("[FMI1] ") + buffer);
            break;
    }
}

void fmi_model_instance::fmi2_logger(void* /*component_env*/,
                                      const char* /*instance_name*/,
                                      int status,
                                      const char* /*category*/,
                                      const char* message, ...)
{
    char buffer[4096];
    va_list args;
    va_start(args, message);
    vsnprintf(buffer, sizeof(buffer), message, args);
    va_end(args);

    switch (status) {
        case fmi::fmi2::fmi2_status_ok:
            cosmo::log::debug(std::string("[FMI2] ") + buffer);
            break;
        case fmi::fmi2::fmi2_status_warning:
            cosmo::log::warn(std::string("[FMI2] ") + buffer);
            break;
        case fmi::fmi2::fmi2_status_error:
            cosmo::log::error(std::string("[FMI2] ") + buffer);
            break;
        default:
            cosmo::log::info(std::string("[FMI2] ") + buffer);
            break;
    }
}

void* fmi_model_instance::fmi1_allocate_memory(size_t n, size_t size)
{
    return calloc(n, size);
}

void fmi_model_instance::fmi1_free_memory(void* obj)
{
    free(obj);
}

void* fmi_model_instance::fmi2_allocate_memory(size_t n, size_t size)
{
    return calloc(n, size);
}

void fmi_model_instance::fmi2_free_memory(void* obj)
{
    free(obj);
}

void fmi_model_instance::fmi3_log_message_callback(void* /*instance_environment*/,
                                                     fmi::fmi3::fmi3_status_t status,
                                                     const char* /*category*/,
                                                     const char* message)
{
    if (!message) return;

    switch (status) {
        case fmi::fmi3::fmi3_ok:
            cosmo::log::debug(std::string("[FMI3] ") + message);
            break;
        case fmi::fmi3::fmi3_warning:
            cosmo::log::warn(std::string("[FMI3] ") + message);
            break;
        case fmi::fmi3::fmi3_error:
            cosmo::log::error(std::string("[FMI3] ") + message);
            break;
        case fmi::fmi3::fmi3_fatal:
            cosmo::log::error(std::string("[FMI3 FATAL] ") + message);
            break;
        default:
            cosmo::log::info(std::string("[FMI3] ") + message);
            break;
    }
}

// ============================================================================
// 参数集管理
// ============================================================================

bool fmi_model_instance::apply_parameter_set(const std::string& name)
{
    auto it = parameter_sets_.find(name);
    if (it == parameter_sets_.end()) {
        cosmo::log::warn("参数集 '" + name + "' 未找到");
        return false;
    }

    if (!fmu_loaded_) {
        cosmo::log::warn("FMU未加载，无法应用参数集");
        return false;
    }

    const auto& params = it->second;
    int applied = 0;

    for (const auto& param : params) {
        // 参数key是variable_identifier，包含model_name和variable_name
        // 但对于单个模型实例，只需要variable_name
        const std::string& var_name = param.first.variable_name;
        fmi::value_reference vr = get_value_reference(var_name);
        if (vr == static_cast<fmi::value_reference>(-1)) {
            cosmo::log::warn("参数变量未找到: " + var_name);
            continue;
        }

        // 根据变量类型设置参数值
        const auto* var_desc = model_description_->get_variable_by_vr(vr);
        if (!var_desc) continue;

        fmi::fmi_status status = fmi::fmi_error;
        switch (var_desc->type) {
            case fmi::variable_type::real: {
                fmi::fmi_real val = (param.second.value_type == scalar_value::type::real)
                    ? param.second.real_value : 0.0;
                status = set_real(vr, val);
                break;
            }
            case fmi::variable_type::integer: {
                fmi::fmi_integer val = (param.second.value_type == scalar_value::type::integer)
                    ? param.second.integer_value : 0;
                status = set_integer(vr, val);
                break;
            }
            case fmi::variable_type::boolean: {
                fmi::fmi_boolean val = (param.second.value_type == scalar_value::type::boolean)
                    ? param.second.boolean_value : false;
                status = set_boolean(vr, val);
                break;
            }
            case fmi::variable_type::string: {
                fmi::fmi_string val = (param.second.value_type == scalar_value::type::string)
                    ? param.second.string_value : "";
                status = set_string(vr, val);
                break;
            }
            default:
                break;
        }

        if (status == fmi::fmi_ok) {
            applied++;
        } else {
            cosmo::log::warn("设置参数 '" + var_name + "' 失败");
        }
    }

    cosmo::log::info("已应用参数集 '" + name + "' 到FMU实例 '" + instance_name_ +
                     "'（" + std::to_string(applied) + "/" +
                     std::to_string(params.size()) + " 个参数成功）");
    return true;
}

// ============================================================================
// 工厂函数
// ============================================================================

std::unique_ptr<model_instance> create_fmi_model_instance(
    const std::string& instance_name,
    const std::string& fmu_path,
    double step_size_hint)
{
    try {
        auto inst = std::unique_ptr<model_instance>(
            new fmi_model_instance(instance_name, fmu_path, step_size_hint));

        auto* fmi_inst = dynamic_cast<fmi_model_instance*>(inst.get());
        if (fmi_inst && !fmi_inst->is_fmu_loaded()) {
            cosmo::log::error("无法创建FMI模型实例: " + fmi_inst->get_last_error());
            return nullptr;
        }
        return inst;
    } catch (const std::exception& e) {
        cosmo::log::warn("FMU加载异常，回退到mock实例: " + fmu_path +
            " (" + e.what() + ")");
        return nullptr;
    } catch (...) {
        cosmo::log::warn("FMU加载未知异常，回退到mock实例: " + fmu_path);
        return nullptr;
    }
}

} // namespace simulation
} // namespace cosmo
