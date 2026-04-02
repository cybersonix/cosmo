/**
 * @file logger.hpp
 * @brief 日志系统接口定义
 * 
 * 本文件定义了COSMO仿真引擎的日志系统接口。
 * 提供统一的日志记录机制，支持多种日志级别和自定义日志处理器。
 * 
 * @author COSMO Development Team
 * @version 0.1.0
 * @date 2026-03-27
 * 
 * @copyright Copyright (c) 2026
 * 
 * @note 日志系统采用单例模式设计，支持线程安全的全局访问
 */

#ifndef COSMO_LOGGER_LOGGER_HPP
#define COSMO_LOGGER_LOGGER_HPP

#include <string>
#include <memory>

namespace cosmo {
namespace log {

/**
 * @brief 日志级别枚举
 * 
 * 定义日志消息的严重程度级别，从低到高排列。
 * 级别用于控制日志输出的详细程度和过滤。
 */
enum level
{
    trace_level,  ///< 跟踪级别：最详细的调试信息
    debug_level,  ///< 调试级别：开发调试信息
    info_level,   ///< 信息级别：一般运行信息
    warn_level,   ///< 警告级别：潜在问题警告
    err_level,    ///< 错误级别：错误信息
    off_level     ///< 关闭级别：禁用所有日志
};

/**
 * @brief 日志记录器抽象基类
 * 
 * 定义了日志记录器的接口规范。
 * 用户可以继承此类实现自定义的日志处理器（如文件日志、网络日志等）。
 * 
 * @note 所有日志方法都应保证线程安全
 */
class logger
{
public:
    /**
     * @brief 默认构造函数
     */
    logger() = default;
    
    /**
     * @brief 虚析构函数
     * 
     * 确保派生类能够正确析构。
     */
    virtual ~logger() = default;

    /**
     * @brief 记录跟踪级别日志
     * @param msg 日志消息内容
     * 
     * 用于记录最详细的程序执行信息，如函数进入/退出、变量值等。
     */
    virtual void log_trace(const std::string& msg) = 0;
    
    /**
     * @brief 记录调试级别日志
     * @param msg 日志消息内容
     * 
     * 用于记录调试信息，帮助开发者理解程序行为。
     */
    virtual void log_debug(const std::string& msg) = 0;
    
    /**
     * @brief 记录信息级别日志
     * @param msg 日志消息内容
     * 
     * 用于记录正常的程序运行信息。
     */
    virtual void log_info(const std::string& msg) = 0;
    
    /**
     * @brief 记录警告级别日志
     * @param msg 日志消息内容
     * 
     * 用于记录可能的问题或异常情况，但程序可以继续运行。
     */
    virtual void log_warn(const std::string& msg) = 0;
    
    /**
     * @brief 记录错误级别日志
     * @param msg 日志消息内容
     * 
     * 用于记录错误信息，表示发生了需要关注的问题。
     */
    virtual void log_error(const std::string& msg) = 0;

    /**
     * @brief 设置日志级别
     * @param lvl 要设置的日志级别
     * 
     * 设置后，只有等于或高于该级别的日志消息才会被记录。
     */
    virtual void set_level(level lvl) = 0;
    
    /**
     * @brief 获取当前日志级别
     * @return 当前的日志级别
     */
    virtual level get_level() const = 0;
};

/**
 * @brief 获取全局日志记录器实例
 * @return 全局日志记录器的智能指针
 * 
 * 如果尚未设置日志记录器，将返回一个默认的控制台日志记录器。
 * 
 * @note 线程安全：此函数是线程安全的
 */
std::shared_ptr<logger> get_logger();

/**
 * @brief 设置全局日志记录器实例
 * @param log 要设置的日志记录器
 * 
 * 替换当前的全局日志记录器。新的日志记录器将立即生效。
 * 
 * @note 线程安全：此函数是线程安全的
 */
void set_logger(std::shared_ptr<logger> log);

/**
 * @name 便捷日志函数
 * 
 * 这些函数提供对全局日志记录器的便捷访问。
 * 如果全局日志记录器未设置，这些函数将无操作。
 * 
 * @{
 */

/**
 * @brief 记录跟踪级别日志
 * @param msg 日志消息
 */
void trace(const std::string& msg);

/**
 * @brief 记录调试级别日志
 * @param msg 日志消息
 */
void debug(const std::string& msg);

/**
 * @brief 记录信息级别日志
 * @param msg 日志消息
 */
void info(const std::string& msg);

/**
 * @brief 记录警告级别日志
 * @param msg 日志消息
 */
void warn(const std::string& msg);

/**
 * @brief 记录错误级别日志
 * @param msg 日志消息
 */
void error(const std::string& msg);

/** @} */

/**
 * @brief 设置全局日志级别
 * @param lvl 要设置的日志级别
 * 
 * 便捷函数，直接设置全局日志记录器的级别。
 */
void set_logging_level(level lvl);

/**
 * @brief 获取全局日志级别
 * @return 当前的日志级别
 * 
 * 便捷函数，直接获取全局日志记录器的级别。
 */
level get_logging_level();

} // namespace log
} // namespace cosmo

/**
 * @name C兼容日志函数
 * 
 * 提供C语言兼容的日志函数接口，便于与其他语言绑定。
 * 
 * @{
 */
extern "C" {
    /**
     * @brief C接口：记录跟踪级别日志
     * @param msg 日志消息（C字符串）
     */
    void cosmo_trace(const char* msg);
    
    /**
     * @brief C接口：记录调试级别日志
     * @param msg 日志消息（C字符串）
     */
    void cosmo_debug(const char* msg);
    
    /**
     * @brief C接口：记录信息级别日志
     * @param msg 日志消息（C字符串）
     */
    void cosmo_info(const char* msg);
    
    /**
     * @brief C接口：记录警告级别日志
     * @param msg 日志消息（C字符串）
     */
    void cosmo_warn(const char* msg);
    
    /**
     * @brief C接口：记录错误级别日志
     * @param msg 日志消息（C字符串）
     */
    void cosmo_error(const char* msg);
}
/** @} */

#endif // COSMO_LOGGER_LOGGER_HPP
