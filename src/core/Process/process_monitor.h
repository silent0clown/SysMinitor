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
    double cpuUsage;           // CPU使用率百分比
    uint64_t memoryUsage;      // 内存使用量(bytes)
    uint64_t workingSetSize;   // 工作集大小(bytes)
    uint64_t pagefileUsage;    // 页面文件使用量(bytes)
    int64_t createTime;        // 创建时间戳
    int32_t priority;          // 进程优先级
    int32_t threadCount;       // 线程数
    std::string commandLine;   // 命令行参数
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

    // 获取当前所有进程快照
    ProcessSnapshot GetProcessSnapshot();
    
    // 获取特定进程的详细信息
    ProcessInfo GetProcessInfo(uint32_t pid);
    
    // 根据进程名查找进程
    std::vector<ProcessInfo> FindProcessesByName(const std::string& name);
    
    // 终止进程
    bool TerminateProcess(uint32_t pid, uint32_t exitCode = 0);
    
    // 获取进程CPU使用率历史（需要持续监控）
    double GetProcessCpuUsage(uint32_t pid);

private:
    bool Initialize();
    void Cleanup();
    
    // 使用WMI获取进程信息
    std::vector<ProcessInfo> GetProcessesViaWMI();
    
    // 使用ToolHelp API获取进程信息
    std::vector<ProcessInfo> GetProcessesViaToolHelp();
    
    // 使用NTQuerySystemInformation获取进程信息
    std::vector<ProcessInfo> GetProcessesViaNTQuery();
    
    // 获取进程用户名
    std::string GetProcessUsername(uint32_t pid);
    
    // 获取进程命令行
    std::string GetProcessCommandLine(uint32_t pid);
    
    // CPU使用率计算相关
    struct ProcessCpuData {
        uint64_t kernelTime;
        uint64_t userTime;
        uint64_t lastUpdate;
    };
    
    std::unordered_map<uint32_t, ProcessCpuData> processCpuData_;
    uint64_t lastSystemTime_;
};

} // namespace sysmonitor