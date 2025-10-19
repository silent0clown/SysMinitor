#include "registry_monitor.h"
#define NOMAXMIN  // 防止 Windows.h 定义 min/max 宏，避免与 STL 冲突
#include <windows.h>
#include <iostream>
#include "../../utils/registry_encode.h"
#include "../../utils/util_time.h"
// 全局变量，保存本次运行创建的备份目录
char g_backupDir[MAX_PATH] = { 0 };

namespace sysmonitor {

using namespace encoding;

/**
 * @brief RegistryMonitor 构造函数
 * 初始化注册表监控器，设置要监控的注册表路径
 */
RegistryMonitor::RegistryMonitor() {}

/**
 * @brief 获取注册表快照
 * @return RegistrySnapshot 包含系统信息、软件信息、网络配置和自启动项的完整快照
 * 
 * 该方法收集系统关键注册表信息，包括：
 * - 系统信息键
 * - 已安装软件
 * - 网络配置
 * - 自启动项目
 * 所有字符串数据都会转换为 UTF-8 编码以便统一处理
 */
RegistrySnapshot RegistryMonitor::GetRegistrySnapshot() {
    RegistrySnapshot snapshot;
    snapshot.timestamp = GET_LOCAL_TIME_MS();  // 使用系统启动后的毫秒数作为时间戳
    
    try {
        // 收集各类注册表信息
        snapshot.systemInfoKeys = GetSystemInfoKeys();
        snapshot.softwareKeys = GetInstalledSoftware();
        snapshot.networkKeys = GetNetworkConfig();
        snapshot.autoStartKeys = GetAutoStartItems();

        // 将所有注册表键值数据转换为 UTF-8 编码
        for (auto& key : snapshot.systemInfoKeys) {
            key.convertToUTF8();
        }
        for (auto& key : snapshot.softwareKeys) {
            key.convertToUTF8();
        }
        for (auto& key : snapshot.networkKeys) {
            key.convertToUTF8();
        }
        for (auto& key : snapshot.autoStartKeys) {
            key.convertToUTF8();
        }
        
        // 输出快照统计信息
        std::cout << "Registry snapshot retrieved successfully - "
                  << "System info: " << snapshot.systemInfoKeys.size() << " items, "
                  << "Software info: " << snapshot.softwareKeys.size() << " items, "
                  << "Network config: " << snapshot.networkKeys.size() << " items, "
                  << "Auto-start items: " << snapshot.autoStartKeys.size() << " items" << std::endl;
                  
    } catch (const std::exception& e) {
        std::cerr << "Exception occurred while getting registry snapshot: " << e.what() << std::endl;
    }
    
    return snapshot;
}

/**
 * @brief 获取自启动项信息
 * @return std::vector<RegistryKey> 包含所有自启动注册表键的向量
 * 
 * 遍历预定义的自启动路径，收集以下类型的自启动项：
 * - 注册表 Run/RunOnce 键
 * - 系统服务
 * - 计划任务
 * - 浏览器帮助对象
 * - Winlogon 相关设置
 * - 组策略设置
 */
std::vector<RegistryKey> RegistryMonitor::GetAutoStartItems() {
    std::vector<RegistryKey> keys;
    
    // 使用成员变量 autoStartPaths 遍历所有自启动路径
    for (const auto& [root, path] : autoStartPaths) {
        try {
            RegistryKey key = GetRegistryKey(root, path);
            
            // 特殊处理服务项，只获取自动启动类型的服务
            if (path.find("Services") != std::string::npos) {
                key = FilterAutoStartServices(key);
            }
            
            // 只有当键包含值或子键时才添加到结果中
            if (!key.values.empty() || !key.subkeys.empty()) {
                // 添加自启动项类型标识符
                key.autoStartType = GetAutoStartType(root, path);
                keys.push_back(key);
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to get auto-start registry: " << path << " - " << e.what() << std::endl;
        }
    }
    
    return keys;
}

/**
 * @brief 过滤自动启动的服务
 * @param servicesKey 包含所有服务的注册表键
 * @return RegistryKey 只包含自动启动服务的过滤后键
 * 
 * 服务启动类型说明：
 * - 2 = 自动启动 (Auto)
 * - 3 = 手动启动 (Manual) 
 * - 4 = 禁用 (Disabled)
 * 只保留启动类型为 2（自动启动）的服务
 */
RegistryKey RegistryMonitor::FilterAutoStartServices(const RegistryKey& servicesKey) {
    RegistryKey filteredKey;
    filteredKey.path = servicesKey.path;
    filteredKey.autoStartType = "AutoStartServices";
    
    // 遍历所有服务子键
    for (const auto& subkeyName : servicesKey.subkeys) {
        try {
            // 构建完整的服务注册表路径
            std::string servicePath = servicesKey.path.substr(servicesKey.path.find("\\") + 1) + "\\" + subkeyName;
            RegistryKey serviceKey = GetRegistryKey(HKEY_LOCAL_MACHINE, servicePath);
            
            // 检查服务启动类型
            bool isAutoStart = false;
            for (const auto& value : serviceKey.values) {
                if (value.name == "Start") {
                    // Start 值: 2=自动, 3=手动, 4=禁用
                    if (value.data == "2" || value.data.find("Auto") != std::string::npos) {
                        isAutoStart = true;
                    }
                    break;
                }
            }
            
            if (isAutoStart) {
                // 只添加自动启动服务的子键名称
                filteredKey.subkeys.push_back(subkeyName);
                
                // 将服务关键信息添加到值列表中
                RegistryValue serviceInfo;
                serviceInfo.name = subkeyName;
                serviceInfo.type = "ServiceInfo";
                
                // 构建服务信息字符串
                std::string info;
                for (const auto& value : serviceKey.values) {
                    if (value.name == "DisplayName" || value.name == "Description" || 
                        value.name == "ImagePath" || value.name == "Start") {
                        info += value.name + "=" + value.data + "; ";
                    }
                }
                serviceInfo.data = info;
                filteredKey.values.push_back(serviceInfo);
            }
        } catch (const std::exception& e) {
            // 跳过无法访问的服务
            continue;
        }
    }
    
    return filteredKey;
}

/**
 * @brief 获取自启动项类型
 * @param root 注册表根键
 * @param path 注册表路径
 * @return std::string 自启动项类型标识符
 * 
 * 根据注册表路径判断自启动项的类型：
 * - Run/RunOnce: 用户或计算机级别的运行项
 * - Services: 自动启动的系统服务
 * - TaskCache: 计划任务
 * - Browser Helper Objects: 浏览器帮助对象
 * - Winlogon: 系统登录相关设置
 * - Policies: 组策略设置
 * - Wow6432Node: 32位兼容性设置
 */
std::string RegistryMonitor::GetAutoStartType(HKEY root, const std::string& path) {
    std::string type;
    
    if (path.find("Run") != std::string::npos) {
        type = (root == HKEY_CURRENT_USER) ? "UserRun" : "MachineRun";
    } else if (path.find("RunOnce") != std::string::npos) {
        type = (root == HKEY_CURRENT_USER) ? "UserRunOnce" : "MachineRunOnce";
    } else if (path.find("Services") != std::string::npos) {
        type = "AutoStartServices";
    } else if (path.find("TaskCache") != std::string::npos) {
        type = "ScheduledTasks";
    } else if (path.find("Browser Helper Objects") != std::string::npos) {
        type = "BrowserHelpers";
    } else if (path.find("Winlogon") != std::string::npos) {
        type = "Winlogon";
    } else if (path.find("Policies") != std::string::npos) {
        type = (root == HKEY_CURRENT_USER) ? "UserPolicies" : "MachinePolicies";
    } else if (path.find("Wow6432Node") != std::string::npos) {
        type = "Wow64Compat";
    } else {
        type = "Other";
    }
    
    return type;
}

/**
 * @brief 获取系统信息注册表键
 * @return std::vector<RegistryKey> 包含系统信息的注册表键向量
 * 
 * 收集以下系统信息：
 * - Windows 版本信息
 * - 系统配置
 * - 硬件信息
 * - 系统设置
 */
std::vector<RegistryKey> RegistryMonitor::GetSystemInfoKeys() {
    std::vector<RegistryKey> keys;
    
    for (const auto& path : systemInfoPaths) {
        try {
            RegistryKey key = GetRegistryKey(HKEY_LOCAL_MACHINE, path);
            key.category = "system";
            if (!key.values.empty() || !key.subkeys.empty()) {
                keys.push_back(key);
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to get system info registry: " << path << " - " << e.what() << std::endl;
        }
    }
    
    return keys;
}

/**
 * @brief 获取已安装软件信息
 * @return std::vector<RegistryKey> 包含软件信息的注册表键向量
 * 
 * 收集以下软件信息：
 * - 机器级别安装的软件 (HKEY_LOCAL_MACHINE)
 * - 当前用户安装的软件 (HKEY_CURRENT_USER)
 * - 32位和64位软件信息
 */
std::vector<RegistryKey> RegistryMonitor::GetInstalledSoftware() {
    std::vector<RegistryKey> keys;
    
    // 获取机器级别的软件信息
    for (const auto& path : softwarePaths) {
        try {
            RegistryKey key = GetRegistryKey(HKEY_LOCAL_MACHINE, path);
            key.category = "software";
            if (!key.values.empty() || !key.subkeys.empty()) {
                keys.push_back(key);
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to get software info registry: " << path << " - " << e.what() << std::endl;
        }
    }
    
    // 获取当前用户的软件信息
    try {
        RegistryKey userKey = GetRegistryKey(HKEY_CURRENT_USER, 
                                           "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall");
        userKey.category = "software_user";
        if (!userKey.values.empty() || !userKey.subkeys.empty()) {
            keys.push_back(userKey);
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to get user software info registry: " << e.what() << std::endl;
    }
    
    return keys;
}

/**
 * @brief 获取网络配置信息
 * @return std::vector<RegistryKey> 包含网络配置的注册表键向量
 * 
 * 收集以下网络配置：
 * - TCP/IP 设置
 * - 网络适配器配置
 * - 网络服务和协议设置
 */
std::vector<RegistryKey> RegistryMonitor::GetNetworkConfig() {
    std::vector<RegistryKey> keys;
    
    for (const auto& path : networkPaths) {
        try {
            RegistryKey key = GetRegistryKey(HKEY_LOCAL_MACHINE, path);
            key.category = "network";
            if (!key.values.empty() || !key.subkeys.empty()) {
                keys.push_back(key);
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to get network config registry: " << path << " - " << e.what() << std::endl;
        }
    }
    
    return keys;
}

/**
 * @brief 获取指定注册表键的完整信息
 * @param root 注册表根键
 * @param subKey 子键路径
 * @return RegistryKey 包含键值对和子键信息的注册表键对象
 * 
 * 该方法执行以下操作：
 * 1. 打开注册表键
 * 2. 枚举所有值（包括默认值）
 * 3. 枚举所有子键名称
 * 4. 安全地处理各种数据类型和编码
 */
RegistryKey RegistryMonitor::GetRegistryKey(HKEY root, const std::string& subKey) {
    RegistryKey key;
    key.path = RegistryEncodingUtils::HKEYToString(root) + "\\" + subKey;
    
    HKEY hKey;
    if (!SafeOpenKey(root, subKey, hKey)) {
        return key;  // 返回空的 RegistryKey 对象
    }
    
    // 枚举注册表值
    DWORD valueCount, maxValueNameLen, maxValueDataLen;
    LONG result = RegQueryInfoKeyA(hKey, NULL, NULL, NULL, NULL, NULL, NULL,
                                 &valueCount, &maxValueNameLen, &maxValueDataLen,
                                 NULL, NULL);
    
    if (result == ERROR_SUCCESS && valueCount > 0) {
        for (DWORD i = 0; i < valueCount; i++) {
            std::vector<char> valueName(maxValueNameLen + 1, 0);
            std::vector<BYTE> valueData;
            DWORD valueNameSize = maxValueNameLen + 1;
            DWORD valueDataSize = 0;
            DWORD valueType;
            
            // 第一次调用获取值数据大小
            result = RegEnumValueA(hKey, i, valueName.data(), &valueNameSize,
                                 NULL, &valueType, NULL, &valueDataSize);
            
            if (result == ERROR_SUCCESS) {
                try {
                    valueData.resize(valueDataSize + 2, 0);  // 额外空间用于字符串终止符
                    // 第二次调用实际读取值数据
                    result = RegEnumValueA(hKey, i, valueName.data(), &valueNameSize,
                                         NULL, &valueType, valueData.data(), &valueDataSize);
                    
                    if (result == ERROR_SUCCESS) {
                        RegistryValue value;
                        value.name = (valueName[0] == '\0') ? "(Default)" : 
                                    RegistryEncodingUtils::SafeString(valueName.data());
                        value.type = RegistryEncodingUtils::RegistryTypeToString(valueType);
                        value.data = RegistryEncodingUtils::RegistryDataToString(valueData.data(), valueDataSize, valueType);
                        value.size = valueDataSize;
                        
                        key.values.push_back(value);
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Exception occurred while processing registry value: " << e.what() << std::endl;
                }
            }
        }
    }
    
    // 枚举子键
    DWORD subkeyCount, maxSubkeyLen;
    result = RegQueryInfoKeyA(hKey, NULL, NULL, NULL, &subkeyCount, &maxSubkeyLen,
                            NULL, NULL, NULL, NULL, NULL, NULL);
    
    if (result == ERROR_SUCCESS && subkeyCount > 0) {
        std::vector<char> subkeyName(maxSubkeyLen + 1, 0);
        
        for (DWORD i = 0; i < subkeyCount; i++) {
            DWORD subkeyNameSize = maxSubkeyLen + 1;
            result = RegEnumKeyExA(hKey, i, subkeyName.data(), &subkeyNameSize,
                                 NULL, NULL, NULL, NULL);
            
            if (result == ERROR_SUCCESS) {
                key.subkeys.push_back(RegistryEncodingUtils::SafeString(subkeyName.data()));
            }
        }
    }
    
    SafeCloseKey(hKey);
    return key;
}

/**
 * @brief 安全地打开注册表键
 * @param root 注册表根键
 * @param subKey 子键路径
 * @param hKey 输出参数，成功时包含打开的键句柄
 * @return bool 成功返回 true，失败返回 false
 * 
 * 使用 KEY_READ 权限打开注册表键，失败时输出错误信息
 */
bool RegistryMonitor::SafeOpenKey(HKEY root, const std::string& subKey, HKEY& hKey) {
    LONG result = RegOpenKeyExA(root, subKey.c_str(), 0, KEY_READ, &hKey);
    
    if (result != ERROR_SUCCESS) {
        std::cerr << "Unable to open registry key: " << RegistryEncodingUtils::HKEYToString(root) << "\\" << subKey 
                  << " - Error: " << result << std::endl;
        return false;
    }
    
    return true;
}

/**
 * @brief 安全地关闭注册表键
 * @param hKey 要关闭的注册表键句柄
 * 
 * 检查句柄有效性后关闭注册表键，避免重复关闭
 */
void RegistryMonitor::SafeCloseKey(HKEY hKey) {
    if (hKey) {
        RegCloseKey(hKey);
    }
}

/**
 * @brief 安全地查询注册表值
 * @param fullPath 完整的注册表路径，格式："HKEY_XXX\Path\To\Key\ValueName"
 * @param value 输出参数，成功时包含查询到的值信息
 * @return bool 成功返回 true，失败返回 false
 * 
 * 从完整路径解析根键、键路径和值名称，然后查询对应的注册表值
 */
bool RegistryMonitor::SafeQueryValue(const std::string& fullPath, RegistryValue& value) {
    // 解析根键部分
    size_t pos = fullPath.find('\\');
    if (pos == std::string::npos) {
        return false;
    }
    
    std::string rootStr = fullPath.substr(0, pos);
    std::string restPath = fullPath.substr(pos + 1);
    
    // 将字符串根键转换为 HKEY 常量
    HKEY hRoot;
    if (rootStr == "HKEY_LOCAL_MACHINE") hRoot = HKEY_LOCAL_MACHINE;
    else if (rootStr == "HKEY_CURRENT_USER") hRoot = HKEY_CURRENT_USER;
    else if (rootStr == "HKEY_CLASSES_ROOT") hRoot = HKEY_CLASSES_ROOT;
    else if (rootStr == "HKEY_USERS") hRoot = HKEY_USERS;
    else if (rootStr == "HKEY_CURRENT_CONFIG") hRoot = HKEY_CURRENT_CONFIG;
    else return false;
    
    // 分离键路径和值名称
    pos = restPath.find_last_of('\\');
    if (pos == std::string::npos) {
        return false;
    }
    
    std::string keyPath = restPath.substr(0, pos);
    std::string valueName = restPath.substr(pos + 1);
    
    HKEY hKey;
    if (RegOpenKeyExA(hRoot, keyPath.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false;
    }
    
    // 查询值数据
    DWORD type, size;
    if (RegQueryValueExA(hKey, valueName.c_str(), NULL, &type, NULL, &size) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return false;
    }
    
    std::vector<BYTE> data(size);
    if (RegQueryValueExA(hKey, valueName.c_str(), NULL, &type, data.data(), &size) == ERROR_SUCCESS) {
        value.name = valueName;
        value.type = RegistryEncodingUtils::RegistryTypeToString(type);
        value.data = RegistryEncodingUtils::RegistryDataToString(data.data(), size, type);
        value.size = size;
    }
    
    RegCloseKey(hKey);
    return true;
}

// 所有注册表路径的集合
const std::vector<std::string> allRegistryPaths = {
    // 系统信息路径
    "HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
    "HKLM\\HARDWARE\\DESCRIPTION\\System",
    "HKLM\\SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ComputerName",
    "HKLM\\SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation",
    
    // 软件相关路径
    "HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
    "HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
    "HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
    
    // 网络相关路径
    "HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters",
    "HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces",
    "HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkList\\Signatures\\Unmanaged",
    
    // 自启动项路径
    "HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
    "HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
    "HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
    "HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
    "HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnceEx",
    "HKLM\\SYSTEM\\CurrentControlSet\\Services",
    "HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree",
    "HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects",
    "HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\UserInit",
    "HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\Shell",
    "HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run",
    "HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run",
    "HKLM\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Run",
    "HKLM\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\RunOnce"
};

//added by Liyongheng 20251017
int RegistryMonitor::SaveReg() {
    printf("=== Registry Backup Tool ===\n\n");

    // 获取 exe 所在目录
    char exeDir[MAX_PATH];
    if (!GetExeDirectory(exeDir, sizeof(exeDir))) {
        printf("Error: Unable to get program directory!\n");
        return 1;
    }

    printf("Program Directory: %s\n", exeDir);

    // 创建本次运行的时间戳备份目录
    if (!CreateBackupDirectory(exeDir, g_backupDir, sizeof(g_backupDir))) {
        printf("Error: Unable to create backup directory!\n");
        return 1;
    }

    printf("Backup Directory: %s\n\n", g_backupDir);

    // 备份所有注册表路径
    printf("Starting to backup all registry paths...\n\n");

    int successCount = 0;
    int totalCount = allRegistryPaths.size();

    for (int i = 0; i < totalCount; i++) {
        const std::string& registryPath = allRegistryPaths[i];
        printf("Backup Progress: %d/%d\n", i + 1, totalCount);

        if (ExportRegistryWithTimestamp(registryPath.c_str(), g_backupDir, i + 1)) {
            successCount++;
            printf("Successfully backed up: %s\n\n", registryPath.c_str());
        }
        else {
            printf("Failed to backup: %s\n\n", registryPath.c_str());
        }
    }

    printf("=== Backup Completed ===\n");
    printf("Successfully backed up: %d/%d registry paths\n", successCount, totalCount);
    printf("Backup files saved in: %s\n", g_backupDir);

    if (successCount == totalCount) {
        printf("All registry backups completed successfully!\n");
        return 0; 
    }
    else {
        printf("Some registry backups failed. Please check permissions or registry paths.\n");
        return -1;
    }
}

/**
 * @brief 获取 exe 程序所在目录
 * @param exeDir 存储目录路径的缓冲区
 * @param bufferSize 缓冲区大小
 * @return 成功返回TRUE，失败返回FALSE
 */
BOOL RegistryMonitor::GetExeDirectory(char* exeDir, size_t bufferSize) {
    // 获取 exe 文件的完整路径
    if (GetModuleFileNameA(NULL, exeDir, (DWORD)bufferSize) == 0) {
        return FALSE;
    }
    
    // 找到最后一个反斜杠，提取目录部分
    char* lastBackslash = strrchr(exeDir, '\\');
    if (lastBackslash != NULL) {
        *lastBackslash = '\0';  // 截断文件名，保留目录
    }
    
    return TRUE;
}

/**
 * @brief 创建以时间戳命名的备份目录
 * @param baseDir 基础目录（exe所在目录）
 * @param backupDir 存储创建的备份目录路径
 * @param bufferSize 缓冲区大小
 * @return 成功返回TRUE，失败返回FALSE
 */
BOOL RegistryMonitor::CreateBackupDirectory(LPCSTR baseDir, char* backupDir, size_t bufferSize) {
    char timestamp[32];
    
    // 获取当前时间戳字符串
    GetCurrentTimestamp(timestamp, sizeof(timestamp));
    
    // 组合备份目录路径：基础目录\备份_时间戳
    _snprintf_s(backupDir, bufferSize, _TRUNCATE,
               "%s\\Backup_%s", baseDir, timestamp);
    
    // 创建目录
    if (CreateDirectoryA(backupDir, NULL)) {
        printf("Created backup directory: %s\n", backupDir);
        return TRUE;
    } else {
        DWORD error = GetLastError();
        if (error == ERROR_ALREADY_EXISTS) {
            printf("Backup directory already exists: %s\n", backupDir);
            return TRUE;
        } else {
            printf("Error: Cannot create backup directory! Error code: %d\n", error);
            return FALSE;
        }
    }
}

/**
 * @brief 导出注册表到指定的备份目录
 * @param registryKey 要导出的注册表键
 * @param backupDir 备份目录路径
 * @return 成功返回TRUE，失败返回FALSE
 */
BOOL RegistryMonitor::ExportRegistryWithTimestamp(LPCSTR registryKey, LPCSTR backupDir,  int callCount) {
    char timestamp[32];
    char regName[256];
    char fileName[256];
    char fullPath[MAX_PATH];
    
    // 获取当前时间戳（用于文件名）
    GetCurrentTimestamp(timestamp, sizeof(timestamp));
    
    // 根据注册表键名自动生成注册表名称
    GenerateRegistryName(registryKey, regName, sizeof(regName));
    
    // 构建文件名：注册表名称_时间戳.reg
    _snprintf_s(fileName, sizeof(fileName), _TRUNCATE,
        "%d_%s_%s.reg", callCount, regName, timestamp);
    
    // 组合完整路径
    if (!CombinePath(fullPath, sizeof(fullPath), backupDir, fileName)) {
        printf("Error: Path combination failed!\n");
        return FALSE;
    }
    
    printf("Registry key: %s\n", registryKey);
    printf("Output file: %s\n", fileName);
    
    // 调用原有的保存函数
    return SaveRegistryWithRegExe(registryKey, fullPath);
}

/**
 * @brief 根据注册表键名自动生成合适的文件名
 * @param registryKey 注册表键
 * @param regName 存储生成名称的缓冲区
 * @param bufferSize 缓冲区大小
 */
void RegistryMonitor::GenerateRegistryName(LPCSTR registryKey, char* regName, size_t bufferSize) {
    const char* lastPart = registryKey;
    const char* temp = registryKey;
    char cleanName[256] = {0};
    int j = 0;
    
    // 找到最后一个反斜杠后的部分
    while (*temp) {
        if (*temp == '\\') {
            lastPart = temp + 1;
        }
        temp++;
    }
    
    // 如果最后一个部分是空的，使用整个键名
    if (*lastPart == '\0') {
        lastPart = registryKey;
    }
    
    // 清理名称：只保留字母、数字和下划线，其他字符替换为下划线
    for (int i = 0; lastPart[i] && j < (int)bufferSize - 1; i++) {
        if (isalnum((unsigned char)lastPart[i]) || lastPart[i] == '_') {
            cleanName[j++] = lastPart[i];
        } else {
            // 避免连续多个下划线
            if (j == 0 || cleanName[j-1] != '_') {
                cleanName[j++] = '_';
            }
        }
    }
    
    // 确保不以_结尾
    if (j > 0 && cleanName[j-1] == '_') {
        cleanName[j-1] = '\0';
    } else {
        cleanName[j] = '\0';
    }
    
    // 如果清理后的名称为空，使用默认名称
    if (strlen(cleanName) == 0) {
        strcpy_s(regName, bufferSize, "RegistryBackup");
    } else {
        strcpy_s(regName, bufferSize, cleanName);
    }
    
    // 特殊处理：如果是根键，使用根键名称
    if (strcmp(registryKey, "HKLM") == 0 || 
        strcmp(registryKey, "HKEY_LOCAL_MACHINE") == 0) {
        strcpy_s(regName, bufferSize, "HKLM_Full");
    }
    else if (strcmp(registryKey, "HKCU") == 0 || 
             strcmp(registryKey, "HKEY_CURRENT_USER") == 0) {
        strcpy_s(regName, bufferSize, "HKCU_Full");
    }
    else if (strcmp(registryKey, "HKCR") == 0 || 
             strcmp(registryKey, "HKEY_CLASSES_ROOT") == 0) {
        strcpy_s(regName, bufferSize, "HKCR_Full");
    }
    else if (strcmp(registryKey, "HKU") == 0 || 
             strcmp(registryKey, "HKEY_USERS") == 0) {
        strcpy_s(regName, bufferSize, "HKU_Full");
    }
}

/**
 * @brief 获取当前时间戳字符串
 * @param timestamp 存储时间戳的缓冲区
 * @param bufferSize 缓冲区大小
 */
void RegistryMonitor::GetCurrentTimestamp(char* timestamp, size_t bufferSize) {
    time_t now;
    struct tm timeinfo;
    
    time(&now);
    localtime_s(&timeinfo, &now);
    
    // 格式: YYYYMMDD_HHMMSS
    strftime(timestamp, bufferSize, "%Y%m%d_%H%M%S", &timeinfo);
}

/**
 * @brief 组合目录路径和文件名
 * @param fullPath 存储完整路径的缓冲区
 * @param bufferSize 缓冲区大小
 * @param dir 目录路径
 * @param fileName 文件名
 * @return 成功返回TRUE，失败返回FALSE
 */
BOOL RegistryMonitor::CombinePath(char* fullPath, size_t bufferSize, LPCSTR dir, LPCSTR fileName) {
    // 检查目录是否以反斜杠结尾
    size_t dirLen = strlen(dir);
    if (dirLen == 0) {
        // 如果目录为空，直接使用文件名
        _snprintf_s(fullPath, bufferSize, _TRUNCATE, "%s", fileName);
        return TRUE;
    }
    
    if (dir[dirLen - 1] == '\\') {
        // 目录已有反斜杠
        _snprintf_s(fullPath, bufferSize, _TRUNCATE, "%s%s", dir, fileName);
    } else {
        // 目录没有反斜杠，添加一个
        _snprintf_s(fullPath, bufferSize, _TRUNCATE, "%s\\%s", dir, fileName);
    }
    
    return TRUE;
}

/**
 * @brief 使用reg.exe导出注册表到指定文件
 * @param registryKey 注册表键
 * @param outputFile 输出文件路径
 * @return 成功返回TRUE，失败返回FALSE
 */
BOOL RegistryMonitor::SaveRegistryWithRegExe(LPCSTR registryKey, LPCSTR outputFile) {
    STARTUPINFOA si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    CHAR commandLine[512] = { 0 };
    DWORD exitCode = 0;
    BOOL success = FALSE;

    // 初始化 STARTUPINFO 结构
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;  // 隐藏窗口

    // 构建命令行：使用 reg.exe 导出注册表
    _snprintf_s(commandLine, sizeof(commandLine), _TRUNCATE,
        "reg.exe export \"%s\" \"%s\" /y",
        registryKey, outputFile);

    printf("Execute command: %s\n", commandLine);
    printf("Backing up registry, please wait...\n");

    // 创建进程
    success = CreateProcessA(
        NULL,               // 应用程序名（NULL表示使用命令行）
        commandLine,        // 命令行
        NULL,               // 进程安全属性
        NULL,               // 线程安全属性
        FALSE,              // 不继承句柄
        CREATE_NO_WINDOW,   // 创建标志：不创建窗口
        NULL,               // 环境块
        NULL,               // 当前目录
        &si,                // 启动信息
        &pi                 // 进程信息
    );

    if (!success) {
        DWORD error = GetLastError();
        printf("Create process failed! Error code: %d\n", error);

        if (error == ERROR_FILE_NOT_FOUND) {
            printf("Error: reg.exe not found\n");
        }
        else if (error == ERROR_ACCESS_DENIED) {
            printf("Error: Access denied, please run as administrator\n");
        }
        return FALSE;
    }

    // 等待进程完成（最多等待6min）
    DWORD waitResult = WaitForSingleObject(pi.hProcess, 6*60*1000);

    if (waitResult == WAIT_OBJECT_0) {
        if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
            if (exitCode == 0) {
                printf("Registry export successful!\n");
                success = TRUE;
            }
            else {
                printf("Registry export failed! Exit code: %d\n", exitCode);

                switch (exitCode) {
                case 1:
                    printf("Error: Cannot find specified registry key or file\n");
                    break;
                case 2:
                    printf("Error: Invalid parameter\n");
                    break;
                case 3:
                    printf("Error: Access denied\n");
                    break;
                case 4:
                    printf("Error: Data type not supported\n");
                    break;
                default:
                    printf("Please check if registry key path and file path are correct\n");
                    break;
                }
                success = FALSE;
            }
        }
        else {
            printf("Cannot get process exit code!\n");
            success = FALSE;
        }
    }
    else if (waitResult == WAIT_TIMEOUT) {
        printf("Operation timeout! Terminating process...\n");
        TerminateProcess(pi.hProcess, 1);
        success = FALSE;
    }
    else {
        printf("Error occurred while waiting for process!\n");
        success = FALSE;
    }

    // 关闭进程和线程句柄
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return success;
}

/**
 * @brief 获取所有备份文件夹和注册表文件信息
 * @param baseDir 基础目录（exe所在目录）
 * @return 包含所有备份信息的map，key为文件夹名称，value为备份信息
 */
std::map<std::string, BackupInfo> RegistryMonitor::GetAllBackupInfo() {
    WIN32_FIND_DATAA findData;
    HANDLE hFind;
    char searchPath[MAX_PATH];
    std::map<std::string, BackupInfo> backupInfo;
    
    char exeDir[MAX_PATH];
    if (!GetExeDirectory(exeDir, sizeof(exeDir))) {
        printf("错误: 无法获取程序所在目录！\n");
        return backupInfo;
    }
    // 构建搜索路径：baseDir\Backup_*
    _snprintf_s(searchPath, sizeof(searchPath), _TRUNCATE, 
               "%s\\Backup_*", exeDir);
    
    hFind = FindFirstFileA(searchPath, &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("No backup files\n");
        return backupInfo;
    }
    
    do {
        // 只处理目录，跳过文件
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // 跳过当前目录和上级目录
            if (strcmp(findData.cFileName, ".") == 0 || 
                strcmp(findData.cFileName, "..") == 0) {
                continue;
            }
            
            // 创建备份信息对象
            BackupInfo info;
            info.folderName = findData.cFileName;
            
            // 构建完整路径
            char folderPath[MAX_PATH];
            _snprintf_s(folderPath, sizeof(folderPath), _TRUNCATE,
                       "%s\\%s", exeDir, findData.cFileName);
            info.folderPath = folderPath;
            
            // 获取文件夹创建时间
            FILETIME ftCreate = findData.ftCreationTime;
            ULARGE_INTEGER ull;
            ull.LowPart = ftCreate.dwLowDateTime;
            ull.HighPart = ftCreate.dwHighDateTime;
            info.createTime = (time_t)((ull.QuadPart - 116444736000000000ULL) / 10000000ULL);
            
            // 获取该文件夹中的所有注册表文件
            char regSearchPath[MAX_PATH];
            WIN32_FIND_DATAA regFindData;
            HANDLE hRegFind;
            
            _snprintf_s(regSearchPath, sizeof(regSearchPath), _TRUNCATE, 
                       "%s\\*.reg", folderPath);
            
            hRegFind = FindFirstFileA(regSearchPath, &regFindData);
            if (hRegFind != INVALID_HANDLE_VALUE) {
                info.totalSize = 0; // 初始化总大小
                
                do {
                    // 只处理文件，跳过目录
                    if (!(regFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                        RegFileInfo regFile;
                        regFile.fileName = regFindData.cFileName;
                        
                        // 获取文件大小
                        ULARGE_INTEGER fileSize;
                        fileSize.LowPart = regFindData.nFileSizeLow;
                        fileSize.HighPart = regFindData.nFileSizeHigh;
                        regFile.fileSize = fileSize.QuadPart;
                        
                        // 添加到文件列表
                        info.regFiles.push_back(regFile);
                        
                        // 累加总大小
                        info.totalSize += regFile.fileSize;
                    }
                } while (FindNextFileA(hRegFind, &regFindData) != 0);
                
                FindClose(hRegFind);
            }
            
            // 将备份信息添加到map中
            backupInfo[info.folderName] = info;
        }
    } while (FindNextFileA(hFind, &findData) != 0);
    
    FindClose(hFind);
    return backupInfo;
}

/**
 * @brief 格式化文件大小，使其更易读
 * @param fileSize 文件大小（字节）
 * @return 格式化后的文件大小字符串
 */
std::string RegistryMonitor::FormatFileSize(long long fileSize) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unitIndex = 0;
    double size = static_cast<double>(fileSize);
    
    while (size >= 1024.0 && unitIndex < 3) {
        size /= 1024.0;
        unitIndex++;
    }
    
    char buffer[32];
    if (unitIndex == 0) {
        snprintf(buffer, sizeof(buffer), "%lld %s", fileSize, units[unitIndex]);
    } else {
        snprintf(buffer, sizeof(buffer), "%.2f %s", size, units[unitIndex]);
    }
    
    return std::string(buffer);
}

/**
 * @brief 打印备份信息
 * @param backupInfo 包含所有备份信息的map
 */
void RegistryMonitor::PrintBackupInfo(const std::map<std::string, BackupInfo>& backupInfo) {
    if (backupInfo.empty()) {
        printf("No backup folders found.\n");
        return;
    }
    
    int folderIndex = 1;
    for (const auto& pair : backupInfo) {
        const BackupInfo& info = pair.second;
        
        printf("Backup Folder %d:\n", folderIndex);
        printf("  Name: %s\n", info.folderName.c_str());
        printf("  Path: %s\n", info.folderPath.c_str());
        printf("  Created: %s", ctime(&info.createTime));
        printf("  Registry Files Count: %zu\n", info.regFiles.size());
        printf("  Total Size: %s\n", FormatFileSize(info.totalSize).c_str());
        
        // 打印该文件夹中的所有注册表文件
        int fileIndex = 1;
        for (const RegFileInfo& regFile : info.regFiles) {
            printf("    File %d: %s (%s)\n", 
                   fileIndex, 
                   regFile.fileName.c_str(), 
                   FormatFileSize(regFile.fileSize).c_str());
            fileIndex++;
        }
        
        folderIndex++;
        printf("\n");
    }
}

// 比较两个文件夹下的reg文件，返回比较结果
FolderComparisonResult RegistryMonitor::compareFolders(const std::string& relativeFolder1, const std::string& relativeFolder2) {
    FolderComparisonResult result;

    // 获取exe所在目录
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string exeDir = exePath;
    size_t lastSlash = exeDir.find_last_of("\\");
    if (lastSlash != std::string::npos) {
        exeDir = exeDir.substr(0, lastSlash);
    }

    // 构建完整路径
    std::string folder1 = exeDir + "\\" + relativeFolder1;
    std::string folder2 = exeDir + "\\" + relativeFolder2;

    result.folder1 = folder1;
    result.folder2 = folder2;
    result.totalComparedFiles = 0;
    result.totalAddedKeys = 0;
    result.totalRemovedKeys = 0;
    result.totalModifiedKeys = 0;

    // 获取两个文件夹中的所有reg文件
    auto files1 = getRegFiles(folder1);
    auto files2 = getRegFiles(folder2);

    // 按照文件名前的数字分组
    auto grouped1 = groupFilesByPrefix(files1);
    auto grouped2 = groupFilesByPrefix(files2);

    // 找出所有共同的数字前缀
    std::set<std::string> allPrefixes;
    for (const auto& pair : grouped1) allPrefixes.insert(pair.first);
    for (const auto& pair : grouped2) allPrefixes.insert(pair.first);

    std::cout << "=========================================" << std::endl;
    std::cout << "       Folder Comparison Result" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "Folder 1: " << folder1 << std::endl;
    std::cout << "Folder 2: " << folder2 << std::endl;
    std::cout << "Total " << allPrefixes.size() << " different prefixes" << std::endl;

    // 比较每个前缀对应的文件
    for (const auto& prefix : allPrefixes) {
        auto it1 = grouped1.find(prefix);
        auto it2 = grouped2.find(prefix);

        if (it1 != grouped1.end() && it2 != grouped2.end()) {
            // 两个文件夹都有这个前缀的文件，进行比较
            std::cout << "\n\n=========================================" << std::endl;
            std::cout << "Comparing files: " << it1->second.filename << " and " << it2->second.filename << std::endl;
            std::cout << "Prefix: " << prefix << std::endl;
            std::cout << "=========================================" << std::endl;

            FileComparisonResult fileResult = compare(it1->second.path, it2->second.path);
            fileResult.file1 = it1->second.filename;
            fileResult.file2 = it2->second.filename;

            result.comparedFiles[prefix] = fileResult;
            result.totalComparedFiles++;
            result.totalAddedKeys += fileResult.addedKeys.size();
            result.totalRemovedKeys += fileResult.removedKeys.size();
            result.totalModifiedKeys += fileResult.modifiedKeys.size();
        }
        else if (it1 != grouped1.end() && it2 == grouped2.end()) {
            // 只在文件夹1中有这个前缀的文件
            std::cout << "\n[*] Prefix " << prefix << " file only exists in folder 1: " << it1->second.filename << std::endl;
            result.filesOnlyInFolder1.push_back(it1->second.filename);
        }
        else if (it1 == grouped1.end() && it2 != grouped2.end()) {
            // 只在文件夹2中有这个前缀的文件
            std::cout << "\n[*] Prefix " << prefix << " file only exists in folder 2: " << it2->second.filename << std::endl;
            result.filesOnlyInFolder2.push_back(it2->second.filename);
        }
    }

    // 输出统计信息
    std::cout << "\n\n=========================================" << std::endl;
    std::cout << "Folder Comparison Statistics" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "Total prefixes: " << allPrefixes.size() << std::endl;
    std::cout << "Compared file pairs: " << result.totalComparedFiles << std::endl;
    std::cout << "Files only in folder 1: " << result.filesOnlyInFolder1.size() << std::endl;
    std::cout << "Files only in folder 2: " << result.filesOnlyInFolder2.size() << std::endl;
    std::cout << "Total added registry keys: " << result.totalAddedKeys << std::endl;
    std::cout << "Total removed registry keys: " << result.totalRemovedKeys << std::endl;
    std::cout << "Total modified registry keys: " << result.totalModifiedKeys << std::endl;

    return result;
}

// 比较两个reg文件，返回比较结果
FileComparisonResult RegistryMonitor::compare(const std::string& file1, const std::string& file2) {
    auto reg1 = parseRegFile(file1);
    auto reg2 = parseRegFile(file2);

    return findDifferences(reg1, reg2);
}

// 获取文件夹中的所有reg文件
std::vector<FileInfo> RegistryMonitor::getRegFiles(const std::string& folder) {
    std::vector<FileInfo> regFiles;

    // Windows平台使用FindFirstFile/FindNextFile
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind;
    std::string searchPath = folder + "\\*.reg";

    hFind = FindFirstFileA(searchPath.c_str(), &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        std::cerr << "cannot access folder: " << folder << " (error code: " << GetLastError() << ")" << std::endl;
        return regFiles;
    }

    do {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            std::string filename = findFileData.cFileName;
            std::string prefix = extractPrefix(filename);
            std::string fullPath = folder + "\\" + filename;

            regFiles.push_back({ fullPath, filename, prefix });
        }
    } while (FindNextFileA(hFind, &findFileData) != 0);

    FindClose(hFind);

    return regFiles;
}

// 提取文件名前缀（数字部分）
std::string RegistryMonitor::extractPrefix(const std::string& filename) {
    // 查找第一个下划线
    size_t underscorePos = filename.find('_');
    if (underscorePos != std::string::npos) {
        return filename.substr(0, underscorePos);
    }

    // 如果没有下划线，返回整个文件名（不含扩展名）
    size_t dotPos = filename.find('.');
    if (dotPos != std::string::npos) {
        return filename.substr(0, dotPos);
    }

    return filename;
}

// 按照前缀分组文件
std::map<std::string, FileInfo> RegistryMonitor::groupFilesByPrefix(const std::vector<FileInfo>& files) {
    std::map<std::string, FileInfo> grouped;

    for (const auto& file : files) {
        // 如果已经有相同前缀的文件，保留文件名较小的（或者可以根据其他规则选择）
        if (grouped.find(file.prefix) == grouped.end() ||
            file.filename < grouped[file.prefix].filename) {
            grouped[file.prefix] = file;
        }
    }

    return grouped;
}

// 检测文件编码并读取内容
std::string RegistryMonitor::readRegFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "cannot open file: " << filename << std::endl;
        return "";
    }

    // 读取文件前几个字节检查BOM
    char bom[3];
    file.read(bom, 2);

    std::string content;

    // 检查是否为UTF-16 LE (Windows注册表文件通常使用此编码)
    if (bom[0] == '\xFF' && bom[1] == '\xFE') {
        // UTF-16 LE编码，读取剩余内容
        std::vector<wchar_t> wideContent;
        wchar_t wc;
        while (file.read(reinterpret_cast<char*>(&wc), sizeof(wchar_t))) {
            wideContent.push_back(wc);
        }

        // 转换为UTF-8
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        content = converter.to_bytes(std::wstring(wideContent.data(), wideContent.size()));
    }
    else {
        // 可能是ANSI或UTF-8，重置文件指针并读取
        file.seekg(0);
        std::stringstream buffer;
        buffer << file.rdbuf();
        content = buffer.str();
    }

    file.close();
    return content;
}

// 解析reg文件
std::map<std::string, RegistryKy> RegistryMonitor::parseRegFile(const std::string& filename) {
    std::map<std::string, RegistryKy> registry;
    std::string content = readRegFile(filename);

    if (content.empty()) {
        return registry;
    }

    std::istringstream stream(content);
    std::string line;
    RegistryKy* currentKey = nullptr;

    while (std::getline(stream, line)) {
        // 移除可能的回车符
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        // 跳过空行和注释
        if (line.empty() || line[0] == ';') {
            continue;
        }

        // 检查是否是注册表路径
        if (line[0] == '[' && line.back() == ']') {
            std::string path = line.substr(1, line.length() - 2);
            registry[path] = RegistryKy{ path, {} };
            currentKey = &registry[path];
        }
        // 检查是否是键值对
        else if (currentKey && line.find('=') != std::string::npos) {
            auto value = parseValueLine(line);
            if (!value.name.empty()) {
                currentKey->values[value.name] = value;
            }
        }
    }

    return registry;
}

// 解析值行
RegistryVal RegistryMonitor::parseValueLine(const std::string& line) {
    RegistryVal value;
    size_t equal_pos = line.find('=');

    if (equal_pos == std::string::npos) {
        return value;
    }

    // 提取名称
    std::string name_part = line.substr(0, equal_pos);
    name_part.erase(0, name_part.find_first_not_of(' '));
    name_part.erase(name_part.find_last_not_of(' ') + 1);

    // 处理默认值 (@) 和普通值
    if (name_part == "\"\"\"\"\"\"" || name_part == "@") {
        value.name = "(默认)";
    }
    else if (name_part.length() >= 2 && name_part[0] == '\"' && name_part.back() == '\"') {
        value.name = name_part.substr(1, name_part.length() - 2);
    }
    else {
        value.name = name_part;
    }

    // 提取类型和数据
    std::string data_part = line.substr(equal_pos + 1);
    data_part.erase(0, data_part.find_first_not_of(' '));

    if (data_part.find(':') != std::string::npos) {
        size_t colon_pos = data_part.find(':');
        value.type = data_part.substr(0, colon_pos);
        value.data = data_part.substr(colon_pos + 1);
    }
    else {
        value.type = "REG_SZ";
        value.data = data_part;
    }

    // 清理数据
    if (value.type == "REG_SZ" && value.data.length() >= 2 &&
        value.data[0] == '\"' && value.data.back() == '\"') {
        value.data = value.data.substr(1, value.data.length() - 2);
    }

    // 处理十六进制数据
    if (value.type.find("REG_") == 0 && value.type != "REG_SZ" && value.type != "REG_EXPAND_SZ") {
        // 保留原始十六进制数据
    }
    else {
        // 对于字符串类型，移除可能的引号
        if (value.data.length() >= 2 && value.data[0] == '\"' && value.data.back() == '\"') {
            value.data = value.data.substr(1, value.data.length() - 2);
        }
    }

    return value;
}

// 查找差异，返回比较结果
FileComparisonResult RegistryMonitor::findDifferences(const std::map<std::string, RegistryKy>& reg1,
    const std::map<std::string, RegistryKy>& reg2) {
    FileComparisonResult result;
    std::set<std::string> allPaths;

    // 收集所有路径
    for (const auto& pair : reg1) allPaths.insert(pair.first);
    for (const auto& pair : reg2) allPaths.insert(pair.first);

    // 比较键
    for (const auto& path : allPaths) {
        auto it1 = reg1.find(path);
        auto it2 = reg2.find(path);

        if (it1 == reg1.end() && it2 != reg2.end()) {
            result.addedKeys.push_back(path);
        }
        else if (it1 != reg1.end() && it2 == reg2.end()) {
            result.removedKeys.push_back(path);
        }
        else if (it1 != reg1.end() && it2 != reg2.end()) {
            if (!(it1->second == it2->second)) {
                result.modifiedKeys.push_back(path);
                // 记录详细的键值变化
                compareKeyValues(it1->second, it2->second, result.keyChanges);
            }
        }
    }

    // 输出结果
    printResults(result);

    return result;
}

// 打印结果
void RegistryMonitor::printResults(const FileComparisonResult& result) {

    // 新增的注册表项
    if (!result.addedKeys.empty()) {
        std::cout << "\nAdded registry keys (" << result.addedKeys.size() << "):" << std::endl;
        std::cout << "-----------------------------------------" << std::endl;
        for (const auto& key : result.addedKeys) {
            std::cout << "[+] " << key << std::endl;
        }
    }

    // 删除的注册表项
    if (!result.removedKeys.empty()) {
        std::cout << "\nRemoved registry keys (" << result.removedKeys.size() << "):" << std::endl;
        std::cout << "-----------------------------------------" << std::endl;
        for (const auto& key : result.removedKeys) {
            std::cout << "[-] " << key << std::endl;
        }
    }

    // 修改的注册表项
    if (!result.modifiedKeys.empty()) {
        std::cout << "\nModified registry keys (" << result.modifiedKeys.size() << "):" << std::endl;
        std::cout << "-----------------------------------------" << std::endl;
        for (const auto& key : result.modifiedKeys) {
            std::cout << "[*] " << key << std::endl;
            auto it = result.keyChanges.find(key);
            if (it != result.keyChanges.end()) {
                std::cout << "    Old value: " << it->second.first << std::endl;
                std::cout << "    New value: " << it->second.second << std::endl;
            }
        }
    }

    // 统计信息
    std::cout << "\n-----------------------------------------" << std::endl;
    std::cout << "File comparison statistics:" << std::endl;
    std::cout << "  Added: " << result.addedKeys.size() << std::endl;
    std::cout << "  Removed: " << result.removedKeys.size() << std::endl;
    std::cout << "  Modified: " << result.modifiedKeys.size() << std::endl;
    std::cout << "-----------------------------------------" << std::endl;
}

// 比较键值对，记录变化
void RegistryMonitor::compareKeyValues(const RegistryKy& key1, const RegistryKy& key2,
    std::map<std::string, std::pair<std::string, std::string>>& keyChanges) {
    std::set<std::string> allValueNames;

    for (const auto& value : key1.values) allValueNames.insert(value.first);
    for (const auto& value : key2.values) allValueNames.insert(value.first);

    std::string oldValues, newValues;

    for (const auto& name : allValueNames) {
        auto it1 = key1.values.find(name);
        auto it2 = key2.values.find(name);

        if (it1 == key1.values.end() && it2 != key2.values.end()) {
            // 新增的值
            newValues += "[+] " + name + " = " + it2->second.type + ":" + it2->second.data + "\n";
        }
        else if (it1 != key1.values.end() && it2 == key2.values.end()) {
            // 删除的值
            oldValues += "[-] " + name + " = " + it1->second.type + ":" + it1->second.data + "\n";
        }
        else if (it1 != key1.values.end() && it2 != key2.values.end()) {
            if (!(it1->second == it2->second)) {
                // 修改的值
                oldValues += "[modify] " + name + " = " + it1->second.type + ":" + it1->second.data + "\n";
                newValues += "[modify] " + name + " = " + it2->second.type + ":" + it2->second.data + "\n";
            }
        }
    }

    keyChanges[key1.path] = std::make_pair(oldValues, newValues);
}

} // namespace sysmonitor