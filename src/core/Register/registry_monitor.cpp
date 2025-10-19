#include "registry_monitor.h"
#include "../../utils/registry_encode.h"
#define NOMAXMIN  // 防止 Windows.h 定义 min/max 宏，避免与 STL 冲突
#include <windows.h>
#include <iostream>

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
    snapshot.timestamp = GetTickCount64();  // 使用系统启动后的毫秒数作为时间戳
    
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

} // namespace sysmonitor