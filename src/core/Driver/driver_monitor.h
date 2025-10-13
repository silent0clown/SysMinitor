#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <string>
#include <vector>
#include <chrono>
#include <windows.h>
#include "../../utils/encode.h"
struct DriverVersion {
    std::string fileVersion;        // 文件版本
    std::string productVersion;     // 产品版本
    std::string companyName;        // 公司名称
    std::string fileDescription;    // 文件描述
    std::string legalCopyright;     // 版权信息
    std::string originalFilename;   // 原始文件名
};

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
    std::string driverType;             // 驱动类型
    std::string hardwareClass;          // 硬件类别
    DWORD pid;                          // 进程ID
    std::string exitCode;               // 退出代码
    std::string win32ExitCode;          // Win32退出代码
    std::string serviceSpecificExitCode;// 服务特定退出代码
    DriverVersion version;              // 版本信息
    std::chrono::system_clock::time_point installTime;
    
    DriverDetail() : pid(0) {}
    
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
        hardwareClass = util::EncodingUtil::SafeString(hardwareClass);
        exitCode = util::EncodingUtil::SafeString(exitCode);
        win32ExitCode = util::EncodingUtil::SafeString(win32ExitCode);
        serviceSpecificExitCode = util::EncodingUtil::SafeString(serviceSpecificExitCode);
        
        // 清理版本信息
        version.fileVersion = util::EncodingUtil::SafeString(version.fileVersion);
        version.productVersion = util::EncodingUtil::SafeString(version.productVersion);
        version.companyName = util::EncodingUtil::SafeString(version.companyName);
        version.fileDescription = util::EncodingUtil::SafeString(version.fileDescription);
        version.legalCopyright = util::EncodingUtil::SafeString(version.legalCopyright);
        version.originalFilename = util::EncodingUtil::SafeString(version.originalFilename);
        
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
    std::vector<DriverDetail> hardwareDrivers;      // 硬件驱动
    std::vector<DriverDetail> runningDrivers;       // 运行中驱动
    std::vector<DriverDetail> stoppedDrivers;       // 已停止驱动
    std::vector<DriverDetail> autoStartDrivers;     // 自动启动驱动
    std::vector<DriverDetail> thirdPartyDrivers;    // 第三方驱动
    
    // 按硬件类别分类
    std::vector<DriverDetail> displayDrivers;       // 显示适配器驱动
    std::vector<DriverDetail> audioDrivers;         // 音频驱动
    std::vector<DriverDetail> networkDrivers;       // 网络适配器驱动
    std::vector<DriverDetail> inputDrivers;         // 输入设备驱动
    std::vector<DriverDetail> storageDrivers;       // 存储控制器驱动
    std::vector<DriverDetail> printerDrivers;       // 打印机驱动
    std::vector<DriverDetail> usbDrivers;           // USB驱动
    std::vector<DriverDetail> bluetoothDrivers;     // 蓝牙驱动
    
    struct {
        size_t totalDrivers;
        size_t runningCount;
        size_t stoppedCount;
        size_t kernelCount;
        size_t fileSystemCount;
        size_t hardwareCount;
        size_t autoStartCount;
        size_t thirdPartyCount;
        // 硬件驱动统计
        size_t displayCount;
        size_t audioCount;
        size_t networkCount;
        size_t inputCount;
        size_t storageCount;
        size_t printerCount;
        size_t usbCount;
        size_t bluetoothCount;
    } stats;
    
    DriverSnapshot() : timestamp(0) {
        stats.totalDrivers = 0;
        stats.runningCount = 0;
        stats.stoppedCount = 0;
        stats.kernelCount = 0;
        stats.fileSystemCount = 0;
        stats.hardwareCount = 0;
        stats.autoStartCount = 0;
        stats.thirdPartyCount = 0;
        stats.displayCount = 0;
        stats.audioCount = 0;
        stats.networkCount = 0;
        stats.inputCount = 0;
        stats.storageCount = 0;
        stats.printerCount = 0;
        stats.usbCount = 0;
        stats.bluetoothCount = 0;
    }
};

class DriverMonitor {
public:
    DriverMonitor();
    ~DriverMonitor();
    
    DriverSnapshot GetDriverSnapshot();
    DriverDetail GetDriverDetail(const std::string& driverName);
    bool IsDriverRunning(const std::string& driverName);

private:
    std::vector<DriverDetail> GetAllDriversViaSCM();
    std::vector<DriverDetail> GetHardwareDriversViaSetupAPI();
    std::vector<DriverDetail> GetDriversViaWMI();
    
    // 版本信息获取
    DriverVersion GetDriverVersionInfo(const std::string& filePath);
    std::string GetFileVersion(const std::string& filePath);
    
    // 硬件驱动分类
    void CategorizeHardwareDrivers(DriverSnapshot& snapshot, const std::vector<DriverDetail>& hardwareDrivers);
    void CategorizeDrivers(DriverSnapshot& snapshot, const std::vector<DriverDetail>& allDrivers);
    void CalculateStatistics(DriverSnapshot& snapshot);
    void SanitizeAllDrivers(DriverSnapshot& snapshot);
    
    // 工具函数
    bool IsHardwareDriver(const DriverDetail& driver);
    std::string DetectHardwareClass(const DriverDetail& driver);
};