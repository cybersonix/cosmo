# COSMO API 参考手册

## 概述

COSMO (Collaborative Open-Source Simulation & Modeling) 是一个支持多FMU联合仿真的开源工业软件。本文档提供完整的API参考。

## C API 参考

### 版本信息

#### cosmo_library_version
```c
cosmo_version cosmo_library_version();
```
获取库版本号。

**返回值:**
- `cosmo_version` 结构体，包含 major, minor, patch

**示例:**
```c
cosmo_version v = cosmo_library_version();
printf("Version: %d.%d.%d\n", v.major, v.minor, v.patch);
```

---

### 日志系统

#### cosmo_set_log_level
```c
void cosmo_set_log_level(const char* level);
```
设置日志级别。

**参数:**
- `level`: 日志级别字符串 ("trace", "debug", "info", "warning", "error", "fatal")

#### cosmo_info / cosmo_debug / cosmo_warn / cosmo_error
```c
void cosmo_info(const char* msg);
void cosmo_debug(const char* msg);
void cosmo_warn(const char* msg);
void cosmo_error(const char* msg);
```
输出不同级别的日志信息。

---

### 仿真管理

#### cosmo_simulation_create_from_structure
```c
cosmo_simulation_t* cosmo_simulation_create_from_structure(
    cosmo_simulation_structure_t* ss,
    double step_size
);
```
从仿真结构创建仿真实例。

**参数:**
- `ss`: 仿真结构句柄
- `step_size`: 仿真步长

**返回值:**
- 仿真实例句柄，失败返回NULL

#### cosmo_simulation_initialize
```c
int cosmo_simulation_initialize(cosmo_simulation_t* sim, double start_time);
```
初始化仿真。

**返回值:**
- 0 表示成功，非0表示失败

#### cosmo_simulation_step
```c
int cosmo_simulation_step(cosmo_simulation_t* sim);
```
执行单步仿真。

#### cosmo_simulation_step_until
```c
void cosmo_simulation_step_until(cosmo_simulation_t* sim, double time_point);
```
仿真运行到指定时间点。

#### cosmo_simulation_terminate
```c
int cosmo_simulation_terminate(cosmo_simulation_t* sim);
```
终止仿真。

#### cosmo_simulation_destroy
```c
void cosmo_simulation_destroy(cosmo_simulation_t* sim);
```
销毁仿真实例，释放资源。

---

### 参数集

#### cosmo_parameter_set_create
```c
cosmo_parameter_set_t* cosmo_parameter_set_create();
```
创建参数集。

#### cosmo_parameter_set_add_real
```c
void cosmo_parameter_set_add_real(
    cosmo_parameter_set_t* ps, 
    const char* name, 
    double value
);
```
添加实数参数。

#### cosmo_parameter_set_destroy
```c
void cosmo_parameter_set_destroy(cosmo_parameter_set_t* ps);
```
销毁参数集。

---

## C++ API 参考

### 命名空间

所有C++ API位于 `cosmo` 命名空间及其子命名空间中：
- `cosmo::fmi` - FMI接口
- `cosmo::simulation` - 仿真核心
- `cosmo::data` - 数据处理
- `cosmo::util` - 工具类

---

### FMI接口 (cosmo::fmi)

#### fmi_interface (FMI 2.0)

```cpp
class fmi_interface {
public:
    bool load_library(const std::string& library_path);
    fmi_component instantiate(const std::string& instance_name, ...);
    fmi_status initialize(fmi_component component, ...);
    fmi_status do_step(fmi_component component, ...);
    fmi_status terminate(fmi_component component);
    // ...
};
```

**主要方法:**

| 方法 | 描述 |
|------|------|
| `load_library()` | 加载FMI动态库 |
| `instantiate()` | 创建模型实例 |
| `initialize()` | 初始化模型 |
| `do_step()` | 执行仿真步进 |
| `get_real()` / `set_real()` | 读写实数变量 |
| `get_integer()` / `set_integer()` | 读写整数变量 |

#### model_description

```cpp
class model_description {
public:
    std::string get_model_name() const;
    std::string get_guid() const;
    fmi_version get_version() const;
    const std::vector<variable_description>& get_variables() const;
    // ...
};
```

---

### 仿真核心 (cosmo::simulation)

#### simulation

```cpp
class simulation {
public:
    fmi::fmi_status initialize(double start_time = 0.0);
    fmi::fmi_status step();
    fmi::fmi_status step_until(double time_point);
    fmi::fmi_status step_for(double duration);
    fmi::fmi_status terminate();
    fmi::fmi_status reset();
    
    void add_model_instance(std::unique_ptr<model_instance> instance);
    void add_listener(std::shared_ptr<simulation_listener> listener);
    void remove_listener(const std::string& name);
    
    double get_current_time() const;
    bool is_initialized() const;
    bool is_terminated() const;
};
```

#### simulation_runner

```cpp
class simulation_runner {
public:
    runner_state get_state() const;
    bool start();
    bool stop();
    bool pause();
    bool resume();
    
    void set_real_time_mode(bool enabled);
    void set_real_time_factor(double factor);
    void set_stop_time(double time);
    
    double get_elapsed_time() const;
    std::string get_statistics() const;
};
```

---

### 数据处理 (cosmo::data)

#### data_router

```cpp
class data_router {
public:
    int add_channel(const channel_endpoint& source, 
                    const channel_endpoint& target,
                    transfer_type type = transfer_type::direct);
    bool remove_channel(int channel_id);
    data_channel* get_channel(int channel_id);
    
    void set_linear_transform(int channel_id, double scale, double offset);
    void set_custom_transform(int channel_id, transform_function func);
    
    std::vector<int> get_output_channels(const std::string& model) const;
    std::vector<int> get_input_channels(const std::string& model) const;
};
```

#### time_series

```cpp
class time_series {
public:
    void add_point(double time, double value);
    void add_point(const data_point& point);
    size_t size() const;
    void clear();
    const std::vector<data_point>& get_points() const;
};
```

---

### 工具类 (cosmo::util)

#### xml_document

```cpp
class xml_document {
public:
    bool load_file(const std::string& filename);
    bool load_string(const std::string& content);
    bool save_file(const std::string& filename) const;
    std::string to_string() const;
    
    std::shared_ptr<xml_node> get_root() const;
    void set_root(std::shared_ptr<xml_node> root);
};
```

---

## 错误处理

### C API 错误处理

```c
// 获取最后错误信息
const char* error_msg = cosmo_last_error_msg();

// 清除错误状态
cosmo_clear_error();
```

### C++ API 错误处理

使用日志系统记录错误：

```cpp
cosmo::log::error("Error message");
cosmo::log::warn("Warning message");
cosmo::log::info("Info message");
cosmo::log::debug("Debug message");
cosmo::log::trace("Trace message");

// 设置日志级别
cosmo::log::set_logging_level(cosmo::log::level::debug);
```

---

## 返回值约定

| 返回值 | 含义 |
|--------|------|
| 0 | 成功 |
| 非0 | 失败，可通过错误接口获取详情 |
| nullptr / NULL | 对象创建失败 |

---

## 线程安全

- **日志系统**: 线程安全
- **仿真核心**: 非线程安全，需外部同步
- **数据路由器**: 非线程安全

---

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 0.1.0 | 2026-03-27 | 初始版本，支持FMI 1.0/2.0/3.0 |

---

*本文档使用中文编写，符合Google C++风格规范。*
