#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include <mutex>
#include <memory>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <ctime>

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
    std::unique_ptr<std::ofstream> logFile_;
    std::string logDir_ = "logs";
    std::string logBaseName_ = "app";
    size_t maxFileSize_ = 10 * 1024 * 1024; // 10MB
    int maxFiles_ = 5;
    std::string currentLogFilePath_;
   
    // 私有构造函数，防止外部实例化
    Logger() {
        initializeLogFile();
    }
    
    ~Logger() {
        if (logFile_ && logFile_->is_open()) {
            logFile_->close();
        }
    }
   
    // 禁止拷贝和赋值
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
   
    /**
     * @brief 初始化日志文件
     */
    void initializeLogFile() {
        std::lock_guard<std::mutex> lock(logMutex_);
        
        try {
            std::filesystem::create_directories(logDir_);
            currentLogFilePath_ = generateLogFilePath();
            logFile_ = std::make_unique<std::ofstream>(currentLogFilePath_, std::ios::app);
            
            if (!logFile_->is_open()) {
                std::cerr << "Failed to open log file: " << currentLogFilePath_ << std::endl;
                return;
            }
            
            // 写入日志文件头
            *logFile_ << "=== Log Started at " << getCurrentTime() << " ===" << std::endl;
            logFile_->flush();
            
        } catch (const std::exception& e) {
            std::cerr << "Log initialization failed: " << e.what() << std::endl;
        }
    }
    
    /**
     * @brief 生成日志文件路径
     * @return 完整的日志文件路径
     */
    std::string generateLogFilePath() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&time_t);
        
        std::ostringstream oss;
        oss << logDir_ << "/" << logBaseName_ << "_"
            << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".log";
        return oss.str();
    }
    
    /**
     * @brief 检查并执行日志轮转
     */
    void rotateLogIfNeeded() {
        if (!logFile_ || !logFile_->is_open()) return;
        
        try {
            // 获取当前文件大小
            auto currentPos = logFile_->tellp();
            if (currentPos > static_cast<std::streampos>(maxFileSize_)) {
                // 关闭当前文件
                logFile_->close();
                
                // 创建新日志文件
                initializeLogFile();
                
                // 清理旧文件
                cleanupOldFiles();
            }
        } catch (const std::exception& e) {
            std::cerr << "Log rotation failed: " << e.what() << std::endl;
        }
    }
    
    /**
     * @brief 清理旧的日志文件
     */
    void cleanupOldFiles() {
        try {
            std::vector<std::filesystem::path> files;
            
            // 收集所有相关的日志文件
            for (const auto& entry : std::filesystem::directory_iterator(logDir_)) {
                if (entry.is_regular_file() && 
                    entry.path().extension() == ".log" &&
                    entry.path().string().find(logBaseName_) != std::string::npos) {
                    files.push_back(entry.path());
                }
            }
            
            // 如果文件数量超过限制，按时间排序并删除最旧的
            if (files.size() > static_cast<size_t>(maxFiles_)) {
                // 按修改时间排序（最旧的在前面）
                std::sort(files.begin(), files.end(),
                         [](const auto& a, const auto& b) {
                             return std::filesystem::last_write_time(a) < 
                                    std::filesystem::last_write_time(b);
                         });
                
                // 删除最旧的文件，直到满足数量限制
                while (files.size() > static_cast<size_t>(maxFiles_)) {
                    std::filesystem::remove(files.front());
                    files.erase(files.begin());
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Log cleanup failed: " << e.what() << std::endl;
        }
    }
   
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
     * @brief 获取当前时间的字符串表示
     * @return 时间字符串
     */
    std::string getCurrentTime() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::tm tm;
        // 使用安全的 localtime_s 替代 localtime
        localtime_s(&tm, &time_t);
        
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        oss << "." << std::setfill('0') << std::setw(3) << ms.count();
        return oss.str();
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
     * @brief 设置日志目录
     * @param dir 日志目录路径
     */
    void setLogDirectory(const std::string& dir) {
        std::lock_guard<std::mutex> lock(logMutex_);
        logDir_ = dir;
        initializeLogFile();
    }
    
    /**
     * @brief 设置日志文件基础名称
     * @param baseName 基础名称
     */
    void setLogBaseName(const std::string& baseName) {
        std::lock_guard<std::mutex> lock(logMutex_);
        logBaseName_ = baseName;
        initializeLogFile();
    }
    
    /**
     * @brief 设置最大文件大小
     * @param size 文件大小（字节）
     */
    void setMaxFileSize(size_t size) {
        std::lock_guard<std::mutex> lock(logMutex_);
        maxFileSize_ = size;
    }
    
    /**
     * @brief 设置最大文件数量
     * @param count 文件数量
     */
    void setMaxFiles(int count) {
        std::lock_guard<std::mutex> lock(logMutex_);
        maxFiles_ = count;
    }
    
    /**
     * @brief 获取当前日志文件路径
     * @return 当前日志文件路径
     */
    std::string getCurrentLogFilePath() const {
        return currentLogFilePath_;
    }
    
    /**
     * @brief 立即刷新日志缓冲区
     */
    void flush() {
        std::lock_guard<std::mutex> lock(logMutex_);
        if (logFile_ && logFile_->is_open()) {
            logFile_->flush();
        }
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
        std::string logMessage = stream.str();
       
        std::lock_guard<std::mutex> lock(logMutex_);
        
        // 格式化日志信息
        std::string formattedLog = "[" + getCurrentTime() + "]"
                                 + "[" + levelToString(level) + "]"
                                 + "[" + extractFileName(file) + ":" + std::to_string(line) + "] "
                                 + logMessage;
        
        // 输出到控制台
        std::cout << formattedLog << std::endl;
        
        // 输出到文件（持久化）
        if (logFile_ && logFile_->is_open()) {
            *logFile_ << formattedLog << std::endl;
            logFile_->flush(); // 立即刷新缓冲区，确保日志写入磁盘
            
            // 检查是否需要轮转日志
            rotateLogIfNeeded();
        }
    }
};

// 日志宏定义 - 保持现有使用方式不变
#define LOG_DEBUG(...)  Logger::getInstance().log(LogLevel::DEBUG,   __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)   Logger::getInstance().log(LogLevel::INFO,    __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)   Logger::getInstance().log(LogLevel::WARNING, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...)  Logger::getInstance().log(LogLevel::E_ERROR,   __FILE__, __LINE__, __VA_ARGS__)

} // namespace snapshot
