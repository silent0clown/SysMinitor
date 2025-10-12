#include "disk_monitor.h"
#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <winioctl.h>
#include <setupapi.h>
#include <ntddscsi.h>
#include <comdef.h>
#include <wbemidl.h>
#include <iostream>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "wbemuuid.lib")

namespace sysmonitor {

// 编码转换工具函数
std::string Gb2312ToUtf8(const std::string& gb2312Str) {
    if (gb2312Str.empty()) return "";
    
    int len = MultiByteToWideChar(CP_ACP, 0, gb2312Str.c_str(), -1, NULL, 0);
    if (len == 0) return "";
    
    std::wstring wstr(len, 0);
    MultiByteToWideChar(CP_ACP, 0, gb2312Str.c_str(), -1, &wstr[0], len);
    
    len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    if (len == 0) return "";
    
    std::string utf8Str(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8Str[0], len, NULL, NULL);
    
    if (!utf8Str.empty() && utf8Str.back() == '\0') {
        utf8Str.pop_back();
    }
    
    return utf8Str;
}

std::string SafeString(const char* str) {
    if (str == nullptr || strlen(str) == 0) {
        return "";
    }
    return Gb2312ToUtf8(str);
}

// Helper: convert BSTR to UTF-8 std::string without using _bstr_t/comsupp
static std::string BSTRToUtf8(BSTR bstr) {
    if (!bstr) return std::string();
    const wchar_t* wstr = static_cast<const wchar_t*>(bstr);
    if (!wstr) return std::string();
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (len <= 0) return std::string();
    std::string out(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &out[0], len, NULL, NULL);
    if (!out.empty() && out.back() == '\0') out.pop_back();
    return out;
}

// 过滤非UTF8字符的辅助函数
std::string FilterToValidUtf8(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length();) {
        unsigned char c = str[i];
        
        if (c <= 0x7F) {
            // ASCII字符
            result += c;
            i++;
        } else if ((c & 0xE0) == 0xC0) {
            // 2字节UTF-8
            if (i + 1 < str.length() && (str[i+1] & 0xC0) == 0x80) {
                result += str.substr(i, 2);
                i += 2;
            } else {
                i++; // 跳过无效字符
            }
        } else if ((c & 0xF0) == 0xE0) {
            // 3字节UTF-8
            if (i + 2 < str.length() && 
                (str[i+1] & 0xC0) == 0x80 && 
                (str[i+2] & 0xC0) == 0x80) {
                result += str.substr(i, 3);
                i += 3;
            } else {
                i++; // 跳过无效字符
            }
        } else if ((c & 0xF8) == 0xF0) {
            // 4字节UTF-8
            if (i + 3 < str.length() && 
                (str[i+1] & 0xC0) == 0x80 && 
                (str[i+2] & 0xC0) == 0x80 && 
                (str[i+3] & 0xC0) == 0x80) {
                result += str.substr(i, 4);
                i += 4;
            } else {
                i++; // 跳过无效字符
            }
        } else {
            // 无效的UTF-8序列，跳过
            i++;
        }
    }
    return result;
}

// WMI辅助函数
bool InitializeWMI(IWbemLocator** ppLoc, IWbemServices** ppSvc) {
    HRESULT hres;

    // 初始化COM
    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) {
        return false;
    }

    // 设置COM安全级别
    hres = CoInitializeSecurity(
        NULL, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_NONE,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE, NULL
    );

    // 创建WMI连接
    hres = CoCreateInstance(
        CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)ppLoc
    );

    if (FAILED(hres)) {
        CoUninitialize();
        return false;
    }

    // 连接到WMI
    BSTR ns = SysAllocString(L"ROOT\\CIMV2");
    hres = (*ppLoc)->ConnectServer(ns, NULL, NULL, 0, NULL, 0, 0, ppSvc);
    SysFreeString(ns);

    if (FAILED(hres)) {
        (*ppLoc)->Release();
        CoUninitialize();
        return false;
    }

    // 设置代理安全级别
    hres = CoSetProxyBlanket(
        *ppSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
        NULL, RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE
    );

    if (FAILED(hres)) {
        (*ppSvc)->Release();
        (*ppLoc)->Release();
        CoUninitialize();
        return false;
    }

    return true;
}

void CleanupWMI(IWbemLocator* pLoc, IWbemServices* pSvc) {
    if (pSvc) pSvc->Release();
    if (pLoc) pLoc->Release();
    CoUninitialize();
}

DiskMonitor::DiskMonitor() : isMonitoring_(false) {
    Initialize();
}

DiskMonitor::~DiskMonitor() {
    Cleanup();
}

bool DiskMonitor::Initialize() {
    return true;
}

void DiskMonitor::Cleanup() {
    if (isMonitoring_) {
        StopIOMonitoring();
    }
}

DiskSnapshot DiskMonitor::GetDiskSnapshot() {
    DiskSnapshot snapshot;
    snapshot.timestamp = GetTickCount64();
    
    snapshot.drives = GetDiskDrives();
    snapshot.partitions = GetPartitions();
    snapshot.performance = GetDiskPerformance();
    
    // 尝试获取SMART数据
    try {
        snapshot.smartData = GetSMARTData();
    } catch (const std::exception& e) {
        std::cerr << "获取SMART数据失败: " << e.what() << std::endl;
    }
    
    return snapshot;
}

std::vector<DiskDriveInfo> DiskMonitor::GetDiskDrives() {
    std::vector<DiskDriveInfo> drives;
    
    // 使用Windows API获取基本磁盘信息
    auto drivesAPI = GetDrivesViaAPI();
    drives.insert(drives.end(), drivesAPI.begin(), drivesAPI.end());
    
    // 使用WMI获取更详细的磁盘信息
    try {
        auto drivesWMI = GetDrivesViaWMI();
        // 合并信息
        for (const auto& wmiDrive : drivesWMI) {
            bool found = false;
            for (auto& apiDrive : drives) {
                // 根据设备ID或大小匹配
                if (apiDrive.deviceId == wmiDrive.deviceId || 
                    apiDrive.totalSize == wmiDrive.totalSize) {
                    // 用WMI的详细信息补充API信息
                    if (!wmiDrive.model.empty() && wmiDrive.model != "Unknown") {
                        apiDrive.model = wmiDrive.model;
                    }
                    if (!wmiDrive.serialNumber.empty()) {
                        apiDrive.serialNumber = wmiDrive.serialNumber;
                    }
                    if (!wmiDrive.interfaceType.empty()) {
                        apiDrive.interfaceType = wmiDrive.interfaceType;
                    }
                    if (!wmiDrive.mediaType.empty()) {
                        apiDrive.mediaType = wmiDrive.mediaType;
                    }
                    if (wmiDrive.bytesPerSector > 0) {
                        apiDrive.bytesPerSector = wmiDrive.bytesPerSector;
                    }
                    found = true;
                    break;
                }
            }
            if (!found) {
                drives.push_back(wmiDrive);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "WMI获取磁盘信息失败: " << e.what() << std::endl;
    }
    
    std::cout << "获取到 " << drives.size() << " 个磁盘驱动器" << std::endl;
    
    return drives;
}

std::vector<DiskDriveInfo> DiskMonitor::GetDrivesViaAPI() {
    std::vector<DiskDriveInfo> drives;
    
    DWORD driveMask = GetLogicalDrives();
    if (driveMask == 0) {
        std::cerr << "GetLogicalDrives 失败" << std::endl;
        return drives;
    }
    
    for (char driveLetter = 'A'; driveLetter <= 'Z'; driveLetter++) {
        if (driveMask & (1 << (driveLetter - 'A'))) {
            std::string rootPath = std::string(1, driveLetter) + ":\\";
            UINT driveType = GetDriveTypeA(rootPath.c_str());
            
            if (driveType == DRIVE_FIXED || driveType == DRIVE_REMOVABLE) {
                DiskDriveInfo info;
                info.deviceId = rootPath;
                
                // 获取磁盘空间信息
                ULARGE_INTEGER totalBytes, freeBytes, totalFreeBytes;
                if (GetDiskFreeSpaceExA(rootPath.c_str(), &freeBytes, &totalBytes, &totalFreeBytes)) {
                    info.totalSize = totalBytes.QuadPart;
                }
                
                // 获取卷信息
                char volumeName[MAX_PATH] = {0};
                char fileSystem[32] = {0};
                DWORD serialNumber = 0, maxComponentLength = 0, fileSystemFlags = 0;
                
                if (GetVolumeInformationA(rootPath.c_str(), volumeName, MAX_PATH,
                                        &serialNumber, &maxComponentLength,
                                        &fileSystemFlags, fileSystem, sizeof(fileSystem))) {
                    info.model = SafeString(volumeName);
                    info.status = "Healthy";
                    info.interfaceType = (driveType == DRIVE_FIXED) ? "Fixed" : "Removable";
                    info.mediaType = "Unknown";
                    info.bytesPerSector = 512;
                } else {
                    info.model = "Unknown";
                    info.status = "Unknown";
                }
                
                drives.push_back(info);
            }
        }
    }
    
    return drives;
}

std::vector<PartitionInfo> DiskMonitor::GetPartitions() {
    std::vector<PartitionInfo> partitions;
    
    // 使用Windows API获取分区信息
    auto partitionsAPI = GetPartitionsViaAPI();
    partitions.insert(partitions.end(), partitionsAPI.begin(), partitionsAPI.end());
    
    std::cout << "获取到 " << partitions.size() << " 个分区" << std::endl;
    
    return partitions;
}

std::vector<PartitionInfo> DiskMonitor::GetPartitionsViaAPI() {
    std::vector<PartitionInfo> partitions;
    
    char driveBuffer[256] = {0};
    DWORD result = GetLogicalDriveStringsA(sizeof(driveBuffer), driveBuffer);
    
    if (result > 0 && result <= sizeof(driveBuffer)) {
        char* drivePtr = driveBuffer;
        while (*drivePtr) {
            std::string driveLetter = drivePtr;
            UINT driveType = GetDriveTypeA(driveLetter.c_str());
            
            if (driveType == DRIVE_FIXED || driveType == DRIVE_REMOVABLE) {
                PartitionInfo partition;
                partition.driveLetter = driveLetter.substr(0, 2);
                
                // 获取磁盘空间
                ULARGE_INTEGER totalBytes, freeBytes, totalFreeBytes;
                if (GetDiskFreeSpaceExA(driveLetter.c_str(), &freeBytes, &totalBytes, &totalFreeBytes)) {
                    partition.totalSize = totalBytes.QuadPart;
                    partition.freeSpace = freeBytes.QuadPart;
                    partition.usedSpace = totalBytes.QuadPart - freeBytes.QuadPart;
                    
                    if (totalBytes.QuadPart > 0) {
                        partition.usagePercentage = (100.0 * partition.usedSpace) / totalBytes.QuadPart;
                    } else {
                        partition.usagePercentage = 0.0;
                    }
                } else {
                    partition.totalSize = 0;
                    partition.freeSpace = 0;
                    partition.usedSpace = 0;
                    partition.usagePercentage = 0.0;
                }
                
                // 获取卷信息
                char volumeName[MAX_PATH] = {0};
                char fileSystem[32] = {0};
                DWORD serialNumber = 0, maxComponentLength = 0, fileSystemFlags = 0;
                
                if (GetVolumeInformationA(driveLetter.c_str(), volumeName, MAX_PATH,
                                        &serialNumber, &maxComponentLength,
                                        &fileSystemFlags, fileSystem, sizeof(fileSystem))) {
                    partition.label = SafeString(volumeName);
                    if (partition.label.empty()) {
                        partition.label = "本地磁盘";
                    }
                    partition.fileSystem = SafeString(fileSystem);
                    partition.serialNumber = serialNumber;
                } else {
                    partition.label = "本地磁盘";
                    partition.fileSystem = "Unknown";
                    partition.serialNumber = 0;
                }
                
                partitions.push_back(partition);
            }
            
            drivePtr += strlen(drivePtr) + 1;
        }
    }
    
    return partitions;
}

std::vector<DiskPerformance> DiskMonitor::GetDiskPerformance() {
    std::vector<DiskPerformance> performance;
    
    try {
        performance = GetPerformanceViaPDH();
        std::cout << "获取到 " << performance.size() << " 个性能计数器" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "PDH获取磁盘性能失败: " << e.what() << std::endl;
    }
    
    return performance;
}

std::vector<DiskPerformance> DiskMonitor::GetPerformanceViaPDH() {
    std::vector<DiskPerformance> performance;
    
    PDH_HQUERY query = NULL;
    PDH_STATUS status;
    
    status = PdhOpenQuery(NULL, 0, &query);
    if (status != ERROR_SUCCESS) {
        std::cerr << "PdhOpenQuery 失败: " << status << std::endl;
        return performance;
    }
    
    char driveBuffer[256] = {0};
    DWORD result = GetLogicalDriveStringsA(sizeof(driveBuffer), driveBuffer);
    
    std::cout << "可用的逻辑驱动器: " << driveBuffer << std::endl;
    
    if (result > 0 && result <= sizeof(driveBuffer)) {
        char* drivePtr = driveBuffer;
        while (*drivePtr) {
            std::string driveLetter = drivePtr;
            UINT driveType = GetDriveTypeA(driveLetter.c_str());
            
            std::cout << "检查驱动器: " << driveLetter << " 类型: " << driveType << std::endl;
            
            if (driveType == DRIVE_FIXED || driveType == DRIVE_REMOVABLE) {
                std::string driveName = driveLetter.substr(0, 2); // "C:", "D:" 等
                
                // 构建计数器路径
                std::string readCounterPath = "\\LogicalDisk(" + driveName + ")\\Disk Read Bytes/sec";
                std::string writeCounterPath = "\\LogicalDisk(" + driveName + ")\\Disk Write Bytes/sec";
                std::string queueCounterPath = "\\LogicalDisk(" + driveName + ")\\Current Disk Queue Length";
                std::string timeCounterPath = "\\LogicalDisk(" + driveName + ")\\% Disk Time";
                
                std::cout << "尝试添加计数器: " << readCounterPath << std::endl;
                
                PDH_HCOUNTER readBytesCounter, writeBytesCounter, queueLengthCounter, diskTimeCounter;
                
                // 检查每个计数器是否可用
                PDH_STATUS readStatus = PdhAddEnglishCounterA(query, readCounterPath.c_str(), 0, &readBytesCounter);
                PDH_STATUS writeStatus = PdhAddEnglishCounterA(query, writeCounterPath.c_str(), 0, &writeBytesCounter);
                PDH_STATUS queueStatus = PdhAddEnglishCounterA(query, queueCounterPath.c_str(), 0, &queueLengthCounter);
                PDH_STATUS timeStatus = PdhAddEnglishCounterA(query, timeCounterPath.c_str(), 0, &diskTimeCounter);
                
                std::cout << "计数器状态 - 读: " << readStatus << ", 写: " << writeStatus 
                          << ", 队列: " << queueStatus << ", 时间: " << timeStatus << std::endl;
                
                if (readStatus == ERROR_SUCCESS && writeStatus == ERROR_SUCCESS && 
                    queueStatus == ERROR_SUCCESS && timeStatus == ERROR_SUCCESS) {
                    
                    // 第一次收集数据（建立基线）
                    PdhCollectQueryData(query);
                    Sleep(100); // 等待100ms收集一些数据
                    
                    // 第二次收集数据（获取实际值）
                    PdhCollectQueryData(query);
                    
                    PDH_FMT_COUNTERVALUE readBytesValue, writeBytesValue, queueLengthValue, diskTimeValue;
                    
                    PDH_STATUS readGetStatus = PdhGetFormattedCounterValue(readBytesCounter, PDH_FMT_DOUBLE, NULL, &readBytesValue);
                    PDH_STATUS writeGetStatus = PdhGetFormattedCounterValue(writeBytesCounter, PDH_FMT_DOUBLE, NULL, &writeBytesValue);
                    PDH_STATUS queueGetStatus = PdhGetFormattedCounterValue(queueLengthCounter, PDH_FMT_DOUBLE, NULL, &queueLengthValue);
                    PDH_STATUS timeGetStatus = PdhGetFormattedCounterValue(diskTimeCounter, PDH_FMT_DOUBLE, NULL, &diskTimeValue);
                    
                    std::cout << "获取计数器值状态 - 读: " << readGetStatus << ", 写: " << writeGetStatus 
                              << ", 队列: " << queueGetStatus << ", 时间: " << timeGetStatus << std::endl;
                    
                    if (readGetStatus == ERROR_SUCCESS && writeGetStatus == ERROR_SUCCESS && 
                        queueGetStatus == ERROR_SUCCESS && timeGetStatus == ERROR_SUCCESS) {
                        
                        DiskPerformance perf;
                        perf.driveLetter = driveName;
                        perf.readBytesPerSec = static_cast<uint64_t>(readBytesValue.doubleValue);
                        perf.writeBytesPerSec = static_cast<uint64_t>(writeBytesValue.doubleValue);
                        perf.readSpeed = perf.readBytesPerSec / (1024.0 * 1024.0);
                        perf.writeSpeed = perf.writeBytesPerSec / (1024.0 * 1024.0);
                        perf.queueLength = queueLengthValue.doubleValue;
                        perf.usagePercentage = diskTimeValue.doubleValue;
                        perf.readCountPerSec = 0;
                        perf.writeCountPerSec = 0;
                        perf.responseTime = 0;
                        
                        std::cout << "性能数据 - " << driveName << ": " 
                                  << "读 " << perf.readSpeed << " MB/s, "
                                  << "写 " << perf.writeSpeed << " MB/s, "
                                  << "使用率 " << perf.usagePercentage << "%, "
                                  << "队列 " << perf.queueLength << std::endl;
                        
                        performance.push_back(perf);
                    }
                    
                    // 清理计数器
                    PdhRemoveCounter(readBytesCounter);
                    PdhRemoveCounter(writeBytesCounter);
                    PdhRemoveCounter(queueLengthCounter);
                    PdhRemoveCounter(diskTimeCounter);
                } else {
                    std::cerr << "无法为驱动器 " << driveName << " 添加性能计数器" << std::endl;
                    
                    // 清理可能已添加的计数器
                    if (readStatus == ERROR_SUCCESS) PdhRemoveCounter(readBytesCounter);
                    if (writeStatus == ERROR_SUCCESS) PdhRemoveCounter(writeBytesCounter);
                    if (queueStatus == ERROR_SUCCESS) PdhRemoveCounter(queueLengthCounter);
                    if (timeStatus == ERROR_SUCCESS) PdhRemoveCounter(diskTimeCounter);
                }
            }
            
            drivePtr += strlen(drivePtr) + 1;
        }
    }
    
    PdhCloseQuery(query);
    
    if (performance.empty()) {
        std::cout << "警告: 没有获取到任何磁盘性能数据" << std::endl;
        
        // 尝试使用物理磁盘计数器作为备选方案
        performance = GetPhysicalDiskPerformance();
    }
    
    return performance;
}

// 备选方案：使用物理磁盘性能计数器
std::vector<DiskPerformance> DiskMonitor::GetPhysicalDiskPerformance() {
    std::vector<DiskPerformance> performance;
    
    PDH_HQUERY query = NULL;
    PDH_STATUS status = PdhOpenQuery(NULL, 0, &query);
    
    if (status != ERROR_SUCCESS) {
        return performance;
    }
    
    std::cout << "尝试使用物理磁盘性能计数器..." << std::endl;
    
    // 使用物理磁盘的总计数器
    PDH_HCOUNTER readBytesCounter, writeBytesCounter, queueLengthCounter, diskTimeCounter;
    
    if (PdhAddEnglishCounterA(query, "\\PhysicalDisk(_Total)\\Disk Read Bytes/sec", 0, &readBytesCounter) == ERROR_SUCCESS &&
        PdhAddEnglishCounterA(query, "\\PhysicalDisk(_Total)\\Disk Write Bytes/sec", 0, &writeBytesCounter) == ERROR_SUCCESS &&
        PdhAddEnglishCounterA(query, "\\PhysicalDisk(_Total)\\Current Disk Queue Length", 0, &queueLengthCounter) == ERROR_SUCCESS &&
        PdhAddEnglishCounterA(query, "\\PhysicalDisk(_Total)\\% Disk Time", 0, &diskTimeCounter) == ERROR_SUCCESS) {
        
        PdhCollectQueryData(query);
        Sleep(100);
        PdhCollectQueryData(query);
        
        PDH_FMT_COUNTERVALUE readBytesValue, writeBytesValue, queueLengthValue, diskTimeValue;
        
        if (PdhGetFormattedCounterValue(readBytesCounter, PDH_FMT_DOUBLE, NULL, &readBytesValue) == ERROR_SUCCESS &&
            PdhGetFormattedCounterValue(writeBytesCounter, PDH_FMT_DOUBLE, NULL, &writeBytesValue) == ERROR_SUCCESS &&
            PdhGetFormattedCounterValue(queueLengthCounter, PDH_FMT_DOUBLE, NULL, &queueLengthValue) == ERROR_SUCCESS &&
            PdhGetFormattedCounterValue(diskTimeCounter, PDH_FMT_DOUBLE, NULL, &diskTimeValue) == ERROR_SUCCESS) {
            
            DiskPerformance perf;
            perf.driveLetter = "_Total";
            perf.readBytesPerSec = static_cast<uint64_t>(readBytesValue.doubleValue);
            perf.writeBytesPerSec = static_cast<uint64_t>(writeBytesValue.doubleValue);
            perf.readSpeed = perf.readBytesPerSec / (1024.0 * 1024.0);
            perf.writeSpeed = perf.writeBytesPerSec / (1024.0 * 1024.0);
            perf.queueLength = queueLengthValue.doubleValue;
            perf.usagePercentage = diskTimeValue.doubleValue;
            perf.readCountPerSec = 0;
            perf.writeCountPerSec = 0;
            perf.responseTime = 0;
            
            std::cout << "物理磁盘总性能: " 
                      << "读 " << perf.readSpeed << " MB/s, "
                      << "写 " << perf.writeSpeed << " MB/s, "
                      << "使用率 " << perf.usagePercentage << "%" << std::endl;
            
            performance.push_back(perf);
        }
        
        PdhRemoveCounter(readBytesCounter);
        PdhRemoveCounter(writeBytesCounter);
        PdhRemoveCounter(queueLengthCounter);
        PdhRemoveCounter(diskTimeCounter);
    }
    
    PdhCloseQuery(query);
    return performance;
}

std::vector<DiskSMARTData> DiskMonitor::GetSMARTData() {
    std::vector<DiskSMARTData> smartData;
    
    IWbemLocator* pLoc = NULL;
    IWbemServices* pSvc = NULL;
    
    if (!InitializeWMI(&pLoc, &pSvc)) {
        return smartData;
    }
    
    try {
        IEnumWbemClassObject* pEnumerator = NULL;
        BSTR lang = SysAllocString(L"WQL");
        BSTR query = SysAllocString(L"SELECT * FROM MSStorageDriver_FailurePredictStatus");
        HRESULT hres = pSvc->ExecQuery(lang, query, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
        SysFreeString(lang);
        SysFreeString(query);
        
        if (SUCCEEDED(hres)) {
            IWbemClassObject* pclsObj = NULL;
            ULONG uReturn = 0;
            
            while (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK) {
                DiskSMARTData data;
                
                VARIANT vtProp;
                
                // 获取实例名称（设备ID）
                if (pclsObj->Get(L"__RELPATH", 0, &vtProp, 0, 0) == S_OK) {
                    data.deviceId = BSTRToUtf8(vtProp.bstrVal);
                    VariantClear(&vtProp);
                }
                
                // 获取预测失败状态
                if (pclsObj->Get(L"PredictFailure", 0, &vtProp, 0, 0) == S_OK) {
                    data.healthStatus = vtProp.boolVal ? 0 : 100; // 如果预测失败则为0，否则为100
                    VariantClear(&vtProp);
                }
                
                // 获取其他SMART属性
                data.temperature = 0; // 需要额外的查询
                data.powerOnHours = 0;
                data.powerOnCount = 0;
                data.badSectors = 0;
                data.readErrorCount = 0;
                data.writeErrorCount = 0;
                data.overallHealth = data.healthStatus > 80 ? "Excellent" : 
                                   data.healthStatus > 60 ? "Good" : 
                                   data.healthStatus > 40 ? "Warning" : "Bad";
                
                smartData.push_back(data);
                pclsObj->Release();
            }
            
            pEnumerator->Release();
        }
    } catch (...) {
        // 忽略异常，SMART数据不是必需的
    }
    
    CleanupWMI(pLoc, pSvc);
    return smartData;
}

std::vector<DiskDriveInfo> DiskMonitor::GetDrivesViaWMI() {
    std::vector<DiskDriveInfo> drives;
    
    IWbemLocator* pLoc = NULL;
    IWbemServices* pSvc = NULL;
    
    if (!InitializeWMI(&pLoc, &pSvc)) {
        return drives;
    }
    
    try {
        IEnumWbemClassObject* pEnumerator = NULL;
        BSTR lang2 = SysAllocString(L"WQL");
        BSTR query2 = SysAllocString(L"SELECT * FROM Win32_DiskDrive");
        HRESULT hres = pSvc->ExecQuery(lang2, query2, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
        SysFreeString(lang2);
        SysFreeString(query2);
        
        if (SUCCEEDED(hres)) {
            IWbemClassObject* pclsObj = NULL;
            ULONG uReturn = 0;
            
            while (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK) {
                DiskDriveInfo drive;
                
                VARIANT vtProp;
                
                // 获取磁盘型号 - 使用安全的字符串转换
                if (pclsObj->Get(L"Model", 0, &vtProp, 0, 0) == S_OK) {
                    drive.model = FilterToValidUtf8(BSTRToUtf8(vtProp.bstrVal));
                    VariantClear(&vtProp);
                }
                
                // 获取序列号
                if (pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0) == S_OK) {
                    drive.serialNumber = FilterToValidUtf8(BSTRToUtf8(vtProp.bstrVal));
                    VariantClear(&vtProp);
                }
                
                // 获取接口类型
                if (pclsObj->Get(L"InterfaceType", 0, &vtProp, 0, 0) == S_OK) {
                    drive.interfaceType = FilterToValidUtf8(BSTRToUtf8(vtProp.bstrVal));
                    VariantClear(&vtProp);
                }
                
                // 获取介质类型
                if (pclsObj->Get(L"MediaType", 0, &vtProp, 0, 0) == S_OK) {
                    drive.mediaType = FilterToValidUtf8(BSTRToUtf8(vtProp.bstrVal));
                    VariantClear(&vtProp);
                }
                
                // 获取总大小
                if (pclsObj->Get(L"Size", 0, &vtProp, 0, 0) == S_OK) {
                    drive.totalSize = vtProp.ullVal;
                    VariantClear(&vtProp);
                }
                
                // 获取扇区大小
                if (pclsObj->Get(L"BytesPerSector", 0, &vtProp, 0, 0) == S_OK) {
                    drive.bytesPerSector = vtProp.uintVal;
                    VariantClear(&vtProp);
                }
                
                // 获取设备ID
                if (pclsObj->Get(L"DeviceID", 0, &vtProp, 0, 0) == S_OK) {
                    drive.deviceId = FilterToValidUtf8(BSTRToUtf8(vtProp.bstrVal));
                    VariantClear(&vtProp);
                }
                
                drive.status = "Healthy";
                
                drives.push_back(drive);
                pclsObj->Release();
            }
            
            pEnumerator->Release();
        }
    } catch (const std::exception& e) {
        std::cerr << "WMI查询磁盘驱动器失败: " << e.what() << std::endl;
    }
    
    CleanupWMI(pLoc, pSvc);
    return drives;
}

std::vector<PartitionInfo> DiskMonitor::GetPartitionsViaWMI() {
    std::vector<PartitionInfo> partitions;
    
    IWbemLocator* pLoc = NULL;
    IWbemServices* pSvc = NULL;
    
    if (!InitializeWMI(&pLoc, &pSvc)) {
        return partitions;
    }
    
    try {
        IEnumWbemClassObject* pEnumerator = NULL;
        BSTR lang3 = SysAllocString(L"WQL");
        BSTR query3 = SysAllocString(L"SELECT * FROM Win32_LogicalDisk");
        HRESULT hres = pSvc->ExecQuery(lang3, query3, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
        SysFreeString(lang3);
        SysFreeString(query3);
        
        if (SUCCEEDED(hres)) {
            IWbemClassObject* pclsObj = NULL;
            ULONG uReturn = 0;
            
            while (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK) {
                PartitionInfo partition;
                
                VARIANT vtProp;
                
                // 获取盘符
                if (pclsObj->Get(L"DeviceID", 0, &vtProp, 0, 0) == S_OK) {
                    partition.driveLetter = FilterToValidUtf8(BSTRToUtf8(vtProp.bstrVal));
                    VariantClear(&vtProp);
                }
                
                // 获取卷标
                if (pclsObj->Get(L"VolumeName", 0, &vtProp, 0, 0) == S_OK) {
                    partition.label = FilterToValidUtf8(BSTRToUtf8(vtProp.bstrVal));
                    VariantClear(&vtProp);
                }
                
                // 获取文件系统
                if (pclsObj->Get(L"FileSystem", 0, &vtProp, 0, 0) == S_OK) {
                    partition.fileSystem = FilterToValidUtf8(BSTRToUtf8(vtProp.bstrVal));
                    VariantClear(&vtProp);
                }
                
                // 获取总大小
                if (pclsObj->Get(L"Size", 0, &vtProp, 0, 0) == S_OK) {
                    partition.totalSize = vtProp.ullVal;
                    VariantClear(&vtProp);
                }
                
                // 获取可用空间
                if (pclsObj->Get(L"FreeSpace", 0, &vtProp, 0, 0) == S_OK) {
                    partition.freeSpace = vtProp.ullVal;
                    partition.usedSpace = partition.totalSize - partition.freeSpace;
                    VariantClear(&vtProp);
                } else {
                    partition.freeSpace = 0;
                    partition.usedSpace = partition.totalSize;
                }
                
                // 计算使用率
                if (partition.totalSize > 0) {
                    partition.usagePercentage = (100.0 * partition.usedSpace) / partition.totalSize;
                } else {
                    partition.usagePercentage = 0.0;
                }
                
                // 获取卷序列号
                if (pclsObj->Get(L"VolumeSerialNumber", 0, &vtProp, 0, 0) == S_OK) {
                    std::wstring serial = vtProp.bstrVal;
                    try {
                        partition.serialNumber = std::stoul(serial, nullptr, 16);
                    } catch (...) {
                        partition.serialNumber = 0;
                    }
                    VariantClear(&vtProp);
                }
                
                partitions.push_back(partition);
                pclsObj->Release();
            }
            
            pEnumerator->Release();
        }
    } catch (const std::exception& e) {
        std::cerr << "WMI查询分区失败: " << e.what() << std::endl;
    }
    
    CleanupWMI(pLoc, pSvc);
    return partitions;
}

bool DiskMonitor::StartIOMonitoring(const std::string& driveLetter) {
    isMonitoring_ = true;
    return true;
}

void DiskMonitor::StopIOMonitoring() {
    isMonitoring_ = false;
    ioMonitorData_.clear();
}

} // namespace sysmonitor