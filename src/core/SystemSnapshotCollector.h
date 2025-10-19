#pragma once
#include "../third_party/nlohmann/json.hpp"
#include "../utils/util_time.h"
#include "CPUInfo/system_info.h"
#include "CPUInfo/cpu_monitor.h"
#include "Memory/memory_monitor.h"
#include "Disk/disk_monitor.h"
#include "Driver/driver_monitor.h"
#include "Register/registry_monitor.h"
#include "Process/process_monitor.h"

namespace sysmonitor {

using json = nlohmann::json;

#include "SystemSnapshot.h"

class SystemSnapshotCollector {
public:
    static SystemSnapshot Collect(CPUMonitor& cpu,
                                  MemoryMonitor& memory,
                                  DiskMonitor& disk,
                                  DriverMonitor& driver,
                                  RegistryMonitor& registry,
                                  ProcessMonitor& process) {
        SystemSnapshot s;
        try { s.cpu = cpu.GetCurrentUsage(); } catch(...) {}
        try { s.memory = memory.GetCurrentUsage(); } catch(...) {}
        try { s.disk = disk.GetDiskSnapshot(); } catch(...) {}
        try { s.driver = driver.GetDriverSnapshot(); } catch(...) {}
        try { s.registry = registry.GetRegistrySnapshot(); } catch(...) {}
        try { s.processes = process.GetProcessSnapshot(); } catch(...) {}
        s.timestamp = GET_LOCAL_TIME_MS();
        return s;
    }
};

} // namespace sysmonitor
