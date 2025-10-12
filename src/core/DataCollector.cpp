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
    LOG_INFO("��ʼ�ɼ�ϵͳ����: {}", name);
    
    auto snapshot = std::make_shared<SystemSnapshot>(name);
    
    try {
        LOG_INFO("�ɼ�ע�������...");
        auto registry_data = collect_registry_data();
        snapshot->set_registry_data(std::move(registry_data));
        
        LOG_INFO("�ɼ�������Ϣ...");
        auto disk_info = collect_disk_info();
        snapshot->set_disk_info(std::move(disk_info));
        
        LOG_INFO("�ɼ�������Ϣ...");
        auto process_info = collect_process_info();
        snapshot->set_process_info(std::move(process_info));
        
        LOG_INFO("ϵͳ���ղɼ����: {}", name);
    } catch (const std::exception& e) {
        LOG_ERROR("�ɼ�ϵͳ����ʱ��������: {}", e.what());
        throw;
    }
    
    return snapshot;
}

std::vector<RegistryValue> DataCollector::collect_registry_data() {
    std::vector<RegistryValue> results;
    // auto logger = AsyncLogger::get_instance();
    
    // Ԥ����Ĺؼ�ע���·��
    std::vector<std::string> key_paths = {
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
        "SYSTEM\\CurrentControlSet\\Services",
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"
    };
    
    for (const auto& path : key_paths) {
        try {
            LOG_DEBUG("�ɼ�ע���·��: HKEY_LOCAL_MACHINE\\{}", path);
            collect_registry_key("HKEY_LOCAL_MACHINE\\" + path, results);
        } catch (const std::exception& e) {
            LOG_WARN("�޷��ɼ�ע���·�� {}: {}", path, e.what());
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
        throw std::runtime_error("�޷���ע����: " + key_path);
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
            
            // �����ݴ���ʵ��Ӧ�������ͽ����ʵ�ת��
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
                    LOG_DEBUG("�ɼ�������Ϣ: {} - Total: {}GB, Free: {}GB", 
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
    
    // ��ȡ���̿���
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        LOG_ERROR("�޷��������̿���");
        return processes;
    }
    
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    if (!Process32First(hProcessSnap, &pe32)) {
        LOG_ERROR("�޷���ȡ��һ������");
        CloseHandle(hProcessSnap);
        return processes;
    }
    
    do {
        ProcessInfo info;
        info.pid = pe32.th32ProcessID;
        info.name = pe32.szExeFile;
        
        // �򿪽��̻�ȡ��ϸ��Ϣ
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
        if (hProcess) {
            // ��ȡ�ڴ���Ϣ
            PROCESS_MEMORY_COUNTERS pmc;
            if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                info.memory_usage = pmc.WorkingSetSize;
            }
            
            // ��ȡCPUʹ����
            info.cpu_usage = calculate_process_cpu_usage(pe32.th32ProcessID);
            
            // ��ȡ��ִ���ļ�·��
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
    LOG_INFO("�ɼ��� {} ��������Ϣ", processes.size());
    return processes;
}

double DataCollector::calculate_process_cpu_usage(uint32_t pid) {
    // ��ʵ�� - ʵ��Ӧ��ʹ��PDH���ȡ׼ȷ��CPUʹ����
    // ���ﷵ��һ��ģ��ֵ
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(0.0, 5.0);
    
    return dis(gen);
}


} // namespace snapshot