/**
 * @file fmi_types.hpp
 * @brief FMI（Functional Mock-up Interface）标准类型定义
 * 
 * 本文件定义了FMI 1.0/2.0/3.0标准所需的所有基础类型、枚举和常量。
 * 所有类型定义均遵循FMI标准规范，确保与符合FMI标准的模型兼容。
 * 
 * @author COSMO Development Team
 * @version 0.1.0
 * @date 2026-03-27
 * 
 * @copyright Copyright (c) 2026
 * 
 * @note 本文件是COSMO共仿真引擎的核心类型定义文件
 */

#ifndef COSMO_FMI_FMI_TYPES_HPP
#define COSMO_FMI_FMI_TYPES_HPP

#include <string>
#include <vector>
#include <cstdint>

namespace cosmo {
namespace fmi {

/**
 * @brief FMI版本枚举
 * 
 * 标识FMI标准的版本号，用于区分不同版本的FMU模型。
 */
enum class fmi_version
{
    fmi_1_0,  ///< FMI 1.0版本（历史版本，基本功能）
    fmi_2_0,  ///< FMI 2.0版本（当前主流，支持共仿真和模型交换）
    fmi_3_0   ///< FMI 3.0版本（最新标准，支持更丰富的数据类型）
};

/**
 * @brief 变量类型枚举
 * 
 * 定义FMU模型中支持的变量数据类型。
 * 这些类型对应于FMI标准中的标量变量类型。
 */
enum class variable_type
{
    real,         ///< 双精度浮点数（64位）
    integer,      ///< 32位有符号整数
    boolean,      ///< 布尔值（true/false）
    string,       ///< 字符串类型
    enumeration,  ///< 枚举类型（基于整数的命名常量）
    binary        ///< 二进制数据（字节数组，FMI 3.0新增）
};

/**
 * @brief 变量因果关系枚举
 * 
 * 定义变量在FMU模型中的因果关系（causality）。
 * 因果关系描述了变量如何与模型外部交互。
 */
enum class causality
{
    parameter,            ///< 参数：仿真前设置，仿真期间不变
    calculated_parameter, ///< 计算参数：在初始化阶段计算的参数
    input,                ///< 输入：从外部接收值的变量
    output,               ///< 输出：向外部提供值的变量
    local,                ///< 局部变量：仅在模型内部使用
    independent           ///< 独立变量：通常是时间变量
};

/**
 * @brief 变量变异性枚举
 * 
 * 定义变量在仿真过程中的变化特性（variability）。
 * 变异性描述了变量何时可以改变其值。
 */
enum class variability
{
    constant,   ///< 常量：值永不改变
    fixed,      ///< 固定：在初始化阶段设置后不变
    tunable,    ///< 可调：在初始化或事件时可能改变
    discrete,   ///< 离散：仅在离散事件时改变
    continuous  ///< 连续：可以连续变化（仅适用于实数变量）
};

/**
 * @name FMI基础类型定义
 * 
 * 这些类型别名提供了FMI标准的跨平台类型定义。
 * 使用类型别名而非原始类型有助于代码的可移植性和可读性。
 * 
 * @{
 */
using value_reference = unsigned int;            ///< 值引用（value reference），变量的唯一标识符
using fmi_status = int;                          ///< 状态码类型
using fmi_real = double;                         ///< FMI实数类型（64位浮点）
using fmi_integer = int;                         ///< FMI整数类型（32位有符号）
using fmi_boolean = bool;                        ///< FMI布尔类型
using fmi_string = std::string;                  ///< FMI字符串类型
using fmi_binary = std::vector<unsigned char>;   ///< FMI二进制类型（字节数组）
/** @} */

/**
 * @name FMI状态常量
 * 
 * 定义FMI函数返回的状态码。
 * 这些常量对应于FMI标准中的fmiStatus/fmi2Status枚举。
 * 
 * @{
 */
const int fmi_ok = 0;       ///< 操作成功完成
const int fmi_warning = 1;  ///< 操作完成但有警告
const int fmi_discard = 2;  ///< 操作被丢弃（步长被拒绝等）
const int fmi_error = 3;    ///< 发生可恢复错误
const int fmi_fatal = 4;    ///< 发生不可恢复错误
const int fmi_pending = 5;  ///< 操作异步执行中（FMI 2.0+）
/** @} */

/**
 * @brief FMI函数调用约定宏
 * 
 * 根据平台定义适当的函数调用约定。
 * Windows平台使用stdcall调用约定，其他平台使用默认约定。
 */
#ifdef _WIN32
#define FMI_FUNCTION_PREFIX __stdcall
#else
#define FMI_FUNCTION_PREFIX
#endif

} // namespace fmi
} // namespace cosmo

#endif // COSMO_FMI_FMI_TYPES_HPP
