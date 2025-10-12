#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <ctime>
#include "../utils/AsyncLogger.h"

namespace snapshot {

struct RegistryValue {
    std::string key_path;
    std::string value_name;
    uint32_t type;
    std::string data;
    
    bool operator==(const RegistryValue& other) const;
    std::string to_string() const;
};

struct DiskInfo {
    std::string drive_letter;
    uint64_t total_size;
    uint64_t free_space;
    uint64_t used_space;
    
    bool operator==(const DiskInfo& other) const;
    std::string to_string() const;
};

struct ProcessInfo {
    uint32_t pid;
    std::string name;
    double cpu_usage;
    uint64_t memory_usage;
    std::string executable_path;
    
    bool operator==(const ProcessInfo& other) const;
    std::string to_string() const;
};

class SystemSnapshot {
public:
    SystemSnapshot();
    explicit SystemSnapshot(const std::string& name);
    
    // 序列化/反序列化
    std::string to_json() const;
    static std::shared_ptr<SystemSnapshot> from_json(const std::string& json_str);
    
    // Getters
    const std::string& get_id() const { return id_; }
    const std::string& get_name() const { return name_; }
    const std::string& get_hostname() const { return hostname_; }
    time_t get_timestamp() const { return timestamp_; }
    
    // 数据访问
    const std::vector<RegistryValue>& get_registry_data() const { return registry_data_; }
    const std::vector<DiskInfo>& get_disk_info() const { return disk_info_; }
    const std::vector<ProcessInfo>& get_process_info() const { return process_info_; }
    
    // 数据设置
    void set_registry_data(std::vector<RegistryValue> data) { registry_data_ = std::move(data); }
    void set_disk_info(std::vector<DiskInfo> info) { disk_info_ = std::move(info); }
    void set_process_info(std::vector<ProcessInfo> info) { process_info_ = std::move(info); }
    
private:
    std::string id_;
    std::string name_;
    std::string hostname_;
    time_t timestamp_;
    
    std::vector<RegistryValue> registry_data_;
    std::vector<DiskInfo> disk_info_;
    std::vector<ProcessInfo> process_info_;
};

} // namespace snapshot