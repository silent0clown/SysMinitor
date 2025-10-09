#include "SystemSnapshot.h"
#include <sstream>
#include <iomanip>
#include <random>
#include <winsock2.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")

namespace snapshot {

bool RegistryValue::operator==(const RegistryValue& other) const {
    return key_path == other.key_path && 
           value_name == other.value_name && 
           type == other.type && 
           data == other.data;
}

std::string RegistryValue::to_string() const {
    std::ostringstream oss;
    oss << "Registry[Path=" << key_path << ", Name=" << value_name 
        << ", Type=" << type << ", Data=" << data << "]";
    return oss.str();
}

bool DiskInfo::operator==(const DiskInfo& other) const {
    return drive_letter == other.drive_letter &&
           total_size == other.total_size &&
           free_space == other.free_space;
}

std::string DiskInfo::to_string() const {
    std::ostringstream oss;
    oss << "Disk[Drive=" << drive_letter << ", Total=" << (total_size / (1024*1024*1024)) 
        << "GB, Free=" << (free_space / (1024*1024*1024)) << "GB]";
    return oss.str();
}

bool ProcessInfo::operator==(const ProcessInfo& other) const {
    return pid == other.pid && name == other.name;
}

std::string ProcessInfo::to_string() const {
    std::ostringstream oss;
    oss << "Process[PID=" << pid << ", Name=" << name 
        << ", CPU=" << cpu_usage << "%, Memory=" << (memory_usage / (1024*1024)) << "MB]";
    return oss.str();
}

SystemSnapshot::SystemSnapshot() {
    // 生成唯一ID
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    std::ostringstream oss;
    oss << std::hex << dis(gen);
    id_ = oss.str();
    
    timestamp_ = std::time(nullptr);
    
    // 获取主机名
    char hostname[256];
    DWORD size = sizeof(hostname);
    if (GetComputerNameA(hostname, &size)) {
        hostname_ = hostname;
    } else {
        hostname_ = "unknown";
    }
}

SystemSnapshot::SystemSnapshot(const std::string& name) : SystemSnapshot() {
    name_ = name;
}

std::string SystemSnapshot::to_json() const {
    // 简化实现，实际应使用nlohmann/json
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"id\": \"" << id_ << "\",\n";
    oss << "  \"name\": \"" << name_ << "\",\n";
    oss << "  \"hostname\": \"" << hostname_ << "\",\n";
    oss << "  \"timestamp\": " << timestamp_ << ",\n";
    oss << "  \"registry_entries\": " << registry_data_.size() << ",\n";
    oss << "  \"disks\": " << disk_info_.size() << ",\n";
    oss << "  \"processes\": " << process_info_.size() << "\n";
    oss << "}";
    return oss.str();
}

std::shared_ptr<SystemSnapshot> SystemSnapshot::from_json(const std::string& json_str) {
    // 简化实现，实际应使用nlohmann/json解析
    auto snapshot = std::make_shared<SystemSnapshot>();
    // 这里应该实现完整的JSON解析
    return snapshot;
}

} // namespace snapshot