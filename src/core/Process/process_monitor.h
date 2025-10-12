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
    double cpuUsage;           // CPUʹ���ʰٷֱ�
    uint64_t memoryUsage;      // �ڴ�ʹ����(bytes)
    uint64_t workingSetSize;   // ��������С(bytes)
    uint64_t pagefileUsage;    // ҳ���ļ�ʹ����(bytes)
    int64_t createTime;        // ����ʱ���
    int32_t priority;          // �������ȼ�
    int32_t threadCount;       // �߳���
    std::string commandLine;   // �����в���
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

    // ��ȡ��ǰ���н��̿���
    ProcessSnapshot GetProcessSnapshot();
    
    // ��ȡ�ض����̵���ϸ��Ϣ
    ProcessInfo GetProcessInfo(uint32_t pid);
    
    // ���ݽ��������ҽ���
    std::vector<ProcessInfo> FindProcessesByName(const std::string& name);
    
    // ��ֹ����
    bool TerminateProcess(uint32_t pid, uint32_t exitCode = 0);
    
    // ��ȡ����CPUʹ������ʷ����Ҫ������أ�
    double GetProcessCpuUsage(uint32_t pid);

private:
    bool Initialize();
    void Cleanup();
    
    // ʹ��WMI��ȡ������Ϣ
    std::vector<ProcessInfo> GetProcessesViaWMI();
    
    // ʹ��ToolHelp API��ȡ������Ϣ
    std::vector<ProcessInfo> GetProcessesViaToolHelp();
    
    // ʹ��NTQuerySystemInformation��ȡ������Ϣ
    std::vector<ProcessInfo> GetProcessesViaNTQuery();
    
    // ��ȡ�����û���
    std::string GetProcessUsername(uint32_t pid);
    
    // ��ȡ����������
    std::string GetProcessCommandLine(uint32_t pid);
    
    // CPUʹ���ʼ������
    struct ProcessCpuData {
        uint64_t kernelTime;
        uint64_t userTime;
        uint64_t lastUpdate;
    };
    
    std::unordered_map<uint32_t, ProcessCpuData> processCpuData_;
    uint64_t lastSystemTime_;
};

} // namespace sysmonitor