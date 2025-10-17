#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <unordered_map>

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
};

struct ProcessSnapshot {
    std::vector<ProcessInfo> processes;
    uint64_t timestamp;
    uint32_t totalProcesses;
    uint32_t totalThreads;
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

private:
    bool Initialize();
    void Cleanup();
    
    // Use WMI to get process information
    std::vector<ProcessInfo> GetProcessesViaWMI();
    
    // Use ToolHelp API to get process information
    std::vector<ProcessInfo> GetProcessesViaToolHelp();
    
    // Use NTQuerySystemInformation to get process information
    std::vector<ProcessInfo> GetProcessesViaNTQuery();
    
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