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

bool SystemMemoryInfo::operator==(const SystemMemoryInfo& other) const {
    return total_physical == other.total_physical &&
           avail_physical == other.avail_physical &&
           total_pagefile == other.total_pagefile &&
           avail_pagefile == other.avail_pagefile;
}

std::string SystemMemoryInfo::to_string() const {
    std::ostringstream oss;
    oss << "Memory[TotalPhysical=" << (total_physical / (1024*1024)) << "MB, AvailPhysical=" 
        << (avail_physical / (1024*1024)) << "MB, TotalPageFile=" 
        << (total_pagefile / (1024*1024)) << "MB, AvailPageFile=" 
        << (avail_pagefile / (1024*1024)) << "MB]";
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
                                    {"executable_path", p.executable_path},
                                    {"private_bytes", p.private_bytes},
                                    {"handle_count", p.handle_count}});
    }

    // 5. system memory info
    j["system_memory"] = {
            {"total_physical", system_memory_.total_physical},
            {"avail_physical", system_memory_.avail_physical},
            {"total_pagefile", system_memory_.total_pagefile},
            {"avail_pagefile", system_memory_.avail_pagefile}};

    // 6. 返回紧凑 JSON 字符串（如需美化可用 dump(4)）
    return j.dump();
}

std::shared_ptr<SystemSnapshot> SystemSnapshot::from_json(const std::string& json_str) {
    using nlohmann::json;
    auto snapshot = std::make_shared<SystemSnapshot>();
    try {
        json j = json::parse(json_str);

        if (j.contains("id") && j["id"].is_string()) snapshot->id_ = j["id"].get<std::string>();
        if (j.contains("name") && j["name"].is_string()) snapshot->name_ = j["name"].get<std::string>();
        if (j.contains("hostname") && j["hostname"].is_string()) snapshot->hostname_ = j["hostname"].get<std::string>();
        if (j.contains("timestamp") && j["timestamp"].is_string()) {
            // 尝试解析与 to_json 使用相同的格式: YYYY-MM-DDTHH:MM:SS
            std::string ts = j["timestamp"].get<std::string>();
            std::tm tm{};
            std::istringstream iss(ts);
            iss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
            if (!iss.fail()) snapshot->timestamp_ = std::mktime(&tm);
        }

        // registry_data
        if (j.contains("registry_data") && j["registry_data"].is_array()) {
            snapshot->registry_data_.clear();
            for (const auto &item : j["registry_data"]) {
                RegistryValue rv;
                if (item.contains("key_path") && item["key_path"].is_string()) rv.key_path = item["key_path"].get<std::string>();
                if (item.contains("value_name") && item["value_name"].is_string()) rv.value_name = item["value_name"].get<std::string>();
                if (item.contains("type") && item["type"].is_number_unsigned()) rv.type = item["type"].get<uint32_t>();
                if (item.contains("data") && item["data"].is_string()) rv.data = item["data"].get<std::string>();
                snapshot->registry_data_.push_back(std::move(rv));
            }
        }

        // disk_info
        if (j.contains("disk_info") && j["disk_info"].is_array()) {
            snapshot->disk_info_.clear();
            for (const auto &item : j["disk_info"]) {
                DiskInfo di;
                if (item.contains("drive_letter") && item["drive_letter"].is_string()) di.drive_letter = item["drive_letter"].get<std::string>();
                if (item.contains("total_size") && item["total_size"].is_number_unsigned()) di.total_size = item["total_size"].get<uint64_t>();
                if (item.contains("free_space") && item["free_space"].is_number_unsigned()) di.free_space = item["free_space"].get<uint64_t>();
                if (item.contains("used_space") && item["used_space"].is_number_unsigned()) di.used_space = item["used_space"].get<uint64_t>();
                snapshot->disk_info_.push_back(std::move(di));
            }
        }

        // process_info
        if (j.contains("process_info") && j["process_info"].is_array()) {
            snapshot->process_info_.clear();
            for (const auto &item : j["process_info"]) {
                ProcessInfo pi{};
                if (item.contains("pid") && item["pid"].is_number_unsigned()) pi.pid = item["pid"].get<uint32_t>();
                if (item.contains("name") && item["name"].is_string()) pi.name = item["name"].get<std::string>();
                if (item.contains("cpu_usage") && item["cpu_usage"].is_number()) pi.cpu_usage = item["cpu_usage"].get<double>();
                if (item.contains("memory_usage") && item["memory_usage"].is_number_unsigned()) pi.memory_usage = item["memory_usage"].get<uint64_t>();
                if (item.contains("executable_path") && item["executable_path"].is_string()) pi.executable_path = item["executable_path"].get<std::string>();
                if (item.contains("private_bytes") && item["private_bytes"].is_number_unsigned()) pi.private_bytes = item["private_bytes"].get<uint64_t>();
                if (item.contains("handle_count") && item["handle_count"].is_number_unsigned()) pi.handle_count = item["handle_count"].get<uint32_t>();
                snapshot->process_info_.push_back(std::move(pi));
            }
        }

        // system_memory
        if (j.contains("system_memory") && j["system_memory"].is_object()) {
            snapshot->system_memory_ = SystemMemoryInfo{};
            auto &m = j["system_memory"];
            if (m.contains("total_physical") && m["total_physical"].is_number_unsigned()) snapshot->system_memory_.total_physical = m["total_physical"].get<uint64_t>();
            if (m.contains("avail_physical") && m["avail_physical"].is_number_unsigned()) snapshot->system_memory_.avail_physical = m["avail_physical"].get<uint64_t>();
            if (m.contains("total_pagefile") && m["total_pagefile"].is_number_unsigned()) snapshot->system_memory_.total_pagefile = m["total_pagefile"].get<uint64_t>();
            if (m.contains("avail_pagefile") && m["avail_pagefile"].is_number_unsigned()) snapshot->system_memory_.avail_pagefile = m["avail_pagefile"].get<uint64_t>();
        }

    } catch (const std::exception &ex) {
        // 解析错误：返回空快照或可考虑抛出异常。这里选择返回部分/默认填充的 snapshot
        // 若项目使用日志，可记录 ex.what()
    }

    return snapshot;
}

} // namespace snapshot