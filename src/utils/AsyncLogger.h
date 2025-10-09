#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include <mutex>
#include <memory>
// #include <spdlog/spdlog.h>
// #include <spdlog/async.h>
// #include <spdlog/sinks/basic_file_sink.h>
// #include <spdlog/sinks/stdout_color_sinks.h>

namespace snapshot {

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    E_ERROR
};

class Logger {
private:
    std::mutex logMutex_;
    LogLevel currentLevel_ = LogLevel::INFO;
   
    // 私有构造函数，防止外部实例化
    Logger() = default;
    ~Logger() = default;
   
    // 禁止拷贝和赋值
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
   
    /**
     * @brief 将日志级别转换为字符串
     * @param level 日志级别
     * @return 对应的字符串表示
     */
    const char* levelToString(LogLevel level) {
        switch(level) {
            case LogLevel::DEBUG:   return "DEBUG";
            case LogLevel::INFO:    return "INFO";
            case LogLevel::WARNING: return "WARN";
            case LogLevel::E_ERROR:   return "ERROR";
            default:                return "UNKNOWN";
        }
    }
   
    /**
     * @brief 获取当前时间的字符串表示（简单版本）
     * @return 时间字符串
     */
    std::string getCurrentTime() {
        // 在实际项目中，可以使用 std::chrono 或第三方库来获取更精确的时间
        time_t now = time(nullptr);
        char timeStr[64];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
        return std::string(timeStr);
    }
   
    /**
     * @brief 提取文件名（去除路径）
     * @param filePath 完整文件路径
     * @return 文件名
     */
    const char* extractFileName(const char* filePath) {
        const char* fileName = filePath;
        for (const char* p = filePath; *p != '\0'; ++p) {
            if (*p == '/' || *p == '\\') {
                fileName = p + 1;
            }
        }
        return fileName;
    }

public:
    /**
     * @brief 获取单例实例
     * @return Logger单例引用
     */
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }
   
    /**
     * @brief 设置日志级别
     * @param level 要设置的日志级别
     */
    void setLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(logMutex_);
        currentLevel_ = level;
    }
   
    /**
     * @brief 获取当前日志级别
     * @return 当前日志级别
     */
    LogLevel getLevel() const {
        return currentLevel_;
    }
   
    /**
     * @brief 核心日志方法
     * @tparam Args 可变参数类型
     * @param level 日志级别
     * @param file 源文件名
     * @param line 源代码行号
     * @param args 日志内容参数
     */
    template<typename... Args>
    void log(LogLevel level, const char* file, int line, Args&&... args) {
        // 检查日志级别是否满足输出条件
        if (level < currentLevel_) return;
       
        // 将参数拼接成字符串
        std::ostringstream stream;
        (stream << ... << std::forward<Args>(args));
       
        std::lock_guard<std::mutex> lock(logMutex_);
       
        // 输出格式化的日志信息
        std::cout << "[" << getCurrentTime() << "]"
                  << "[" << levelToString(level) << "]"
                  << "[" << extractFileName(file) << ":" << line << "] "
                  << stream.str() << std::endl;
    }
};

// 日志宏定义 - 这是关键，确保捕获调用者的文件位置
#define LOG_DEBUG(...)  Logger::getInstance().log(LogLevel::DEBUG,   __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)   Logger::getInstance().log(LogLevel::INFO,    __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)   Logger::getInstance().log(LogLevel::WARNING, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...)  Logger::getInstance().log(LogLevel::E_ERROR,   __FILE__, __LINE__, __VA_ARGS__)



} // namespace snapshot