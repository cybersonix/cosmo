# COSMO 架构设计文档

## 文档信息

| 项目 | 内容 |
|------|------|
| 项目名称 | COSMO (Collaborative Open-Source Simulation & Modeling) |
| 版本 | 0.1.0 |
| 编写日期 | 2026-03-27 |

---

## 1. 架构概述

### 1.1 设计目标

1. **零依赖**：仅使用 C++11 标准库和操作系统原生 API（dlopen、unzip 等）
2. **C++11 兼容**：支持 GCC 4.8+ 等旧编译器
3. **模块化**：高内聚低耦合，每个模块职责单一
4. **双接口**：C++ 面向对象 API + C 语言平坦 API
5. **可扩展**：支持自定义仿真算法、数据转换函数、监听器

### 1.2 系统架构

COSMO 采用五层架构设计，自底向上分别为：

```
┌──────────────────────────────────────────────────────────────┐
│                     应用层 (Application)                      │
│  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐    │
│  │  C API        │  │  C++ API      │  │  CLI 工具     │    │
│  │ (cosmo.h)     │  │ (namespace)   │  │ (cli_main)    │    │
│  └───────────────┘  └───────────────┘  └───────────────┘    │
├──────────────────────────────────────────────────────────────┤
│                   仿真管理层 (Simulation)                      │
│  ┌────────────┐ ┌────────────┐ ┌────────────┐              │
│  │  Engine    │ │  Structure │ │  Scenario  │              │
│  │  (仿真核心) │ │  (结构配置) │ │  (场景系统) │              │
│  └────────────┘ └────────────┘ └────────────┘              │
│  ┌────────────┐ ┌────────────┐ ┌────────────┐              │
│  │  Runner    │ │  Listener  │ │  ModelInst │              │
│  │  (运行器)  │ │  (监听器)  │ │  (模型实例) │              │
│  └────────────┘ └────────────┘ └────────────┘              │
├──────────────────────────────────────────────────────────────┤
│                   数据处理层 (Data)                           │
│  ┌────────────┐ ┌────────────┐ ┌────────────┐              │
│  │  Router    │ │  Channel   │ │  Processor │              │
│  │  (数据路由) │ │  (数据通道) │ │  (数据处理器)│              │
│  └────────────┘ └────────────┘ └────────────┘              │
├──────────────────────────────────────────────────────────────┤
│                   FMI 接口层 (FMI)                           │
│  ┌──────┐ ┌──────┐ ┌──────┐ ┌────────┐ ┌──────┐           │
│  │FMI1.0│ │FMI2.0│ │FMI3.0│ │ FMU    │ │ Model │           │
│  │      │ │      │ │      │ │Processor│ │ Desc │           │
│  └──────┘ └──────┘ └──────┘ └────────┘ └──────┘           │
├──────────────────────────────────────────────────────────────┤
│                   SSP 接口层 (SSP)                           │
│  ┌────────────┐ ┌────────────┐                              │
│  │  SSP Loader│ │  SSP Types │                              │
│  └────────────┘ └────────────┘                              │
├──────────────────────────────────────────────────────────────┤
│                   基础工具层 (Util)                           │
│  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────────┐                   │
│  │ XML  │ │ File │ │ Log  │ │ DynLib   │                   │
│  │Parser│ │System│ │      │ │ Loader   │                   │
│  └──────┘ └──────┘ └──────┘ └──────────┘                   │
└──────────────────────────────────────────────────────────────┘
```

---

## 2. 模块详细设计

### 2.1 基础工具层 (cosmo::util)

基础工具层为上层所有模块提供底层支撑。

#### 2.1.1 XML 解析器 (`xml_parser`)

| 属性 | 说明 |
|------|------|
| 文件 | `include/cosmo/util/xml_parser.hpp` + `src/util/xml_parser.cpp` |
| 行数 | 363 + 540 = 903 行 |
| 职责 | 轻量级 XML DOM 解析，用于解析 modelDescription.xml 和场景 XML |
| 设计决策 | 自研实现，替代 pugixml。采用树形节点结构 (xml_node / xml_document) |

**核心类：**
```
xml_document      -- 文档容器，提供 load_file / load_string / save_file
xml_node          -- 节点，支持 tag / attributes / children
xml_attribute     -- 属性键值对
```

#### 2.1.2 动态库加载器 (`dynamic_library`)

| 属性 | 说明 |
|------|------|
| 文件 | `include/cosmo/util/dynamic_library.hpp` + `src/util/dynamic_library.cpp` |
| 行数 | 73 + 172 = 245 行 |
| 职责 | 跨平台动态库加载封装（dlopen / LoadLibrary） |
| 设计决策 | RAII 模式管理库句柄，加载时获取函数指针 |

**核心类：**
```
dynamic_library   -- 封装 dlopen/dlsym/dlclose
  + load(path)    -- 加载共享库
  + get_symbol()  -- 获取导出函数指针
  + is_loaded()   -- 查询加载状态
```

#### 2.1.3 文件系统工具 (`filesystem`)

| 属性 | 说明 |
|------|------|
| 文件 | `include/cosmo/util/filesystem.hpp` + `src/util/filesystem.cpp` |
| 行数 | 41 + 148 = 189 行 |
| 职责 | 文件读写、路径操作、目录遍历 |
| 设计决策 | 自由函数接口，不封装为类 |

**核心函数：**
```
read_file() / write_file()    -- 文件读写
file_exists()                  -- 文件存在检查
file_extension()               -- 扩展名提取
parent_directory()             -- 父目录提取
```

#### 2.1.4 日志系统 (`logger`)

| 属性 | 说明 |
|------|------|
| 文件 | `include/cosmo/logger/logger.hpp` + `src/logger/console_logger.cpp` |
| 行数 | 241 + 132 = 373 行 |
| 职责 | 多级别日志输出（trace/debug/info/warn/error/fatal） |
| 设计决策 | 单例模式，线程安全，提供 C++ 和 C 两套接口 |

**核心接口：**
```
cosmo::log::info(msg)          -- C++ 接口
cosmo::log::set_logging_level(cosmo::log::level::debug)
cosmo_info(msg)                -- C 兼容接口
cosmo_set_log_level("debug")   -- C API
```

---

### 2.2 FMI 接口层 (cosmo::fmi)

FMI 接口层封装 FMI 1.0/2.0/3.0 三个版本的函数调用。

#### 2.2.1 类型定义 (`fmi_types`)

| 属性 | 说明 |
|------|------|
| 文件 | `include/cosmo/fmi/fmi_types.hpp` |
| 行数 | 135 行 |
| 职责 | 定义 FMI 标准的类型别名和状态枚举 |

**核心类型：**
```
fmi_real = double
fmi_integer = int
fmi_boolean = int
fmi_string = const char*
fmi_value_reference = unsigned int
fmi_status { OK, Warning, Discard, Error, Fatal, Pending }
```

#### 2.2.2 FMI 1.0 接口 (`fmi1_interface`)

| 属性 | 说明 |
|------|------|
| 文件 | `include/cosmo/fmi/fmi1_interface.hpp` + `src/fmi/fmi1_interface.cpp` |
| 行数 | 407 + 501 = 908 行 |
| 职责 | 封装 FMI 1.0 Co-Simulation 函数调用 |

**核心函数：**
```
instantiate_slave()   -- 创建实例
initialize_slave()    -- 初始化
do_step()             -- 执行步进
terminate_slave()     -- 终止
get_real/set_real()   -- 变量读写
```

#### 2.2.3 FMI 2.0 接口 (`fmi_interface`)

| 属性 | 说明 |
|------|------|
| 文件 | `include/cosmo/fmi/fmi_interface.hpp` + `src/fmi/fmi_interface.cpp` |
| 行数 | 603 + 470 = 1073 行 |
| 职责 | 封装 FMI 2.0 Co-Simulation 函数调用 |

#### 2.2.4 FMI 3.0 接口 (`fmi3_interface`)

| 属性 | 说明 |
|------|------|
| 文件 | `include/cosmo/fmi/fmi3_interface.hpp` + `src/fmi/fmi3_interface.cpp` |
| 行数 | 315 + 515 = 830 行 |
| 职责 | 封装 FMI 3.0 Co-Simulation 函数调用 |

#### 2.2.5 FMU 处理器 (`fmu_processor`)

| 属性 | 说明 |
|------|------|
| 文件 | `include/cosmo/fmi/fmu_processor.hpp` + `src/fmi/fmu_processor.cpp` |
| 行数 | 94 + 436 = 530 行 |
| 职责 | FMU 文件解压、二进制检测、modelDescription.xml 解析 |

**处理流程：**
```
FMU 文件 (.fmu)
  ↓ unzip
解压目录
  ↓ parse modelDescription.xml
ModelDescription (变量列表、GUID、版本)
  ↓ detect platform binary
二进制路径 (binaries/linux64/*.so)
```

#### 2.2.6 模型描述 (`model_description`)

| 属性 | 说明 |
|------|------|
| 文件 | `include/cosmo/fmi/model_description.hpp` + `src/fmi/model_description.cpp` |
| 行数 | 221 + 98 = 319 行 |
| 职责 | 解析和存储 modelDescription.xml 的结构化数据 |

---

### 2.3 仿真管理层 (cosmo::simulation)

仿真管理层是 COSMO 的核心，负责仿真生命周期管理、模型协调和数据传输。

#### 2.3.1 仿真引擎 (`simulation`)

| 属性 | 说明 |
|------|------|
| 文件 | `include/cosmo/simulation/simulation.hpp` + `src/simulation/simulation.cpp` |
| 行数 | 169 + 658 = 827 行 |
| 职责 | 仿真主控：管理模型实例、执行步进、传输连接数据、通知监听器 |

**仿真生命周期：**
```
Construction → initialize(t0) → [step()]→* → terminate() → [reset()]→
                                    ↑ step_until(t) / step_for(dt)
```

**步进执行流程：**
```
1. notify_pre_step(time, step_size)      -- 通知所有监听器
2. transfer_connection_data()            -- 传输连接数据 (source → target)
3. for each model_instance:
     model->step(time, step_size)        -- 执行各模型步进
4. notify_post_step(time, step_size)     -- 通知所有监听器
5. current_time_ += step_size            -- 推进仿真时间
6. step_count_++                         -- 增加步数
```

**连接数据传输：**
```
connection_binding {
    source_model, source_variable   -- 数据来源
    target_model, target_variable   -- 数据目标
    modifier                        -- 可选转换函数 (double → double)
}
```

#### 2.3.2 仿真算法 (`algorithm` / `fixed_step_algorithm`)

| 属性 | 说明 |
|------|------|
| 文件 | `include/cosmo/simulation/simulation.hpp` (内嵌) |
| 职责 | 定义算法接口和固定步长实现 |

```
algorithm (抽象接口)
  ├── initialize()
  ├── step(current_time, step_size)
  ├── terminate()
  └── get_step_size()

fixed_step_algorithm (默认实现)
  └── 简单返回 OK，由 simulation 类控制步进节奏
```

#### 2.3.3 仿真结构 (`simulation_structure`)

| 属性 | 说明 |
|------|------|
| 文件 | `include/cosmo/simulation/simulation_structure.hpp` + `src/simulation/simulation_structure.cpp` |
| 行数 | 126 + 308 = 434 行 |
| 职责 | 声明式定义仿真配置（模型、连接、参数），然后一次性加载为仿真实例 |

**配置与加载模式：**
```
simulation_structure (配置)
  ├── add_model(name, fmu_path)         -- 添加模型
  ├── make_connection(src, tgt)         -- 建立连接
  ├── add_parameter_set(name, params)   -- 设置参数
  └── load(algorithm) → simulation*     -- 加载为仿真实例
```

#### 2.3.4 模型实例 (`model_instance` / `fmi_model_instance`)

| 属性 | 说明 |
|------|------|
| 文件 | `include/cosmo/simulation/model_instance.hpp` + `fmi_model_instance.hpp` |
| 职责 | 模型实例抽象和 FMU 模型实例实现 |

```
model_instance (抽象基类)
  ├── get_name()
  ├── initialize() / step() / terminate()
  ├── get_real() / set_real()
  └── get_variables()

fmi_model_instance (FMU 实现)
  ├── 持有 fmi_interface + component handle
  ├── 封装 FMU 函数调用
  └── 自动管理 FMU 生命周期

mock_model_instance (测试用模拟实现)
  └── 内存中维护变量值，无需真实 FMU
```

#### 2.3.5 场景系统 (`scenario`)

| 属性 | 说明 |
|------|------|
| 文件 | `include/cosmo/simulation/scenario.hpp` + `src/simulation/scenario.cpp` |
| 行数 | 198 + 415 = 613 行 |
| 职责 | 按时间点修改变量值的事件驱动系统 |

**场景同时继承 simulation_listener，在步进中自动触发：**
```
scenario::on_pre_step(time, step) {
    while (next_action_time <= time) {
        execute_action(next_action);  // 设置变量值
        advance_action_pointer();
    }
}
```

#### 2.3.6 仿真监听器 (`simulation_listener`)

| 属性 | 说明 |
|------|------|
| 文件 | `include/cosmo/simulation/simulation_listener.hpp` |
| 行数 | 60 行 |
| 职责 | 仿真生命周期事件回调接口 |

```
simulation_listener (抽象接口)
  ├── on_pre_init(start_time)
  ├── on_post_init(start_time)
  ├── on_pre_step(time, step_size)
  ├── on_post_step(time, step_size)
  ├── on_pre_terminate()
  ├── on_post_terminate()
  └── on_reset()
```

#### 2.3.7 仿真运行器 (`simulation_runner`)

| 属性 | 说明 |
|------|------|
| 文件 | `include/cosmo/simulation/simulation_runner.hpp` + `src/simulation/simulation_runner.cpp` |
| 行数 | 97 + 221 = 318 行 |
| 职责 | 仿真运行控制（实时模式、暂停/恢复） |

---

### 2.4 数据处理层 (cosmo::data)

#### 2.4.1 数据路由器 (`data_router`)

| 属性 | 说明 |
|------|------|
| 文件 | `include/cosmo/data/data_router.hpp` + `src/data/data_router.cpp` |
| 行数 | 324 + 347 = 671 行 |
| 职责 | 多通道数据传输管理 |

**通道类型：**
```
transfer_type {
    direct,     -- 直接传递
    linear,     -- 线性变换 y = kx + b
    custom,     -- 自定义函数
    disabled    -- 禁用
}
```

#### 2.4.2 数据处理器 (`data_processor`)

| 属性 | 说明 |
|------|------|
| 文件 | `include/cosmo/data/data_processor.hpp` + `src/data/data_processor.cpp` |
| 行数 | 100 + 239 = 339 行 |
| 职责 | 时间序列数据处理、统计分析、CSV 导入导出 |

---

### 2.5 应用层

#### 2.5.1 C API (`cosmo.h`)

| 属性 | 说明 |
|------|------|
| 文件 | `include/cosmo/cosmo.h` + `src/cosmo/cosmo.cpp` |
| 行数 | 844 + 726 = 1570 行 |
| 职责 | C 语言平面 API，使用不透明句柄封装 C++ 对象 |

**设计模式：不透明句柄 (Opaque Pointer)**
```c
// 前向声明，用户不感知具体类型
typedef struct cosmo_simulation_t cosmo_simulation_t;
typedef struct cosmo_simulation_structure_t cosmo_simulation_structure_t;
typedef struct cosmo_csv_writer_t cosmo_csv_writer_t;

// 内部通过 reinterpret_cast 映射到 C++ 对象
cosmo_simulation_t* handle = reinterpret_cast<cosmo_simulation_t*>(cpp_sim.get());
```

**API 分组：**
| 分组 | 函数数 | 功能 |
|------|--------|------|
| 版本信息 | 1 | cosmo_library_version() |
| 日志系统 | 5 | set_log_level / info / debug / warn / error |
| 仿真结构 | 6 | create / destroy / add_model / make_connection / add_parameter_set |
| 仿真实例 | 8 | create_from_structure / initialize / step / step_until / terminate / destroy / reset |
| 变量访问 | 6 | get/set real/integer/boolean |
| 监听器 | 6 | create / destroy / callbacks / csv_writer |
| 场景 | 5 | create / destroy / add actions |
| 运行器 | 4 | create / destroy / start / stop |
| 参数集 | 4 | create / destroy / add_real / add_integer |
| 错误处理 | 2 | last_error_msg / clear_error |

#### 2.5.2 CLI 工具

| 属性 | 说明 |
|------|------|
| 文件 | `include/cosmo/cli/cli.hpp` + `src/cli/cli.cpp` + `src/cli/cli_main.cpp` |
| 行数 | 67 + 221 + 122 = 410 行 |
| 职责 | 命令行参数解析和仿真执行入口 |

---

## 3. 数据流设计

### 3.1 仿真步进数据流

```
              ┌──────────────┐
              │  simulation  │
              │  .step()     │
              └──────┬───────┘
                     │ 1. notify_pre_step
                     ▼
              ┌──────────────┐
              │  Listeners   │ ← csv_writer, scenario 等
              └──────────────┘
                     │ 2. transfer_connection_data
                     ▼
         ┌───────────────────────┐
         │   Connection Bindings │
         │ source::var → target::var │
         │ (可选 modifier 转换)   │
         └───────────┬───────────┘
                     │ 3. model->step()
                     ▼
    ┌────────┐ ┌────────┐ ┌────────┐
    │ Model A│ │ Model B│ │ Model C│
    │  (FMU) │ │  (FMU) │ │ (Mock) │
    └────────┘ └────────┘ └────────┘
                     │ 4. notify_post_step
                     ▼
              ┌──────────────┐
              │  Listeners   │
              └──────────────┘
```

### 3.2 FMU 加载数据流

```
FMU 文件路径
  ↓ fmu_processor::process()
解压到临时目录 (system("unzip ..."))
  ↓ 读取临时目录/modelDescription.xml
xml_parser::load_file()
  ↓ 解析 XML DOM
model_description (name, GUID, FMI version, variables[])
  ↓ 检测平台二进制
binaries/{platform}/*.so 或 *.dll
  ↓ dynamic_library::load()
已加载的共享库句柄
  ↓ dlsym 获取 FMI 函数
fmi_interface (instantiate, step, terminate, ...)
  ↓ fmi_interface::instantiate()
component handle (FMI 模型实例)
  ↓ 封装为
fmi_model_instance → model_instance
```

---

## 4. 关键设计决策

### 4.1 为什么选择 C++11

- CentOS 7 (GCC 4.8.5) 在工业环境中广泛使用
- 避免引入 C++14/17/20 特性，确保最大兼容性
- C++11 已提供 unique_ptr、lambda、range-for 等足够特性

### 4.2 为什么自研所有基础组件

| 组件 | 被替代的第三方库 | 原因 |
|------|-----------------|------|
| XML 解析器 | pugixml | 消除依赖，pugixml 约 5000 行引入过多代码 |
| 日志系统 | spdlog | spdlog 需要 C++11 并使用大量模板，自研 373 行即可满足 |
| 测试框架 | Catch2 | Catch2 依赖 C++14，自研 97 行轻量框架 |
| CLI 解析 | CLI11 | CLI11 需要 C++14，自研简单解析器即可 |

### 4.3 不透明句柄模式

C API 使用 `typedef struct cosmo_xxx_t cosmo_xxx_t;` 前向声明 + `reinterpret_cast` 实现指针映射：
- 用户只看到不透明指针，无法访问内部结构
- C++ 对象通过 `reinterpret_cast` 双向转换
- 资源管理由 C++ 侧自动完成

### 4.4 仿真结构与仿真实例分离

```
simulation_structure (声明式配置)  →  simulation (可执行实例)
```
用户先通过 `simulation_structure` 声明模型、连接和参数，然后调用 `load()` 一次性创建仿真实例。这种设计：
- 分离配置阶段和执行阶段
- 配置可序列化（未来支持 JSON/XML 导入导出）
- 同一配置可创建多个独立实例

---

## 5. 模块依赖关系

```
                    cli
                     │
              cosmo (C API)
                     │
         ┌───────────┼───────────┐
         │           │           │
    simulation     ssp        remote
         │
    ┌────┼────┐
    │    │    │
  data  fmi  listeners
    │    │
    │  ┌─┴──┐
    │  │    │
    │ fmi1 fmi3
    │
  ┌─┴──┐
  │    │
util  logger
(xml,fs,dynlib)
```

**依赖原则：**
- 只允许上层依赖下层，禁止反向依赖
- 基础工具层 (util) 和日志层 (logger) 无外部依赖
- FMI 层只依赖 util 和 logger
- 仿真层依赖 FMI 层和 data 层
- 应用层依赖仿真层

---

## 6. 目录结构映射

```
cosmo/
├── include/cosmo/           # 公共 API（头文件）
│   ├── cosmo.h              #   C API 声明 (844 行)
│   ├── cli/                 #   CLI 模块
│   ├── data/                #   数据处理模块
│   ├── fmi/                 #   FMI 接口模块 (6 个头文件)
│   ├── listeners/           #   监听器模块
│   ├── logger/              #   日志模块
│   ├── remote/              #   远程执行模块
│   ├── simulation/          #   仿真引擎模块 (9 个头文件)
│   ├── ssp/                 #   SSP 模块
│   ├── test/                #   测试框架
│   └── util/                #   工具模块 (3 个头文件)
├── src/                     # 实现代码
│   ├── cli/                 #   CLI (2 个文件)
│   ├── cosmo/               #   C API 实现 (1 个文件)
│   ├── data/                #   数据 (2 个文件)
│   ├── fmi/                 #   FMI (5 个文件)
│   ├── listeners/           #   监听器 (1 个文件)
│   ├── logger/              #   日志 (1 个文件)
│   ├── remote/              #   远程 (1 个文件)
│   ├── simulation/          #   仿真 (6 个文件)
│   ├── ssp/                 #   SSP (1 个文件)
│   ├── test/                #   测试 (22 个文件)
│   └── util/                #   工具 (3 个文件)
├── examples/                # 示例程序 (3 个文件)
└── docs/                    # 项目文档
```

---

## 7. 扩展点

### 7.1 自定义仿真算法

实现 `cosmo::simulation::algorithm` 接口即可：

```cpp
class my_algorithm : public cosmo::simulation::algorithm {
    fmi::fmi_status initialize() override { /* ... */ }
    fmi::fmi_status step(double t, double dt) override { /* ... */ }
    fmi::fmi_status terminate() override { /* ... */ }
    double get_step_size() const override { /* ... */ }
};
```

### 7.2 自定义监听器

实现 `cosmo::simulation::simulation_listener` 接口：

```cpp
class my_listener : public cosmo::simulation::simulation_listener {
    void on_post_step(double time, double step_size) override {
        // 每步后执行自定义逻辑
    }
};
```

### 7.3 自定义数据转换

通过 lambda 或函数对象：

```cpp
structure->make_connection("model1::x", "model2::y",
    [](double v) { return v * 3.14; });  // 自定义转换
```

---

*本文档版本 0.1.0，最后更新 2026-03-27*
