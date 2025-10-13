#pragma once
#include <atomic>
#include <thread>
#include <functional>
#include <memory>
#include <mutex>
#include <cstdint>

namespace sysmonitor {

struct MemoryUsage {
    uint64_t totalPhysical;      // bytes
    uint64_t availablePhysical;  // bytes
    uint64_t usedPhysical;       // bytes
    double usedPercent;          // 0.0 - 100.0
    uint64_t timestamp;
};

class MemoryMonitor {
public:
    using UsageCallback = std::function<void(const MemoryUsage&)>;

    MemoryMonitor();
    ~MemoryMonitor();

    MemoryMonitor(const MemoryMonitor&) = delete;
    MemoryMonitor& operator=(const MemoryMonitor&) = delete;

    bool Initialize();
    void StartMonitoring(int intervalMs = 1000);
    void StopMonitoring();

    MemoryUsage GetCurrentUsage();

    void SetUsageCallback(UsageCallback callback) {
        std::lock_guard<std::mutex> lk(callbackMutex_);
        callback_ = callback;
    }

    bool IsRunning() const { return isRunning_; }

private:
    void MonitoringLoop();
    bool UpdateUsageData();

private:
    std::atomic<bool> isRunning_{false};
    std::unique_ptr<std::thread> monitorThread_;
    UsageCallback callback_;
    mutable std::mutex callbackMutex_;
    int intervalMs_;

    MemoryUsage memoryUsage_;
    std::mutex usageMutex_;
    std::atomic<double> currentUsage_{0.0};
};

} // namespace sysmonitor