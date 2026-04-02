/**
 * @file xml_parser.hpp
 * @brief 轻量级XML解析器
 * 
 * 本文件提供了一个不依赖第三方库的轻量级XML解析器。
 * 支持DOM风格的XML解析、生成和基本查询操作。
 * 
 * @author COSMO Development Team
 * @version 0.1.0
 * @date 2026-03-27
 * 
 * @copyright Copyright (c) 2026
 * 
 * @note 此解析器专用于解析FMI modelDescription.xml和SSP配置文件
 * @warning 不支持完整的XML标准（如DTD、命名空间、CDATA等高级特性）
 */

#ifndef COSMO_UTIL_XML_PARSER_HPP
#define COSMO_UTIL_XML_PARSER_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <fstream>
#include <sstream>

namespace cosmo {
namespace util {

/**
 * @brief XML属性映射类型
 * 
 * 存储元素属性的键值对，键和值均为字符串。
 */
using xml_attributes = std::map<std::string, std::string>;

/**
 * @brief XML节点类
 * 
 * 表示XML文档中的一个元素节点。
 * 包含节点名称、文本内容、属性和子节点列表。
 */
class xml_node
{
public:
    /**
     * @brief 构造函数
     * @param name 节点名称
     */
    xml_node(const std::string& name = "");
    
    /**
     * @brief 默认析构函数
     */
    ~xml_node() = default;

    /** @name 节点名称访问 @{ */
    
    /**
     * @brief 获取节点名称
     * @return 节点名称字符串
     */
    const std::string& get_name() const;
    
    /**
     * @brief 设置节点名称
     * @param name 新的节点名称
     */
    void set_name(const std::string& name);
    
    /** @} */

    /** @name 节点文本内容访问 @{ */
    
    /**
     * @brief 获取节点文本内容
     * @return 节点内的文本内容
     */
    const std::string& get_text() const;
    
    /**
     * @brief 设置节点文本内容
     * @param text 文本内容
     */
    void set_text(const std::string& text);
    
    /** @} */

    /** @name 属性操作 @{ */
    
    /**
     * @brief 检查是否存在指定属性
     * @param name 属性名称
     * @return 存在返回true，否则返回false
     */
    bool has_attribute(const std::string& name) const;
    
    /**
     * @brief 获取属性值
     * @param name 属性名称
     * @param default_value 默认值（属性不存在时返回）
     * @return 属性值或默认值
     */
    std::string get_attribute(const std::string& name, const std::string& default_value = "") const;
    
    /**
     * @brief 设置属性值
     * @param name 属性名称
     * @param value 属性值
     */
    void set_attribute(const std::string& name, const std::string& value);
    
    /**
     * @brief 获取所有属性
     * @return 属性映射的const引用
     */
    const xml_attributes& get_attributes() const;
    
    /** @} */

    /** @name 子节点操作 @{ */
    
    /**
     * @brief 添加子节点
     * @param child 子节点指针
     * 
     * 将子节点添加到当前节点的子节点列表末尾。
     */
    void add_child(std::shared_ptr<xml_node> child);
    
    /**
     * @brief 获取指定名称的第一个子节点
     * @param name 子节点名称
     * @return 子节点指针，未找到返回nullptr
     */
    std::shared_ptr<xml_node> get_child(const std::string& name) const;
    
    /**
     * @brief 获取指定名称的所有子节点
     * @param name 子节点名称（为空字符串返回所有子节点）
     * @return 子节点指针列表
     */
    std::vector<std::shared_ptr<xml_node>> get_children(const std::string& name = "") const;
    
    /**
     * @brief 获取子节点数量
     * @return 子节点数量
     */
    size_t child_count() const;
    
    /** @} */

    /** @name 节点查找 @{ */
    
    /**
     * @brief 查找节点（深度优先）
     * @param name 要查找的节点名称
     * @return 找到的节点指针，未找到返回nullptr
     * 
     * 在当前节点的子树中深度优先查找指定名称的节点。
     * 返回第一个匹配的节点。
     */
    std::shared_ptr<xml_node> find_node(const std::string& name) const;
    
    /**
     * @brief 查找所有节点（深度优先）
     * @param name 要查找的节点名称
     * @return 所有匹配的节点指针列表
     * 
     * 在当前节点的子树中查找所有指定名称的节点。
     */
    std::vector<std::shared_ptr<xml_node>> find_all_nodes(const std::string& name) const;
    
    /** @} */

    /**
     * @brief 转换为XML字符串
     * @param indent 缩进级别（用于格式化）
     * @return XML格式的字符串
     */
    std::string to_string(int indent = 0) const;

private:
    std::string name_;                              ///< 节点名称
    std::string text_;                              ///< 节点文本内容
    xml_attributes attributes_;                     ///< 节点属性映射
    std::vector<std::shared_ptr<xml_node>> children_;  ///< 子节点列表
};

/**
 * @brief XML文档类
 * 
 * 表示一个完整的XML文档。
 * 提供文档级别的解析、加载、保存和序列化功能。
 */
class xml_document
{
public:
    /**
     * @brief 默认构造函数
     * 
     * 创建一个空的XML文档。
     */
    xml_document();
    
    /**
     * @brief 默认析构函数
     */
    ~xml_document() = default;

    /**
     * @brief 从文件加载XML文档
     * @param filename 文件路径
     * @return 加载成功返回true，失败返回false
     * 
     * 从指定路径加载XML文件并解析。
     * 失败时可通过get_error()获取错误信息。
     */
    bool load_file(const std::string& filename);

    /**
     * @brief 从字符串加载XML文档
     * @param content XML内容字符串
     * @return 加载成功返回true，失败返回false
     * 
     * 解析字符串中的XML内容。
     */
    bool load_string(const std::string& content);

    /**
     * @brief 获取根节点
     * @return 根节点指针，文档为空返回nullptr
     */
    std::shared_ptr<xml_node> get_root() const;

    /**
     * @brief 设置根节点
     * @param root 根节点指针
     */
    void set_root(std::shared_ptr<xml_node> root);

    /**
     * @brief 保存到文件
     * @param filename 文件路径
     * @return 保存成功返回true，失败返回false
     */
    bool save_file(const std::string& filename) const;

    /**
     * @brief 转换为XML字符串
     * @return XML格式的字符串
     * 
     * 包含XML声明和完整的文档内容。
     */
    std::string to_string() const;

    /**
     * @brief 获取错误信息
     * @return 错误描述字符串
     * 
     * 获取最近一次操作失败的错误信息。
     */
    const std::string& get_error() const;

private:
    std::shared_ptr<xml_node> root_;  ///< 根节点
    std::string error_;               ///< 错误信息

    /**
     * @name 内部解析方法
     * 
     * 这些方法是XML解析器的内部实现细节。
     * 
     * @{
     */
    
    /**
     * @brief 解析XML内容
     * @param content XML字符串
     * @return 解析成功返回true
     */
    bool parse(const std::string& content);
    
    /**
     * @brief 解析节点
     * @param content XML字符串
     * @param pos 当前解析位置
     * @param parent 父节点
     * @return 解析后的新位置，失败返回std::string::npos
     */
    size_t parse_node(const std::string& content, size_t pos, std::shared_ptr<xml_node> parent);
    
    /**
     * @brief 解析标签
     * @param content XML字符串
     * @param pos 当前位置
     * @param tag_name 输出参数，标签名称
     * @param attributes 输出参数，属性映射
     * @param is_closing 输出参数，是否为结束标签
     * @param is_self_closing 输出参数，是否为自闭合标签
     * @return 解析后的新位置
     */
    size_t parse_tag(const std::string& content, size_t pos, std::string& tag_name, 
                     xml_attributes& attributes, bool& is_closing, bool& is_self_closing);
    
    /**
     * @brief 解析属性
     * @param content XML字符串
     * @param pos 当前位置
     * @param attributes 输出参数，属性映射
     * @return 解析后的新位置
     */
    size_t parse_attributes(const std::string& content, size_t pos, xml_attributes& attributes);
    
    /**
     * @brief 解析文本内容
     * @param content XML字符串
     * @param start 起始位置
     * @param end 结束位置
     * @return 解码后的文本
     */
    std::string parse_text(const std::string& content, size_t start, size_t end);
    
    /**
     * @brief 去除字符串两端空白
     * @param str 输入字符串
     * @return 处理后的字符串
     */
    std::string trim(const std::string& str);
    
    /**
     * @brief 解码XML实体
     * @param str 包含实体的字符串
     * @return 解码后的字符串
     * 
     * 解码 &lt; &gt; &amp; &quot; &apos; 等预定义实体。
     */
    std::string decode_entities(const std::string& str);
    
    /**
     * @brief 编码XML实体
     * @param str 原始字符串
     * @return 编码后的字符串
     * 
     * 将特殊字符编码为XML实体。
     */
    std::string encode_entities(const std::string& str);
    
    /** @} */
};

/**
 * @brief 创建XML文档实例（工厂函数）
 * @return 新的XML文档实例
 */
std::unique_ptr<xml_document> create_xml_document();

/**
 * @brief 编码XML特殊字符
 * @param str 原始字符串
 * @return 编码后的字符串
 */
std::string encode_xml_entities(const std::string& str);

} // namespace util
} // namespace cosmo

#endif // COSMO_UTIL_XML_PARSER_HPP
