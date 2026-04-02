/**
 * @file model_description.hpp
 * @brief FMU模型描述解析
 *
 * 本文件定义了FMI modelDescription.xml文件的解析结果存储结构。
 * 用于表示FMU模型的元数据信息，包括变量定义、模型属性等。
 *
 * @author COSMO Development Team
 * @version 0.1.0
 * @date 2026-03-27
 *
 * @copyright Copyright (c) 2026
 *
 * @note 对应FMI标准中的ModelDescription元素
 */

#ifndef COSMO_FMI_MODEL_DESCRIPTION_HPP
#define COSMO_FMI_MODEL_DESCRIPTION_HPP

#include "cosmo/fmi/fmi_types.hpp"

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace cosmo {
namespace fmi {

/**
 * @brief 变量描述结构体
 *
 * 描述FMU模型中的一个变量（标量变量）。
 * 包含变量的名称、类型、属性等信息。
 *
 * @note 对应FMI标准中的ScalarVariable元素
 */
struct variable_description
{
    std::string name;                ///< 变量名称
    value_reference vr;              ///< 值引用（唯一标识符）
    variable_type type;              ///< 变量数据类型
    causality causality_type;        ///< 因果关系
    variability variability_type;    ///< 变异性
    std::string description;         ///< 变量描述

    // 默认值
    fmi_real default_real;           ///< 实数默认值
    fmi_integer default_integer;     ///< 整数默认值
    fmi_boolean default_boolean;     ///< 布尔默认值
    fmi_string default_string;       ///< 字符串默认值

    /**
     * @brief 默认构造函数
     *
     * 初始化所有成员为默认值。
     */
    variable_description()
        : vr(0)
        , type(variable_type::real)
        , causality_type(causality::local)
        , variability_type(variability::continuous)
        , default_real(0.0)
        , default_integer(0)
        , default_boolean(false)
    {}
};

/**
 * @brief 模型描述类
 *
 * 表示FMU模型的完整描述信息。
 * 从modelDescription.xml文件解析得到。
 *
 * @note 此类是解析modelDescription.xml的结果容器
 */
class model_description
{
private:
    std::string model_name;                                   ///< 模型名称
    std::string guid;                                         ///< 全局唯一标识符
    fmi_version version;                                      ///< FMI版本
    std::string description;                                  ///< 模型描述

    std::vector<variable_description> variables;              ///< 变量列表
    std::map<std::string, value_reference> variable_name_to_vr; ///< 名称到值引用的映射

public:
    /**
     * @brief 默认构造函数
     */
    model_description();

    /** @name Getter方法 @{ */

    /**
     * @brief 获取模型名称
     * @return 模型名称
     */
    const std::string& get_model_name() const { return model_name; }

    /**
     * @brief 获取GUID
     * @return 全局唯一标识符
     */
    const std::string& get_guid() const { return guid; }

    /**
     * @brief 获取FMI版本
     * @return FMI版本枚举
     */
    fmi_version get_version() const { return version; }

    /**
     * @brief 获取模型描述
     * @return 描述字符串
     */
    const std::string& get_description() const { return description; }

    /**
     * @brief 获取所有变量
     * @return 变量描述列表的const引用
     */
    const std::vector<variable_description>& get_variables() const { return variables; }

    /** @} */

    /** @name Setter方法 @{ */

    /**
     * @brief 设置模型名称
     * @param name 模型名称
     */
    void set_model_name(const std::string& name) { model_name = name; }

    /**
     * @brief 设置GUID
     * @param g GUID字符串
     */
    void set_guid(const std::string& g) { guid = g; }

    /**
     * @brief 设置FMI版本
     * @param v 版本枚举
     */
    void set_version(fmi_version v) { version = v; }

    /**
     * @brief 设置模型描述
     * @param desc 描述字符串
     */
    void set_description(const std::string& desc) { description = desc; }

    /** @} */

    /** @name 变量管理 @{ */

    /**
     * @brief 添加变量
     * @param var 变量描述
     *
     * 将变量添加到模型描述中，并建立名称到值引用的映射。
     */
    void add_variable(const variable_description& var);

    /**
     * @brief 根据名称获取变量
     * @param name 变量名称
     * @return 变量描述指针，未找到返回nullptr
     */
    const variable_description* get_variable_by_name(const std::string& name) const;

    /**
     * @brief 根据值引用获取变量
     * @param vr 值引用
     * @return 变量描述指针，未找到返回nullptr
     */
    const variable_description* get_variable_by_vr(value_reference vr) const;

    /**
     * @brief 根据名称获取值引用
     * @param name 变量名称
     * @return 值引用，未找到返回-1
     */
    value_reference get_vr_by_name(const std::string& name) const;

    /**
     * @brief 根据值引用获取名称
     * @param vr 值引用
     * @return 变量名称，未找到返回空字符串
     */
    std::string get_name_by_vr(value_reference vr) const;

    /**
     * @brief 根据因果关系获取变量
     * @param c 因果关系
     * @return 符合条件的变量指针列表
     */
    std::vector<const variable_description*> get_variables_by_causality(causality c) const;

    /**
     * @brief 根据类型获取变量
     * @param t 变量类型
     * @return 符合条件的变量指针列表
     */
    std::vector<const variable_description*> get_variables_by_type(variable_type t) const;

    /** @} */
};

/**
 * @brief 创建模型描述实例（工厂函数）
 * @return 新的模型描述实例
 */
std::unique_ptr<model_description> create_model_description();

} // namespace fmi
} // namespace cosmo

#endif // COSMO_FMI_MODEL_DESCRIPTION_HPP
