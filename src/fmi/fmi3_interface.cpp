/**
 * @file fmi3_interface.cpp
 * @brief FMI 3.0接口实现
 * 
 * 实现FMI 3.0标准定义的模型实例操作，支持Co-Simulation和Model Exchange模式。
 * 
 * @copyright Copyright (c) 2024
 */

#include "cosmo/fmi/fmi3_interface.hpp"
#include "cosmo/logger/logger.hpp"
#include "cosmo/util/dynamic_library.hpp"
#include <cstring>

namespace cosmo {
namespace fmi {
namespace fmi3 {

namespace {

template<typename FT>
FT symbol_cast(void* sym) {
    union { void* ptr; FT func; } u;
    u.ptr = sym;
    return u.func;
}

} // anonymous namespace

fmi3_interface::fmi3_interface()
    : get_version_(nullptr)
    , instantiate_(nullptr)
    , free_instance_(nullptr)
    , enter_initialization_mode_(nullptr)
    , exit_initialization_mode_(nullptr)
    , terminate_(nullptr)
    , reset_(nullptr)
    , get_float64_(nullptr)
    , get_int32_(nullptr)
    , get_boolean_(nullptr)
    , get_string_(nullptr)
    , set_float64_(nullptr)
    , set_int32_(nullptr)
    , set_boolean_(nullptr)
    , set_string_(nullptr)
    , do_step_(nullptr)
{
}

fmi3_interface::~fmi3_interface()
{
    unload();
}

fmi3_interface::fmi3_interface(fmi3_interface&& other) noexcept
    : library_(std::move(other.library_))
    , error_(std::move(other.error_))
    , get_version_(other.get_version_)
    , instantiate_(other.instantiate_)
    , free_instance_(other.free_instance_)
    , enter_initialization_mode_(other.enter_initialization_mode_)
    , exit_initialization_mode_(other.exit_initialization_mode_)
    , terminate_(other.terminate_)
    , reset_(other.reset_)
    , get_float64_(other.get_float64_)
    , get_int32_(other.get_int32_)
    , get_boolean_(other.get_boolean_)
    , get_string_(other.get_string_)
    , set_float64_(other.set_float64_)
    , set_int32_(other.set_int32_)
    , set_boolean_(other.set_boolean_)
    , set_string_(other.set_string_)
    , do_step_(other.do_step_)
{
    other.get_version_ = nullptr;
    other.instantiate_ = nullptr;
    other.free_instance_ = nullptr;
    other.enter_initialization_mode_ = nullptr;
    other.exit_initialization_mode_ = nullptr;
    other.terminate_ = nullptr;
    other.reset_ = nullptr;
    other.get_float64_ = nullptr;
    other.get_int32_ = nullptr;
    other.get_boolean_ = nullptr;
    other.get_string_ = nullptr;
    other.set_float64_ = nullptr;
    other.set_int32_ = nullptr;
    other.set_boolean_ = nullptr;
    other.set_string_ = nullptr;
    other.do_step_ = nullptr;
}

fmi3_interface& fmi3_interface::operator=(fmi3_interface&& other) noexcept
{
    if (this != &other) {
        unload();
        library_ = std::move(other.library_);
        error_ = std::move(other.error_);
        get_version_ = other.get_version_;
        instantiate_ = other.instantiate_;
        free_instance_ = other.free_instance_;
        enter_initialization_mode_ = other.enter_initialization_mode_;
        exit_initialization_mode_ = other.exit_initialization_mode_;
        terminate_ = other.terminate_;
        reset_ = other.reset_;
        get_float64_ = other.get_float64_;
        get_int32_ = other.get_int32_;
        get_boolean_ = other.get_boolean_;
        get_string_ = other.get_string_;
        set_float64_ = other.set_float64_;
        set_int32_ = other.set_int32_;
        set_boolean_ = other.set_boolean_;
        set_string_ = other.set_string_;
        do_step_ = other.do_step_;

        other.get_version_ = nullptr;
        other.instantiate_ = nullptr;
        other.free_instance_ = nullptr;
        other.enter_initialization_mode_ = nullptr;
        other.exit_initialization_mode_ = nullptr;
        other.terminate_ = nullptr;
        other.reset_ = nullptr;
        other.get_float64_ = nullptr;
        other.get_int32_ = nullptr;
        other.get_boolean_ = nullptr;
        other.get_string_ = nullptr;
        other.set_float64_ = nullptr;
        other.set_int32_ = nullptr;
        other.set_boolean_ = nullptr;
        other.set_string_ = nullptr;
        other.do_step_ = nullptr;
    }
    return *this;
}

bool fmi3_interface::load_library(const std::string& library_path)
{
    unload();

    library_ = util::create_dynamic_library();
    if (!library_->load(library_path)) {
        error_ = "Failed to load library: " + library_path + " - " + library_->get_error();
        library_.reset();
        cosmo::log::error(error_);
        return false;
    }

    if (!load_function_pointers()) {
        unload();
        return false;
    }

    cosmo::log::info("FMI 3.0 library loaded: " + library_path);
    return true;
}

void fmi3_interface::unload()
{
    library_.reset();
    get_version_ = nullptr;
    instantiate_ = nullptr;
    free_instance_ = nullptr;
    enter_initialization_mode_ = nullptr;
    exit_initialization_mode_ = nullptr;
    terminate_ = nullptr;
    reset_ = nullptr;
    get_float64_ = nullptr;
    get_int32_ = nullptr;
    get_boolean_ = nullptr;
    get_string_ = nullptr;
    set_float64_ = nullptr;
    set_int32_ = nullptr;
    set_boolean_ = nullptr;
    set_string_ = nullptr;
    do_step_ = nullptr;
    error_.clear();
}

bool fmi3_interface::is_loaded() const
{
    return library_ && library_->is_loaded();
}

bool fmi3_interface::load_function_pointers()
{
    get_version_ = symbol_cast<fmi3_get_version_ft>(library_->get_symbol("fmi3GetVersion"));
    if (!get_version_) {
        error_ = "Failed to load fmi3GetVersion";
        return false;
    }

    instantiate_ = symbol_cast<fmi3_instantiate_ft>(library_->get_symbol("fmi3InstantiateCoSimulation"));
    if (!instantiate_) {
        error_ = "Failed to load fmi3InstantiateCoSimulation";
        return false;
    }

    free_instance_ = symbol_cast<fmi3_free_instance_ft>(library_->get_symbol("fmi3FreeInstance"));
    if (!free_instance_) {
        error_ = "Failed to load fmi3FreeInstance";
        return false;
    }

    enter_initialization_mode_ = symbol_cast<fmi3_enter_initialization_mode_ft>(library_->get_symbol("fmi3EnterInitializationMode"));
    if (!enter_initialization_mode_) {
        error_ = "Failed to load fmi3EnterInitializationMode";
        return false;
    }

    exit_initialization_mode_ = symbol_cast<fmi3_exit_initialization_mode_ft>(library_->get_symbol("fmi3ExitInitializationMode"));
    if (!exit_initialization_mode_) {
        error_ = "Failed to load fmi3ExitInitializationMode";
        return false;
    }

    terminate_ = symbol_cast<fmi3_terminate_ft>(library_->get_symbol("fmi3Terminate"));
    if (!terminate_) {
        error_ = "Failed to load fmi3Terminate";
        return false;
    }

    reset_ = symbol_cast<fmi3_reset_ft>(library_->get_symbol("fmi3Reset"));
    if (!reset_) {
        error_ = "Failed to load fmi3Reset";
        return false;
    }

    get_float64_ = symbol_cast<fmi3_get_float64_ft>(library_->get_symbol("fmi3GetFloat64"));
    if (!get_float64_) {
        error_ = "Failed to load fmi3GetFloat64";
        return false;
    }

    get_int32_ = symbol_cast<fmi3_get_int32_ft>(library_->get_symbol("fmi3GetInt32"));
    if (!get_int32_) {
        error_ = "Failed to load fmi3GetInt32";
        return false;
    }

    get_boolean_ = symbol_cast<fmi3_get_boolean_ft>(library_->get_symbol("fmi3GetBoolean"));
    if (!get_boolean_) {
        error_ = "Failed to load fmi3GetBoolean";
        return false;
    }

    get_string_ = symbol_cast<fmi3_get_string_ft>(library_->get_symbol("fmi3GetString"));
    if (!get_string_) {
        error_ = "Failed to load fmi3GetString";
        return false;
    }

    set_float64_ = symbol_cast<fmi3_set_float64_ft>(library_->get_symbol("fmi3SetFloat64"));
    if (!set_float64_) {
        error_ = "Failed to load fmi3SetFloat64";
        return false;
    }

    set_int32_ = symbol_cast<fmi3_set_int32_ft>(library_->get_symbol("fmi3SetInt32"));
    if (!set_int32_) {
        error_ = "Failed to load fmi3SetInt32";
        return false;
    }

    set_boolean_ = symbol_cast<fmi3_set_boolean_ft>(library_->get_symbol("fmi3SetBoolean"));
    if (!set_boolean_) {
        error_ = "Failed to load fmi3SetBoolean";
        return false;
    }

    set_string_ = symbol_cast<fmi3_set_string_ft>(library_->get_symbol("fmi3SetString"));
    if (!set_string_) {
        error_ = "Failed to load fmi3SetString";
        return false;
    }

    do_step_ = symbol_cast<fmi3_do_step_ft>(library_->get_symbol("fmi3DoStep"));
    if (!do_step_) {
        error_ = "Failed to load fmi3DoStep";
        return false;
    }

    return true;
}

const char* fmi3_interface::get_version()
{
    if (!get_version_) {
        error_ = "Library not loaded";
        return nullptr;
    }
    return get_version_();
}

fmi3_instance_t fmi3_interface::instantiate(
    const std::string& instance_name,
    const std::string& instantiation_token,
    const std::string& resource_path,
    bool visible,
    bool logging_on,
    fmi3_instance_environment_t instance_environment,
    fmi3_callback_functions_t* callbacks)
{
    if (!instantiate_) {
        error_ = "Library not loaded";
        return nullptr;
    }

    fmi3_instance_t instance = instantiate_(
        instance_name.c_str(),
        instantiation_token.c_str(),
        resource_path.c_str(),
        visible ? 1 : 0,
        logging_on ? 1 : 0,
        instance_environment,
        callbacks);

    if (!instance) {
        error_ = "FMI 3.0 instantiation failed";
        cosmo::log::error(error_);
    } else {
        cosmo::log::info("FMI 3.0 model instantiated: " + instance_name);
    }

    return instance;
}

void fmi3_interface::free_instance(fmi3_instance_t instance)
{
    if (free_instance_ && instance) {
        free_instance_(instance);
    }
}

fmi3_status_t fmi3_interface::enter_initialization_mode(
    fmi3_instance_t instance,
    bool tolerance_defined,
    fmi3_float64_t tolerance,
    fmi3_float64_t start_time,
    bool stop_time_defined,
    fmi3_float64_t stop_time)
{
    if (!enter_initialization_mode_ || !instance) {
        return fmi3_error;
    }
    return enter_initialization_mode_(instance, tolerance_defined ? 1 : 0, tolerance,
                                       start_time, stop_time_defined ? 1 : 0, stop_time);
}

fmi3_status_t fmi3_interface::exit_initialization_mode(fmi3_instance_t instance)
{
    if (!exit_initialization_mode_ || !instance) {
        return fmi3_error;
    }
    return exit_initialization_mode_(instance);
}

fmi3_status_t fmi3_interface::terminate(fmi3_instance_t instance)
{
    if (!terminate_ || !instance) {
        return fmi3_error;
    }
    return terminate_(instance);
}

fmi3_status_t fmi3_interface::reset(fmi3_instance_t instance)
{
    if (!reset_ || !instance) {
        return fmi3_error;
    }
    return reset_(instance);
}

fmi3_status_t fmi3_interface::get_float64(
    fmi3_instance_t instance,
    const fmi3_value_reference_t vr[],
    fmi3_size_t nvr,
    fmi3_float64_t values[],
    fmi3_size_t n_values)
{
    if (!get_float64_ || !instance) {
        return fmi3_error;
    }
    return get_float64_(instance, vr, nvr, values, n_values);
}

fmi3_status_t fmi3_interface::get_int32(
    fmi3_instance_t instance,
    const fmi3_value_reference_t vr[],
    fmi3_size_t nvr,
    fmi3_int32_t values[],
    fmi3_size_t n_values)
{
    if (!get_int32_ || !instance) {
        return fmi3_error;
    }
    return get_int32_(instance, vr, nvr, values, n_values);
}

fmi3_status_t fmi3_interface::get_boolean(
    fmi3_instance_t instance,
    const fmi3_value_reference_t vr[],
    fmi3_size_t nvr,
    fmi3_boolean_t values[],
    fmi3_size_t n_values)
{
    if (!get_boolean_ || !instance) {
        return fmi3_error;
    }
    return get_boolean_(instance, vr, nvr, values, n_values);
}

fmi3_status_t fmi3_interface::get_string(
    fmi3_instance_t instance,
    const fmi3_value_reference_t vr[],
    fmi3_size_t nvr,
    fmi3_string_t values[],
    fmi3_size_t n_values)
{
    if (!get_string_ || !instance) {
        return fmi3_error;
    }
    return get_string_(instance, vr, nvr, values, n_values);
}

fmi3_status_t fmi3_interface::set_float64(
    fmi3_instance_t instance,
    const fmi3_value_reference_t vr[],
    fmi3_size_t nvr,
    const fmi3_float64_t values[],
    fmi3_size_t n_values)
{
    if (!set_float64_ || !instance) {
        return fmi3_error;
    }
    return set_float64_(instance, vr, nvr, values, n_values);
}

fmi3_status_t fmi3_interface::set_int32(
    fmi3_instance_t instance,
    const fmi3_value_reference_t vr[],
    fmi3_size_t nvr,
    const fmi3_int32_t values[],
    fmi3_size_t n_values)
{
    if (!set_int32_ || !instance) {
        return fmi3_error;
    }
    return set_int32_(instance, vr, nvr, values, n_values);
}

fmi3_status_t fmi3_interface::set_boolean(
    fmi3_instance_t instance,
    const fmi3_value_reference_t vr[],
    fmi3_size_t nvr,
    const fmi3_boolean_t values[],
    fmi3_size_t n_values)
{
    if (!set_boolean_ || !instance) {
        return fmi3_error;
    }
    return set_boolean_(instance, vr, nvr, values, n_values);
}

fmi3_status_t fmi3_interface::set_string(
    fmi3_instance_t instance,
    const fmi3_value_reference_t vr[],
    fmi3_size_t nvr,
    const fmi3_string_t values[],
    fmi3_size_t n_values)
{
    if (!set_string_ || !instance) {
        return fmi3_error;
    }
    return set_string_(instance, vr, nvr, values, n_values);
}

fmi3_status_t fmi3_interface::do_step(
    fmi3_instance_t instance,
    fmi3_float64_t current_communication_point,
    fmi3_float64_t communication_step_size,
    bool no_set_fmu_state_prior,
    bool* event_handling_needed,
    bool* terminate_simulation,
    bool* early_return,
    fmi3_float64_t* last_successful_time)
{
    if (!do_step_ || !instance) {
        return fmi3_error;
    }

    fmi3_boolean_t event_needed = 0;
    fmi3_boolean_t terminate_flag = 0;
    fmi3_boolean_t early_ret = 0;

    fmi3_status_t status = do_step_(instance, current_communication_point, communication_step_size,
                                     no_set_fmu_state_prior ? 1 : 0,
                                     &event_needed, &terminate_flag, &early_ret, last_successful_time);

    if (event_handling_needed) {
        *event_handling_needed = (event_needed != 0);
    }
    if (terminate_simulation) {
        *terminate_simulation = (terminate_flag != 0);
    }
    if (early_return) {
        *early_return = (early_ret != 0);
    }

    return status;
}

const std::string& fmi3_interface::get_error() const
{
    return error_;
}

// 工厂函数
std::unique_ptr<fmi3_interface> create_fmi3_interface()
{
    return std::unique_ptr<fmi3_interface>(new fmi3_interface());
}

} // namespace fmi3
} // namespace fmi
} // namespace cosmo
