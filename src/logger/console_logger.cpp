#include "cosmo/logger/logger.hpp"
#include <iostream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <cstring>

namespace cosmo {
namespace log {

// 全局 logger 互斥锁，保护 g_logger 本身的访问
static std::mutex g_logger_mutex;

class console_logger : public logger
{
public:
    console_logger() : level_(info_level) {}

    void log_trace(const std::string& msg) override
    {
        if (level_ <= trace_level) {
            log("TRACE", msg);
        }
    }

    void log_debug(const std::string& msg) override
    {
        if (level_ <= debug_level) {
            log("DEBUG", msg);
        }
    }

    void log_info(const std::string& msg) override
    {
        if (level_ <= info_level) {
            log("INFO", msg);
        }
    }

    void log_warn(const std::string& msg) override
    {
        if (level_ <= warn_level) {
            log("WARN", msg);
        }
    }

    void log_error(const std::string& msg) override
    {
        if (level_ <= err_level) {
            log("ERROR", msg);
        }
    }

    void set_level(level lvl) override
    {
        level_ = lvl;
    }
    
    level get_level() const override
    {
        return level_;
    }

private:
    level level_;
    std::mutex mutex_;

    void log(const std::string& level_str, const std::string& msg)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::system_clock::now();
        std::time_t time_t = std::chrono::system_clock::to_time_t(now);
        
        // 使用线程安全的 localtime_r 替代 localtime
        std::tm tm_buf;
        std::tm* tm_info = localtime_r(&time_t, &tm_buf);
        if (!tm_info) {
            std::cout << "[" << level_str << "] " << msg << "\n";
            return;
        }
        char buffer[100];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
        
        std::cout << "[" << buffer << "] [" << level_str << "] " << msg << "\n";
    }
};

// Global logger instance
static std::shared_ptr<logger> g_logger = std::make_shared<console_logger>();

std::shared_ptr<logger> get_logger()
{
    std::lock_guard<std::mutex> lock(g_logger_mutex);
    return g_logger;
}

void set_logger(std::shared_ptr<logger> log)
{
    std::lock_guard<std::mutex> lock(g_logger_mutex);
    g_logger = log;
}

void trace(const std::string& msg)
{
    std::lock_guard<std::mutex> lock(g_logger_mutex);
    g_logger->log_trace(msg);
}

void debug(const std::string& msg)
{
    std::lock_guard<std::mutex> lock(g_logger_mutex);
    g_logger->log_debug(msg);
}

void info(const std::string& msg)
{
    std::lock_guard<std::mutex> lock(g_logger_mutex);
    g_logger->log_info(msg);
}

void warn(const std::string& msg)
{
    std::lock_guard<std::mutex> lock(g_logger_mutex);
    g_logger->log_warn(msg);
}

void error(const std::string& msg)
{
    std::lock_guard<std::mutex> lock(g_logger_mutex);
    g_logger->log_error(msg);
}

void set_logging_level(level lvl)
{
    std::lock_guard<std::mutex> lock(g_logger_mutex);
    g_logger->set_level(lvl);
}

level get_logging_level()
{
    std::lock_guard<std::mutex> lock(g_logger_mutex);
    return g_logger->get_level();
}

} // namespace log
} // namespace cosmo
