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
    double usagePercentage;  // ����ʹ����%
    uint64_t responseTime;   // ��Ӧʱ��(ms)
};

struct DiskSMARTData {
    std::string deviceId;
    int temperature;         // �¶ȡ�C
    int healthStatus;        // ����״̬ 0-100
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

    // ��ȡ���̿���
    DiskSnapshot GetDiskSnapshot();
    
    // ��ȡ������������Ϣ
    std::vector<DiskDriveInfo> GetDiskDrives();
    
    // ��ȡ������Ϣ
    std::vector<PartitionInfo> GetPartitions();
    
    // ��ȡ������������
    std::vector<DiskPerformance> GetDiskPerformance();
    
    // ��ȡSMART���� (��Ҫ����ԱȨ��)
    std::vector<DiskSMARTData> GetSMARTData();
    
    // ����ض�������ʵʱIO
    bool StartIOMonitoring(const std::string& driveLetter);
    void StopIOMonitoring();

private:
    bool Initialize();
    void Cleanup();
    
    // ʹ��WMI��ȡ������Ϣ
    std::vector<DiskDriveInfo> GetDrivesViaWMI();
    std::vector<PartitionInfo> GetPartitionsViaWMI();
    
    // ʹ��Windows API��ȡ������Ϣ
    std::vector<DiskDriveInfo> GetDrivesViaAPI();
    std::vector<PartitionInfo> GetPartitionsViaAPI();
    
    // ʹ��Performance Data Helper (PDH) ��ȡ��������
    std::vector<DiskPerformance> GetPerformanceViaPDH();
    std::vector<DiskPerformance> GetPhysicalDiskPerformance();
    
    // ʹ��DeviceIoControl��ȡSMART����
    DiskSMARTData GetSMARTDataForDrive(const std::string& deviceId);
    
    // IO������
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