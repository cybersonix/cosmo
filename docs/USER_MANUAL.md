# COSMO 用户手册

## 目录

1. [简介](#简介)
2. [安装](#安装)
3. [快速入门](#快速入门)
4. [核心概念](#核心概念)
5. [使用指南](#使用指南)
6. [高级功能](#高级功能)
7. [故障排除](#故障排除)

---

## 简介

COSMO (Collaborative Open-Source Simulation & Modeling) 是一个开源的多领域联合仿真平台，支持FMI (Functional Mock-up Interface) 标准，能够连接多个FMU (Functional Mock-up Unit) 进行协同仿真。

### 主要特性

- **多FMI版本支持**: 同时支持FMI 1.0、2.0和3.0
- **灵活的仿真算法**: 固定步长仿真算法，支持自定义算法扩展
- **实时仿真**: 支持实时模式和非实时模式
- **数据传输**: 支持模型间变量连接和数据转换
- **场景仿真**: 支持时间事件驱动的场景定义
- **数据记录**: 支持CSV格式数据导出
- **SSP支持**: 支持系统结构和参数化 (SSP) 标准

### 系统要求

- **操作系统**: Linux (CentOS 7+ / Ubuntu 16.04+)
- **编译器**: GCC 4.8+ 或 Clang 3.4+
- **C++标准**: C++11
- **依赖**: 无第三方库依赖

---

## 安装

### 从源码编译

```bash
# 克隆仓库
git clone https://github.com/example/cosmo.git
cd cosmo

# 创建构建目录
mkdir build && cd build

# 配置
cmake ..

# 编译
make -j4

# 安装 (可选)
sudo make install
```

### 编译选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `COSMO_BUILD_TESTS` | ON | 构建测试程序 |
| `COSMO_BUILD_EXAMPLES` | ON | 构建示例程序 |
| `COSMO_BUILD_SHARED` | OFF | 构建共享库 |

---

## 快速入门

### 第一个COSMO程序

```cpp
#include "cosmo/cosmo.h"
#include <stdio.h>

int main() {
    // 设置日志级别
    cosmo_set_log_level("info");
    
    // 打印版本信息
    cosmo_version v = cosmo_library_version();
    printf("COSMO Version: %d.%d.%d\n", v.major, v.minor, v.patch);
    
    // 创建仿真结构
    cosmo_simulation_structure_t* structure = cosmo_simulation_structure_create();
    
    // 添加模型
    cosmo_simulation_structure_add_model(structure, "model1", "path/to/model.fmu");
    
    // 创建连接
    cosmo_simulation_structure_make_real_connection(structure, 
        "model1::output", "model2::input");
    
    // 创建仿真
    cosmo_simulation_t* sim = cosmo_simulation_create_from_structure(structure, 0.01);
    
    // 初始化和运行
    cosmo_simulation_initialize(sim, 0.0);
    cosmo_simulation_step_until(sim, 10.0);
    cosmo_simulation_terminate(sim);
    
    // 清理
    cosmo_simulation_destroy(sim);
    cosmo_simulation_structure_destroy(structure);
    
    return 0;
}
```

### 编译运行

```bash
gcc -o my_sim my_sim.c -lcosmo -I/usr/local/include -L/usr/local/lib
./my_sim
```

---

## 核心概念

### FMU (Functional Mock-up Unit)

FMU是根据FMI标准封装的模型单元，包含：
- 模型描述文件 (modelDescription.xml)
- 动态库文件 (.so/.dll)
- 资源文件

### 仿真结构 (Simulation Structure)

仿真结构定义了：
- 参与的模型实例
- 模型间的连接关系
- 参数设置

### 数据传输

支持多种数据传输模式：
- **直接传输**: 1:1数值传递
- **线性转换**: y = kx + b
- **比例缩放**: y = kx
- **自定义函数**: 用户定义转换逻辑

---

## 使用指南

### 1. 创建仿真结构

```cpp
#include "cosmo/simulation/simulation_structure.hpp"

auto structure = cosmo::simulation::create_simulation_structure();

// 添加模型
structure->add_model("car", "models/car.fmu");
structure->add_model("engine", "models/engine.fmu");

// 设置参数
std::map<std::string, cosmo::scalar_value> params;
params["mass"] = 1500.0;
structure->add_model("car", "models/car.fmu", params);
```

### 2. 定义变量连接

```cpp
// 简单连接
structure->make_connection("car::speed", "engine::input_speed");

// 带转换的连接
auto modifier = [](double value) { return value * 0.27778; }; // km/h to m/s
structure->make_connection("car::speed_kmh", "engine::speed_ms", modifier);
```

### 3. 配置仿真运行器

```cpp
auto sim = structure->load(std::move(algorithm));
auto runner = cosmo::simulation::create_simulation_runner(sim.get());

// 实时模式
runner->set_real_time_mode(true);
runner->set_real_time_factor(1.0);  // 1倍速

// 设置停止时间
runner->set_stop_time(60.0);  // 运行60秒
```

### 4. 使用场景定义

```cpp
cosmo::simulation::scenario sc;

// 在1秒时设置参数
sc.add_real_action(1.0, "car::throttle", 0.5);

// 在5秒时改变参数
sc.add_real_action(5.0, "car::throttle", 0.8);

// 添加布尔动作
sc.add_boolean_action(10.0, "engine::ignition", false);

// 添加仿真监听器
sim->add_listener(std::make_shared<cosmo::simulation::scenario>(sc));
```

### 5. 数据记录

```cpp
// 创建CSV配置
cosmo::listeners::csv_config csv_cfg;
csv_cfg.set_downsample_factor(10);  // 每10步记录一次

// 创建CSV写入器
auto csv = cosmo::listeners::create_csv_writer("output.csv", csv_cfg);

// 添加监听器
sim->add_listener(std::move(csv));
```

---

## 高级功能

### 实时仿真

```cpp
// 启用实时模式
runner->set_real_time_mode(true);

// 设置仿真速度
runner->set_real_time_factor(0.5);  // 半速
runner->set_real_time_factor(2.0);  // 2倍速
```

### 数据路由器

```cpp
auto router = cosmo::data::create_data_router();

// 定义端点
cosmo::data::channel_endpoint source("model1", "output", 1);
cosmo::data::channel_endpoint target("model2", "input", 2);

// 添加直接通道
int id = router->add_channel(source, target, cosmo::data::transfer_type::direct);

// 添加线性转换通道
int id2 = router->add_channel(source, target, cosmo::data::transfer_type::linear);
router->get_channel(id2)->set_linear_transform(2.0, 0.0);  // y = 2x

// 自定义转换
int id3 = router->add_channel(source, target, cosmo::data::transfer_type::custom);
router->get_channel(id3)->set_custom_transform([](double x) {
    return std::max(0.0, x);  // 饱和到0
});
```

### SSP文件加载

```cpp
auto loader = cosmo::ssp::create_ssp_loader();
auto ssp_desc = loader->load("system.ssp");

if (ssp_desc) {
    // 从SSP创建仿真结构
    auto structure = ssp_desc->create_simulation_structure();
    auto sim = structure->load(std::move(algorithm));
}
```

---

## 故障排除

### 常见问题

#### 1. FMU加载失败

**症状**: `Failed to load FMU: xxx`

**解决**:
- 检查FMU文件路径是否正确
- 验证FMU文件是否完整（可用unzip测试）
- 检查动态库架构是否与系统匹配

#### 2. 变量连接失败

**症状**: `Variable not found: xxx`

**解决**:
- 检查变量名称拼写
- 确认变量在modelDescription.xml中存在
- 验证变量类型是否匹配

#### 3. 实时模式卡顿

**症状**: 仿真运行不流畅

**解决**:
- 降低实时因子
- 减小仿真步长
- 关闭不必要的数据记录

### 调试技巧

```cpp
// 启用详细日志
cosmo::log::set_logging_level(cosmo::log::level::trace);

// 添加调试监听器
class debug_listener : public cosmo::simulation::simulation_listener {
public:
    void on_pre_step(double time, double step) override {
        cosmo::log::debug("Step at t=" + std::to_string(time));
    }
};

sim->add_listener(std::make_shared<debug_listener>());
```

---

## 附录

### 术语表

| 术语 | 说明 |
|------|------|
| FMI | Functional Mock-up Interface，功能样机接口标准 |
| FMU | Functional Mock-up Unit，功能样机单元 |
| SSP | System Structure and Parameterization，系统结构与参数化标准 |
| Co-simulation | 联合仿真，多个模型协同运行 |

### 相关资源

- FMI标准: https://fmi-standard.org/
- SSP标准: https://ssp-standard.org/
- COSMO文档: https://cosmo.readthedocs.io/

---

*本手册使用中文编写，版本 0.1.0*
