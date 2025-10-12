#include "DataCollector.h"
#include "../utils/AsyncLogger.h"
#include <windows.h>
#include <psapi.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <tchar.h>
#include <tlhelp32.h>
#include <algorithm>
#include <random>
#include <thread>
#include <chrono>
// #include <processthreadsapi.h> // For QueryFullProcessImageNameA

#pragma comment(lib, "pdh.lib")

static uint64_t filetime_to_uint64(const FILETIME &ft) {
    return (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
}

namespace snapshot {

DataCollector::DataCollector() : last_system_cpu_time_(0), processor_count_(0) {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    processor_count_ = sysInfo.dwNumberOfProcessors;
}

std::shared_ptr<SystemSnapshot> DataCollector::collect_snapshot(const std::string& name) {
    // auto logger = AsyncLogger::get_instance();
    LOG_INFO("开始采集系统快照: {}", name);
    
    auto snapshot = std::make_shared<SystemSnapshot>(name);
    
    try {
        LOG_INFO("采集注册表数据...");
        auto registry_data = collect_registry_data();
        snapshot->set_registry_data(std::move(registry_data));
        
        LOG_INFO("采集磁盘信息...");
        auto disk_info = collect_disk_info();
        snapshot->set_disk_info(std::move(disk_info));
        
        LOG_INFO("采集系统内存信息...");
        auto system_memory = collect_system_memory_info();
        snapshot->set_system_memory(system_memory);

        LOG_INFO("初始化 CPU 计数器以便采样...");
        // 初始化 CPU 计数基线，然后等待一小段时间再采样进程，
        // 以便 calculate_process_cpu_usage 能基于差分计算出有效值。
        initialize_cpu_counters();
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        LOG_INFO("采集进程信息...");
        auto process_info = collect_process_info();
        snapshot->set_process_info(std::move(process_info));
        
        LOG_INFO("系统快照采集完成: {}", name);
    } catch (const std::exception& e) {
        LOG_ERROR("采集系统快照时发生错误: {}", e.what());
        throw;
    }
    
    return snapshot;
}

std::vector<RegistryValue> DataCollector::collect_registry_data() {
    std::vector<RegistryValue> results;
    // auto logger = AsyncLogger::get_instance();
    
    // 预定义的关键注册表路径
    std::vector<std::string> key_paths = {
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
        "SYSTEM\\CurrentControlSet\\Services",
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"
    };
    
    for (const auto& path : key_paths) {
        try {
            LOG_DEBUG("采集注册表路径: HKEY_LOCAL_MACHINE\\{}", path);
            collect_registry_key("HKEY_LOCAL_MACHINE\\" + path, results);
        } catch (const std::exception& e) {
            LOG_WARN("无法采集注册表路径 {}: {}", path, e.what());
        }
    }
    
    return results;
}

void DataCollector::collect_registry_key(const std::string& key_path, std::vector<RegistryValue>& results) {
    HKEY hKey;
    std::string actual_path = key_path.substr(key_path.find('\\') + 1);
    HKEY hRoot = (key_path.find("HKEY_LOCAL_MACHINE") == 0) ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
    
    LONG result = RegOpenKeyExA(hRoot, actual_path.c_str(), 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) {
        throw std::runtime_error("无法打开注册表键: " + key_path);
    }
    
    char value_name[16383];
    DWORD value_name_size;
    DWORD value_type;
    BYTE value_data[16383];
    DWORD value_data_size;
    DWORD index = 0;
    
    while (true) {
        value_name_size = sizeof(value_name);
        value_data_size = sizeof(value_data);
        
        result = RegEnumValueA(hKey, index, value_name, &value_name_size,
                              nullptr, &value_type, value_data, &value_data_size);
        
        if (result == ERROR_NO_MORE_ITEMS) {
            break;
        }
        
        if (result == ERROR_SUCCESS) {
            RegistryValue reg_value;
            reg_value.key_path = key_path;
            reg_value.value_name = value_name;
            reg_value.type = value_type;
            
            // 简化数据处理，实际应根据类型进行适当转换
            if (value_type == REG_SZ || value_type == REG_EXPAND_SZ) {
                reg_value.data = std::string(reinterpret_cast<char*>(value_data));
            } else if (value_type == REG_DWORD) {
                DWORD data = *reinterpret_cast<DWORD*>(value_data);
                reg_value.data = std::to_string(data);
            } else {
                reg_value.data = "[Binary Data]";
            }
            
            results.push_back(reg_value);
        }
        
        index++;
    }
    
    RegCloseKey(hKey);
}


std::vector<DiskInfo> DataCollector::collect_disk_info() {
    std::vector<DiskInfo> disks;
    // auto logger = AsyncLogger::get_instance();
    
    DWORD drives = GetLogicalDrives();
    for (char drive = 'A'; drive <= 'Z'; drive++) {
        if (drives & (1 << (drive - 'A'))) {
            std::string root_path = std::string(1, drive) + ":\\";
            UINT drive_type = GetDriveTypeA(root_path.c_str());
            
            if (drive_type == DRIVE_FIXED || drive_type == DRIVE_REMOVABLE) {
                ULARGE_INTEGER total_bytes, free_bytes, available_bytes;
                
                if (GetDiskFreeSpaceExA(root_path.c_str(), &available_bytes, &total_bytes, &free_bytes)) {
                    DiskInfo info;
                    info.drive_letter = std::string(1, drive);
                    info.total_size = total_bytes.QuadPart;
                    info.free_space = free_bytes.QuadPart;
                    info.used_space = total_bytes.QuadPart - free_bytes.QuadPart;
                    
                    disks.push_back(info);
                    LOG_DEBUG("采集磁盘信息: {} - Total: {}GB, Free: {}GB", 
                                 info.drive_letter,
                                 info.total_size / (1024*1024*1024),
                                 info.free_space / (1024*1024*1024));
                }
            }
        }
    }
    
    return disks;
}

std::vector<ProcessInfo> DataCollector::collect_process_info() {
    std::vector<ProcessInfo> processes;
    // auto logger = AsyncLogger::get_instance();
    
    // 获取进程快照
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        LOG_ERROR("无法创建进程快照");
        return processes;
    }
    
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    if (!Process32First(hProcessSnap, &pe32)) {
        LOG_ERROR("无法获取第一个进程");
        CloseHandle(hProcessSnap);
        return processes;
    }
    
    do {
        ProcessInfo info;
        info.pid = pe32.th32ProcessID;
        info.name = pe32.szExeFile;
        
        // 打开进程获取详细信息
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
        if (hProcess) {
            // 获取内存信息
            PROCESS_MEMORY_COUNTERS pmc;
            if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                info.memory_usage = pmc.WorkingSetSize;
                // 如果可用，使用扩展结构获取 PrivateUsage
                PROCESS_MEMORY_COUNTERS_EX pmce;
                if (GetProcessMemoryInfo(hProcess, reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmce), sizeof(pmce))) {
                    info.private_bytes = pmce.PrivateUsage;
                } else {
                    info.private_bytes = 0;
                }
            }
            
            // 获取CPU使用率
            info.cpu_usage = calculate_process_cpu_usage(pe32.th32ProcessID);
            
            // 获取可执行文件路径
            char path[MAX_PATH];
            DWORD path_size = MAX_PATH;
            if (QueryFullProcessImageNameA(hProcess, 0, path, &path_size)) {
                info.executable_path = path;
            }
            
            CloseHandle(hProcess);
        }
        
        processes.push_back(info);
        
    } while (Process32Next(hProcessSnap, &pe32));
    
    CloseHandle(hProcessSnap);
    LOG_INFO("采集了 {} 个进程信息", processes.size());
    return processes;
}

SystemMemoryInfo DataCollector::collect_system_memory_info() {
    SystemMemoryInfo memInfo = {};
    MEMORYSTATUSEX mse;
    mse.dwLength = sizeof(mse);
    if (GlobalMemoryStatusEx(&mse)) {
        memInfo.total_physical = mse.ullTotalPhys;
        memInfo.avail_physical = mse.ullAvailPhys;
        memInfo.total_pagefile = mse.ullTotalPageFile;
        memInfo.avail_pagefile = mse.ullAvailPageFile;
    }
    return memInfo;
}

double DataCollector::calculate_process_cpu_usage(uint32_t pid) {
    // 实际实现：使用 GetProcessTimes 与 GetSystemTimes 的差分法计算百分比
    // 打开进程以获取时间信息
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!hProcess) {
        // 无权限或进程已结束
        return 0.0;
    }

    FILETIME ftCreation, ftExit, ftKernel, ftUser;
    if (!GetProcessTimes(hProcess, &ftCreation, &ftExit, &ftKernel, &ftUser)) {
        CloseHandle(hProcess);
        return 0.0;
    }

    uint64_t procTime = filetime_to_uint64(ftKernel) + filetime_to_uint64(ftUser);

    // 获取系统时间（kernel + user）
    FILETIME sysIdle, sysKernel, sysUser;
    if (!GetSystemTimes(&sysIdle, &sysKernel, &sysUser)) {
        CloseHandle(hProcess);
        return 0.0;
    }

    uint64_t sysTime = filetime_to_uint64(sysKernel) + filetime_to_uint64(sysUser);

    double cpuPercent = 0.0;

    auto it = cpu_cache_.find(pid);
    if (it != cpu_cache_.end()) {
        ProcessCpuData &prev = it->second;
        uint64_t prevProc = prev.last_time;
        uint64_t prevSys = prev.last_system_time;

        uint64_t deltaProc = 0;
        uint64_t deltaSys = 0;
        if (procTime >= prevProc) deltaProc = procTime - prevProc; else deltaProc = 0;
        if (sysTime >= prevSys) deltaSys = sysTime - prevSys; else deltaSys = 0;

        if (deltaSys > 0) {
            // FILETIME 的单位是 100ns ticks，比例相同可直接用
            cpuPercent = (double)deltaProc / (double)deltaSys * 100.0 * static_cast<double>(processor_count_);
        } else {
            cpuPercent = 0.0;
        }

        // 更新缓存
        prev.last_time = procTime;
        prev.last_system_time = sysTime;
    } else {
        // 首次见到该进程：初始化缓存项并返回 0（下一次将有基线）
        ProcessCpuData data;
        data.last_time = procTime;
        data.last_system_time = sysTime;
        cpu_cache_[pid] = data;
        cpuPercent = 0.0;
    }

    CloseHandle(hProcess);

    // clamp 值，防止极端读数（例如多核上理论上可能超过100）
    if (cpuPercent < 0.0) cpuPercent = 0.0;
    // 上限为 100 * processor_count_
    double maxPossible = 100.0 * static_cast<double>(processor_count_);
    if (cpuPercent > maxPossible) cpuPercent = maxPossible;

    return cpuPercent;
}

void DataCollector::initialize_cpu_counters() {
    // 初始化系统和进程 CPU 计数缓存，以便后续计算使用差值
    cpu_cache_.clear();

    // 获取系统总的 CPU 时间（User + Kernel + Idle）
    FILETIME idleTime, kernelTime, userTime;
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        uint64_t k = filetime_to_uint64(kernelTime);
        uint64_t u = filetime_to_uint64(userTime);
        // 合并 kernel + user 作为系统活动时间
        last_system_cpu_time_ = k + u;
    } else {
        last_system_cpu_time_ = 0;
    }

    // 枚举当前进程以初始化每个进程的时间戳
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(hProcessSnap, &pe32)) {
        CloseHandle(hProcessSnap);
        return;
    }

    do {
        uint32_t pid = pe32.th32ProcessID;
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
        if (hProcess) {
            FILETIME ftCreation, ftExit, ftKernel, ftUser;
            if (GetProcessTimes(hProcess, &ftCreation, &ftExit, &ftKernel, &ftUser)) {
                uint64_t kt = filetime_to_uint64(ftKernel);
                uint64_t ut = filetime_to_uint64(ftUser);
                ProcessCpuData data;
                data.last_time = kt + ut;
                data.last_system_time = last_system_cpu_time_;
                cpu_cache_[pid] = data;
            }
            CloseHandle(hProcess);
        }
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
}

} // namespace snapshot