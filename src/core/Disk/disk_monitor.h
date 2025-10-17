#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

namespace sysmonitor {

struct DiskDriveInfo {
    std::string model;
    std::string serialNumber;
    std::string interfaceType;
    std::string mediaType;  // HDD, SSD
    uint64_t totalSize;
    uint64_t bytesPerSector;
    std::string status;
    std::string deviceId;
};

struct PartitionInfo {
    std::string driveLetter;
    std::string label;
    std::string fileSystem;
    uint64_t totalSize;
    uint64_t freeSpace;
    uint64_t usedSpace;
    double usagePercentage;
    uint32_t serialNumber;
};

struct DiskPerformance {
    std::string driveLetter;
    double readSpeed;        // MB/s
    double writeSpeed;       // MB/s
    uint64_t readBytesPerSec;
    uint64_t writeBytesPerSec;
    uint64_t readCountPerSec;
    uint64_t writeCountPerSec;
    double queueLength;
    double usagePercentage;  // 磁盘使用率%
    uint64_t responseTime;   // 响应时间(ms)
};

struct DiskSMARTData {
    std::string deviceId;
    int temperature;         // 温度°C
    int healthStatus;        // 健康状态 0-100
    uint64_t powerOnHours;
    uint64_t powerOnCount;
    uint64_t badSectors;
    uint64_t readErrorCount;
    uint64_t writeErrorCount;
    std::string overallHealth; // Excellent, Good, Warning, Bad
};

struct DiskSnapshot {
    std::vector<DiskDriveInfo> drives;
    std::vector<PartitionInfo> partitions;
    std::vector<DiskPerformance> performance;
    std::vector<DiskSMARTData> smartData;
    uint64_t timestamp;
};

class DiskMonitor {
public:
    DiskMonitor();
    ~DiskMonitor();

    // 获取磁盘快照
    DiskSnapshot GetDiskSnapshot();
    
    // 获取磁盘驱动器信息
    std::vector<DiskDriveInfo> GetDiskDrives();
    
    // 获取分区信息
    std::vector<PartitionInfo> GetPartitions();
    
    // 获取磁盘性能数据
    std::vector<DiskPerformance> GetDiskPerformance();
    
    // 获取SMART数据 (需要管理员权限)
    std::vector<DiskSMARTData> GetSMARTData();
    
    // 监控特定分区的实时IO
    bool StartIOMonitoring(const std::string& driveLetter);
    void StopIOMonitoring();

private:
    bool Initialize();
    void Cleanup();
    
    // 使用WMI获取磁盘信息
    std::vector<DiskDriveInfo> GetDrivesViaWMI();
    std::vector<PartitionInfo> GetPartitionsViaWMI();
    
    // 使用Windows API获取磁盘信息
    std::vector<DiskDriveInfo> GetDrivesViaAPI();
    std::vector<PartitionInfo> GetPartitionsViaAPI();
    
    // 使用Performance Data Helper (PDH) 获取性能数据
    std::vector<DiskPerformance> GetPerformanceViaPDH();
    std::vector<DiskPerformance> GetPhysicalDiskPerformance();
    
    // 使用DeviceIoControl获取SMART数据
    DiskSMARTData GetSMARTDataForDrive(const std::string& deviceId);
    
    // IO监控相关
    struct IOMonitorData {
        uint64_t lastReadBytes;
        uint64_t lastWriteBytes;
        uint64_t lastReadCount;
        uint64_t lastWriteCount;
        uint64_t lastTimestamp;
    };
    
    std::unordered_map<std::string, IOMonitorData> ioMonitorData_;
    bool isMonitoring_;
};

} // namespace sysmonitor