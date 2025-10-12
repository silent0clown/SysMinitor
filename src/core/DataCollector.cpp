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
// #include <processthreadsapi.h> // For QueryFullProcessImageNameA

#pragma comment(lib, "pdh.lib")

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

double DataCollector::calculate_process_cpu_usage(uint32_t pid) {
    // 简化实现 - 实际应该使用PDH库获取准确的CPU使用率
    // 这里返回一个模拟值
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(0.0, 5.0);
    
    return dis(gen);
}


} // namespace snapshot