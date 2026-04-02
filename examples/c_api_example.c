/**
 * @file c_api_example.c
 * @brief COSMO C语言接口使用示例
 *
 * 演示如何使用C API创建和执行联合仿真。
 * 此文件使用纯C语言编写，展示跨语言调用能力。
 */

#include "cosmo/cosmo.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main()
{
    int ret;
    double value;

    printf("=== COSMO C API 示例 ===\n\n");

    /* 设置日志级别 */
    cosmo_set_log_level("info");

    /* 显示版本信息 */
    cosmo_version ver = cosmo_library_version();
    printf("COSMO版本: %d.%d.%d\n\n", ver.major, ver.minor, ver.patch);

    /* 1. 创建仿真结构 */
    printf("[1] 创建仿真结构...\n");
    cosmo_simulation_structure_t* ss = cosmo_simulation_structure_create();

    /* 2. 添加模型 */
    ret = cosmo_simulation_structure_add_model(ss, "motor", "motor.fmu");
    ret = cosmo_simulation_structure_add_model(ss, "sensor", "sensor.fmu");

    /* 3. 注册变量 */
    cosmo_simulation_structure_register_variable(ss, "motor", "real", "speed");
    cosmo_simulation_structure_register_variable(ss, "motor", "real", "torque");
    cosmo_simulation_structure_register_variable(ss, "sensor", "real", "speed");
    cosmo_simulation_structure_register_variable(ss, "sensor", "real", "noise");

    /* 4. 建立连接 */
    cosmo_simulation_structure_make_real_connection(ss, "motor::speed", "sensor::speed");

    /* 5. 创建仿真 */
    printf("[2] 创建仿真实例...\n");
    cosmo_simulation_t* sim = cosmo_simulation_create_from_structure(ss, 0.01);
    if (!sim) {
        printf("错误: %s\n", cosmo_last_error_msg());
        cosmo_simulation_structure_destroy(ss);
        return 1;
    }

    /* 6. 初始化 */
    printf("[3] 初始化仿真...\n");
    ret = cosmo_simulation_initialize(sim, 0.0);
    if (ret != 0) {
        printf("初始化失败，错误码: %d\n", ret);
        cosmo_simulation_destroy(sim);
        cosmo_simulation_structure_destroy(ss);
        return 1;
    }

    /* 7. 设置初始条件 */
    cosmo_simulation_set_real(sim, "motor::speed", 0.0);
    cosmo_simulation_set_real(sim, "motor::torque", 50.0);

    /* 8. 执行仿真 */
    printf("[4] 执行仿真（1秒）...\n");
    cosmo_simulation_step_until(sim, 1.0);

    /* 9. 读取结果 */
    cosmo_simulation_get_real(sim, "motor::speed", &value);
    printf("  motor::speed = %.6f\n", value);

    cosmo_simulation_get_real(sim, "sensor::speed", &value);
    printf("  sensor::speed = %.6f\n", value);

    printf("  仿真时间: %.4f\n", cosmo_simulation_get_time(sim));

    /* 10. 终止并清理 */
    printf("\n[5] 清理资源...\n");
    cosmo_simulation_terminate(sim);
    cosmo_simulation_destroy(sim);
    cosmo_simulation_structure_destroy(ss);

    printf("=== 示例完成 ===\n");
    return 0;
}
