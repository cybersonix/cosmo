# COSMO 需求规格说明书

## 文档信息

| 项目 | 内容 |
|------|------|
| 项目名称 | COSMO (Collaborative Open-Source Simulation & Modeling) |
| 版本 | 0.1.0 |
| 编写日期 | 2026-03-27 |
| 状态 | 已批准 |

---

## 1. 引言

### 1.1 编写目的

本文档详细描述 COSMO 联合仿真引擎的功能需求和非功能需求，为项目设计、开发、测试和验收提供依据。

### 1.2 项目背景

COSMO 是基于开源项目 ecos 深度重构的多领域联合仿真引擎。ecos 原项目依赖 C++20 和多个第三方库（spdlog、pugixml、flatbuffers、CLI11、Catch2 等），在 CentOS 7 等旧系统上无法编译，且存在供应链安全风险。COSMO 的目标是在保留全部核心功能的前提下，实现零第三方依赖和 C++11 兼容。

### 1.3 术语定义

| 术语 | 全称 | 定义 |
|------|------|------|
| FMI | Functional Mock-up Interface | 功能样机接口标准，用于模型交换和联合仿真 |
| FMU | Functional Mock-up Unit | 功能样机单元，FMI 标准的模型封装格式 |
| SSP | System Structure and Parameterization | 系统结构与参数化标准 |
| Co-Simulation | Co-Simulation | 联合仿真，多个模型按固定步长协同推进 |
| CS | Co-Simulation | FMI 标准中的联合仿真类型 |
| ME | Model Exchange | FMI 标准中的模型交换类型 |

---

## 2. 功能需求

### 2.1 FMI 标准支持 [FR-FMI-001]

#### FR-FMI-001: FMI 多版本支持

| 编号 | 需求 | 优先级 | 状态 |
|------|------|--------|------|
| FR-FMI-001a | 支持 FMI 1.0 Co-Simulation 标准 | 高 | 已实现 |
| FR-FMI-001b | 支持 FMI 2.0 Co-Simulation 标准 | 高 | 已实现 |
| FR-FMI-001c | 支持 FMI 3.0 Co-Simulation 标准 | 中 | 已实现 |

**验收标准：**
- 能够加载符合 FMI 1.0/2.0/3.0 标准的 FMU 文件
- 能够实例化、初始化、步进和终止 FMU 模型
- 能够读取和设置 FMU 的 Real、Integer、Boolean、String 类型变量

### 2.2 FMU 文件处理 [FR-FMU-002]

| 编号 | 需求 | 优先级 | 状态 |
|------|------|--------|------|
| FR-FMU-002a | 支持解压 FMU 文件（ZIP 格式） | 高 | 已实现 |
| FR-FMU-002b | 解析 modelDescription.xml 获取模型元数据 | 高 | 已实现 |
| FR-FMU-002c | 自动检测并加载匹配当前平台的二进制库 | 高 | 已实现 |
| FR-FMU-002d | 支持变量描述信息查询（名称、类型、因果性、值引用） | 高 | 已实现 |

**验收标准：**
- 输入 FMU 文件路径，输出模型名称、GUID、FMI 版本、变量列表
- 自动选择 linux64/win64/darwin64 目录下的正确二进制

### 2.3 仿真引擎 [FR-SIM-001]

| 编号 | 需求 | 优先级 | 状态 |
|------|------|--------|------|
| FR-SIM-001a | 支持固定步长仿真算法 | 高 | 已实现 |
| FR-SIM-001b | 支持自定义算法扩展接口 | 中 | 已实现 |
| FR-SIM-001c | 仿真生命周期管理：创建→初始化→步进→终止 | 高 | 已实现 |
| FR-SIM-001d | 支持仿真重置，允许重新初始化 | 中 | 已实现 |
| FR-SIM-001e | 支持步进到指定时间点 (step_until) | 高 | 已实现 |
| FR-SIM-001f | 支持步进指定时长 (step_for) | 中 | 已实现 |
| FR-SIM-001g | 模型实例管理（添加、查询、遍历） | 高 | 已实现 |

**验收标准：**
- 仿真步进正确推进时间，步长误差不超过浮点精度
- 未初始化时执行步进返回错误状态
- 终止后执行步进返回错误状态
- 重置后可重新初始化和运行

### 2.4 模型间连接与数据传输 [FR-CONN-001]

| 编号 | 需求 | 优先级 | 状态 |
|------|------|--------|------|
| FR-CONN-001a | 支持模型间变量连接（source→target） | 高 | 已实现 |
| FR-CONN-001b | 支持带转换函数的连接（线性变换、自定义函数） | 中 | 已实现 |
| FR-CONN-001c | 步进前自动传输连接数据 | 高 | 已实现 |
| FR-CONN-001d | 支持数据路由器多通道管理 | 中 | 已实现 |

**验收标准：**
- 连接数据在步进前正确从源模型传输到目标模型
- 带修饰器的连接正确应用转换函数
- 连接目标模型不存在时安全忽略

### 2.5 场景系统 [FR-SCN-001]

| 编号 | 需求 | 优先级 | 状态 |
|------|------|--------|------|
| FR-SCN-001a | 支持按时间点设置变量值（Real/Integer/Boolean/String） | 中 | 已实现 |
| FR-SCN-001b | 支持从 XML 文件加载场景配置 | 中 | 已实现 |
| FR-SCN-001c | 动作按时间自动排序执行 | 高 | 已实现 |

**验收标准：**
- 场景动作在指定仿真时间精确触发
- 多个同一时间的动作保持注册顺序
- 无效变量标识符安全跳过

### 2.6 数据记录 [FR-DATA-001]

| 编号 | 需求 | 优先级 | 状态 |
|------|------|--------|------|
| FR-DATA-001a | 支持 CSV 格式结果导出 | 高 | 已实现 |
| FR-DATA-001b | 支持降采样记录（每隔 N 步记录一次） | 中 | 已实现 |
| FR-DATA-001c | 支持注册指定变量进行记录 | 中 | 已实现 |
| FR-DATA-001d | 支持时间序列数据存储与查询 | 低 | 已实现 |

### 2.7 SSP 标准 [FR-SSP-001]

| 编号 | 需求 | 优先级 | 状态 |
|------|------|--------|------|
| FR-SSP-001a | 支持 SSP 文件解析 | 中 | 已实现（基础） |
| FR-SSP-001b | 从 SSP 描述创建仿真结构 | 中 | 已实现（基础） |

### 2.8 API 接口 [FR-API-001]

| 编号 | 需求 | 优先级 | 状态 |
|------|------|--------|------|
| FR-API-001a | 提供完整的 C++ 面向对象 API | 高 | 已实现 |
| FR-API-001b | 提供完整的 C 语言平面 API（cosmo.h） | 高 | 已实现 |
| FR-API-001c | C API 使用不透明句柄模式封装 C++ 对象 | 高 | 已实现 |
| FR-API-001d | C API 支持跨语言调用（Python ctypes 等） | 中 | 已实现 |

### 2.9 CLI 工具 [FR-CLI-001]

| 编号 | 需求 | 优先级 | 状态 |
|------|------|--------|------|
| FR-CLI-001a | 命令行仿真执行入口 | 中 | 已实现 |
| FR-CLI-001b | 支持命令行参数解析 | 中 | 已实现 |
| FR-CLI-001c | 支持 --help、--version 等标准选项 | 低 | 已实现 |

### 2.10 远程执行 [FR-REMOTE-001]

| 编号 | 需求 | 优先级 | 状态 |
|------|------|--------|------|
| FR-REMOTE-001a | 远程仿真执行基础框架 | 低 | 已实现（基础） |
| FR-REMOTE-001b | 远程端点类型定义 | 低 | 已实现 |

---

## 3. 非功能需求

### 3.1 兼容性需求 [NFR-COMPAT-001]

| 编号 | 需求 | 优先级 | 状态 |
|------|------|--------|------|
| NFR-COMPAT-001a | 兼容 C++11 标准（GCC 4.8+, Clang 3.3+, MSVC 2015+） | 高 | 已实现 |
| NFR-COMPAT-001b | 跨平台支持（Linux、Windows、macOS） | 高 | 已实现 |
| NFR-COMPAT-001c | 不依赖任何第三方库 | 高 | 已实现 |

### 3.2 代码质量需求 [NFR-QUALITY-001]

| 编号 | 需求 | 优先级 | 状态 |
|------|------|--------|------|
| NFR-QUALITY-001a | 遵循 Google C++ Style Guide | 中 | 已实现 |
| NFR-QUALITY-001b | 提供 clang-format 格式化配置 | 中 | 已实现 |
| NFR-QUALITY-001c | 提供 clang-tidy 静态分析配置 | 中 | 已实现 |
| NFR-QUALITY-001d | 编译零警告（-Wall -Wextra） | 高 | 已实现 |
| NFR-QUALITY-001e | 代码注释使用中文 | 低 | 已实现 |

### 3.3 测试需求 [NFR-TEST-001]

| 编号 | 需求 | 优先级 | 状态 |
|------|------|--------|------|
| NFR-TEST-001a | 自研轻量级测试框架（不依赖 Catch2 等第三方库） | 高 | 已实现 |
| NFR-TEST-001b | 测试覆盖全部核心模块 | 高 | 已实现 |
| NFR-TEST-001c | 包含单元测试、集成测试和边界测试 | 高 | 已实现 |
| NFR-TEST-001d | 集成 CTest 自动化测试 | 中 | 已实现 |
| NFR-TEST-001e | 支持真实 FMU 联合仿真集成测试 | 中 | 已实现 |

### 3.4 构建与部署需求 [NFR-BUILD-001]

| 编号 | 需求 | 优先级 | 状态 |
|------|------|--------|------|
| NFR-BUILD-001a | 使用 CMake 构建系统（CMake 3.10+） | 高 | 已实现 |
| NFR-BUILD-001b | 支持静态库和共享库构建 | 中 | 已实现 |
| NFR-BUILD-001c | 支持 CPack 打包（DEB/RPM/TGZ） | 中 | 已实现 |
| NFR-BUILD-001d | 支持 find_package 安装后查找 | 中 | 已实现 |
| NFR-BUILD-001e | 支持 GitHub Actions CI/CD | 中 | 已实现 |

### 3.5 文档需求 [NFR-DOC-001]

| 编号 | 需求 | 优先级 | 状态 |
|------|------|--------|------|
| NFR-DOC-001a | API 参考文档 | 高 | 已实现 |
| NFR-DOC-001b | 用户手册（安装、使用、故障排除） | 高 | 已实现 |
| NFR-DOC-001c | 开发指南（规范、架构、贡献流程） | 高 | 已实现 |
| NFR-DOC-001d | 需求规格说明书 | 高 | 已实现 |
| NFR-DOC-001e | 架构设计文档 | 高 | 已实现 |
| NFR-DOC-001f | 测试报告 | 高 | 已实现 |
| NFR-DOC-001g | 变更日志 | 中 | 已实现 |

### 3.6 安全与自主可控需求 [NFR-SEC-001]

| 编号 | 需求 | 优先级 | 状态 |
|------|------|--------|------|
| NFR-SEC-001a | 零第三方依赖，消除供应链风险 | 高 | 已实现 |
| NFR-SEC-001b | 全部代码从零编写，无知识产权争议 | 高 | 已实现 |
| NFR-SEC-001c | 开源代码，完全可审计 | 高 | 已实现 |
| NFR-SEC-001d | MIT License 开源许可 | 中 | 已实现 |

---

## 4. 约束条件

### 4.1 技术约束

- 编译器最低版本：GCC 4.8.5（CentOS 7 默认）
- 操作系统：Linux (CentOS 7+, Ubuntu 16.04+)、Windows、macOS
- C++ 标准：C++11（不支持 C++14/17/20 特性）
- 不允许使用任何第三方库

### 4.2 设计约束

- 必须提供 C 语言平面 API（不透明句柄模式）
- 命名空间统一使用 `cosmo`
- 不允许使用 RTTI（dynamic_cast）和异常（除测试框架外）
- 不允许使用 STL 中的 shared_mutex、regex 等高版本特性

---

## 5. 需求追踪矩阵

| 需求编号 | 对应模块 | 头文件 | 源文件 | 测试文件 |
|---------|---------|--------|--------|---------|
| FR-FMI-001a | fmi | fmi1_interface.hpp | fmi1_interface.cpp | fmi1_test.cpp |
| FR-FMI-001b | fmi | fmi_interface.hpp | fmi_interface.cpp | fmi_test.cpp |
| FR-FMI-001c | fmi | fmi3_interface.hpp | fmi3_interface.cpp | fmi3_test.cpp |
| FR-FMU-002 | fmi | fmu_processor.hpp, model_description.hpp | fmu_processor.cpp, model_description.cpp | fmu_processor_test.cpp |
| FR-SIM-001 | simulation | simulation.hpp | simulation.cpp | simulation_test.cpp |
| FR-CONN-001 | simulation/data | simulation_structure.hpp, data_router.hpp | data_router.cpp | data_router_test.cpp |
| FR-SCN-001 | simulation | scenario.hpp | scenario.cpp | scenario_test.cpp |
| FR-DATA-001 | listeners | csv_writer.hpp | csv_writer.cpp | csv_writer_test.cpp |
| FR-SSP-001 | ssp | ssp_loader.hpp, ssp_types.hpp | ssp_loader.cpp | ssp_test.cpp |
| FR-API-001 | cosmo | cosmo.h | cosmo.cpp | cosmo_api_test.cpp |
| FR-CLI-001 | cli | cli.hpp | cli.cpp, cli_main.cpp | cli_test.cpp |
| FR-REMOTE-001 | remote | remote_executor.hpp, remote_types.hpp | remote_executor.cpp | remote_test.cpp |
| NFR-TEST-001a | test | test_framework.hpp | - | framework_test.cpp |
| NFR-BUILD-001 | - | CMakeLists.txt | - | - |

---

## 6. 验收标准

### 6.1 功能验收

- [x] 能够加载 FMI 1.0 FMU 并执行联合仿真
- [x] 能够加载 FMI 2.0 FMU（接口已实现，待更多 FMU 验证）
- [x] 能够加载 FMI 3.0 FMU（接口已实现，待更多 FMU 验证）
- [x] Mass-Spring-Damper 三体联合仿真正确运行并输出 CSV

### 6.2 质量验收

- [x] GCC 4.8.5 编译通过，零警告
- [x] 全部测试用例通过
- [x] clang-format 检查通过
- [x] CI/CD 流水线正常

### 6.3 文档验收

- [x] API 参考文档完整
- [x] 用户手册完整
- [x] 开发指南完整
- [x] 需求规格说明书完整
- [x] 架构设计文档完整
- [x] 测试报告完整

---

*本文档版本 0.1.0，最后更新 2026-03-27*
