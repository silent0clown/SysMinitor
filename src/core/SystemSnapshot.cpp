#include "SystemSnapshot.h"
#include <sstream>
#include <iomanip>
#include <random>
#include <winsock2.h>
#include <iphlpapi.h>
#include "nlohmann/json.hpp"
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
static std::string time_to_string(time_t t) {
  std::tm tm{};
#if defined(_WIN32)
  localtime_s(&tm, &t);
#else
  localtime_r(&t, &tm);
#endif
  std::ostringstream os;
  os << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
  return os.str();
}

std::string SystemSnapshot::to_json() const {
    // 简化实现，实际应使用nlohmann/json
  using nlohmann::json;

  // 1. 构造 JSON 对象
  json j;
  j["id"] = id_;
  j["name"] = name_;
  j["hostname"] = hostname_;
  j["timestamp"] = time_to_string(timestamp_);

  // 2. registry_data
  j["registry_data"] = json::array();
  for (const auto &rv : registry_data_) {
    j["registry_data"].push_back({{"key_path", rv.key_path},
                                  {"value_name", rv.value_name},
                                  {"type", rv.type},
                                  {"data", rv.data}});
  }

  // 3. disk_info
  j["disk_info"] = json::array();
  for (const auto &d : disk_info_) {
    j["disk_info"].push_back({{"drive_letter", d.drive_letter},
                              {"total_size", d.total_size},
                              {"free_space", d.free_space},
                              {"used_space", d.used_space}});
  }

  // 4. process_info
  j["process_info"] = json::array();
  for (const auto &p : process_info_) {
    j["process_info"].push_back({{"pid", p.pid},
                                 {"name", p.name},
                                 {"cpu_usage", p.cpu_usage},
                                 {"memory_usage", p.memory_usage},
                                 {"executable_path", p.executable_path}});
  }

  // 5. 返回紧凑 JSON 字符串（如需美化可用 dump(4)）
  return j.dump();
}

std::shared_ptr<SystemSnapshot> SystemSnapshot::from_json(const std::string& json_str) {
    // 简化实现，实际应使用nlohmann/json解析
    auto snapshot = std::make_shared<SystemSnapshot>();
    // 这里应该实现完整的JSON解析
    return snapshot;
}

} // namespace snapshot