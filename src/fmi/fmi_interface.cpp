#include "cosmo/fmi/fmi_interface.hpp"
#include "cosmo/logger/logger.hpp"

namespace cosmo {
namespace fmi {

fmi_interface::fmi_interface()
    : get_version_(nullptr)
    , get_types_platform_(nullptr)
    , set_debug_logging_(nullptr)
    , instantiate_(nullptr)
    , free_instance_(nullptr)
    , setup_experiment_(nullptr)
    , enter_initialization_mode_(nullptr)
    , exit_initialization_mode_(nullptr)
    , terminate_(nullptr)
    , reset_(nullptr)
    , get_real_(nullptr)
    , get_integer_(nullptr)
    , get_boolean_(nullptr)
    , get_string_(nullptr)
    , set_real_(nullptr)
    , set_integer_(nullptr)
    , set_boolean_(nullptr)
    , set_string_(nullptr)
    , do_step_(nullptr)
{
}

fmi_interface::~fmi_interface()
{
    unload();
}

fmi_interface::fmi_interface(fmi_interface&& other) noexcept
    : library_(std::move(other.library_))
    , error_(std::move(other.error_))
    , get_version_(other.get_version_)
    , get_types_platform_(other.get_types_platform_)
    , set_debug_logging_(other.set_debug_logging_)
    , instantiate_(other.instantiate_)
    , free_instance_(other.free_instance_)
    , setup_experiment_(other.setup_experiment_)
    , enter_initialization_mode_(other.enter_initialization_mode_)
    , exit_initialization_mode_(other.exit_initialization_mode_)
    , terminate_(other.terminate_)
    , reset_(other.reset_)
    , get_real_(other.get_real_)
    , get_integer_(other.get_integer_)
    , get_boolean_(other.get_boolean_)
    , get_string_(other.get_string_)
    , set_real_(other.set_real_)
    , set_integer_(other.set_integer_)
    , set_boolean_(other.set_boolean_)
    , set_string_(other.set_string_)
    , do_step_(other.do_step_)
{
    other.get_version_ = nullptr;
    other.get_types_platform_ = nullptr;
    other.set_debug_logging_ = nullptr;
    other.instantiate_ = nullptr;
    other.free_instance_ = nullptr;
    other.setup_experiment_ = nullptr;
    other.enter_initialization_mode_ = nullptr;
    other.exit_initialization_mode_ = nullptr;
    other.terminate_ = nullptr;
    other.reset_ = nullptr;
    other.get_real_ = nullptr;
    other.get_integer_ = nullptr;
    other.get_boolean_ = nullptr;
    other.get_string_ = nullptr;
    other.set_real_ = nullptr;
    other.set_integer_ = nullptr;
    other.set_boolean_ = nullptr;
    other.set_string_ = nullptr;
    other.do_step_ = nullptr;
}

fmi_interface& fmi_interface::operator=(fmi_interface&& other) noexcept
{
    if (this != &other) {
        unload();
        library_ = std::move(other.library_);
        error_ = std::move(other.error_);
        get_version_ = other.get_version_;
        get_types_platform_ = other.get_types_platform_;
        set_debug_logging_ = other.set_debug_logging_;
        instantiate_ = other.instantiate_;
        free_instance_ = other.free_instance_;
        setup_experiment_ = other.setup_experiment_;
        enter_initialization_mode_ = other.enter_initialization_mode_;
        exit_initialization_mode_ = other.exit_initialization_mode_;
        terminate_ = other.terminate_;
        reset_ = other.reset_;
        get_real_ = other.get_real_;
        get_integer_ = other.get_integer_;
        get_boolean_ = other.get_boolean_;
        get_string_ = other.get_string_;
        set_real_ = other.set_real_;
        set_integer_ = other.set_integer_;
        set_boolean_ = other.set_boolean_;
        set_string_ = other.set_string_;
        do_step_ = other.do_step_;
        other.get_version_ = nullptr;
        other.get_types_platform_ = nullptr;
        other.set_debug_logging_ = nullptr;
        other.instantiate_ = nullptr;
        other.free_instance_ = nullptr;
        other.setup_experiment_ = nullptr;
        other.enter_initialization_mode_ = nullptr;
        other.exit_initialization_mode_ = nullptr;
        other.terminate_ = nullptr;
        other.reset_ = nullptr;
        other.get_real_ = nullptr;
        other.get_integer_ = nullptr;
        other.get_boolean_ = nullptr;
        other.get_string_ = nullptr;
        other.set_real_ = nullptr;
        other.set_integer_ = nullptr;
        other.set_boolean_ = nullptr;
        other.set_string_ = nullptr;
        other.do_step_ = nullptr;
    }
    return *this;
}

bool fmi_interface::load_library(const std::string& library_path)
{
    if (is_loaded()) {
        unload();
    }

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

void fmi_interface::unload()
{
    if (library_) {
        library_->unload();
        library_.reset();
    }
    error_.clear();
    get_version_ = nullptr;
    get_types_platform_ = nullptr;
    set_debug_logging_ = nullptr;
    instantiate_ = nullptr;
    free_instance_ = nullptr;
    setup_experiment_ = nullptr;
    enter_initialization_mode_ = nullptr;
    exit_initialization_mode_ = nullptr;
    terminate_ = nullptr;
    reset_ = nullptr;
    get_real_ = nullptr;
    get_integer_ = nullptr;
    get_boolean_ = nullptr;
    get_string_ = nullptr;
    set_real_ = nullptr;
    set_integer_ = nullptr;
    set_boolean_ = nullptr;
    set_string_ = nullptr;
    do_step_ = nullptr;
}

bool fmi_interface::is_loaded() const
{
    return library_ && library_->is_loaded();
}

const char* fmi_interface::get_version()
{
    if (!get_version_) {
        error_ = "get_version function not loaded";
        return nullptr;
    }
    return get_version_();
}

const char* fmi_interface::get_types_platform()
{
    if (!get_types_platform_) {
        error_ = "get_types_platform function not loaded";
        return nullptr;
    }
    return get_types_platform_();
}

fmi2::fmi2_component_t fmi_interface::instantiate(
    const std::string& instance_name,
    fmi2::fmi2_integer_t fmu_type,
    const std::string& fmu_guid,
    const std::string& fmu_resource_location,
    const fmi2::fmi2_callback_functions_t* functions,
    bool visible,
    bool logging_on)
{
    if (!instantiate_) {
        error_ = "instantiate function not loaded";
        return nullptr;
    }
    return instantiate_(
        instance_name.c_str(),
        fmu_type,
        fmu_guid.c_str(),
        fmu_resource_location.c_str(),
        functions,
        visible ? 1 : 0,
        logging_on ? 1 : 0
    );
}

void fmi_interface::free_instance(fmi2::fmi2_component_t component)
{
    if (free_instance_) {
        free_instance_(component);
    }
}

fmi2::fmi2_status_t fmi_interface::set_debug_logging(
    fmi2::fmi2_component_t component,
    bool logging_on,
    size_t n_categories,
    const char* categories[])
{
    if (!set_debug_logging_) {
        error_ = "set_debug_logging function not loaded";
        return fmi2::fmi2_status_error;
    }
    return set_debug_logging_(component, logging_on ? 1 : 0, n_categories, categories);
}

fmi2::fmi2_status_t fmi_interface::setup_experiment(
    fmi2::fmi2_component_t component,
    bool tolerance_defined,
    double tolerance,
    double start_time,
    bool stop_time_defined,
    double stop_time)
{
    if (!setup_experiment_) {
        error_ = "setup_experiment function not loaded";
        return fmi2::fmi2_status_error;
    }
    return setup_experiment_(
        component,
        tolerance_defined ? 1 : 0,
        tolerance,
        start_time,
        stop_time_defined ? 1 : 0,
        stop_time
    );
}

fmi2::fmi2_status_t fmi_interface::enter_initialization_mode(fmi2::fmi2_component_t component)
{
    if (!enter_initialization_mode_) {
        error_ = "enter_initialization_mode function not loaded";
        return fmi2::fmi2_status_error;
    }
    return enter_initialization_mode_(component);
}

fmi2::fmi2_status_t fmi_interface::exit_initialization_mode(fmi2::fmi2_component_t component)
{
    if (!exit_initialization_mode_) {
        error_ = "exit_initialization_mode function not loaded";
        return fmi2::fmi2_status_error;
    }
    return exit_initialization_mode_(component);
}

fmi2::fmi2_status_t fmi_interface::terminate(fmi2::fmi2_component_t component)
{
    if (!terminate_) {
        error_ = "terminate function not loaded";
        return fmi2::fmi2_status_error;
    }
    return terminate_(component);
}

fmi2::fmi2_status_t fmi_interface::reset(fmi2::fmi2_component_t component)
{
    if (!reset_) {
        error_ = "reset function not loaded";
        return fmi2::fmi2_status_error;
    }
    return reset_(component);
}

fmi2::fmi2_status_t fmi_interface::get_real(
    fmi2::fmi2_component_t component,
    const fmi2::fmi2_value_reference_t* vr,
    size_t nvr,
    fmi2::fmi2_real_t* value)
{
    if (!get_real_) {
        error_ = "get_real function not loaded";
        return fmi2::fmi2_status_error;
    }
    return get_real_(component, vr, nvr, value);
}

fmi2::fmi2_status_t fmi_interface::get_integer(
    fmi2::fmi2_component_t component,
    const fmi2::fmi2_value_reference_t* vr,
    size_t nvr,
    fmi2::fmi2_integer_t* value)
{
    if (!get_integer_) {
        error_ = "get_integer function not loaded";
        return fmi2::fmi2_status_error;
    }
    return get_integer_(component, vr, nvr, value);
}

fmi2::fmi2_status_t fmi_interface::get_boolean(
    fmi2::fmi2_component_t component,
    const fmi2::fmi2_value_reference_t* vr,
    size_t nvr,
    fmi2::fmi2_boolean_t* value)
{
    if (!get_boolean_) {
        error_ = "get_boolean function not loaded";
        return fmi2::fmi2_status_error;
    }
    return get_boolean_(component, vr, nvr, value);
}

fmi2::fmi2_status_t fmi_interface::get_string(
    fmi2::fmi2_component_t component,
    const fmi2::fmi2_value_reference_t* vr,
    size_t nvr,
    fmi2::fmi2_string_t* value)
{
    if (!get_string_) {
        error_ = "get_string function not loaded";
        return fmi2::fmi2_status_error;
    }
    return get_string_(component, vr, nvr, value);
}

fmi2::fmi2_status_t fmi_interface::set_real(
    fmi2::fmi2_component_t component,
    const fmi2::fmi2_value_reference_t* vr,
    size_t nvr,
    const fmi2::fmi2_real_t* value)
{
    if (!set_real_) {
        error_ = "set_real function not loaded";
        return fmi2::fmi2_status_error;
    }
    return set_real_(component, vr, nvr, value);
}

fmi2::fmi2_status_t fmi_interface::set_integer(
    fmi2::fmi2_component_t component,
    const fmi2::fmi2_value_reference_t* vr,
    size_t nvr,
    const fmi2::fmi2_integer_t* value)
{
    if (!set_integer_) {
        error_ = "set_integer function not loaded";
        return fmi2::fmi2_status_error;
    }
    return set_integer_(component, vr, nvr, value);
}

fmi2::fmi2_status_t fmi_interface::set_boolean(
    fmi2::fmi2_component_t component,
    const fmi2::fmi2_value_reference_t* vr,
    size_t nvr,
    const fmi2::fmi2_boolean_t* value)
{
    if (!set_boolean_) {
        error_ = "set_boolean function not loaded";
        return fmi2::fmi2_status_error;
    }
    return set_boolean_(component, vr, nvr, value);
}

fmi2::fmi2_status_t fmi_interface::set_string(
    fmi2::fmi2_component_t component,
    const fmi2::fmi2_value_reference_t* vr,
    size_t nvr,
    const fmi2::fmi2_string_t* value)
{
    if (!set_string_) {
        error_ = "set_string function not loaded";
        return fmi2::fmi2_status_error;
    }
    return set_string_(component, vr, nvr, value);
}

fmi2::fmi2_status_t fmi_interface::do_step(
    fmi2::fmi2_component_t component,
    double current_communication_point,
    double communication_step_size,
    bool no_set_fmu_state_prior_to_current_point)
{
    if (!do_step_) {
        error_ = "do_step function not loaded";
        return fmi2::fmi2_status_error;
    }
    return do_step_(
        component,
        current_communication_point,
        communication_step_size,
        no_set_fmu_state_prior_to_current_point ? 1 : 0
    );
}

const std::string& fmi_interface::get_error() const
{
    return error_;
}

namespace {

// 安全的 void* → 函数指针转换（绕过 ISO C++ pedantic 限制）
template<typename FT>
FT symbol_cast(void* sym) {
    union { void* ptr; FT func; } u;
    u.ptr = sym;
    return u.func;
}

} // anonymous namespace

bool fmi_interface::load_function_pointers()
{
    if (!library_ || !library_->is_loaded()) {
        error_ = "Library not loaded";
        return false;
    }

    get_version_ = symbol_cast<fmi2::fmi2_get_version_ft>(library_->get_symbol("fmi2GetVersion"));
    get_types_platform_ = symbol_cast<fmi2::fmi2_get_types_platform_ft>(library_->get_symbol("fmi2GetTypesPlatform"));
    set_debug_logging_ = symbol_cast<fmi2::fmi2_set_debug_logging_ft>(library_->get_symbol("fmi2SetDebugLogging"));
    instantiate_ = symbol_cast<fmi2::fmi2_instantiate_ft>(library_->get_symbol("fmi2Instantiate"));
    free_instance_ = symbol_cast<fmi2::fmi2_free_instance_ft>(library_->get_symbol("fmi2FreeInstance"));
    setup_experiment_ = symbol_cast<fmi2::fmi2_setup_experiment_ft>(library_->get_symbol("fmi2SetupExperiment"));
    enter_initialization_mode_ = symbol_cast<fmi2::fmi2_enter_initialization_mode_ft>(library_->get_symbol("fmi2EnterInitializationMode"));
    exit_initialization_mode_ = symbol_cast<fmi2::fmi2_exit_initialization_mode_ft>(library_->get_symbol("fmi2ExitInitializationMode"));
    terminate_ = symbol_cast<fmi2::fmi2_terminate_ft>(library_->get_symbol("fmi2Terminate"));
    reset_ = symbol_cast<fmi2::fmi2_reset_ft>(library_->get_symbol("fmi2Reset"));
    get_real_ = symbol_cast<fmi2::fmi2_get_real_ft>(library_->get_symbol("fmi2GetReal"));
    get_integer_ = symbol_cast<fmi2::fmi2_get_integer_ft>(library_->get_symbol("fmi2GetInteger"));
    get_boolean_ = symbol_cast<fmi2::fmi2_get_boolean_ft>(library_->get_symbol("fmi2GetBoolean"));
    get_string_ = symbol_cast<fmi2::fmi2_get_string_ft>(library_->get_symbol("fmi2GetString"));
    set_real_ = symbol_cast<fmi2::fmi2_set_real_ft>(library_->get_symbol("fmi2SetReal"));
    set_integer_ = symbol_cast<fmi2::fmi2_set_integer_ft>(library_->get_symbol("fmi2SetInteger"));
    set_boolean_ = symbol_cast<fmi2::fmi2_set_boolean_ft>(library_->get_symbol("fmi2SetBoolean"));
    set_string_ = symbol_cast<fmi2::fmi2_set_string_ft>(library_->get_symbol("fmi2SetString"));
    do_step_ = symbol_cast<fmi2::fmi2_do_step_ft>(library_->get_symbol("fmi2DoStep"));

    // 检查必需的函数
    if (!instantiate_ || !free_instance_ || !do_step_) {
        error_ = "Failed to load required FMI functions";
        return false;
    }

    return true;
}

// 工厂函数
std::unique_ptr<fmi_interface> create_fmi_interface()
{
    return std::unique_ptr<fmi_interface>(new fmi_interface());
}

} // namespace fmi
} // namespace cosmo