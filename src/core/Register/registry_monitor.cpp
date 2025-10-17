#include "registry_monitor.h"
#include "../../utils/registry_encode.h"
#define NOMAXMIN
#include <windows.h>
#include <iostream>

namespace sysmonitor {

using namespace encoding;

RegistryMonitor::RegistryMonitor() {}

RegistrySnapshot RegistryMonitor::GetRegistrySnapshot() {
    RegistrySnapshot snapshot;
    snapshot.timestamp = GetTickCount64();
    
    try {
        snapshot.systemInfoKeys = GetSystemInfoKeys();
        snapshot.softwareKeys = GetInstalledSoftware();
        snapshot.networkKeys = GetNetworkConfig();
        snapshot.autoStartKeys = GetAutoStartItems();

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

std::vector<RegistryKey> RegistryMonitor::GetAutoStartItems() {
    std::vector<RegistryKey> keys;
    
    // Use member variable autoStartPaths to avoid variable name conflicts
    for (const auto& [root, path] : autoStartPaths) {
        try {
            RegistryKey key = GetRegistryKey(root, path);
            
            // Special handling for service items, only get services with auto-start type
            if (path.find("Services") != std::string::npos) {
                key = FilterAutoStartServices(key);
            }
            
            if (!key.values.empty() || !key.subkeys.empty()) {
                // Add auto-start item type identifier
                key.autoStartType = GetAutoStartType(root, path);
                keys.push_back(key);
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to get auto-start registry: " << path << " - " << e.what() << std::endl;
        }
    }
    
    return keys;
}

// Implement FilterAutoStartServices method
RegistryKey RegistryMonitor::FilterAutoStartServices(const RegistryKey& servicesKey) {
    RegistryKey filteredKey;
    filteredKey.path = servicesKey.path;
    filteredKey.autoStartType = "AutoStartServices";
    
    // Iterate through all service subkeys
    for (const auto& subkeyName : servicesKey.subkeys) {
        try {
            std::string servicePath = servicesKey.path.substr(servicesKey.path.find("\\") + 1) + "\\" + subkeyName;
            RegistryKey serviceKey = GetRegistryKey(HKEY_LOCAL_MACHINE, servicePath);
            
            // Check service start type
            bool isAutoStart = false;
            for (const auto& value : serviceKey.values) {
                if (value.name == "Start") {
                    // Start value: 2=Auto, 3=Manual, 4=Disabled
                    if (value.data == "2" || value.data.find("Auto") != std::string::npos) {
                        isAutoStart = true;
                    }
                    break;
                }
            }
            
            if (isAutoStart) {
                // Only add auto-start service subkey names
                filteredKey.subkeys.push_back(subkeyName);
                
                // Add service key information to values
                RegistryValue serviceInfo;
                serviceInfo.name = subkeyName;
                serviceInfo.type = "ServiceInfo";
                
                // Build service information string
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
            // Skip inaccessible services
            continue;
        }
    }
    
    return filteredKey;
}

// Implement GetAutoStartType method
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

std::vector<RegistryKey> RegistryMonitor::GetInstalledSoftware() {
    std::vector<RegistryKey> keys;
    
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
    
    // Get current user software information
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

RegistryKey RegistryMonitor::GetRegistryKey(HKEY root, const std::string& subKey) {
    RegistryKey key;
    key.path = RegistryEncodingUtils::HKEYToString(root) + "\\" + subKey;
    
    HKEY hKey;
    if (!SafeOpenKey(root, subKey, hKey)) {
        return key;
    }
    
    // Enumerate values
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
            
            result = RegEnumValueA(hKey, i, valueName.data(), &valueNameSize,
                                 NULL, &valueType, NULL, &valueDataSize);
            
            if (result == ERROR_SUCCESS) {
                try {
                    valueData.resize(valueDataSize + 2, 0);
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
    
    // Enumerate subkeys
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

bool RegistryMonitor::SafeOpenKey(HKEY root, const std::string& subKey, HKEY& hKey) {
    LONG result = RegOpenKeyExA(root, subKey.c_str(), 0, KEY_READ, &hKey);
    
    if (result != ERROR_SUCCESS) {
        std::cerr << "Unable to open registry key: " << RegistryEncodingUtils::HKEYToString(root) << "\\" << subKey 
                  << " - Error: " << result << std::endl;
        return false;
    }
    
    return true;
}

void RegistryMonitor::SafeCloseKey(HKEY hKey) {
    if (hKey) {
        RegCloseKey(hKey);
    }
}

bool RegistryMonitor::SafeQueryValue(const std::string& fullPath, RegistryValue& value) {
    size_t pos = fullPath.find('\\');
    if (pos == std::string::npos) {
        return false;
    }
    
    std::string rootStr = fullPath.substr(0, pos);
    std::string restPath = fullPath.substr(pos + 1);
    
    HKEY hRoot;
    if (rootStr == "HKEY_LOCAL_MACHINE") hRoot = HKEY_LOCAL_MACHINE;
    else if (rootStr == "HKEY_CURRENT_USER") hRoot = HKEY_CURRENT_USER;
    else if (rootStr == "HKEY_CLASSES_ROOT") hRoot = HKEY_CLASSES_ROOT;
    else if (rootStr == "HKEY_USERS") hRoot = HKEY_USERS;
    else if (rootStr == "HKEY_CURRENT_CONFIG") hRoot = HKEY_CURRENT_CONFIG;
    else return false;
    
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