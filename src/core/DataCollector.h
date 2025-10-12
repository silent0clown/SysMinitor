#pragma once
#include "SystemSnapshot.h"
#include <memory>
#include <vector>
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
// #include <tlhelp32.h>
namespace snapshot {

class DataCollector {
public:
    DataCollector();
    ~DataCollector() = default;
    
    // 采集完整快照
    std::shared_ptr<SystemSnapshot> collect_snapshot(const std::string& name = "");
    
    // 单独采集各个组件
    std::vector<RegistryValue> collect_registry_data();
    std::vector<DiskInfo> collect_disk_info();
    std::vector<ProcessInfo> collect_process_info();
    SystemMemoryInfo collect_system_memory_info();

private:
    // 注册表采集
    void collect_registry_key(const std::string& key_path, std::vector<RegistryValue>& results);
    std::vector<std::string> get_registry_subkeys(HKEY hKey);
    
    // 进程CPU使用率计算
    void initialize_cpu_counters();
    double calculate_process_cpu_usage(uint32_t pid);
    
    struct ProcessCpuData {
        uint64_t last_time;
        uint64_t last_system_time;
    };
    
    std::unordered_map<uint32_t, ProcessCpuData> cpu_cache_;
    uint64_t last_system_cpu_time_;
    uint32_t processor_count_;
};

} // namespace snapshot