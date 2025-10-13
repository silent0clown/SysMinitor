#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <string>
#include <vector>
#include <chrono>
#include <windows.h>
#include "../../utils/encode.h"

struct DriverDetail {
    std::string name;                   // 驱动名称
    std::string displayName;            // 显示名称
    std::string description;            // 描述
    std::string state;                  // 状态
    std::string startType;              // 启动类型
    std::string binaryPath;             // 驱动文件路径
    std::string serviceType;            // 服务类型
    std::string errorControl;           // 错误控制
    std::string account;                // 运行账户
    std::string group;                  // 加载组
    std::string tagId;                  // 标签ID
    std::string driverType;             // 驱动类型 (Kernel/FileSystem)
    DWORD pid;                          // 进程ID（如果运行中）
    std::string exitCode;               // 退出代码
    std::string win32ExitCode;          // Win32退出代码
    std::string serviceSpecificExitCode;// 服务特定退出代码
    std::chrono::system_clock::time_point installTime; // 安装时间
    
    // 转换为UTF-8编码
    DriverDetail& convertToUTF8() {
        name = util::EncodingUtil::SafeString(name);
        displayName = util::EncodingUtil::SafeString(displayName);
        description = util::EncodingUtil::SafeString(description);
        state = util::EncodingUtil::SafeString(state);
        startType = util::EncodingUtil::SafeString(startType);
        binaryPath = util::EncodingUtil::SafeString(binaryPath);
        serviceType = util::EncodingUtil::SafeString(serviceType);
        errorControl = util::EncodingUtil::SafeString(errorControl);
        account = util::EncodingUtil::SafeString(account);
        group = util::EncodingUtil::SafeString(group);
        tagId = util::EncodingUtil::SafeString(tagId);
        driverType = util::EncodingUtil::SafeString(driverType);
        exitCode = util::EncodingUtil::SafeString(exitCode);
        win32ExitCode = util::EncodingUtil::SafeString(win32ExitCode);
        serviceSpecificExitCode = util::EncodingUtil::SafeString(serviceSpecificExitCode);
        return *this;
    }
    
    bool operator==(const DriverDetail& other) const {
        return name == other.name;
    }
};

struct DriverSnapshot {
    uint64_t timestamp;
    std::vector<DriverDetail> kernelDrivers;        // 内核驱动
    std::vector<DriverDetail> fileSystemDrivers;    // 文件系统驱动
    std::vector<DriverDetail> runningDrivers;       // 正在运行的驱动
    std::vector<DriverDetail> stoppedDrivers;       // 已停止的驱动
    std::vector<DriverDetail> autoStartDrivers;     // 自动启动的驱动
    std::vector<DriverDetail> thirdPartyDrivers;    // 第三方驱动
    
    // 统计信息
    struct {
        size_t totalDrivers;
        size_t runningCount;
        size_t stoppedCount;
        size_t kernelCount;
        size_t fileSystemCount;
        size_t autoStartCount;
        size_t thirdPartyCount;
    } stats;
};

class DriverMonitor {
public:
    DriverMonitor();
    ~DriverMonitor();
    
    /**
     * @brief 获取驱动快照
     * @return 完整的驱动快照
     */
    DriverSnapshot GetDriverSnapshot();
    
    /**
     * @brief 获取特定驱动的详细信息
     * @param driverName 驱动名称
     * @return 驱动详细信息
     */
    DriverDetail GetDriverDetail(const std::string& driverName);
    
    /**
     * @brief 检查驱动是否正在运行
     * @param driverName 驱动名称
     * @return true-正在运行
     */
    bool IsDriverRunning(const std::string& driverName);

private:
    /**
     * @brief 通过SCM获取所有驱动
     */
    std::vector<DriverDetail> GetAllDriversViaSCM();
    
    /**
     * @brief 获取内核驱动
     */
    std::vector<DriverDetail> GetKernelDrivers();
    
    /**
     * @brief 获取文件系统驱动
     */
    std::vector<DriverDetail> GetFileSystemDrivers();
    
    /**
     * @brief 获取正在运行的驱动
     */
    std::vector<DriverDetail> GetRunningDrivers();
    
    /**
     * @brief 获取已停止的驱动
     */
    std::vector<DriverDetail> GetStoppedDrivers();
    
    /**
     * @brief 获取自动启动的驱动
     */
    std::vector<DriverDetail> GetAutoStartDrivers();
    
    /**
     * @brief 获取第三方驱动
     */
    std::vector<DriverDetail> GetThirdPartyDrivers();
    
    /**
     * @brief 分类驱动
     */
    void CategorizeDrivers(DriverSnapshot& snapshot, const std::vector<DriverDetail>& allDrivers);
    
    /**
     * @brief 计算统计信息
     */
    void CalculateStatistics(DriverSnapshot& snapshot);
    
    /**
     * @brief 清理所有驱动的编码
     */
    void SanitizeAllDrivers(DriverSnapshot& snapshot);
};