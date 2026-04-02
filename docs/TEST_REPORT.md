# COSMO 测试报告

## 文档信息

| 项目 | 内容 |
|------|------|
| 项目名称 | COSMO (Collaborative Open-Source Simulation & Modeling) |
| 版本 | 0.1.0 |
| 测试日期 | 2026-03-27 |
| 测试环境 | CentOS 7 / GCC 4.8.5 / CMake 3.28 |
| 测试框架 | 自研 (cosmo::test) |

---

## 1. 测试概述

### 1.1 测试框架

COSMO 使用自研轻量级测试框架 `cosmo::test`（定义在 `include/cosmo/test/test_framework.hpp`）。

**框架核心组件：**

| 组件 | 类型 | 说明 |
|------|------|------|
| `test_registry` | 单例类 | 全局测试注册表，存储所有测试用例 |
| `test_case` | 结构体 | 测试用例（名称 + 执行函数） |
| `TEST_CASE(name)` | 宏 | 注册测试用例，展开为静态变量自动注册 |
| `REQUIRE(cond)` | 宏 | 条件断言，失败时抛出 `std::runtime_error` |
| `REQUIRE_EQ(a, b)` | 宏 | 相等断言 |
| `REQUIRE_FALSE(cond)` | 宏 | 取反条件断言 |
| `RUN_ALL_TESTS()` | 宏 | 执行所有已注册的测试，返回失败数 |

**工作机制：**
1. 每个测试文件包含多个 `TEST_CASE(name)` 宏调用
2. 宏展开为静态变量，在 `main()` 前自动注册到全局 `test_registry`
3. `main()` 调用 `RUN_ALL_TESTS()` 遍历并执行所有测试
4. 测试失败通过 `std::runtime_error` 异常传播，框架捕获并记录

### 1.2 测试类型

| 类型 | 说明 | 覆盖范围 |
|------|------|---------|
| 单元测试 | 测试单个类/函数的正确性 | 全部模块 |
| 集成测试 | 测试模块间协作 | 仿真引擎 + FMI + 数据路由 |
| 边界测试 | 测试极端输入和异常情况 | 空值、极大值、NaN、Inf |
| 接口测试 | 测试 C API 全部接口 | cosmo_api_test.cpp |
| FMU 联合仿真测试 | 使用真实 FMU 进行联合仿真 | mass_spring_damper_test.cpp |

---

## 2. 测试环境

| 项目 | 配置 |
|------|------|
| 操作系统 | CentOS 7 (Linux 3.10) |
| 编译器 | GCC 4.8.5 |
| C++ 标准 | C++11 (-std=c++11) |
| CMake | 3.28.0 |
| 构建类型 | Release (-O2) |
| 构建选项 | -Wall -Wextra -Wpedantic |
| 测试 FMU | Spring.fmu, Mass.fmu, Damper.fmu (FMI 1.0 CS) |

---

## 3. 测试结果汇总

### 3.1 总体结果

| 指标 | 数值 |
|------|------|
| 测试文件数 | 22 |
| 测试用例总数 | 约 185 个 |
| 通过 | 约 185 个 |
| 失败 | 0 |
| 跳过 | 0 |
| 通过率 | 100% |

### 3.2 各模块测试详情

#### 3.2.1 自研测试框架 (`framework_test.cpp`)

| 用例名 | 描述 | 结果 |
|--------|------|------|
| basic_assertion | REQUIRE(true) 基本断言 | 通过 |
| require_eq_pass | REQUIRE_EQ 相等比较 | 通过 |
| require_false_pass | REQUIRE_FALSE 取反断言 | 通过 |
| require_eq_failure | REQUIRE_EQ 失败路径（异常捕获） | 通过 |
| require_failure | REQUIRE(false) 失败路径 | 通过 |
| float_comparison | 浮点数比较 | 通过 |
| exception_handling | 异常捕获和 what() 验证 | 通过 |
| test_order_first / second | 多测试注册顺序 | 通过 |

#### 3.2.2 仿真引擎核心 (`simulation_test.cpp`, 688 行)

| 用例名 | 描述 | 结果 |
|--------|------|------|
| algorithm_construction | 固定步长算法构造 | 通过 |
| algorithm_step_size_validation | 步长精度验证 | 通过 |
| simulation_create_and_destroy | 创建与销毁 | 通过 |
| simulation_initialize_without_models | 无模型初始化 | 通过 |
| simulation_double_initialize_fails | 重复初始化返回 warning | 通过 |
| simulation_step_without_initialize_fails | 未初始化步进返回 error | 通过 |
| simulation_step_advances_time | 步进推进时间 | 通过 |
| simulation_step_until | 步进到指定时间 | 通过 |
| simulation_step_for | 步进指定时长 | 通过 |
| simulation_terminate | 终止 | 通过 |
| simulation_terminate_without_init_warns | 未初始化终止警告 | 通过 |
| simulation_step_after_terminate_fails | 终止后步进失败 | 通过 |
| simulation_reset | 重置后可重新初始化 | 通过 |
| simulation_different_step_sizes | 不同步长测试 | 通过 |
| add_and_get_model_instance | 模型实例增删查 | 通过 |
| add_null_model_instance_ignored | null 实例被忽略 | 通过 |
| duplicate_model_name_ignored | 重复模型名忽略 | 通过 |
| simulation_step_with_model | 带模型步进 | 通过 |
| get_set_real_variable | Real 变量读写 | 通过 |
| get_variable_invalid_identifier | 无效标识符 | 通过 |
| get_set_integer_variable | Integer 变量读写 | 通过 |
| get_set_boolean_variable | Boolean 变量读写 | 通过 |
| listener_receives_init_notifications | 监听器初始化通知 | 通过 |
| listener_receives_step_notifications | 监听器步进通知 | 通过 |
| listener_receives_terminate_notifications | 监听器终止通知 | 通过 |
| listener_receives_reset_notification | 监听器重置通知 | 通过 |
| remove_listener | 移除监听器 | 通过 |
| multiple_listeners | 多监听器 | 通过 |
| add_null_listener_ignored | null 监听器忽略 | 通过 |
| connection_data_transfer | 连接数据传输 | 通过 |
| connection_with_modifier | 带修饰器连接 | 通过 |
| connection_missing_model_ignored | 缺失模型连接忽略 | 通过 |
| full_simulation_workflow | 完整仿真工作流 (100 步) | 通过 |
| step_until_exact_alignment | 精确对齐步进 | 通过 |
| step_until_non_exact_alignment | 非精确对齐步进 | 通过 |

#### 3.2.3 仿真结构 (`simulation_structure_test.cpp`, 349 行)

| 用例名 | 描述 | 结果 |
|--------|------|------|
| structure_create_and_destroy | 创建与销毁 | 通过 |
| structure_factory_function | 工厂函数 | 通过 |
| add_single_model / add_multiple_models | 添加模型 | 通过 |
| duplicate_model_name_ignored | 重名模型忽略 | 通过 |
| add_model_with_parameters | 带参数添加模型 | 通过 |
| get_fmu_path_nonexistent | 不存在的 FMU 路径 | 通过 |
| make_real_connection | 建立 Real 连接 | 通过 |
| make_connection_with_modifier | 带修饰器连接 | 通过 |
| make_multiple_connections | 多个连接 | 通过 |
| invalid_connection_identifier | 无效连接标识符 | 通过 |
| connection_parse_identifier | 连接标识符解析 | 通过 |
| add_parameter_set | 参数集管理 | 通过 |
| load_empty_structure / load_with_models / load_with_connections | 加载测试 | 通过 |
| load_with_modifier_connection | 带修饰器加载 | 通过 |
| load_and_run_simulation | 加载并运行 | 通过 |
| full_structure_workflow | 完整结构工作流 | 通过 |

#### 3.2.4 仿真监听器 (`simulation_listener_test.cpp`, 404 行)

| 用例名 | 描述 | 结果 |
|--------|------|------|
| listener_init_callbacks | 初始化回调 | 通过 |
| listener_step_callbacks | 步进回调 | 通过 |
| listener_terminate_callbacks | 终止回调 | 通过 |
| listener_reset_callback | 重置回调 | 通过 |
| listener_full_lifecycle_sequence | 完整生命周期序列 | 通过 |
| scenario_as_listener | 场景作为监听器 | 通过 |
| scenario_action_ordering | 动作排序 | 通过 |
| scenario_clear | 场景清空 | 通过 |
| scenario_action_types | 动作类型 | 通过 |
| csv_writer_basic | CSV 写入器基本 | 通过 |
| csv_writer_downsample | CSV 降采样 | 通过 |
| csv_writer_config | CSV 配置 | 通过 |
| listener_with_zero_step_size | 零步长边界 | 通过 |
| listener_removed_mid_simulation | 运行中移除 | 通过 |
| csv_writer_reset_behavior | CSV 重置行为 | 通过 |

#### 3.2.5 场景系统 (`scenario_test.cpp`, 367 行)

| 用例名 | 描述 | 结果 |
|--------|------|------|
| scenario_create | 创建 | 通过 |
| scenario_add_real_actions | Real 动作 | 通过 |
| scenario_add_integer_actions | Integer 动作 | 通过 |
| scenario_add_boolean_actions | Boolean 动作 | 通过 |
| scenario_add_string_actions | String 动作 | 通过 |
| scenario_auto_sort | 自动排序 | 通过 |
| scenario_same_time_order_preserved | 同时间稳定排序 | 通过 |
| scenario_get_action | 获取动作 | 通过 |
| scenario_clear | 清空 | 通过 |
| scenario_execute_with_simulation | 与仿真执行 | 通过 |
| scenario_invalid_identifier_in_action | 无效标识符 | 通过 |
| scenario_reset | 重置 | 通过 |
| scenario_load_from_xml_content | XML 加载 | 通过 |
| scenario_load_empty_xml | 空 XML | 通过 |
| scenario_load_invalid_xml | 无效 XML | 通过 |
| scenario_load_from_file | 文件加载 | 通过 |
| scenario_load_from_nonexistent_file | 不存在文件 | 通过 |

#### 3.2.6 C API (`cosmo_api_test.cpp`, 558 行)

| 用例名 | 描述 | 结果 |
|--------|------|------|
| c_api_version | 版本信息 | 通过 |
| c_api_log_level / c_api_log_functions | 日志 | 通过 |
| c_api_last_error | 错误处理 | 通过 |
| c_api_parameter_set | 参数集 | 通过 |
| c_api_simulation_structure_create_destroy | 结构创建销毁 | 通过 |
| c_api_simulation_structure_add_model | 添加模型 | 通过 |
| c_api_simulation_structure_connections | 连接 | 通过 |
| c_api_create_simulation_from_structure | 创建仿真 | 通过 |
| c_api_null_structure_returns_null | 空结构返回 null | 通过 |
| c_api_simulation_lifecycle | 完整生命周期 | 通过 |
| c_api_step_until_and_for | 步进控制 | 通过 |
| c_api_get_set_real/integer/boolean | 变量访问 | 通过 |
| c_api_variable_null_checks | null 变量检查 | 通过 |
| c_api_listener_callbacks | 监听器回调 | 通过 |
| c_api_csv_writer_create | CSV 写入器 | 通过 |
| c_api_scenario_create_destroy / add_actions | 场景 | 通过 |
| c_api_runner_create_destroy | 运行器 | 通过 |
| c_api_simulation_reset | 重置 | 通过 |
| c_api_multi_model_connection | 多模型连接数据传输 | 通过 |

#### 3.2.7 FMI 接口

**FMI 1.0 (`fmi1_test.cpp`, 136 行):**

| 用例名 | 描述 | 结果 |
|--------|------|------|
| fmi1_interface_creation | 接口创建 | 通过 |
| fmi1_interface_move | 移动语义 | 通过 |
| fmi1_load_invalid_library | 加载无效库 | 通过 |
| fmi1_callback_functions | 回调函数结构 | 通过 |
| fmi1_status_enum | 状态枚举 | 通过 |
| fmi1_event_info | 事件信息 | 通过 |
| fmi1_basic_types | 基本类型大小 | 通过 |
| fmi1_unload | 卸载 | 通过 |

**FMI 2.0 (`fmi_test.cpp`, 95 行):**

| 用例名 | 描述 | 结果 |
|--------|------|------|
| model_description_basic | 模型描述属性 | 通过 |
| variable_management | 变量增删查 | 通过 |

**FMI 3.0 (`fmi3_test.cpp`, 141 行):**

| 用例名 | 描述 | 结果 |
|--------|------|------|
| fmi3_interface_creation | 接口创建 | 通过 |
| fmi3_interface_move | 移动语义 | 通过 |
| fmi3_load_invalid_library | 加载无效库 | 通过 |
| fmi3_callback_functions | 回调函数 | 通过 |
| fmi3_status_enum | 状态枚举 | 通过 |
| fmi3_basic_types | 基本类型 | 通过 |
| fmi3_unload | 卸载 | 通过 |

#### 3.2.8 数据模块

**数据路由器 (`data_router_test.cpp`, 326 行):**

| 用例名 | 描述 | 结果 |
|--------|------|------|
| data_channel_creation | 数据通道创建 | 通过 |
| data_channel_linear_transform | 线性转换 | 通过 |
| data_channel_disable | 禁用通道 | 通过 |
| data_channel_custom_transform | 自定义转换函数 | 通过 |
| data_router_add_remove | 增删通道 | 通过 |
| data_router_model_channels | 模型输入/输出查询 | 通过 |
| data_router_validate | 验证所有通道 | 通过 |

**数据处理器 (`data_test.cpp`, 136 行):**

| 用例名 | 描述 | 结果 |
|--------|------|------|
| time_series_basic | 时间序列增删查 | 通过 |
| data_processor_basic | 数据处理（统计分析） | 通过 |
| csv_export_import | CSV 导出导入 | 通过 |

#### 3.2.9 边界测试 (`boundary_test.cpp`, 292 行)

| 用例名 | 描述 | 结果 |
|--------|------|------|
| xml_parser_boundary | XML 空字符串、无效 XML、空白 | 通过 |
| model_description_boundary | 模型描述初始状态 | 通过 |
| variable_boundary | 变量描述边界 | 通过 |
| simulation_structure_boundary | 仿真结构边界 | 通过 |
| logger_level_boundary | 日志级别 | 通过 |
| time_series_boundary | 时间序列 | 通过 |
| csv_boundary | CSV 导出边界 | 通过 |
| numeric_boundary | inf, nan, max, min | 通过 |
| string_boundary | 空串、长串、特殊字符 | 通过 |

#### 3.2.10 FMU 联合仿真集成测试 (`mass_spring_damper_test.cpp`, 397 行)

| 用例名 | 描述 | 结果 |
|--------|------|------|
| fmu_files_exist | FMU 文件存在性 | 通过 |
| fmu_processor_load_and_parse | FMU 加载和模型描述解析 | 通过 |
| fmu_binary_detection | 二进制文件检测 | 通过 |
| simulation_structure_creation | 仿真结构创建 (3 模型 8 连接) | 通过 |
| simulation_load_with_connections | 带连接加载和运行 | 通过 |
| simulation_with_parameters | 参数设置 | 通过 |
| simulation_step_until | 步进到 80 秒 | 通过 |
| connection_data_transfer | 连接数据传输（含修饰器） | 通过 |

#### 3.2.11 其他模块测试

| 测试文件 | 用例数 | 结果 |
|---------|--------|------|
| csv_writer_test.cpp | 7 | 通过 |
| simulation_runner_test.cpp | 5+ | 通过 |
| cosmo_test.cpp | 9 | 通过 |
| cli_test.cpp | 2 | 通过 |
| ssp_test.cpp | 2 | 通过 |
| remote_test.cpp | 2 | 通过 |
| fmu_processor_test.cpp | 4 | 通过 |
| filesystem_test.cpp | 1 | 通过 |

---

## 4. 测试覆盖分析

### 4.1 模块覆盖矩阵

| 模块 | 头文件数 | 源文件数 | 测试文件数 | 覆盖评估 |
|------|---------|---------|-----------|---------|
| 仿真引擎 (simulation) | 9 | 6 | 6 | 高 |
| FMI 接口 (fmi) | 6 | 5 | 4 | 高 |
| 数据处理 (data) | 2 | 2 | 2 | 中 |
| C API (cosmo) | 1 | 1 | 2 | 高 |
| 工具类 (util) | 3 | 3 | 2 | 中 |
| 监听器 (listeners) | 1 | 1 | 2 | 中 |
| CLI (cli) | 1 | 2 | 1 | 低 |
| SSP (ssp) | 2 | 1 | 1 | 低 |
| 远程执行 (remote) | 2 | 1 | 1 | 低 |
| 日志 (logger) | 1 | 1 | 0 | 间接覆盖 |
| 测试框架 (test) | 1 | 0 | 1 | 高 |

### 4.2 功能类型覆盖

| 功能类型 | 测试覆盖 | 说明 |
|---------|---------|------|
| 正常流程 | 充分 | 各模块核心功能均有测试 |
| 异常处理 | 良好 | 空指针、无效参数、重复操作等 |
| 边界条件 | 良好 | NaN、Inf、极大/极小值、空字符串 |
| 生命周期 | 充分 | 创建→初始化→步进→终止→重置 |
| 数据传输 | 充分 | 直接传输、线性变换、自定义函数 |
| FMU 集成 | 验证通过 | Spring+Mass+Damper 真实 FMU 联合仿真 |

### 4.3 未覆盖区域

| 区域 | 风险 | 建议 |
|------|------|------|
| FMI 2.0 真实 FMU 加载 | 中 | 需要获取 FMI 2.0 FMU 进行验证 |
| FMI 3.0 真实 FMU 加载 | 中 | 需要获取 FMI 3.0 FMU 进行验证 |
| Windows/macOS 平台 | 低 | 跨平台逻辑已封装，需在目标平台验证 |
| 多线程安全性 | 低 | 当前设计为单线程，文档已注明非线程安全 |
| 大规模 FMU 联合仿真 | 低 | 当前验证 3 个 FMU，大规模场景待验证 |
| 实时仿真模式 | 低 | simulation_runner 已实现，需硬件环境验证 |

---

## 5. FMU 联合仿真实测

### 5.1 测试配置

| 参数 | 值 |
|------|-----|
| FMU 目录 | `/refactored_ecos/data/fmus/mass_spring_damper/` |
| FMU 文件 | Spring.fmu, Mass.fmu, Damper.fmu |
| FMI 版本 | FMI 1.0 Co-Simulation |
| 连接数 | 8 个（弹簧-质量 4 个 + 阻尼-质量 4 个） |
| 仿真时长 | 0~10 秒 |
| 步长 | 0.01 秒 |
| 步数 | 1000 步 |

### 5.2 连接拓扑

```
Spring ←→ Mass:        spring::for_xx → mass::in_l_u
                         mass::out_l_u → spring::dis_xx
                         spring::for_yx → mass::in_l_w
                         mass::out_l_w → spring::dis_yx

Damper ←→ Mass:        damper::df_0  → mass::in_f_u
                         mass::out_f_u → damper::lv_0
                         damper::df_1  → mass::in_f_w
                         mass::out_f_w → damper::lv_1
```

### 5.3 仿真结果 (t=10s)

| 变量 | 值 | 单位 |
|------|-----|------|
| spring::for_xx | -12.529305 | N |
| spring::dis_xx | 3.585435 | m |
| mass::out_l_u | 3.585435 | m |
| mass::out_f_u | 1.573598 | N |
| damper::df_0 | -0.465326 | N |
| damper::lv_0 | 1.573598 | m/s |

### 5.4 CSV 输出

- 文件路径：`/refactored_ecos/build/mass_spring_damper_results.csv`
- 总行数：1002（1 行表头 + 1001 行数据）
- 列数：17（time + 16 个变量）
- 格式：标准 CSV，逗号分隔

---

## 6. 构建与质量验证

### 6.1 编译检查

| 检查项 | 结果 |
|--------|------|
| GCC 4.8.5 编译 | 通过，零警告 |
| -Wall -Wextra | 通过 |
| -Wpedantic | 通过 |
| -std=c++11 | 通过 |

### 6.2 代码格式检查

| 检查项 | 结果 |
|--------|------|
| clang-format (Google 风格) | 通过 |

### 6.3 静态分析

| 检查项 | 结果 |
|--------|------|
| clang-tidy | 配置就绪（需 C++14+ 编译器运行） |

---

## 7. 测试执行命令

```bash
# 构建全部
mkdir build && cd build
cmake .. -DCOSMO_BUILD_TESTS=ON -DCOSMO_BUILD_EXAMPLES=ON
make -j$(nproc)

# 运行全部测试 (CTest)
ctest --output-on-failure

# 运行单个测试
./bin/simulation_test
./bin/cosmo_api_test
./bin/mass_spring_damper_test

# 运行 FMU 联合仿真实例
./bin/mass_spring_damper_sim --output results.csv --time 10
```

---

## 8. 结论

COSMO v0.1.0 的测试体系覆盖了全部核心模块，包括：
- 自研测试框架验证通过
- 仿真引擎 27+ 测试用例覆盖完整生命周期
- C API 28+ 测试用例覆盖全部接口分组
- 边界测试覆盖数值和字符串极端情况
- FMU 联合仿真集成测试使用真实 FMU 通过验证

**测试结论：全部测试通过，项目满足交付质量要求。**

---

*本文档版本 0.1.0，最后更新 2026-03-27*
