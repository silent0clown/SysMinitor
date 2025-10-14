#include "memory_monitor.h"
#define NOMINMAX
#include <windows.h>
#include <algorithm>

namespace sysmonitor {

MemoryMonitor::MemoryMonitor() : intervalMs_(1000) {
    memoryUsage_.totalPhysical = 0;
    memoryUsage_.availablePhysical = 0;
    memoryUsage_.usedPhysical = 0;
    memoryUsage_.usedPercent = 0.0;
    memoryUsage_.timestamp = 0;
}

MemoryMonitor::~MemoryMonitor() {
    StopMonitoring();
}

bool MemoryMonitor::Initialize() {
    return UpdateUsageData();
}

void MemoryMonitor::StartMonitoring(int intervalMs) {
    if (isRunning_) return;

    intervalMs_ = intervalMs;
    isRunning_ = true;
    monitorThread_ = std::make_unique<std::thread>(&MemoryMonitor::MonitoringLoop, this);
}

void MemoryMonitor::StopMonitoring() {
    isRunning_ = false;

    if (monitorThread_ && monitorThread_->joinable()) {
        monitorThread_->join();
    }
    monitorThread_.reset();
}

void MemoryMonitor::MonitoringLoop() {
    while (isRunning_.load(std::memory_order_acquire)) {
        if (UpdateUsageData()) {
            MemoryUsage snapshot;
            {
                std::lock_guard<std::mutex> lk(usageMutex_);
                snapshot = memoryUsage_;
            }

            if (callback_) {
                callback_(snapshot);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs_));
    }
}

MemoryUsage MemoryMonitor::GetCurrentUsage() {
    std::lock_guard<std::mutex> lk(usageMutex_);
    return memoryUsage_;
}

bool MemoryMonitor::UpdateUsageData() {
    MEMORYSTATUSEX stat;
    stat.dwLength = sizeof(stat);
    if (!GlobalMemoryStatusEx(&stat)) {
        return false;
    }

    uint64_t total = stat.ullTotalPhys;
    uint64_t avail = stat.ullAvailPhys;
    uint64_t used = (total > avail) ? (total - avail) : 0;
    double percent = 0.0;
    if (total != 0) {
        percent = 100.0 * static_cast<double>(used) / static_cast<double>(total);
    }
    percent = std::clamp(percent, 0.0, 100.0);

    uint64_t ts = GetTickCount64();

    currentUsage_.store(percent, std::memory_order_release);
    {
        std::lock_guard<std::mutex> lk(usageMutex_);
        memoryUsage_.totalPhysical = total;
        memoryUsage_.availablePhysical = avail;
        memoryUsage_.usedPhysical = used;
        memoryUsage_.usedPercent = percent;
        memoryUsage_.timestamp = ts;
    }

    return true;
}

} // namespace sysmonitor