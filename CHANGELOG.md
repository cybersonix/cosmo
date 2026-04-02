# Changelog

All notable changes to the COSMO project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2026-03-27

### Added
- 仿真引擎核心框架：仿真生命周期管理（初始化/步进/终止/重置）
- 固定步长仿真算法 (fixed_step_algorithm)
- 模型实例抽象基类和模拟实现 (mock_model_instance)
- 仿真结构配置 (simulation_structure)：支持模型注册、变量连接和参数集
- 场景系统 (scenario)：支持按时间点执行实数/整数/布尔/字符串动作
- 监听器框架 (simulation_listener)：生命周期事件回调通知
- CSV结果写入器 (csv_writer)：支持降采样和自动变量记录
- 仿真运行器 (simulation_runner)：支持异步执行和实时因子控制
- C语言接口 (cosmo.h)：完整的C API，支持跨语言调用
- XML解析器：轻量级XML解析，支持模型描述和场景配置
- 动态库加载器：跨平台动态库加载抽象
- 文件系统工具：文件读写和路径操作
- 日志系统：多级别日志输出
- FMI类型定义：FMI 1.0/2.0/3.0标准类型和状态码
- 模型描述解析：FMU modelDescription.xml解析支持
- FMU处理器：FMU文件解压和动态库加载框架
- SSP加载器框架：System Structure Package解析基础
- 数据路由器：多通道数据传输管理
- 数据处理器：时间序列数据处理和合并
- 远程执行框架：分布式仿真基础架构
- CLI命令行工具：仿真配置和执行入口
- 完整单元测试套件：199个测试用例覆盖全部模块
- 构建系统：CMake构建，支持CTest/CPack集成
- 项目文档：API参考、开发指南、用户手册

### Design Decisions
- 零第三方依赖：仅使用C++11标准库和操作系统原生API
- 模块化架构：核心引擎、FMI桥接、数据管理、工具库分层解耦
- C/C++双接口：C++面向对象API + C语言平坦API
- 不透明句柄模式：C API通过不透明指针封装C++对象
