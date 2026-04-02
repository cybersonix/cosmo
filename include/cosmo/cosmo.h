/**
 * @file cosmo.h
 * @brief COSMO共仿真引擎C API头文件
 *
 * 本文件提供COSMO共仿真引擎的C语言接口。
 * 通过此API，C语言和其他语言（如Python）可以调用COSMO功能。
 *
 * @author COSMO Development Team
 * @version 0.1.0
 * @date 2026-03-27
 *
 * @copyright Copyright (c) 2026
 *
 * @note API设计采用"动词+对象"命名风格，与FMI标准术语保持一致性
 */

#ifndef COSMO_COSMO_H
#define COSMO_COSMO_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief API可见性宏定义
 */
#if defined(_WIN32) || defined(__CYGWIN__)
#    define COSMO_API __declspec(dllexport)
#else
#    define COSMO_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 不透明类型
 * ============================================================================ */

typedef struct cosmo_sim cosmo_sim_t;          ///< 仿真实例
typedef struct cosmo_runner cosmo_runner_t;      ///< 异步运行器
typedef struct cosmo_observer cosmo_observer_t;    ///< 事件观察器
typedef struct cosmo_simstruct cosmo_simstruct_t;  ///< 仿真拓扑结构
typedef struct cosmo_params cosmo_params_t;        ///< 参数集合
typedef struct cosmo_scenario cosmo_scenario_t;    ///< 仿真场景

/* ============================================================================
 * 基础类型
 * ============================================================================ */

/**
 * @brief 库版本信息
 */
typedef struct cosmo_version_info
{
    int major;   ///< 主版本
    int minor;   ///< 次版本
    int patch;   ///< 修订版本
} cosmo_version_info;

/**
 * @brief 仿真快照（当前时刻的运行状态）
 */
typedef struct cosmo_snapshot
{
    double t;         ///< 当前仿真时刻
    size_t num_steps; ///< 已执行步数
} cosmo_snapshot;

/**
 * @brief 事件观察器回调表
 *
 * 用户通过设置回调函数来响应仿真生命周期事件。
 * 所有字段默认为NULL，仅设置需要的回调即可。
 */
typedef struct cosmo_event_callbacks
{
    void (*on_setup)(double t0);                      ///< 仿真初始化前
    void (*on_ready)(double t0);                      ///< 仿真初始化后
    void (*on_pre_step)(double t, double h);           ///< 单步执行前
    void (*on_post_step)(double t, double h);          ///< 单步执行后
    void (*on_pre_halt)(void);                         ///< 仿真终止前
    void (*on_post_halt)(void);                        ///< 仿真终止后
    void (*on_restart)(void);                          ///< 仿真重置
} cosmo_event_callbacks;

/* ============================================================================
 * 错误处理
 * ============================================================================ */

COSMO_API const char* cosmo_get_last_error(void);
COSMO_API void cosmo_clear_error(void);

/* ============================================================================
 * 日志
 * ============================================================================ */

COSMO_API void cosmo_set_log_level(const char* level);
COSMO_API void cosmo_log_trace(const char* msg);
COSMO_API void cosmo_log_debug(const char* msg);
COSMO_API void cosmo_log_info(const char* msg);
COSMO_API void cosmo_log_warn(const char* msg);
COSMO_API void cosmo_log_error(const char* msg);

/* ============================================================================
 * 版本
 * ============================================================================ */

COSMO_API cosmo_version_info cosmo_get_version(void);
COSMO_API const char* cosmo_get_version_str(void);

/* ============================================================================
 * 参数集 (cosmo_params)
 * ============================================================================ */

COSMO_API cosmo_params_t* cosmo_params_new(void);
COSMO_API void cosmo_params_free(cosmo_params_t* p);

COSMO_API void cosmo_params_set_real(cosmo_params_t* p, const char* name, double value);
COSMO_API void cosmo_params_set_int(cosmo_params_t* p, const char* name, int value);
COSMO_API void cosmo_params_set_bool(cosmo_params_t* p, const char* name, int value);
COSMO_API void cosmo_params_set_str(cosmo_params_t* p, const char* name, const char* value);

/* ============================================================================
 * 仿真拓扑结构 (cosmo_simstruct)
 * ============================================================================ */

COSMO_API cosmo_simstruct_t* cosmo_simstruct_new(void);
COSMO_API void cosmo_simstruct_free(cosmo_simstruct_t* ss);

COSMO_API int cosmo_simstruct_add_fmu(cosmo_simstruct_t* ss,
                                      const char* inst_name,
                                      const char* fmu_path);

COSMO_API void cosmo_simstruct_link_real(cosmo_simstruct_t* ss,
                                         const char* src,
                                         const char* dst);

COSMO_API void cosmo_simstruct_link_int(cosmo_simstruct_t* ss,
                                        const char* src,
                                        const char* dst);

COSMO_API void cosmo_simstruct_link_bool(cosmo_simstruct_t* ss,
                                         const char* src,
                                         const char* dst);

COSMO_API void cosmo_simstruct_link_str(cosmo_simstruct_t* ss,
                                        const char* src,
                                        const char* dst);

COSMO_API int cosmo_simstruct_attach_params(cosmo_simstruct_t* ss,
                                            const char* name,
                                            const cosmo_params_t* p);

COSMO_API int cosmo_simstruct_declare_var(cosmo_simstruct_t* ss,
                                           const char* model,
                                           const char* vartype,
                                           const char* varname);

/* ============================================================================
 * 仿真实例 (cosmo_sim)
 * ============================================================================ */

COSMO_API cosmo_sim_t* cosmo_sim_build(const cosmo_simstruct_t* ss, double h);
COSMO_API cosmo_sim_t* cosmo_sim_from_ssp(const char* ssp_path, double h);
COSMO_API void cosmo_sim_free(cosmo_sim_t* sim);

COSMO_API int cosmo_sim_init(cosmo_sim_t* sim, double t0);
COSMO_API int cosmo_sim_step(cosmo_sim_t* sim);
COSMO_API void cosmo_sim_advance_to(cosmo_sim_t* sim, double t_end);
COSMO_API void cosmo_sim_advance(cosmo_sim_t* sim, double dt);
COSMO_API int cosmo_sim_halt(cosmo_sim_t* sim);
COSMO_API int cosmo_sim_reset(cosmo_sim_t* sim);

COSMO_API double cosmo_sim_time(const cosmo_sim_t* sim);

COSMO_API int cosmo_sim_get_real(const cosmo_sim_t* sim, const char* var, double* out);
COSMO_API int cosmo_sim_set_real(cosmo_sim_t* sim, const char* var, double val);
COSMO_API int cosmo_sim_get_int(const cosmo_sim_t* sim, const char* var, int* out);
COSMO_API int cosmo_sim_set_int(cosmo_sim_t* sim, const char* var, int val);
COSMO_API int cosmo_sim_get_bool(const cosmo_sim_t* sim, const char* var, int* out);
COSMO_API int cosmo_sim_set_bool(cosmo_sim_t* sim, const char* var, int val);

COSMO_API void cosmo_sim_attach_observer(cosmo_sim_t* sim,
                                         const char* name,
                                         cosmo_observer_t* obs);
COSMO_API void cosmo_sim_detach_observer(cosmo_sim_t* sim, const char* name);

/* ============================================================================
 * 异步运行器 (cosmo_runner)
 * ============================================================================ */

COSMO_API cosmo_runner_t* cosmo_runner_new(cosmo_sim_t* sim);
COSMO_API void cosmo_runner_free(cosmo_runner_t* r);

COSMO_API void cosmo_runner_start(cosmo_runner_t* r);
COSMO_API void cosmo_runner_stop(cosmo_runner_t* r);
COSMO_API void cosmo_runner_pause(cosmo_runner_t* r);
COSMO_API void cosmo_runner_resume(cosmo_runner_t* r);
COSMO_API void cosmo_runner_set_speed(cosmo_runner_t* r, double speed);
COSMO_API double cosmo_runner_get_speed(const cosmo_runner_t* r);
COSMO_API int cosmo_runner_active(const cosmo_runner_t* r);
COSMO_API void cosmo_runner_join(cosmo_runner_t* r);

/* ============================================================================
 * 事件观察器 (cosmo_observer)
 * ============================================================================ */

COSMO_API cosmo_observer_t* cosmo_observer_new(cosmo_event_callbacks cb);
COSMO_API void cosmo_observer_free(cosmo_observer_t* obs);

/* ============================================================================
 * CSV输出观察器
 * ============================================================================ */

COSMO_API cosmo_observer_t* cosmo_csv_new(const char* path);
COSMO_API int cosmo_csv_add_var(cosmo_observer_t* obs, const char* var_pattern);
COSMO_API void cosmo_csv_set_decimate(cosmo_observer_t* obs, int factor);

/* ============================================================================
 * 场景 (cosmo_scenario)
 * ============================================================================ */

COSMO_API cosmo_scenario_t* cosmo_scenario_new(void);
COSMO_API cosmo_scenario_t* cosmo_scenario_open(const char* path);
COSMO_API void cosmo_scenario_free(cosmo_scenario_t* sc);

COSMO_API void cosmo_scenario_add_real(cosmo_scenario_t* sc,
                                       double t,
                                       const char* var,
                                       double val);
COSMO_API void cosmo_scenario_add_int(cosmo_scenario_t* sc,
                                      double t,
                                      const char* var,
                                      int val);
COSMO_API void cosmo_scenario_add_bool(cosmo_scenario_t* sc,
                                       double t,
                                       const char* var,
                                       int val);
COSMO_API void cosmo_scenario_add_str(cosmo_scenario_t* sc,
                                      double t,
                                      const char* var,
                                      const char* val);

/* ============================================================================
 * SSP加载
 * ============================================================================ */

COSMO_API cosmo_simstruct_t* cosmo_simstruct_from_ssp(const char* ssp_path);

/* ============================================================================
 * 兼容性别名 (旧API)
 *
 * 以下宏将旧名称映射到新名称，保持向后兼容。
 * 新代码请直接使用新API。
 * ============================================================================ */

#define cosmo_last_error_msg         cosmo_get_last_error
#define cosmo_clear_error_fn         cosmo_clear_error
#define cosmo_set_log_level         cosmo_set_log_level
#define cosmo_trace                cosmo_log_trace
#define cosmo_debug                cosmo_log_debug
#define cosmo_info                 cosmo_log_info
#define cosmo_warn                 cosmo_log_warn
#define cosmo_error                cosmo_log_error
#define cosmo_library_version       cosmo_get_version
#define cosmo_library_version_string cosmo_get_version_str

#define cosmo_version              cosmo_version_info
#define cosmo_simulation_info       cosmo_snapshot

#define cosmo_simulation_t          cosmo_sim_t
#define cosmo_simulation_runner_t    cosmo_runner_t
#define cosmo_simulation_listener_t  cosmo_observer_t
#define cosmo_simulation_structure_t cosmo_simstruct_t
#define cosmo_parameter_set_t       cosmo_params_t
#define cosmo_listener_config       cosmo_event_callbacks

#define cosmo_parameter_set_create()          cosmo_params_new()
#define cosmo_parameter_set_destroy(p)         cosmo_params_free(p)
#define cosmo_parameter_set_add_real(p,n,v)     cosmo_params_set_real(p,n,v)
#define cosmo_parameter_set_add_integer(p,n,v)   cosmo_params_set_int(p,n,v)
#define cosmo_parameter_set_add_boolean(p,n,v)   cosmo_params_set_bool(p,n,v)
#define cosmo_parameter_set_add_string(p,n,v)    cosmo_params_set_str(p,n,v)

#define cosmo_simulation_structure_create()                 cosmo_simstruct_new()
#define cosmo_simulation_structure_destroy(s)                 cosmo_simstruct_free(s)
#define cosmo_simulation_structure_add_model(s,a,b)            cosmo_simstruct_add_fmu(s,a,b)
#define cosmo_simulation_structure_make_real_connection(s,a,b)    cosmo_simstruct_link_real(s,a,b)
#define cosmo_simulation_structure_make_integer_connection(s,a,b)  cosmo_simstruct_link_int(s,a,b)
#define cosmo_simulation_structure_make_boolean_connection(s,a,b)  cosmo_simstruct_link_bool(s,a,b)
#define cosmo_simulation_structure_make_string_connection(s,a,b)   cosmo_simstruct_link_str(s,a,b)
#define cosmo_simulation_structure_add_parameter_set(s,a,b)       cosmo_simstruct_attach_params(s,a,b)
#define cosmo_simulation_structure_register_variable(s,a,b,c)     cosmo_simstruct_declare_var(s,a,b,c)

#define cosmo_simulation_create_from_structure(s,h)  cosmo_sim_build(s,h)
#define cosmo_simulation_create_from_ssp(p,h)        cosmo_sim_from_ssp(p,h)
#define cosmo_simulation_destroy(s)                  cosmo_sim_free(s)
#define cosmo_simulation_initialize(s,t)              cosmo_sim_init(s,t)
#define cosmo_simulation_step(s)                     cosmo_sim_step(s)
#define cosmo_simulation_step_until(s,t)              cosmo_sim_advance_to(s,t)
#define cosmo_simulation_step_for(s,d)                cosmo_sim_advance(s,d)
#define cosmo_simulation_terminate(s)                 cosmo_sim_halt(s)
#define cosmo_simulation_reset(s)                     cosmo_sim_reset(s)
#define cosmo_simulation_get_time(s)                  cosmo_sim_time(s)
#define cosmo_simulation_get_real(s,v,o)               cosmo_sim_get_real(s,v,o)
#define cosmo_simulation_set_real(s,v,val)             cosmo_sim_set_real(s,v,val)
#define cosmo_simulation_get_integer(s,v,o)             cosmo_sim_get_int(s,v,o)
#define cosmo_simulation_set_integer(s,v,val)            cosmo_sim_set_int(s,v,val)
#define cosmo_simulation_get_boolean(s,v,o)             cosmo_sim_get_bool(s,v,o)
#define cosmo_simulation_set_boolean(s,v,val)            cosmo_sim_set_bool(s,v,val)
#define cosmo_simulation_add_listener(s,n,o)           cosmo_sim_attach_observer(s,n,o)
#define cosmo_simulation_remove_listener(s,n)            cosmo_sim_detach_observer(s,n)

#define cosmo_simulation_runner_create(s)               cosmo_runner_new(s)
#define cosmo_simulation_runner_destroy(r)               cosmo_runner_free(r)
#define cosmo_simulation_runner_start(r)                cosmo_runner_start(r)
#define cosmo_simulation_runner_stop(r)                 cosmo_runner_stop(r)
#define cosmo_simulation_runner_pause(r)                 cosmo_runner_pause(r)
#define cosmo_simulation_runner_resume(r)                cosmo_runner_resume(r)
#define cosmo_simulation_runner_set_real_time_factor(r,f) cosmo_runner_set_speed(r,f)
#define cosmo_simulation_runner_get_real_time_factor(r)    cosmo_runner_get_speed(r)
#define cosmo_simulation_runner_is_running(r)            cosmo_runner_active(r)
#define cosmo_simulation_runner_wait(r)                  cosmo_runner_join(r)

#define cosmo_listener_create(c)                       cosmo_observer_new(c)
#define cosmo_listener_destroy(o)                       cosmo_observer_free(o)

#define cosmo_csv_writer_create(p)                       cosmo_csv_new(p)
#define cosmo_csv_writer_register_variable(o,v)           cosmo_csv_add_var(o,v)
#define cosmo_csv_writer_set_downsample_factor(o,f)      cosmo_csv_set_decimate(o,f)

#define cosmo_scenario_create()                         cosmo_scenario_new()
#define cosmo_scenario_load_from_file(p)                 cosmo_scenario_open(p)
#define cosmo_scenario_destroy(s)                       cosmo_scenario_free(s)
#define cosmo_scenario_add_real_action(s,t,v,val)        cosmo_scenario_add_real(s,t,v,val)
#define cosmo_scenario_add_integer_action(s,t,v,val)     cosmo_scenario_add_int(s,t,v,val)
#define cosmo_scenario_add_boolean_action(s,t,v,val)     cosmo_scenario_add_bool(s,t,v,val)
#define cosmo_scenario_add_string_action(s,t,v,val)      cosmo_scenario_add_str(s,t,v,val)

#define cosmo_load_ssp(p)                             cosmo_simstruct_from_ssp(p)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* COSMO_COSMO_H */
