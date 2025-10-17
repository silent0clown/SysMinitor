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
   
    // Private constructor to prevent external instantiation
    Logger() {
        initializeLogFile();
    }
    
    ~Logger() {
        if (logFile_ && logFile_->is_open()) {
            logFile_->close();
        }
    }
   
    // Disable copy and assignment
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
   
    /**
     * @brief Initialize log file
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
            
            // Write log file header
            *logFile_ << "=== Log Started at " << getCurrentTime() << " ===" << std::endl;
            logFile_->flush();
            
        } catch (const std::exception& e) {
            std::cerr << "Log initialization failed: " << e.what() << std::endl;
        }
    }
    
    /**
     * @brief Generate log file path
     * @return Complete log file path
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
     * @brief Check and perform log rotation
     */
    void rotateLogIfNeeded() {
        if (!logFile_ || !logFile_->is_open()) return;
        
        try {
            // Get current file size
            auto currentPos = logFile_->tellp();
            if (currentPos > static_cast<std::streampos>(maxFileSize_)) {
                // Close current file
                logFile_->close();
                
                // Create new log file
                initializeLogFile();
                
                // Clean up old files
                cleanupOldFiles();
            }
        } catch (const std::exception& e) {
            std::cerr << "Log rotation failed: " << e.what() << std::endl;
        }
    }
    
    /**
     * @brief Clean up old log files
     */
    void cleanupOldFiles() {
        try {
            std::vector<std::filesystem::path> files;
            
            // Collect all relevant log files
            for (const auto& entry : std::filesystem::directory_iterator(logDir_)) {
                if (entry.is_regular_file() && 
                    entry.path().extension() == ".log" &&
                    entry.path().string().find(logBaseName_) != std::string::npos) {
                    files.push_back(entry.path());
                }
            }
            
            // If file count exceeds limit, sort by time and delete oldest
            if (files.size() > static_cast<size_t>(maxFiles_)) {
                // Sort by modification time (oldest first)
                std::sort(files.begin(), files.end(),
                         [](const auto& a, const auto& b) {
                             return std::filesystem::last_write_time(a) < 
                                    std::filesystem::last_write_time(b);
                         });
                
                // Delete oldest files until count limit is satisfied
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
     * @brief Convert log level to string
     * @param level Log level
     * @return Corresponding string representation
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
     * @brief Get current time as string
     * @return Time string
     */
    std::string getCurrentTime() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::tm tm;
        // Use safe localtime_s instead of localtime
        localtime_s(&tm, &time_t);
        
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        oss << "." << std::setfill('0') << std::setw(3) << ms.count();
        return oss.str();
    }
   
    /**
     * @brief Extract file name (remove path)
     * @param filePath Complete file path
     * @return File name
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
     * @brief Get singleton instance
     * @return Logger singleton reference
     */
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }
   
    /**
     * @brief Set log level
     * @param level Log level to set
     */
    void setLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(logMutex_);
        currentLevel_ = level;
    }
   
    /**
     * @brief Get current log level
     * @return Current log level
     */
    LogLevel getLevel() const {
        return currentLevel_;
    }
    
    /**
     * @brief Set log directory
     * @param dir Log directory path
     */
    void setLogDirectory(const std::string& dir) {
        std::lock_guard<std::mutex> lock(logMutex_);
        logDir_ = dir;
        initializeLogFile();
    }
    
    /**
     * @brief Set log file base name
     * @param baseName Base name
     */
    void setLogBaseName(const std::string& baseName) {
        std::lock_guard<std::mutex> lock(logMutex_);
        logBaseName_ = baseName;
        initializeLogFile();
    }
    
    /**
     * @brief Set maximum file size
     * @param size File size in bytes
     */
    void setMaxFileSize(size_t size) {
        std::lock_guard<std::mutex> lock(logMutex_);
        maxFileSize_ = size;
    }
    
    /**
     * @brief Set maximum number of files
     * @param count File count
     */
    void setMaxFiles(int count) {
        std::lock_guard<std::mutex> lock(logMutex_);
        maxFiles_ = count;
    }
    
    /**
     * @brief Get current log file path
     * @return Current log file path
     */
    std::string getCurrentLogFilePath() const {
        return currentLogFilePath_;
    }
    
    /**
     * @brief Immediately flush log buffer
     */
    void flush() {
        std::lock_guard<std::mutex> lock(logMutex_);
        if (logFile_ && logFile_->is_open()) {
            logFile_->flush();
        }
    }
   
    /**
     * @brief Core logging method
     * @tparam Args Variadic parameter types
     * @param level Log level
     * @param file Source file name
     * @param line Source code line number
     * @param args Log content parameters
     */
    template<typename... Args>
    void log(LogLevel level, const char* file, int line, Args&&... args) {
        // Check if log level meets output condition
        if (level < currentLevel_) return;
       
        // Concatenate parameters into string
        std::ostringstream stream;
        (stream << ... << std::forward<Args>(args));
        std::string logMessage = stream.str();
       
        std::lock_guard<std::mutex> lock(logMutex_);
        
        // Format log information
        std::string formattedLog = "[" + getCurrentTime() + "]"
                                 + "[" + levelToString(level) + "]"
                                 + "[" + extractFileName(file) + ":" + std::to_string(line) + "] "
                                 + logMessage;
        
        // Output to console
        std::cout << formattedLog << std::endl;
        
        // Output to file (persistent)
        if (logFile_ && logFile_->is_open()) {
            *logFile_ << formattedLog << std::endl;
            logFile_->flush(); // Immediately flush buffer to ensure log is written to disk
            
            // Check if log rotation is needed
            rotateLogIfNeeded();
        }
    }
};

// Log macros - maintain existing usage pattern
#define LOG_DEBUG(...)  Logger::getInstance().log(LogLevel::DEBUG,   __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)   Logger::getInstance().log(LogLevel::INFO,    __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)   Logger::getInstance().log(LogLevel::WARNING, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...)  Logger::getInstance().log(LogLevel::E_ERROR,   __FILE__, __LINE__, __VA_ARGS__)

} // namespace snapshot