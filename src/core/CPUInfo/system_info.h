#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace sysmonitor {

struct CPUInfo {
    std::string name;
    std::string vendor;
    uint32_t physicalCores;
    uint32_t logicalCores;
    uint32_t packages;
    uint32_t baseFrequency; // MHz
    uint32_t maxFrequency;  // MHz
    std::string architecture;
};

struct CPUUsage {
    double totalUsage;
    std::vector<double> coreUsages;
    uint64_t timestamp;
};

class SystemInfo {
public:
    static CPUInfo GetCPUInfo();
    static std::string GetCPUName();
    static uint32_t GetPhysicalCoreCount();
    static uint32_t GetLogicalCoreCount();
    static double GetCPUTemperature(); // 注意：Windows默认不支持，可能需要第三方驱动
};

} // namespace sysmonitor