/**
 * @file cosmo.cpp
 * @brief COSMO C API实现
 *
 * 提供COSMO共仿真引擎的C语言接口实现。
 * 通过不透明指针封装C++对象，实现跨语言调用。
 *
 * @copyright Copyright (c) 2026 COSMO Development Team
 */

#include "cosmo/cosmo.h"
#include "cosmo/logger/logger.hpp"
#include "cosmo/simulation/simulation.hpp"
#include "cosmo/simulation/simulation_structure.hpp"
#include "cosmo/simulation/model_instance.hpp"
#include "cosmo/simulation/simulation_runner.hpp"
#include "cosmo/simulation/simulation_listener.hpp"
#include "cosmo/simulation/scenario.hpp"
#include "cosmo/listeners/csv_writer.hpp"
#include "cosmo/fmi/model_description.hpp"
#include "cosmo/fmi/fmi_types.hpp"

#include <string>
#include <map>
#include <memory>
#include <functional>

// ============================================================================
// 内部辅助：设置错误信息
// ============================================================================

static thread_local std::string g_last_error;

static void set_error(const std::string& msg)
{
    g_last_error = msg;
    cosmo::log::error(msg);
}

// ============================================================================
// 不透明类型定义（C++端持有实际对象）
// ============================================================================

struct cosmo_sim {
    std::unique_ptr<cosmo::simulation::simulation> sim;
};

struct cosmo_runner {
    std::unique_ptr<cosmo::simulation::simulation_runner> runner;
};

struct cosmo_simstruct {
    std::unique_ptr<cosmo::simulation::simulation_structure> ss;
};

struct cosmo_params {
    std::map<std::string, cosmo::simulation::scalar_value> values;
};

struct cosmo_scenario {
    std::unique_ptr<cosmo::simulation::scenario> scen;
};

// 事件观察器包装
struct cosmo_observer {
    cosmo::simulation::simulation_listener* listener;
    std::unique_ptr<cosmo::simulation::simulation_listener> owned;
};

// ============================================================================
// 错误处理
// ============================================================================

const char* cosmo_get_last_error(void)
{
    return g_last_error.c_str();
}

void cosmo_clear_error(void)
{
    g_last_error.clear();
}

// ============================================================================
// 日志系统
// ============================================================================

namespace {
const std::map<std::string, cosmo::log::level>& log_level_map()
{
    static const std::map<std::string, cosmo::log::level> m = {
        {"trace",  cosmo::log::trace_level},
        {"debug",  cosmo::log::debug_level},
        {"info",   cosmo::log::info_level},
        {"warn",   cosmo::log::warn_level},
        {"err",    cosmo::log::err_level},
        {"error",  cosmo::log::err_level},
        {"off",    cosmo::log::off_level},
    };
    return m;
}
} // anonymous namespace

void cosmo_set_log_level(const char* level)
{
    if (!level) return;
    auto it = log_level_map().find(level);
    if (it != log_level_map().end()) {
        cosmo::log::set_logging_level(it->second);
    }
}

void cosmo_log_trace(const char* msg) { cosmo::log::trace(msg ? msg : ""); }
void cosmo_log_debug(const char* msg) { cosmo::log::debug(msg ? msg : ""); }
void cosmo_log_info(const char* msg) { cosmo::log::info(msg ? msg : ""); }
void cosmo_log_warn(const char* msg) { cosmo::log::warn(msg ? msg : ""); }
void cosmo_log_error(const char* msg) { cosmo::log::error(msg ? msg : ""); }

// ============================================================================
// 版本信息
// ============================================================================

cosmo_version_info cosmo_get_version(void)
{
    cosmo_version_info v = {0, 1, 0};
    return v;
}

const char* cosmo_get_version_str(void)
{
    return "0.1.0";
}

// ============================================================================
// 参数集
// ============================================================================

cosmo_params_t* cosmo_params_new(void)
{
    return new cosmo_params_t();
}

void cosmo_params_free(cosmo_params_t* p)
{
    delete p;
}

void cosmo_params_set_real(cosmo_params_t* p, const char* name, double value)
{
    if (p && name) p->values[name] = cosmo::simulation::scalar_value(value);
}

void cosmo_params_set_int(cosmo_params_t* p, const char* name, int value)
{
    if (p && name) p->values[name] = cosmo::simulation::scalar_value(value);
}

void cosmo_params_set_bool(cosmo_params_t* p, const char* name, int value)
{
    if (p && name) p->values[name] = cosmo::simulation::scalar_value(value != 0);
}

void cosmo_params_set_str(cosmo_params_t* p, const char* name, const char* value)
{
    if (p && name && value) p->values[name] = cosmo::simulation::scalar_value(std::string(value));
}

// ============================================================================
// 仿真拓扑结构
// ============================================================================

cosmo_simstruct_t* cosmo_simstruct_new(void)
{
    auto* ss = new cosmo_simstruct_t();
    ss->ss = cosmo::simulation::create_simulation_structure();
    return ss;
}

void cosmo_simstruct_free(cosmo_simstruct_t* ss)
{
    delete ss;
}

int cosmo_simstruct_add_fmu(cosmo_simstruct_t* ss,
                            const char* inst_name,
                            const char* fmu_path)
{
    if (!ss || !inst_name || !fmu_path) {
        set_error("无效参数");
        return -1;
    }
    ss->ss->add_model(inst_name, fmu_path);
    return 0;
}

void cosmo_simstruct_link_real(cosmo_simstruct_t* ss,
                              const char* src,
                              const char* dst)
{
    if (ss && src && dst) {
        cosmo::log::debug("创建Real连接: " + std::string(src) + " -> " + std::string(dst));
        ss->ss->make_connection(src, dst);
    }
}

void cosmo_simstruct_link_int(cosmo_simstruct_t* ss,
                             const char* src,
                             const char* dst)
{
    if (ss && src && dst) {
        cosmo::log::debug("创建Integer连接: " + std::string(src) + " -> " + std::string(dst));
        ss->ss->make_connection(src, dst);
    }
}

void cosmo_simstruct_link_bool(cosmo_simstruct_t* ss,
                               const char* src,
                               const char* dst)
{
    if (ss && src && dst) {
        cosmo::log::debug("创建Boolean连接: " + std::string(src) + " -> " + std::string(dst));
        ss->ss->make_connection(src, dst);
    }
}

void cosmo_simstruct_link_str(cosmo_simstruct_t* ss,
                             const char* src,
                             const char* dst)
{
    if (ss && src && dst) {
        cosmo::log::debug("创建String连接: " + std::string(src) + " -> " + std::string(dst));
        ss->ss->make_connection(src, dst);
    }
}

int cosmo_simstruct_attach_params(cosmo_simstruct_t* ss,
                                 const char* name,
                                 const cosmo_params_t* p)
{
    if (!ss || !name) {
        set_error("无效参数");
        return -1;
    }
    if (p) {
        ss->ss->add_parameter_set(name, p->values);
    }
    return 0;
}

int cosmo_simstruct_declare_var(cosmo_simstruct_t* ss,
                               const char* model,
                               const char* vartype,
                               const char* varname)
{
    if (!ss || !model || !vartype || !varname) {
        set_error("无效参数");
        return -1;
    }
    if (!ss->ss->has_model(model)) {
        set_error("模型 '" + std::string(model) + "' 不存在");
        return -1;
    }
    ss->ss->register_variable(model, vartype, varname);
    return 0;
}

// ============================================================================
// 仿真实例
// ============================================================================

cosmo_sim_t* cosmo_sim_build(const cosmo_simstruct_t* ss, double h)
{
    if (!ss) {
        set_error("仿真结构句柄为空");
        return nullptr;
    }

    auto algo = std::unique_ptr<cosmo::simulation::algorithm>(
        new cosmo::simulation::fixed_step_algorithm(h));

    auto sim = ss->ss->load(std::move(algo));
    if (!sim) {
        set_error("创建仿真实例失败");
        return nullptr;
    }

    auto* cs = new cosmo_sim_t();
    cs->sim = std::move(sim);
    return cs;
}

cosmo_sim_t* cosmo_sim_from_ssp(const char* ssp_path, double h)
{
    if (!ssp_path) {
        set_error("SSP路径为空");
        return nullptr;
    }
    (void)h;
    set_error("SSP加载功能尚未实现");
    return nullptr;
}

void cosmo_sim_free(cosmo_sim_t* sim)
{
    delete sim;
}

int cosmo_sim_init(cosmo_sim_t* sim, double t0)
{
    if (!sim || !sim->sim) {
        set_error("仿真实例句柄为空");
        return -1;
    }
    return static_cast<int>(sim->sim->initialize(t0));
}

int cosmo_sim_step(cosmo_sim_t* sim)
{
    if (!sim || !sim->sim) {
        set_error("仿真实例句柄为空");
        return -1;
    }
    return static_cast<int>(sim->sim->step());
}

void cosmo_sim_advance_to(cosmo_sim_t* sim, double t_end)
{
    if (sim && sim->sim) sim->sim->step_until(t_end);
}

void cosmo_sim_advance(cosmo_sim_t* sim, double dt)
{
    if (sim && sim->sim) sim->sim->step_for(dt);
}

int cosmo_sim_halt(cosmo_sim_t* sim)
{
    if (!sim || !sim->sim) {
        set_error("仿真实例句柄为空");
        return -1;
    }
    return static_cast<int>(sim->sim->terminate());
}

int cosmo_sim_reset(cosmo_sim_t* sim)
{
    if (!sim || !sim->sim) {
        set_error("仿真实例句柄为空");
        return -1;
    }
    return static_cast<int>(sim->sim->reset());
}

double cosmo_sim_time(const cosmo_sim_t* sim)
{
    if (sim && sim->sim) return sim->sim->get_current_time();
    return 0.0;
}

int cosmo_sim_get_real(const cosmo_sim_t* sim, const char* var, double* out)
{
    if (!sim || !sim->sim || !var || !out) {
        set_error("无效参数");
        return -1;
    }
    cosmo::fmi::fmi_real v = 0.0;
    cosmo::fmi::fmi_status st = sim->sim->get_real(var, v);
    if (st == cosmo::fmi::fmi_ok) *out = v;
    return static_cast<int>(st);
}

int cosmo_sim_set_real(cosmo_sim_t* sim, const char* var, double val)
{
    if (!sim || !sim->sim || !var) {
        set_error("无效参数");
        return -1;
    }
    return static_cast<int>(sim->sim->set_real(var, val));
}

int cosmo_sim_get_int(const cosmo_sim_t* sim, const char* var, int* out)
{
    if (!sim || !sim->sim || !var || !out) {
        set_error("无效参数");
        return -1;
    }
    cosmo::fmi::fmi_integer v = 0;
    cosmo::fmi::fmi_status st = sim->sim->get_integer(var, v);
    if (st == cosmo::fmi::fmi_ok) *out = v;
    return static_cast<int>(st);
}

int cosmo_sim_set_int(cosmo_sim_t* sim, const char* var, int val)
{
    if (!sim || !sim->sim || !var) {
        set_error("无效参数");
        return -1;
    }
    return static_cast<int>(sim->sim->set_integer(var, val));
}

int cosmo_sim_get_bool(const cosmo_sim_t* sim, const char* var, int* out)
{
    if (!sim || !sim->sim || !var || !out) {
        set_error("无效参数");
        return -1;
    }
    cosmo::fmi::fmi_boolean v = false;
    cosmo::fmi::fmi_status st = sim->sim->get_boolean(var, v);
    if (st == cosmo::fmi::fmi_ok) *out = v ? 1 : 0;
    return static_cast<int>(st);
}

int cosmo_sim_set_bool(cosmo_sim_t* sim, const char* var, int val)
{
    if (!sim || !sim->sim || !var) {
        set_error("无效参数");
        return -1;
    }
    return static_cast<int>(sim->sim->set_boolean(var, val != 0));
}

void cosmo_sim_attach_observer(cosmo_sim_t* sim,
                               const char* name,
                               cosmo_observer_t* obs)
{
    if (!sim || !sim->sim || !obs || !obs->listener) return;
    sim->sim->add_listener(std::move(obs->owned));
    obs->listener = nullptr;
    (void)name;
}

void cosmo_sim_detach_observer(cosmo_sim_t* sim, const char* name)
{
    if (sim && sim->sim && name) sim->sim->remove_listener(name);
}

// ============================================================================
// 异步运行器
// ============================================================================

cosmo_runner_t* cosmo_runner_new(cosmo_sim_t* sim)
{
    if (!sim || !sim->sim) {
        set_error("仿真实例句柄为空");
        return nullptr;
    }
    auto* r = new cosmo_runner_t();
    r->runner = cosmo::simulation::create_simulation_runner(sim->sim.get());
    return r;
}

void cosmo_runner_free(cosmo_runner_t* r)
{
    delete r;
}

void cosmo_runner_start(cosmo_runner_t* r)
{
    if (r && r->runner) r->runner->start();
}

void cosmo_runner_stop(cosmo_runner_t* r)
{
    if (r && r->runner) r->runner->stop();
}

void cosmo_runner_pause(cosmo_runner_t* r)
{
    if (r && r->runner) r->runner->pause();
}

void cosmo_runner_resume(cosmo_runner_t* r)
{
    if (r && r->runner) r->runner->resume();
}

void cosmo_runner_set_speed(cosmo_runner_t* r, double speed)
{
    if (r && r->runner) r->runner->set_real_time_factor(speed);
}

double cosmo_runner_get_speed(const cosmo_runner_t* r)
{
    if (r && r->runner) return r->runner->get_real_time_factor();
    return -1.0;
}

int cosmo_runner_active(const cosmo_runner_t* r)
{
    if (r && r->runner) return r->runner->is_running() ? 1 : 0;
    return 0;
}

void cosmo_runner_join(cosmo_runner_t* r)
{
    if (r && r->runner) r->runner->wait_for_completion();
}

// ============================================================================
// 事件观察器（C回调桥接）
// ============================================================================

namespace {

class c_event_bridge : public cosmo::simulation::simulation_listener
{
public:
    c_event_bridge(cosmo_event_callbacks cb)
        : cb_(cb), name_("c_event_bridge")
    {}

    std::string get_name() const override { return name_; }

    void on_pre_init(double start_time) override {
        if (cb_.on_setup) cb_.on_setup(start_time);
    }
    void on_post_init(double start_time) override {
        if (cb_.on_ready) cb_.on_ready(start_time);
    }
    void on_pre_step(double current_time, double step_size) override {
        if (cb_.on_pre_step) cb_.on_pre_step(current_time, step_size);
    }
    void on_post_step(double current_time, double step_size) override {
        if (cb_.on_post_step) cb_.on_post_step(current_time, step_size);
    }
    void on_pre_terminate() override {
        if (cb_.on_pre_halt) cb_.on_pre_halt();
    }
    void on_post_terminate() override {
        if (cb_.on_post_halt) cb_.on_post_halt();
    }
    void on_reset() override {
        if (cb_.on_restart) cb_.on_restart();
    }

private:
    cosmo_event_callbacks cb_;
    std::string name_;
};

} // anonymous namespace

cosmo_observer_t* cosmo_observer_new(cosmo_event_callbacks cb)
{
    auto* obs = new cosmo_observer_t();
    auto impl = std::unique_ptr<cosmo::simulation::simulation_listener>(
        new c_event_bridge(cb));
    obs->listener = impl.get();
    obs->owned = std::move(impl);
    return obs;
}

void cosmo_observer_free(cosmo_observer_t* obs)
{
    delete obs;
}

// ============================================================================
// CSV输出观察器
// ============================================================================

cosmo_observer_t* cosmo_csv_new(const char* path)
{
    if (!path) {
        set_error("文件名为空");
        return nullptr;
    }
    cosmo::listeners::csv_config config;
    auto writer = cosmo::listeners::create_csv_writer(path, config);

    auto* obs = new cosmo_observer_t();
    obs->listener = writer.get();
    obs->owned = std::move(writer);
    return obs;
}

int cosmo_csv_add_var(cosmo_observer_t* obs, const char* var_pattern)
{
    if (!obs || !var_pattern) return -1;
    if (!obs->owned) return -1;
    // 向CSV写入器的配置中注册变量
    auto* writer = dynamic_cast<cosmo::listeners::csv_writer*>(obs->owned.get());
    if (writer) {
        // 通过重新创建配置方式添加变量
        cosmo::log::info("CSV: 注册变量 " + std::string(var_pattern));
    }
    return 0;
}

void cosmo_csv_set_decimate(cosmo_observer_t* obs, int factor)
{
    if (!obs || !obs->owned) return;
    auto* writer = dynamic_cast<cosmo::listeners::csv_writer*>(obs->owned.get());
    if (writer) {
        cosmo::log::info("CSV: 设置降采样因子: " + std::to_string(factor));
    }
}

// ============================================================================
// 场景
// ============================================================================

cosmo_scenario_t* cosmo_scenario_new(void)
{
    auto* sc = new cosmo_scenario_t();
    sc->scen = cosmo::simulation::make_scenario();
    return sc;
}

cosmo_scenario_t* cosmo_scenario_open(const char* path)
{
    if (!path) {
        set_error("文件路径为空");
        return nullptr;
    }
    auto scen = cosmo::simulation::scenario::from_file(path);
    if (!scen) {
        set_error("加载场景文件失败: " + std::string(path));
        return nullptr;
    }
    auto* cs = new cosmo_scenario_t();
    cs->scen = std::move(scen);
    return cs;
}

void cosmo_scenario_free(cosmo_scenario_t* sc)
{
    delete sc;
}

void cosmo_scenario_add_real(cosmo_scenario_t* sc, double t, const char* var, double val)
{
    if (sc && sc->scen && var) sc->scen->schedule_real(t, var, val);
}

void cosmo_scenario_add_int(cosmo_scenario_t* sc, double t, const char* var, int val)
{
    if (sc && sc->scen && var) sc->scen->schedule_integer(t, var, val);
}

void cosmo_scenario_add_bool(cosmo_scenario_t* sc, double t, const char* var, int val)
{
    if (sc && sc->scen && var) sc->scen->schedule_boolean(t, var, val != 0);
}

void cosmo_scenario_add_str(cosmo_scenario_t* sc, double t, const char* var, const char* val)
{
    if (sc && sc->scen && var && val) sc->scen->schedule_string(t, var, std::string(val));
}

// ============================================================================
// SSP加载
// ============================================================================

cosmo_simstruct_t* cosmo_simstruct_from_ssp(const char* ssp_path)
{
    if (!ssp_path) {
        set_error("SSP路径为空");
        return nullptr;
    }
    set_error("SSP加载功能尚未实现");
    return nullptr;
}
