# COSMO 开发指南

## 目录

1. [开发环境搭建](#开发环境搭建)
2. [代码规范](#代码规范)
3. [架构设计](#架构设计)
4. [模块说明](#模块说明)
5. [测试规范](#测试规范)
6. [调试技巧](#调试技巧)
7. [发布流程](#发布流程)

---

## 开发环境搭建

### 系统要求

- **操作系统**: Linux (CentOS 7+ / Ubuntu 16.04+)
- **编译器**: GCC 4.8+ 或 Clang 3.4+
- **构建工具**: CMake 3.10+
- **版本控制**: Git 2.0+

### 开发工具推荐

| 工具 | 用途 | 推荐配置 |
|------|------|----------|
| VS Code | 代码编辑 | 安装C/C++插件 |
| CLion | IDE | 直接导入CMake项目 |
| GDB | 调试 | 配合IDE使用 |
| Valgrind | 内存检测 | 用于检测内存泄漏 |

### 项目结构

```
cosmo/
├── CMakeLists.txt          # 主构建配置
├── README.md               # 项目说明
├── docs/                   # 文档目录
│   ├── API_REFERENCE.md    # API参考
│   ├── USER_MANUAL.md      # 用户手册
│   ├── DEVELOPMENT_GUIDE.md # 本文件
│   ├── REQUIREMENTS.md     # 需求文档
│   ├── ARCHITECTURE.md     # 架构设计文档
│   └── TEST_REPORT.md      # 测试文档
├── include/cosmo/          # 公共头文件
│   ├── cosmo.h             # C API主头文件
│   ├── cli/                # CLI接口
│   ├── data/               # 数据处理
│   ├── fmi/                # FMI接口
│   ├── listeners/          # 监听器
│   ├── logger/             # 日志系统
│   ├── remote/             # 远程执行
│   ├── simulation/         # 仿真核心
│   ├── ssp/                # SSP相关
│   ├── test/               # 测试框架
│   └── util/               # 工具类
├── src/                    # 源文件
│   ├── cli/                # CLI实现
│   ├── cosmo/              # C API实现
│   ├── data/               # 数据处理实现
│   ├── fmi/                # FMI实现
│   ├── listeners/          # 监听器实现
│   ├── logger/             # 日志实现
│   ├── remote/             # 远程执行实现
│   ├── simulation/         # 仿真实现
│   ├── ssp/                # SSP实现
│   ├── test/               # 测试代码
│   └── util/               # 工具实现
├── examples/               # 示例程序
│   ├── basic_simulation.cpp
│   ├── c_api_example.c
│   └── mass_spring_damper_sim.cpp
└── build/                  # 构建输出
```

---

## 代码规范

### 命名规范

| 类型 | 规范 | 示例 |
|------|------|------|
| 类名 | PascalCase | `class SimulationRunner` |
| 函数名 | snake_case | `void run_simulation()` |
| 变量名 | snake_case | `int step_count` |
| 成员变量 | snake_case + 后缀 | `std::string name_` |
| 宏定义 | 全大写 + 下划线 | `#define MAX_SIZE 100` |
| 常量 | k + PascalCase | `const int kMaxIterations` |
| 枚举 | 小写 + 后缀 | `enum class status_code` |

### 文件组织

```cpp
/**
 * @file filename.cpp
 * @brief 简短描述
 * 
 * 详细描述，说明文件的功能和用途。
 * 
 * @copyright Copyright (c) 2024
 */

// 1. 系统头文件
#include <vector>
#include <string>

// 2. 第三方库头文件（本项目无）

// 3. 本项目头文件
#include "cosmo/module/header.hpp"

// 4. 命名空间
namespace cosmo {
namespace module {

// 代码实现

} // namespace module
} // namespace cosmo
```

### 注释规范

**文件头注释**（必须）:
```cpp
/**
 * @file filename.cpp
 * @brief 简短描述
 * 
 * 详细描述...
 * 
 * @author 作者名
 * @version 版本号
 * @date 日期
 * @copyright Copyright (c) 年份
 * 
 * @note 注意事项
 * @warning 警告信息
 */
```

**类注释**:
```cpp
/**
 * @brief 类简要说明
 * 
 * 详细说明类的功能和使用方法。
 * 
 * @tparam T 模板参数说明
 * 
 * @note 使用注意事项
 */
class MyClass {
```

**函数注释**:
```cpp
/**
 * @brief 函数简要说明
 * @param param1 参数1说明
 * @param param2 参数2说明
 * @return 返回值说明
 * @throw 可能抛出的异常
 * 
 * @code
 * // 使用示例
 * auto result = my_function(arg1, arg2);
 * @endcode
 */
```

### 代码风格

**缩进**: 4个空格（不使用Tab）

**大括号**:
```cpp
// 类定义
class MyClass {
public:
    MyClass();
    ~MyClass();
};

// 函数定义
void my_function() {
    if (condition) {
        do_something();
    } else {
        do_other();
    }
}
```

**行长度**: 不超过100字符

**空行**:
- 函数之间：1个空行
- 类定义中访问修饰符前后：1个空行
- 逻辑代码块之间：适当空行

---

## 架构设计

### 整体架构

```
┌─────────────────────────────────────────────────────────────┐
│                        应用层 (CLI/API)                      │
├─────────────────────────────────────────────────────────────┤
│                      仿真管理层 (Simulation)                 │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │   Runner     │  │   Scenario   │  │   Listener   │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
├─────────────────────────────────────────────────────────────┤
│                      数据处理层 (Data)                       │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │    Router    │  │   Channel    │  │  TimeSeries  │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
├─────────────────────────────────────────────────────────────┤
│                      FMI接口层 (FMI)                         │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │ FMI 1.0  │  │ FMI 2.0  │  │ FMI 3.0  │  │  Parser  │   │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘   │
├─────────────────────────────────────────────────────────────┤
│                      基础工具层 (Util)                       │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │   XML    │  │   File   │  │   Log    │  │   DLL    │   │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### 设计原则

1. **单一职责**: 每个类只负责一个功能
2. **开闭原则**: 对扩展开放，对修改关闭
3. **依赖倒置**: 依赖抽象接口而非具体实现
4. **无第三方依赖**: 仅使用C++标准库

---

## 模块说明

### FMI模块 (cosmo/fmi)

**职责**: 封装FMI标准接口

**主要类**:
- `fmi_interface` - FMI 2.0主接口
- `fmi1_interface` - FMI 1.0接口
- `fmi3_interface` - FMI 3.0接口
- `model_description` - 模型描述解析
- `fmu_processor` - FMU文件处理

**关键设计**:
```cpp
// 工厂模式创建接口
auto fmi2 = cosmo::fmi::create_fmi_interface();
auto fmi1 = cosmo::fmi::fmi1::create_fmi1_interface();
auto fmi3 = cosmo::fmi::fmi3::create_fmi3_interface();
```

### 仿真模块 (cosmo/simulation)

**职责**: 仿真引擎核心

**主要类**:
- `simulation` - 仿真主控
- `simulation_runner` - 运行控制
- `scenario` - 场景定义
- `model_instance` - 模型实例

**生命周期**:
```
create -> initialize -> [step]* -> terminate -> destroy
```

### 数据模块 (cosmo/data)

**职责**: 数据路由和处理

**主要类**:
- `data_router` - 数据路由器
- `data_channel` - 数据传输通道
- `time_series` - 时间序列数据

### 工具模块 (cosmo/util)

**职责**: 基础工具类

**主要类**:
- `xml_parser` - XML解析器
- `dynamic_library` - 动态库加载
- `filesystem` - 文件系统工具

---

## 测试规范

### 测试框架

使用自定义轻量级测试框架：

```cpp
#include "cosmo/test/test_framework.hpp"

TEST_CASE(my_test) {
    REQUIRE(condition);      // 基本条件检查
    REQUIRE_EQ(a, b);        // 相等检查
    REQUIRE_FALSE(condition); // 取反条件检查
}

int main() {
    return RUN_ALL_TESTS();
}
```

### 测试覆盖率要求

| 模块 | 覆盖率目标 | 当前状态 |
|------|-----------|----------|
| FMI接口 | 90%+ | 85% |
| 仿真核心 | 90%+ | 90% |
| 数据处理 | 90%+ | 95% |
| 工具类 | 80%+ | 80% |

### 测试类型

1. **单元测试**: 测试单个函数/类
2. **集成测试**: 测试模块间协作
3. **边界测试**: 测试边界条件
4. **性能测试**: 测试性能指标

### 测试文件命名

```
<module>_test.cpp    # 单元测试
integration_test.cpp # 集成测试
boundary_test.cpp    # 边界测试
```

---

## 调试技巧

### 日志调试

```cpp
// 设置日志级别
cosmo::log::set_logging_level(cosmo::log::level::debug);

// 输出日志
cosmo::log::debug("Debug info: " + std::to_string(value));
cosmo::log::info("Normal info");
cosmo::log::error("Error occurred");
```

### GDB调试

```bash
# 编译调试版本
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# 启动GDB
gdb ./bin/my_test

# 常用命令
(gdb) break main          # 设置断点
(gdb) run                 # 运行
(gdb) next                # 单步执行
(gdb) print variable      # 打印变量
(gdb) backtrace           # 查看调用栈
```

### 内存检测

```bash
# 使用Valgrind检测内存泄漏
valgrind --leak-check=full --show-leak-kinds=all ./bin/my_test

# 检测未初始化内存使用
valgrind --track-origins=yes ./bin/my_test
```

---

## 发布流程

### 版本号规则

使用语义化版本: `主版本.次版本.修订号`

- **主版本**: 不兼容的API修改
- **次版本**: 向下兼容的功能添加
- **修订号**: 向下兼容的问题修复

### 发布检查清单

- [ ] 所有测试通过
- [ ] 代码覆盖率达标
- [ ] 文档已更新
- [ ] 版本号已更新
- [ ] 变更日志已更新
- [ ] 无编译警告
- [ ] 内存泄漏检查通过

### 编译发布版本

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4
make test
sudo make install
```

---

## 贡献指南

### 提交规范

**提交信息格式**:
```
<type>: <subject>

<body>

<footer>
```

**类型**:
- `feat`: 新功能
- `fix`: 修复
- `docs`: 文档
- `style`: 格式
- `refactor`: 重构
- `test`: 测试
- `chore`: 构建

**示例**:
```
feat: 添加FMI 3.0支持

实现FMI 3.0接口封装，支持Co-Simulation模式。

Closes #123
```

### 代码审查

- 所有代码必须通过代码审查
- 审查者需检查：功能正确性、代码规范、测试覆盖
- 重大问题需修改后重新审查

---

*本指南使用中文编写，版本 0.1.0*
