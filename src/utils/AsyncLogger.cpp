#include "Logger.h"
#include <vector>
#include <thread>

// 示例类
class UserManager {
public:
    void login(const std::string& username, int userId) {
        LOG_INFO("User login - Username: ", username, ", ID: ", userId);
       
        if (userId <= 0) {
            LOG_ERROR("Invalid user ID: ", userId);
            return;
        }
       
        LOG_DEBUG("Login process completed for user: ", username);
    }
   
    void processUsers() {
        std::vector<std::string> users = {"Alice", "Bob", "Charlie"};
       
        for (size_t i = 0; i < users.size(); ++i) {
            LOG_INFO("Processing user [", i, "]: ", users[i]);
        }
       
        LOG_WARN("User list processing completed");
    }
};

class DatabaseConnector {
public:
    bool connect(const std::string& host, int port) {
        LOG_INFO("Attempting to connect to database: ", host, ":", port);
       
        if (host.empty()) {
            LOG_ERROR("Database host cannot be empty");
            return false;
        }
       
        // 模拟连接失败
        if (port == 0) {
            LOG_ERROR("Connection failed - invalid port: ", port);
            return false;
        }
       
        LOG_INFO("Database connection established successfully");
        return true;
    }
};

// 多线程测试函数
void threadFunction(int threadId) {
    for (int i = 0; i < 3; ++i) {
        LOG_INFO("Thread ", threadId, " - iteration ", i);
    }
}

int main() {
    // 设置日志级别（可以在程序启动时配置）
    Logger::getInstance().setLevel(LogLevel::DEBUG);
   
    LOG_INFO("=== Application Started ===");
    LOG_DEBUG("Debug level logging is enabled");
   
    // 测试基本日志功能
    UserManager userManager;
    userManager.login("john_doe", 1001);
    userManager.login("invalid_user", -1);
    userManager.processUsers();
   
    DatabaseConnector dbConnector;
    dbConnector.connect("localhost", 5432);
    dbConnector.connect("", 0);  // 这会触发错误日志
   
    // 测试变量和复杂表达式
    int successCount = 5;
    int failureCount = 2;
    double successRate = static_cast<double>(successCount) / (successCount + failureCount) * 100;
   
    LOG_INFO("Operation completed - Success: ", successCount,
             ", Failures: ", failureCount,
             ", Success Rate: ", successRate, "%");
   
    // 多线程测试
    LOG_INFO("=== Starting Multi-thread Test ===");
    std::vector<std::thread> threads;
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back(threadFunction, i + 1);
    }
   
    for (auto& thread : threads) {
        thread.join();
    }
   
    // 动态调整日志级别
    LOG_INFO("Changing log level to WARNING");
    Logger::getInstance().setLevel(LogLevel::WARNING);
   
    LOG_DEBUG("This debug message should NOT appear");  // 不会输出
    LOG_INFO("This info message should NOT appear");    // 不会输出
    LOG_WARN("This warning message WILL appear");       // 会输出
    LOG_ERROR("This error message WILL appear");        // 会输出
   
    LOG_INFO("=== Application Finished ===");
   
    return 0;
}