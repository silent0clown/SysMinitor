#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <pdh.h>
#include <pdhmsg.h>
#include <algorithm>
#include <mutex>
#include <vector>
#include "cpu_monitor.h"
#include "../../utils/util_time.h"

#pragma comment(lib, "pdh.lib")

namespace sysmonitor {

// ??????????????λ???????????λ??
static DWORD CountSetBits(ULONG_PTR bitMask) {
    DWORD count = 0;
    while (bitMask) {
        count += bitMask & 1;
        bitMask >>= 1;
    }
    return count;
}

CPUMonitor::CPUMonitor() : intervalMs_(1000) {
    cpuInfo_ = SystemInfo::GetCPUInfo();
}

CPUMonitor::~CPUMonitor() {
    StopMonitoring();
}

bool CPUMonitor::Initialize() {
    return UpdateUsageData();
}

void CPUMonitor::StartMonitoring(int intervalMs) {
    if (isRunning_) return;
    
    intervalMs_ = intervalMs;
    isRunning_ = true;
    monitorThread_ = std::make_unique<std::thread>(&CPUMonitor::MonitoringLoop, this);
}

void CPUMonitor::StopMonitoring() {
    isRunning_ = false;
    if (monitorThread_ && monitorThread_->joinable()) {
        monitorThread_->join();
    }
}

void CPUMonitor::MonitoringLoop() {
    while (isRunning_) {
        double usage = CalculateUsage();
        CPUUsage usageData;
        usageData.totalUsage = usage;
        usageData.timestamp = GET_LOCAL_TIME_MS();

        if (callback_) {
            callback_(usageData);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs_));
    }
}

CPUUsage CPUMonitor::GetCurrentUsage() {
    CPUUsage usage;
    usage.timestamp = GET_LOCAL_TIME_MS();
    if (!UpdateUsageData()) {
        usage.totalUsage = -1.0;
        return usage;
    }

    usage.totalUsage = currentUsage.load();

    try {
        std::lock_guard<std::mutex> lk(coreUsageMutex_);
        usage.coreUsages = currentCoreUsages_;
    } catch (...) {
        usage.coreUsages.clear();
    }

    return usage;
}

bool CPUMonitor::UpdateUsageData() {
    FILETIME idleTime, kernelTime, userTime;
    
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        return false;
    }
    
    ULARGE_INTEGER idle, kernel, user;
    idle.LowPart = idleTime.dwLowDateTime;
    idle.HighPart = idleTime.dwHighDateTime;
    kernel.LowPart = kernelTime.dwLowDateTime;
    kernel.HighPart = kernelTime.dwHighDateTime;
    user.LowPart = userTime.dwLowDateTime;
    user.HighPart = userTime.dwHighDateTime;
    
    uint64_t totalTime = (kernel.QuadPart + user.QuadPart);
    uint64_t idleTimeValue = idle.QuadPart;
    
    if (lastTotalTime_ != 0 && lastIdleTime_ != 0) {
        uint64_t totalDiff = totalTime - lastTotalTime_;
        uint64_t idleDiff = idleTimeValue - lastIdleTime_;
        
        if (totalDiff > 0) {
            double usage = 100.0 * (static_cast<double>(totalDiff) - static_cast<double>(idleDiff)) / static_cast<double>(totalDiff);
            usage = usage < 0.0 ? 0.0 : (usage > 100.0 ? 100.0 : usage);
            currentUsage.store(usage);
            std::cout << "Debug: CPU Usage = " << usage << "%" << std::endl;
        } else {
            currentUsage.store(0.0);
        }
    } else {
        currentUsage.store(0.0);
    }

    lastTotalTime_ = totalTime;
    lastIdleTime_ = idleTimeValue;

    try {
        HMODULE ntdll = GetModuleHandleA("ntdll.dll");
        if (ntdll) {
            using NtQuerySystemInformation_t = NTSTATUS(WINAPI*)(int, PVOID, ULONG, PULONG);
            auto NtQuerySystemInformation = reinterpret_cast<NtQuerySystemInformation_t>(GetProcAddress(ntdll, "NtQuerySystemInformation"));
            if (NtQuerySystemInformation) {
                uint32_t cores = cpuInfo_.logicalCores > 0 ? cpuInfo_.logicalCores : SystemInfo::GetLogicalCoreCount();

                struct SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION {
                    LARGE_INTEGER IdleTime;
                    LARGE_INTEGER KernelTime;
                    LARGE_INTEGER UserTime;
                    LARGE_INTEGER DpcTime;
                    LARGE_INTEGER InterruptTime;
                    ULONG InterruptCount;
                };

                std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION> info;
                info.resize(cores);
                ULONG returnLength = 0;

                NTSTATUS status = NtQuerySystemInformation(8, info.data(), static_cast<ULONG>(sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * info.size()), &returnLength);
                if (status == 0) {
                    if (lastCoreTotalTimes_.size() != cores) {
                        lastCoreTotalTimes_.assign(cores, 0);
                        lastCoreIdleTimes_.assign(cores, 0);
                        currentCoreUsages_.assign(cores, 0.0);
                    }

                    std::lock_guard<std::mutex> lk(coreUsageMutex_);
                    for (uint32_t i = 0; i < cores; ++i) {
                        uint64_t k = static_cast<uint64_t>(info[i].KernelTime.QuadPart);
                        uint64_t u = static_cast<uint64_t>(info[i].UserTime.QuadPart);
                        uint64_t idle = static_cast<uint64_t>(info[i].IdleTime.QuadPart);
                        uint64_t total = k + u;

                        if (lastCoreTotalTimes_[i] != 0 || lastCoreIdleTimes_[i] != 0) {
                            uint64_t totalDiff = total - lastCoreTotalTimes_[i];
                            uint64_t idleDiff = idle - lastCoreIdleTimes_[i];
                            double usage = 0.0;
                            if (totalDiff > 0) {
                                usage = 100.0 * (static_cast<double>(totalDiff) - static_cast<double>(idleDiff)) / static_cast<double>(totalDiff);
                                if (usage < 0.0) usage = 0.0;
                                if (usage > 100.0) usage = 100.0;
                            }
                            currentCoreUsages_[i] = usage;
                        } else {
                            currentCoreUsages_[i] = 0.0;
                        }

                        lastCoreTotalTimes_[i] = total;
                        lastCoreIdleTimes_[i] = idle;
                    }
                }
            }
        }
    } catch (...) {

    }

    return true;
}
double CPUMonitor::CalculateUsage() {
    FILETIME idleTime, kernelTime, userTime;
    
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        return -1.0;
    }
    
    ULARGE_INTEGER idle, kernel, user;
    idle.LowPart = idleTime.dwLowDateTime;
    idle.HighPart = idleTime.dwHighDateTime;
    kernel.LowPart = kernelTime.dwLowDateTime;
    kernel.HighPart = kernelTime.dwHighDateTime;
    user.LowPart = userTime.dwLowDateTime;
    user.HighPart = userTime.dwHighDateTime;
    
    uint64_t totalTime = (kernel.QuadPart + user.QuadPart);
    uint64_t idleTimeValue = idle.QuadPart;
    
    if (lastTotalTime_ == 0 || lastIdleTime_ == 0) {
        // 第一次调用，只记录时间，不计算使用率
        lastTotalTime_ = totalTime;
        lastIdleTime_ = idleTimeValue;
        return 0.0;
    }
    
    uint64_t totalDiff = totalTime - lastTotalTime_;
    uint64_t idleDiff = idleTimeValue - lastIdleTime_;
    
    lastTotalTime_ = totalTime;
    lastIdleTime_ = idleTimeValue;
    
    if (totalDiff == 0) {
        return 0.0;
    }
    
    double usage = 100.0 * (totalDiff - idleDiff) / totalDiff;
    return std::max(0.0, std::min(100.0, usage));
}
// SystemInfo ??????
CPUInfo SystemInfo::GetCPUInfo() {
    CPUInfo info;
    
    // ????????
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    info.logicalCores = sysInfo.dwNumberOfProcessors;
    
    // ????????????
    switch (sysInfo.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64:
            info.architecture = "x64";
            break;
        case PROCESSOR_ARCHITECTURE_ARM:
            info.architecture = "ARM";
            break;
        case PROCESSOR_ARCHITECTURE_IA64:
            info.architecture = "Intel Itanium";
            break;
        case PROCESSOR_ARCHITECTURE_INTEL:
            info.architecture = "x86";
            break;
        default:
            info.architecture = "Unknown";
    }
    
    // ??????????????????
    DWORD returnLength = 0;
    GetLogicalProcessorInformation(NULL, &returnLength);
    
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        auto buffer = std::make_unique<BYTE[]>(returnLength);
        auto processorInfo = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION>(buffer.get());
        
        if (GetLogicalProcessorInformation(processorInfo, &returnLength)) {
            DWORD processorCoreCount = 0;
            DWORD processorPackageCount = 0;
            
            DWORD byteOffset = 0;
            while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength) {
                switch (processorInfo->Relationship) {
                    case RelationProcessorCore:
                        processorCoreCount++;
                        info.physicalCores = processorCoreCount;
                        break;
                    case RelationProcessorPackage:
                        processorPackageCount++;
                        info.packages = processorPackageCount;
                        break;
                }
                byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
                processorInfo++;
            }
        }
    }
    
    // ?????????CPU????????
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        
        char processorName[256] = {0};
        DWORD size = sizeof(processorName);
        if (RegQueryValueEx(hKey, "ProcessorNameString", NULL, NULL, 
                           (LPBYTE)processorName, &size) == ERROR_SUCCESS) {
            info.name = processorName;
        }
        
        DWORD mhz;
        size = sizeof(mhz);
        if (RegQueryValueEx(hKey, "~MHz", NULL, NULL, 
                           (LPBYTE)&mhz, &size) == ERROR_SUCCESS) {
            info.baseFrequency = mhz;
        }
        
        RegCloseKey(hKey);
    }
    
    return info;
}

uint32_t SystemInfo::GetPhysicalCoreCount() {
    return GetCPUInfo().physicalCores;
}

uint32_t SystemInfo::GetLogicalCoreCount() {
    return GetCPUInfo().logicalCores;
}

std::string SystemInfo::GetCPUName() {
    return GetCPUInfo().name;
}

} // namespace sysmonitor