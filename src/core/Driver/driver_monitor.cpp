#include "driver_monitor.h"
#include <windows.h>
#include <winsvc.h>
#include <iostream>
#include <algorithm>

#pragma comment(lib, "advapi32.lib")

DriverMonitor::DriverMonitor() {
    // 初始化代码
}

DriverMonitor::~DriverMonitor() {
    // 清理代码
}

DriverSnapshot DriverMonitor::GetDriverSnapshot() {
    DriverSnapshot snapshot;
    snapshot.timestamp = GetTickCount64();
    
    try {
        // 获取所有驱动
        auto allDrivers = GetAllDriversViaSCM();
        
        // 分类驱动
        CategorizeDrivers(snapshot, allDrivers);
        
        // 计算统计信息
        CalculateStatistics(snapshot);
        
        // 编码清理
        SanitizeAllDrivers(snapshot);
        
        std::cout << "获取驱动快照成功 - "
                  << "内核驱动: " << snapshot.kernelDrivers.size() << " 个, "
                  << "文件系统驱动: " << snapshot.fileSystemDrivers.size() << " 个, "
                  << "运行中: " << snapshot.runningDrivers.size() << " 个" << std::endl;
                  
    } catch (const std::exception& e) {
        std::cerr << "获取驱动快照时发生异常: " << e.what() << std::endl;
    }
    
    return snapshot;
}

std::vector<DriverDetail> DriverMonitor::GetAllDriversViaSCM() {
    std::vector<DriverDetail> drivers;
    
    SC_HANDLE scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);
    if (!scm) {
        throw std::runtime_error("无法打开服务控制管理器");
    }
    
    DWORD bufferSize = 0;
    DWORD serviceCount = 0;
    DWORD resumeHandle = 0;
    
    // 获取所需缓冲区大小
    EnumServicesStatusEx(
        scm, 
        SC_ENUM_PROCESS_INFO, 
        SERVICE_DRIVER, 
        SERVICE_STATE_ALL,
        nullptr, 
        0, 
        &bufferSize, 
        &serviceCount, 
        &resumeHandle, 
        nullptr);
    
    if (GetLastError() != ERROR_MORE_DATA) {
        CloseServiceHandle(scm);
        return drivers;
    }
    
    // 分配缓冲区并获取服务信息
    std::vector<BYTE> buffer(bufferSize);
    ENUM_SERVICE_STATUS_PROCESS* services = 
        reinterpret_cast<ENUM_SERVICE_STATUS_PROCESS*>(buffer.data());
    
    if (EnumServicesStatusEx(
        scm, 
        SC_ENUM_PROCESS_INFO, 
        SERVICE_DRIVER, 
        SERVICE_STATE_ALL,
        buffer.data(), 
        bufferSize, 
        &bufferSize, 
        &serviceCount, 
        &resumeHandle, 
        nullptr)) {
        
        for (DWORD i = 0; i < serviceCount; ++i) {
            DriverDetail driver;
            driver.name = services[i].lpServiceName ? services[i].lpServiceName : "";
            driver.displayName = services[i].lpDisplayName ? services[i].lpDisplayName : "";
            
            // 获取详细服务信息
            SC_HANDLE service = OpenService(scm, services[i].lpServiceName, 
                                          SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS);
            if (service) {
                // 获取服务配置
                DWORD configSize = 0;
                QueryServiceConfig(service, nullptr, 0, &configSize);
                if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                    std::vector<BYTE> configBuffer(configSize);
                    QUERY_SERVICE_CONFIG* config = 
                        reinterpret_cast<QUERY_SERVICE_CONFIG*>(configBuffer.data());
                    
                    if (QueryServiceConfig(service, config, configSize, &configSize)) {
                        driver.binaryPath = config->lpBinaryPathName ? config->lpBinaryPathName : "";
                        driver.account = config->lpServiceStartName ? config->lpServiceStartName : "";
                        
                        // 启动类型
                        switch (config->dwStartType) {
                            case SERVICE_BOOT_START: 
                                driver.startType = "Boot";
                                break;
                            case SERVICE_SYSTEM_START: 
                                driver.startType = "System";
                                break;
                            case SERVICE_AUTO_START: 
                                driver.startType = "Auto";
                                break;
                            case SERVICE_DEMAND_START: 
                                driver.startType = "Manual";
                                break;
                            case SERVICE_DISABLED: 
                                driver.startType = "Disabled";
                                break;
                            default: 
                                driver.startType = "Unknown";
                                break;
                        }
                        
                        // 服务类型
                        switch (config->dwServiceType) {
                            case SERVICE_KERNEL_DRIVER:
                                driver.serviceType = "Kernel Driver";
                                driver.driverType = "Kernel";
                                break;
                            case SERVICE_FILE_SYSTEM_DRIVER:
                                driver.serviceType = "File System Driver";
                                driver.driverType = "FileSystem";
                                break;
                            default:
                                driver.serviceType = "Unknown";
                                driver.driverType = "Unknown";
                                break;
                        }
                        
                        // 错误控制
                        switch (config->dwErrorControl) {
                            case SERVICE_ERROR_IGNORE: 
                                driver.errorControl = "Ignore";
                                break;
                            case SERVICE_ERROR_NORMAL: 
                                driver.errorControl = "Normal";
                                break;
                            case SERVICE_ERROR_SEVERE: 
                                driver.errorControl = "Severe";
                                break;
                            case SERVICE_ERROR_CRITICAL: 
                                driver.errorControl = "Critical";
                                break;
                            default: 
                                driver.errorControl = "Unknown";
                                break;
                        }
                    }
                }
                
                // 获取服务状态
                SERVICE_STATUS_PROCESS status;
                DWORD bytesNeeded;
                if (QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, 
                                       reinterpret_cast<LPBYTE>(&status), 
                                       sizeof(status), &bytesNeeded)) {
                    driver.pid = status.dwProcessId;
                    
                    switch (status.dwCurrentState) {
                        case SERVICE_RUNNING: 
                            driver.state = "Running";
                            break;
                        case SERVICE_STOPPED: 
                            driver.state = "Stopped";
                            break;
                        case SERVICE_START_PENDING: 
                            driver.state = "Start Pending";
                            break;
                        case SERVICE_STOP_PENDING: 
                            driver.state = "Stop Pending";
                            break;
                        case SERVICE_PAUSE_PENDING: 
                            driver.state = "Pause Pending";
                            break;
                        case SERVICE_PAUSED: 
                            driver.state = "Paused";
                            break;
                        case SERVICE_CONTINUE_PENDING: 
                            driver.state = "Continue Pending";
                            break;
                        default: 
                            driver.state = "Unknown";
                            break;
                    }
                    
                    driver.win32ExitCode = std::to_string(status.dwWin32ExitCode);
                    driver.serviceSpecificExitCode = std::to_string(status.dwServiceSpecificExitCode);
                    driver.exitCode = std::to_string(status.dwWaitHint);
                }
                
                CloseServiceHandle(service);
            }
            
            drivers.push_back(driver);
        }
    }
    
    CloseServiceHandle(scm);
    return drivers;
}

void DriverMonitor::CategorizeDrivers(DriverSnapshot& snapshot, const std::vector<DriverDetail>& allDrivers) {
    for (const auto& driver : allDrivers) {
        // 按类型分类
        if (driver.driverType == "Kernel") {
            snapshot.kernelDrivers.push_back(driver);
        } else if (driver.driverType == "FileSystem") {
            snapshot.fileSystemDrivers.push_back(driver);
        }
        
        // 按状态分类
        if (driver.state == "Running") {
            snapshot.runningDrivers.push_back(driver);
        } else if (driver.state == "Stopped") {
            snapshot.stoppedDrivers.push_back(driver);
        }
        
        // 按启动类型分类
        if (driver.startType == "Auto" || driver.startType == "Boot" || driver.startType == "System") {
            snapshot.autoStartDrivers.push_back(driver);
        }
        
        // 简单判断第三方驱动（非Microsoft驱动）
        if (driver.binaryPath.find("Microsoft") == std::string::npos && 
            driver.binaryPath.find("Windows") == std::string::npos) {
            snapshot.thirdPartyDrivers.push_back(driver);
        }
    }
}

void DriverMonitor::CalculateStatistics(DriverSnapshot& snapshot) {
    size_t total = snapshot.kernelDrivers.size() + snapshot.fileSystemDrivers.size();
    
    snapshot.stats.totalDrivers = total;
    snapshot.stats.runningCount = snapshot.runningDrivers.size();
    snapshot.stats.stoppedCount = snapshot.stoppedDrivers.size();
    snapshot.stats.kernelCount = snapshot.kernelDrivers.size();
    snapshot.stats.fileSystemCount = snapshot.fileSystemDrivers.size();
    snapshot.stats.autoStartCount = snapshot.autoStartDrivers.size();
    snapshot.stats.thirdPartyCount = snapshot.thirdPartyDrivers.size();
}

void DriverMonitor::SanitizeAllDrivers(DriverSnapshot& snapshot) {
    auto sanitizeVector = [](std::vector<DriverDetail>& drivers) {
        for (auto& driver : drivers) {
            driver.convertToUTF8();
        }
    };
    
    sanitizeVector(snapshot.kernelDrivers);
    sanitizeVector(snapshot.fileSystemDrivers);
    sanitizeVector(snapshot.runningDrivers);
    sanitizeVector(snapshot.stoppedDrivers);
    sanitizeVector(snapshot.autoStartDrivers);
    sanitizeVector(snapshot.thirdPartyDrivers);
}

// 其他方法的实现...
DriverDetail DriverMonitor::GetDriverDetail(const std::string& driverName) {
    auto snapshot = GetDriverSnapshot();
    std::string safeName = util::EncodingUtil::SafeString(driverName);
    
    // 在所有分类中查找驱动
    std::vector<std::vector<DriverDetail>> allCategories = {
        snapshot.kernelDrivers,
        snapshot.fileSystemDrivers,
        snapshot.runningDrivers,
        snapshot.stoppedDrivers,
        snapshot.autoStartDrivers,
        snapshot.thirdPartyDrivers
    };
    
    for (const auto& category : allCategories) {
        auto it = std::find_if(category.begin(), category.end(),
            [&safeName](const DriverDetail& driver) {
                return util::EncodingUtil::SafeString(driver.name) == safeName;
            });
        
        if (it != category.end()) {
            return *it;
        }
    }
    
    throw std::runtime_error("驱动未找到: " + driverName);
}

bool DriverMonitor::IsDriverRunning(const std::string& driverName) {
    try {
        auto driver = GetDriverDetail(driverName);
        return driver.state == "Running";
    } catch (const std::exception&) {
        return false;
    }
}