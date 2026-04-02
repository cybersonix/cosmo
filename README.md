<div align="center">
  <div style="position: relative; display: inline-block; margin-bottom: 20px;">
    <h1 style="font-size: 72px; font-weight: 900; color: transparent; background: linear-gradient(135deg, #1e69c5, #28b581); background-clip: text; -webkit-background-clip: text; -webkit-text-fill-color: transparent; margin: 0; font-family: 'Orbitron', 'Segoe UI', sans-serif; text-transform: uppercase; letter-spacing: 3px; text-shadow: 0 0 20px rgba(96, 165, 250, 0.3);">
      COSMO
    </h1>
  </div>

  <p style="font-size: 16px; color: #94a3b8; margin: 0 0 30px 0; font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; letter-spacing: 1px; text-transform: uppercase;">
    Collaborative Open-Source Simulation & Modeling
  </p>
</div>

COSMO (Collaborative Open-Source Simulation & Modeling) 是一个基于 C++11 的开源多学科联合仿真引擎，用于 FMI (Functional Mock-up Interface) 标准的模型联合仿真。

## 特性

- **FMI 支持**: 支持 FMI 1.0、2.0 和 3.0 标准
- **SSP 支持**: 支持 System Structure Package (SSP) 标准
- **跨平台**: 支持 Windows、Linux 和 macOS
- **C++11 标准**: 完全兼容 C++11 标准
- **零依赖**: 不依赖任何第三方库
- **模块化设计**: 清晰的模块化架构，易于扩展

## 架构

### 核心模块

1. **XML 解析器** (`cosmo::util::xml_parser`)
   - 自研的 C++11 兼容 XML 解析器
   - 支持 DOM 解析，用于 SSP 和 FMU 的 XML 文件处理

2. **FMU 处理器** (`cosmo::fmi::fmu_processor`)
   - FMU 文件解压和解析
   - modelDescription.xml 解析
   - 跨平台 ZIP 解压支持

3. **动态库加载器** (`cosmo::util::dynamic_library`)
   - 跨平台动态库加载
   - 支持 Windows (LoadLibrary)、Linux (dlopen)、macOS (dlopen)

4. **FMI 接口** (`cosmo::fmi::fmi_interface`)
   - FMI 1.0、2.0 和 3.0 标准函数封装
   - 动态加载 FMU 库中的函数
   - 提供统一的 FMI 调用接口

5. **仿真引擎** (`cosmo::simulation`)
   - 固定步长仿真算法
   - 模型实例管理
   - 仿真生命周期管理

6. **SSP 加载器** (`cosmo::ssp::ssp_loader`)
   - SSP 文件解析
   - 组件和连接管理
   - 系统结构描述

7. **CLI 接口** (`cosmo::cli`)
   - 命令行界面
   - simulate 命令支持
   - 参数解析

## 构建

### 要求

- CMake 3.14+
- C++11 兼容编译器 (GCC 4.8+, Clang 3.3+, MSVC 2015+)
- Linux/macOS: unzip 命令
- Windows: PowerShell 或 7-Zip

### 构建步骤

```bash
mkdir build
cd build
cmake ..
make
```

### 测试

```bash
cd build
./bin/cosmo_comprehensive_test
```

## 使用示例

### C++ API

```cpp
#include "cosmo/simulation/simulation.hpp"

// 创建仿真
auto sim = std::unique_ptr<cosmo::simulation::simulation>(
    new cosmo::simulation::simulation(
        std::unique_ptr<cosmo::simulation::algorithm>(
            new cosmo::simulation::fixed_step_algorithm(0.01)
        )
    )
);

// 初始化
sim->initialize(0.0);

// 运行仿真
sim->step_until(10.0);

// 终止
sim->terminate();
```

### C API

```c
#include "cosmo/cosmo.h"

// 设置日志级别
cosmo_set_log_level("info");

// 获取版本信息
cosmo_version version = cosmo_library_version();
printf("COSMO version: %d.%d.%d\n", version.major, version.minor, version.patch);
```

### CLI

```bash
# 运行FMU仿真
./cosmo simulate --path model.fmu --step-size 0.01 --stop-time 10.0

# 运行SSP仿真
./cosmo simulate --path system.ssp --step-size 0.01 --stop-time 10.0
```

## 项目结构

```
cosmo/
├── include/
│   └── cosmo/
│       ├── cosmo.h              # C API头文件
│       ├── cli/                 # CLI接口
│       ├── data/                # 数据处理
│       ├── fmi/                 # FMI相关
│       ├── logger/              # 日志系统
│       ├── remote/              # 远程执行
│       ├── simulation/          # 仿真引擎
│       ├── ssp/                 # SSP相关
│       ├── test/                # 测试框架
│       └── util/                # 工具类
├── src/
│   ├── cli/                     # CLI实现
│   ├── cosmo/                   # 核心实现
│   ├── data/                    # 数据处理实现
│   ├── fmi/                     # FMI实现
│   ├── logger/                  # 日志实现
│   ├── remote/                  # 远程执行实现
│   ├── simulation/              # 仿真引擎实现
│   ├── ssp/                     # SSP实现
│   ├── test/                    # 测试实现
│   └── util/                    # 工具类实现
├── CMakeLists.txt               # 构建配置
└── README.md                    # 本文件
```

## 设计原则

1. **C++11 兼容**: 所有代码完全兼容 C++11 标准
2. **零依赖**: 不依赖任何第三方库
3. **跨平台**: 支持 Windows、Linux、macOS
4. **模块化**: 清晰的模块划分，易于维护和扩展
5. **标准化**: 遵循 FMI 和 SSP 国际标准
6. **自主可控**: 完全自主研发，满足 A 级验收标准

## 文档

- [API 参考手册](docs/API_REFERENCE.md) - 完整的 C/C++ API 文档
- [用户手册](docs/USER_MANUAL.md) - 详细的使用指南
- [开发指南](docs/DEVELOPMENT_GUIDE.md) - 开发规范和架构设计
- [需求规格说明书](docs/REQUIREMENTS.md) - 功能需求和非功能需求
- [架构设计文档](docs/ARCHITECTURE.md) - 五层架构、模块设计、数据流
- [测试报告](docs/TEST_REPORT.md) - 测试框架、用例详情、覆盖分析

## 测试

当前测试覆盖包含以下测试套件（共约 185 个测试用例，分布在 22 个测试文件中）：

| 测试模块             | 测试文件 | 测试数量 | 状态   |
| -------------------- | -------- | -------- | ------ |
| 仿真核心             | simulation_test.cpp | 27+ | 通过 |
| 仿真结构配置         | simulation_structure_test.cpp | 16+ | 通过 |
| 仿真监听器           | simulation_listener_test.cpp | 15+ | 通过 |
| 场景系统             | scenario_test.cpp | 18+ | 通过 |
| C API 接口           | cosmo_api_test.cpp | 28+ | 通过 |
| 数据路由器           | data_router_test.cpp | 13+ | 通过 |
| CSV 写入器           | csv_writer_test.cpp | 7 | 通过 |
| 边界条件             | boundary_test.cpp | 11+ | 通过 |
| FMI 1.0 接口         | fmi1_test.cpp | 7+ | 通过 |
| FMI 2.0 接口         | fmi_test.cpp | 2+ | 通过 |
| FMI 3.0 接口         | fmi3_test.cpp | 7+ | 通过 |
| 集成仿真             | mass_spring_damper_test.cpp | 8 | 通过 |

运行所有测试：

```bash
cd build
for test in ./bin/*_test; do $test; done
```

## 许可证

MIT License

## 贡献

欢迎提交 Issue 和 Pull Request 来改进项目。

## 版本历史

- **v0.1.0** - 初始版本
  - FMI 1.0/2.0/3.0 完整支持
  - SSP 标准支持
  - 数据路由系统
  - 场景仿真功能
  - 完整文档和测试