#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include "../../utils/util_time.h"

namespace sysmonitor {

struct ProcessInfo {
    uint32_t pid;
    uint32_t parentPid;
    std::string name;
    std::string fullPath;
    std::string state;
    std::string username;
    double cpuUsage;           // CPU usage percentage
    uint64_t memoryUsage;      // Memory usage (bytes)
    uint64_t workingSetSize;   // Working set size (bytes)
    uint64_t pagefileUsage;    // Pagefile usage (bytes)
    int64_t createTime;        // Creation timestamp
    int32_t priority;          // Process priority
    int32_t threadCount;       // Thread count
    std::string commandLine;   // Command line parameters
    uint32_t handleCount;      // 句柄数
    uint32_t gdiCount;         // GDI对象数
    uint32_t userCount;        // USER对象数
};

struct ProcessSnapshot {
    std::vector<ProcessInfo> processes;
    uint64_t timestamp;
    uint32_t totalProcesses;
    uint32_t totalThreads;
    uint32_t totalHandles;     // 总句柄数
    uint32_t totalGdiObjects;  // 总GDI对象数
    uint32_t totalUserObjects; // 总USER对象数
};

struct HandleStatistics {
    uint32_t totalHandles;
    uint32_t totalGdiObjects;
    uint32_t totalUserObjects;
    std::unordered_map<uint32_t, uint32_t> processHandles;
    std::unordered_map<uint32_t, uint32_t> processGdiObjects;
    std::unordered_map<uint32_t, uint32_t> processUserObjects;
};

class ProcessMonitor {
public:
    ProcessMonitor();
    ~ProcessMonitor();

    // Get current process snapshot
    ProcessSnapshot GetProcessSnapshot();
    
    // Get detailed information for specific process
    ProcessInfo GetProcessInfo(uint32_t pid);
    
    // Find processes by name
    std::vector<ProcessInfo> FindProcessesByName(const std::string& name);
    
    // Terminate process
    bool TerminateProcess(uint32_t pid, uint32_t exitCode = 0);
    
    // Get process CPU usage history (requires continuous monitoring)
    double GetProcessCpuUsage(uint32_t pid);

    // 新增：获取进程句柄数
    uint32_t GetProcessHandleCount1(uint32_t pid);
    
    // 新增：获取进程GDI对象数
    uint32_t GetProcessGdiCount(uint32_t pid);
    
    // 新增：获取进程USER对象数  
    uint32_t GetProcessUserCount(uint32_t pid);
    
    // 新增：获取所有进程的句柄统计
    HandleStatistics GetHandleStatistics();

private:
    bool Initialize();
    void Cleanup();
    
    // Use WMI to get process information
    std::vector<ProcessInfo> GetProcessesViaToolHelp();
    
    // Get process username
    std::string GetProcessUsername(uint32_t pid);
    
    // Get process command line
    std::string GetProcessCommandLine(uint32_t pid);
    
    // CPU usage calculation related
    struct ProcessCpuData {
        uint64_t kernelTime;
        uint64_t userTime;
        uint64_t lastUpdate;
    };
    
    std::unordered_map<uint32_t, ProcessCpuData> processCpuData_;
    uint64_t lastSystemTime_;
};

} // namespace sysmonitor