#pragma once
#include "system_info.h"
#include <atomic>
#include <thread>
#include <functional>
#include <memory>

namespace sysmonitor {

class CPUMonitor {
public:
    using UsageCallback = std::function<void(const CPUUsage&)>;

    CPUMonitor();
    ~CPUMonitor();

    // 禁用拷贝
    CPUMonitor(const CPUMonitor&) = delete;
    CPUMonitor& operator=(const CPUMonitor&) = delete;

    bool Initialize();
    void StartMonitoring(int intervalMs = 1000);
    void StopMonitoring();
    
    CPUUsage GetCurrentUsage();
    CPUInfo GetCPUInfo() const { return cpuInfo_; }
    
    void SetUsageCallback(UsageCallback callback) {
        callback_ = callback;
    }

    bool IsRunning() const { return isRunning_; }

private:
    void MonitoringLoop();
    bool UpdateUsageData();
    double CalculateUsage();

private:
    std::atomic<bool> isRunning_{false};
    std::unique_ptr<std::thread> monitorThread_;
    UsageCallback callback_;
    int intervalMs_;
    CPUInfo cpuInfo_;
    
    // CPU使用率计算相关
    std::atomic<double> currentUsage{0.0};
    uint64_t lastTotalTime_{0};
    uint64_t lastIdleTime_{0};
    std::vector<uint64_t> lastCoreTimes_;
};

} // namespace sysmonitor