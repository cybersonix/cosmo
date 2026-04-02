/**
 * @file fmi1_interface.cpp
 * @brief FMI 1.0接口实现
 */

#include "cosmo/fmi/fmi1_interface.hpp"
#include "cosmo/logger/logger.hpp"
#include "cosmo/util/dynamic_library.hpp"

namespace cosmo {
namespace fmi {
namespace fmi1 {

fmi1_interface::fmi1_interface()
    : is_cs_(false)
    , get_version_(nullptr)
    , instantiate_model_(nullptr)
    , instantiate_slave_(nullptr)
    , free_model_instance_(nullptr)
    , free_slave_instance_(nullptr)
    , initialize_slave_(nullptr)
    , terminate_slave_(nullptr)
    , reset_slave_(nullptr)
    , do_step_(nullptr)
    , set_time_(nullptr)
    , initialize_(nullptr)
    , get_derivatives_(nullptr)
    , get_event_indicators_(nullptr)
    , event_update_(nullptr)
    , get_continuous_states_(nullptr)
    , set_continuous_states_(nullptr)
    , get_real_(nullptr)
    , get_integer_(nullptr)
    , get_boolean_(nullptr)
    , get_string_(nullptr)
    , set_real_(nullptr)
    , set_integer_(nullptr)
    , set_boolean_(nullptr)
    , set_string_(nullptr)
{
}

fmi1_interface::~fmi1_interface()
{
    unload();
}

fmi1_interface::fmi1_interface(fmi1_interface&& other) noexcept
    : library_(std::move(other.library_))
    , error_(std::move(other.error_))
    , is_cs_(other.is_cs_)
{
    // 移动函数指针
    get_version_ = other.get_version_;
    instantiate_model_ = other.instantiate_model_;
    instantiate_slave_ = other.instantiate_slave_;
    free_model_instance_ = other.free_model_instance_;
    free_slave_instance_ = other.free_slave_instance_;
    set_time_ = other.set_time_;
    initialize_ = other.initialize_;
    initialize_slave_ = other.initialize_slave_;
    terminate_slave_ = other.terminate_slave_;
    reset_slave_ = other.reset_slave_;
    get_derivatives_ = other.get_derivatives_;
    get_event_indicators_ = other.get_event_indicators_;
    event_update_ = other.event_update_;
    get_continuous_states_ = other.get_continuous_states_;
    set_continuous_states_ = other.set_continuous_states_;
    do_step_ = other.do_step_;
    get_real_ = other.get_real_;
    get_integer_ = other.get_integer_;
    get_boolean_ = other.get_boolean_;
    get_string_ = other.get_string_;
    set_real_ = other.set_real_;
    set_integer_ = other.set_integer_;
    set_boolean_ = other.set_boolean_;
    set_string_ = other.set_string_;

    // 将源对象所有函数指针置零
    other.get_version_ = nullptr;
    other.instantiate_model_ = nullptr;
    other.instantiate_slave_ = nullptr;
    other.free_model_instance_ = nullptr;
    other.free_slave_instance_ = nullptr;
    other.set_time_ = nullptr;
    other.initialize_ = nullptr;
    other.initialize_slave_ = nullptr;
    other.terminate_slave_ = nullptr;
    other.reset_slave_ = nullptr;
    other.get_derivatives_ = nullptr;
    other.get_event_indicators_ = nullptr;
    other.event_update_ = nullptr;
    other.get_continuous_states_ = nullptr;
    other.set_continuous_states_ = nullptr;
    other.do_step_ = nullptr;
    other.get_real_ = nullptr;
    other.get_integer_ = nullptr;
    other.get_boolean_ = nullptr;
    other.get_string_ = nullptr;
    other.set_real_ = nullptr;
    other.set_integer_ = nullptr;
    other.set_boolean_ = nullptr;
    other.set_string_ = nullptr;
}

fmi1_interface& fmi1_interface::operator=(fmi1_interface&& other) noexcept
{
    if (this != &other) {
        unload();
        library_ = std::move(other.library_);
        error_ = std::move(other.error_);
        is_cs_ = other.is_cs_;

        get_version_ = other.get_version_;
        instantiate_model_ = other.instantiate_model_;
        instantiate_slave_ = other.instantiate_slave_;
        free_model_instance_ = other.free_model_instance_;
        free_slave_instance_ = other.free_slave_instance_;
        set_time_ = other.set_time_;
        initialize_ = other.initialize_;
        initialize_slave_ = other.initialize_slave_;
        terminate_slave_ = other.terminate_slave_;
        reset_slave_ = other.reset_slave_;
        get_derivatives_ = other.get_derivatives_;
        get_event_indicators_ = other.get_event_indicators_;
        event_update_ = other.event_update_;
        get_continuous_states_ = other.get_continuous_states_;
        set_continuous_states_ = other.set_continuous_states_;
        do_step_ = other.do_step_;
        get_real_ = other.get_real_;
        get_integer_ = other.get_integer_;
        get_boolean_ = other.get_boolean_;
        get_string_ = other.get_string_;
        set_real_ = other.set_real_;
        set_integer_ = other.set_integer_;
        set_boolean_ = other.set_boolean_;
        set_string_ = other.set_string_;

        other.get_version_ = nullptr;
        other.instantiate_model_ = nullptr;
        other.instantiate_slave_ = nullptr;
        other.free_model_instance_ = nullptr;
        other.free_slave_instance_ = nullptr;
        other.set_time_ = nullptr;
        other.initialize_ = nullptr;
        other.initialize_slave_ = nullptr;
        other.terminate_slave_ = nullptr;
        other.reset_slave_ = nullptr;
        other.get_derivatives_ = nullptr;
        other.get_event_indicators_ = nullptr;
        other.event_update_ = nullptr;
        other.get_continuous_states_ = nullptr;
        other.set_continuous_states_ = nullptr;
        other.do_step_ = nullptr;
        other.get_real_ = nullptr;
        other.get_integer_ = nullptr;
        other.get_boolean_ = nullptr;
        other.get_string_ = nullptr;
        other.set_real_ = nullptr;
        other.set_integer_ = nullptr;
        other.set_boolean_ = nullptr;
        other.set_string_ = nullptr;
    }
    return *this;
}

bool fmi1_interface::load_library(const std::string& library_path)
{
    return load_library(library_path, "");
}

bool fmi1_interface::load_library(const std::string& library_path, const std::string& model_identifier)
{
    if (is_loaded()) {
        unload();
    }

    model_identifier_ = model_identifier;

    library_ = util::create_dynamic_library();
    if (!library_->load(library_path)) {
        error_ = library_->get_error();
        return false;
    }

    if (!load_function_pointers()) {
        unload();
        return false;
    }

    return true;
}

void fmi1_interface::unload()
{
    if (library_) {
        library_->unload();
        library_.reset();
    }
    error_.clear();

    // 重置所有函数指针
    get_version_ = nullptr;
    instantiate_model_ = nullptr;
    instantiate_slave_ = nullptr;
    free_model_instance_ = nullptr;
    free_slave_instance_ = nullptr;
    set_time_ = nullptr;
    initialize_ = nullptr;
    initialize_slave_ = nullptr;
    terminate_slave_ = nullptr;
    reset_slave_ = nullptr;
    get_derivatives_ = nullptr;
    get_event_indicators_ = nullptr;
    event_update_ = nullptr;
    get_continuous_states_ = nullptr;
    set_continuous_states_ = nullptr;
    do_step_ = nullptr;
    get_real_ = nullptr;
    get_integer_ = nullptr;
    get_boolean_ = nullptr;
    get_string_ = nullptr;
    set_real_ = nullptr;
    set_integer_ = nullptr;
    set_boolean_ = nullptr;
    set_string_ = nullptr;
}

bool fmi1_interface::is_loaded() const
{
    return library_ && library_->is_loaded();
}

namespace {

template<typename FT>
FT symbol_cast(void* sym) {
    union { void* ptr; FT func; } u;
    u.ptr = sym;
    return u.func;
}

} // anonymous namespace

bool fmi1_interface::load_function_pointers()
{
    if (!library_ || !library_->is_loaded()) {
        error_ = "Library not loaded";
        return false;
    }

    // 构建函数名前缀（FMI 1.0标准使用模型标识符作为前缀）
    std::string prefix = model_identifier_.empty() ? "" : model_identifier_ + "_";

    // 尝试加载Co-Simulation函数（先尝试带前缀的名称）
    instantiate_slave_ = symbol_cast<fmi1_instantiate_slave_ft>(
        library_->get_symbol(prefix + "fmiInstantiateSlave"));
    do_step_ = symbol_cast<fmi1_do_step_ft>(
        library_->get_symbol(prefix + "fmiDoStep"));

    // 如果带前缀的找不到，尝试无前缀（兼容）
    if (!instantiate_slave_) {
        instantiate_slave_ = symbol_cast<fmi1_instantiate_slave_ft>(
            library_->get_symbol("fmiInstantiateSlave"));
    }
    if (!do_step_) {
        do_step_ = symbol_cast<fmi1_do_step_ft>(
            library_->get_symbol("fmiDoStep"));
    }

    // 如果找到CS函数，标记为CS模式
    is_cs_ = (instantiate_slave_ != nullptr && do_step_ != nullptr);

    // 加载通用函数
    get_version_ = symbol_cast<fmi1_get_version_ft>(
        library_->get_symbol(prefix + "fmiGetVersion"));
    if (!get_version_) {
        get_version_ = symbol_cast<fmi1_get_version_ft>(
            library_->get_symbol("fmiGetVersion"));
    }
    get_real_ = symbol_cast<fmi1_get_real_ft>(
        library_->get_symbol(prefix + "fmiGetReal"));
    if (!get_real_) {
        get_real_ = symbol_cast<fmi1_get_real_ft>(
            library_->get_symbol("fmiGetReal"));
    }
    get_integer_ = symbol_cast<fmi1_get_integer_ft>(
        library_->get_symbol(prefix + "fmiGetInteger"));
    if (!get_integer_) {
        get_integer_ = symbol_cast<fmi1_get_integer_ft>(
            library_->get_symbol("fmiGetInteger"));
    }
    get_boolean_ = symbol_cast<fmi1_get_boolean_ft>(
        library_->get_symbol(prefix + "fmiGetBoolean"));
    if (!get_boolean_) {
        get_boolean_ = symbol_cast<fmi1_get_boolean_ft>(
            library_->get_symbol("fmiGetBoolean"));
    }
    get_string_ = symbol_cast<fmi1_get_string_ft>(
        library_->get_symbol(prefix + "fmiGetString"));
    if (!get_string_) {
        get_string_ = symbol_cast<fmi1_get_string_ft>(
            library_->get_symbol("fmiGetString"));
    }
    set_real_ = symbol_cast<fmi1_set_real_ft>(
        library_->get_symbol(prefix + "fmiSetReal"));
    if (!set_real_) {
        set_real_ = symbol_cast<fmi1_set_real_ft>(
            library_->get_symbol("fmiSetReal"));
    }
    set_integer_ = symbol_cast<fmi1_set_integer_ft>(
        library_->get_symbol(prefix + "fmiSetInteger"));
    if (!set_integer_) {
        set_integer_ = symbol_cast<fmi1_set_integer_ft>(
            library_->get_symbol("fmiSetInteger"));
    }
    set_boolean_ = symbol_cast<fmi1_set_boolean_ft>(
        library_->get_symbol(prefix + "fmiSetBoolean"));
    if (!set_boolean_) {
        set_boolean_ = symbol_cast<fmi1_set_boolean_ft>(
            library_->get_symbol("fmiSetBoolean"));
    }
    set_string_ = symbol_cast<fmi1_set_string_ft>(
        library_->get_symbol(prefix + "fmiSetString"));
    if (!set_string_) {
        set_string_ = symbol_cast<fmi1_set_string_ft>(
            library_->get_symbol("fmiSetString"));
    }

    // 加载CS专用函数
    free_slave_instance_ = symbol_cast<fmi1_free_slave_instance_ft>(
        library_->get_symbol(prefix + "fmiFreeSlaveInstance"));
    if (!free_slave_instance_) {
        free_slave_instance_ = symbol_cast<fmi1_free_slave_instance_ft>(
            library_->get_symbol("fmiFreeSlaveInstance"));
    }
    initialize_slave_ = symbol_cast<fmi1_initialize_slave_ft>(
        library_->get_symbol(prefix + "fmiInitializeSlave"));
    if (!initialize_slave_) {
        initialize_slave_ = symbol_cast<fmi1_initialize_slave_ft>(
            library_->get_symbol("fmiInitializeSlave"));
    }
    terminate_slave_ = symbol_cast<fmi1_terminate_slave_ft>(
        library_->get_symbol(prefix + "fmiTerminateSlave"));
    if (!terminate_slave_) {
        terminate_slave_ = symbol_cast<fmi1_terminate_slave_ft>(
            library_->get_symbol("fmiTerminateSlave"));
    }
    reset_slave_ = symbol_cast<fmi1_reset_slave_ft>(
        library_->get_symbol(prefix + "fmiResetSlave"));
    if (!reset_slave_) {
        reset_slave_ = symbol_cast<fmi1_reset_slave_ft>(
            library_->get_symbol("fmiResetSlave"));
    }

    if (!get_version_) {
        error_ = "Failed to load required FMI 1.0 functions (tried prefix: '" + prefix + "')";
        return false;
    }

    return true;
}

const char* fmi1_interface::get_version()
{
    if (!get_version_) {
        error_ = "get_version function not loaded";
        return nullptr;
    }
    return get_version_();
}

fmi1_component_t fmi1_interface::instantiate_slave(
    const std::string& instance_name,
    const std::string& guid,
    const std::string& fmu_location,
    fmi1_real_t timeout,
    bool visible,
    bool interactive,
    const fmi1_callback_functions_t* functions,
    bool logging_on)
{
    if (!instantiate_slave_) {
        error_ = "instantiate_slave function not loaded";
        return nullptr;
    }
    return instantiate_slave_(
        instance_name.c_str(),
        guid.c_str(),
        fmu_location.c_str(),
        "application/x-fmu-sharedlibrary",
        timeout,
        visible ? 1 : 0,
        interactive ? 1 : 0,
        const_cast<fmi1_callback_functions_t*>(functions),
        logging_on ? 1 : 0
    );
}

void fmi1_interface::free_slave_instance(fmi1_component_t component)
{
    if (free_slave_instance_) {
        free_slave_instance_(component);
    }
}

fmi1_status_t fmi1_interface::initialize_slave(
    fmi1_component_t component,
    fmi1_real_t start_time,
    bool stop_time_defined,
    fmi1_real_t stop_time)
{
    if (!initialize_slave_) {
        error_ = "initialize_slave function not loaded";
        return fmi1_error;
    }
    return initialize_slave_(
        component,
        start_time,
        stop_time_defined ? 1 : 0,
        stop_time
    );
}

fmi1_status_t fmi1_interface::terminate_slave(fmi1_component_t component)
{
    if (!terminate_slave_) {
        error_ = "terminate_slave function not loaded";
        return fmi1_error;
    }
    return terminate_slave_(component);
}

fmi1_status_t fmi1_interface::reset_slave(fmi1_component_t component)
{
    if (!reset_slave_) {
        error_ = "reset_slave function not loaded";
        return fmi1_error;
    }
    return reset_slave_(component);
}

fmi1_status_t fmi1_interface::do_step(
    fmi1_component_t component,
    fmi1_real_t current_communication_point,
    fmi1_real_t communication_step_size,
    bool new_step)
{
    if (!do_step_) {
        error_ = "do_step function not loaded";
        return fmi1_error;
    }
    return do_step_(
        component,
        current_communication_point,
        communication_step_size,
        new_step ? 1 : 0
    );
}

fmi1_status_t fmi1_interface::get_real(
    fmi1_component_t component,
    const fmi1_value_reference_t vr[],
    size_t nvr,
    fmi1_real_t value[])
{
    if (!get_real_) {
        error_ = "get_real function not loaded";
        return fmi1_error;
    }
    return get_real_(component, vr, nvr, value);
}

fmi1_status_t fmi1_interface::get_integer(
    fmi1_component_t component,
    const fmi1_value_reference_t vr[],
    size_t nvr,
    fmi1_integer_t value[])
{
    if (!get_integer_) {
        error_ = "get_integer function not loaded";
        return fmi1_error;
    }
    return get_integer_(component, vr, nvr, value);
}

fmi1_status_t fmi1_interface::get_boolean(
    fmi1_component_t component,
    const fmi1_value_reference_t vr[],
    size_t nvr,
    fmi1_boolean_t value[])
{
    if (!get_boolean_) {
        error_ = "get_boolean function not loaded";
        return fmi1_error;
    }
    return get_boolean_(component, vr, nvr, value);
}

fmi1_status_t fmi1_interface::get_string(
    fmi1_component_t component,
    const fmi1_value_reference_t vr[],
    size_t nvr,
    fmi1_string_t value[])
{
    if (!get_string_) {
        error_ = "get_string function not loaded";
        return fmi1_error;
    }
    return get_string_(component, vr, nvr, value);
}

fmi1_status_t fmi1_interface::set_real(
    fmi1_component_t component,
    const fmi1_value_reference_t vr[],
    size_t nvr,
    const fmi1_real_t value[])
{
    if (!set_real_) {
        error_ = "set_real function not loaded";
        return fmi1_error;
    }
    return set_real_(component, vr, nvr, value);
}

fmi1_status_t fmi1_interface::set_integer(
    fmi1_component_t component,
    const fmi1_value_reference_t vr[],
    size_t nvr,
    const fmi1_integer_t value[])
{
    if (!set_integer_) {
        error_ = "set_integer function not loaded";
        return fmi1_error;
    }
    return set_integer_(component, vr, nvr, value);
}

fmi1_status_t fmi1_interface::set_boolean(
    fmi1_component_t component,
    const fmi1_value_reference_t vr[],
    size_t nvr,
    const fmi1_boolean_t value[])
{
    if (!set_boolean_) {
        error_ = "set_boolean function not loaded";
        return fmi1_error;
    }
    return set_boolean_(component, vr, nvr, value);
}

fmi1_status_t fmi1_interface::set_string(
    fmi1_component_t component,
    const fmi1_value_reference_t vr[],
    size_t nvr,
    const fmi1_string_t value[])
{
    if (!set_string_) {
        error_ = "set_string function not loaded";
        return fmi1_error;
    }
    return set_string_(component, vr, nvr, value);
}

const std::string& fmi1_interface::get_error() const
{
    return error_;
}

// 工厂函数
std::unique_ptr<fmi1_interface> create_fmi1_interface()
{
    return std::unique_ptr<fmi1_interface>(new fmi1_interface());
}

} // namespace fmi1
} // namespace fmi
} // namespace cosmo
