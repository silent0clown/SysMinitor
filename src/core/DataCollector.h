#pragma once
#include "SystemSnapshot.h"
#include <memory>
#include <vector>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX 
#include <windows.h>
// #include <tlhelp32.h>
namespace snapshot {

class DataCollector {
public:
    DataCollector();
    ~DataCollector() = default;
    
    // �ɼ���������
    std::shared_ptr<SystemSnapshot> collect_snapshot(const std::string& name = "");
    
    // �����ɼ��������
    std::vector<RegistryValue> collect_registry_data();
    std::vector<DiskInfo> collect_disk_info();
    std::vector<ProcessInfo> collect_process_info();

private:
    // ע���ɼ�
    void collect_registry_key(const std::string& key_path, std::vector<RegistryValue>& results);
    std::vector<std::string> get_registry_subkeys(HKEY hKey);
    
    // ����CPUʹ���ʼ���
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